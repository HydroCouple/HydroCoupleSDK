// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   idbasedexchangeitems.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Id-based exchange items (plan §7a) — IIdBasedComponentDataItem
 *         over the typed hyperslab data plane.
 *
 * IdBasedInput<T> and IdBasedOutput<T> attach values to string identifiers
 * rather than positions: the identifier dimension is dimension 0 of
 * shape(), values live in the IdBasedComponentDataItem storage mixin, and
 * providers/consumers match on identifier count and DataKind. The default
 * retrieveValuesFromProvider() pulls the provider's leading values by
 * position — id-to-id reordering belongs in an adapted output or a
 * subclass override, exactly like the positional 1-D items.
 *
 * \threadsafety Not thread-safe on their own.
 */

#ifndef HYDROCOUPLESDK_IDBASEDEXCHANGEITEMS_H
#define HYDROCOUPLESDK_IDBASEDEXCHANGEITEMS_H

#include "hydrocouplesdk/data/abstractinput.h"
#include "hydrocouplesdk/data/abstractoutput.h"
#include "hydrocouplesdk/data/idbasedcomponentdataitem.h"
#include "hydrocouplesdk/core/dimension.h"

#include <string>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Id-based input item: consumes values keyed by identifier.
 *
 * \details The input owns its identifier dimension (id "identifiers", role
 * Entity). Matching is by position, not by name: value i of the provider
 * lands on identifier i of this input, whatever the provider's identifiers
 * are. A provider whose order differs needs a subclass that maps by name, or
 * an adapted output that reorders.
 *
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class IdBasedInput
    : public AbstractInput,
      public IdBasedComponentDataItem<T>,
      public virtual HydroCouple::IIdBasedComponentDataItem
{
public:
    /*!
     * \brief Construct with identifiers (one value slot per id).
     * \param[in] id          Item identifier.
     * \param[in] identifiers Entity identifiers (dimension 0 order).
     * \param[in] valueDef    Value definition (borrowed).
     * \param[in] mc          Owning component (borrowed; may be nullptr).
     */
    IdBasedInput(std::string_view id,
                 const std::vector<std::string> &identifiers,
                 ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
        : AbstractInput(id, {&m_identifierDimension}, valueDef, mc),
          IdBasedComponentDataItem<T>(identifiers),
          m_identifierDimension("identifiers", "Identifier dimension")
    {
    }

    ~IdBasedInput() override = default;

    // ── IIdBasedComponentDataItem ─────────────────────────────────────────

    //! The entity identifiers, in dimension-0 order.
    //! \return A copy of the list.
    [[nodiscard]] std::vector<std::string> identifiers() const override
    {
        return IdBasedComponentDataItem<T>::identifiers();
    }

    //! The identifier axis, owned by this item.
    //! \return The member dimension (id "identifiers").
    [[nodiscard]] HydroCouple::IDimension *identifierDimension() const override
    {
        return const_cast<Dimension *>(&m_identifierDimension);
    }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return IdBasedComponentDataItem<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return IdBasedComponentDataItem<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override
    {
        return IdBasedComponentDataItem<T>::getSlab(destination, start, count,
                                                    message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!IdBasedComponentDataItem<T>::setSlab(source, start, count,
                                                  message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── IInput ────────────────────────────────────────────────────────────

    /*!
     * \brief Accepts a 1-D provider of the same DataKind carrying at least
     *        as many values as this input has identifiers.
     * \details The provider need not be id-based, and its identifiers are
     *          not compared.
     * \param[in]  provider Candidate output.
     * \param[out] message  "provider is null", "provider DataKind differs"
     *                      or "provider must be 1-D with at least N values";
     *                      cleared on success.
     * \return true when the provider fits.
     */
    [[nodiscard]] bool canConsume(HydroCouple::IOutput *provider,
                                  std::string &message) const override
    {
        if (!provider)
        {
            message = "provider is null";
            return false;
        }
        if (provider->dataKind() != IdBasedComponentDataItem<T>::storageKind())
        {
            message = "provider DataKind differs";
            return false;
        }
        const auto providerShape = provider->shape();
        if (providerShape.size() != 1 ||
            providerShape[0] <
                static_cast<int64_t>(IdBasedComponentDataItem<T>::length()))
        {
            message = "provider must be 1-D with at least " +
                      std::to_string(IdBasedComponentDataItem<T>::length()) +
                      " values";
            return false;
        }
        message.clear();
        return true;
    }

    /*!
     * \brief Pulls the provider's leading length() values by position.
     * \details Fires no value-changed signal (unlike Input1D) and ignores a
     *          failed read, which happens when the provider has shrunk below
     *          length().
     */
    void retrieveValuesFromProvider() override
    {
        HydroCouple::IOutput *p = provider();
        if (!p)
            return;
        const int64_t count =
            static_cast<int64_t>(IdBasedComponentDataItem<T>::length());
        const std::array<int64_t, 1> shapeArray{count};
        BufferDescriptor destination;
        destination.data  = IdBasedComponentDataItem<T>::rawData().data();
        destination.kind  = IdBasedComponentDataItem<T>::storageKind();
        destination.rank  = 1;
        destination.shape = shapeArray.data();
        destination.space = MemorySpace::Host;
        const std::array<int64_t, 1> start{0};
        (void)p->getValuesInto(destination, start, shapeArray, nullptr);
    }

    //! Empty hook; override to apply the values to the model.
    void applyData() override {}

private:
    Dimension m_identifierDimension; //!< Identifier axis ("identifiers", role Entity).
};

/*!
 * \brief Id-based output item: produces values keyed by identifier.
 *
 * \details Owns its identifier dimension (id "identifiers", role Entity).
 * The component writes values by identifier with valueAt() or by position
 * with operator[] or setValuesFrom(); consumers read them by position.
 *
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class IdBasedOutput
    : public AbstractOutput,
      public IdBasedComponentDataItem<T>,
      public virtual HydroCouple::IIdBasedComponentDataItem
{
public:
    /*!
     * \brief Construct with identifiers (one value slot per id).
     * \param[in] id          Item identifier.
     * \param[in] identifiers Entity identifiers (dimension 0 order); values
     *                        start at \c T{}.
     * \param[in] valueDef    Value definition (borrowed).
     * \param[in] mc          Producing component (borrowed; may be nullptr).
     */
    IdBasedOutput(std::string_view id,
                  const std::vector<std::string> &identifiers,
                  ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
        : AbstractOutput(id, {&m_identifierDimension}, valueDef, mc),
          IdBasedComponentDataItem<T>(identifiers),
          m_identifierDimension("identifiers", "Identifier dimension")
    {
    }

    ~IdBasedOutput() override = default;

    // ── IIdBasedComponentDataItem ─────────────────────────────────────────

    //! The entity identifiers, in dimension-0 order.
    //! \return A copy of the list.
    [[nodiscard]] std::vector<std::string> identifiers() const override
    {
        return IdBasedComponentDataItem<T>::identifiers();
    }

    //! The identifier axis, owned by this item.
    //! \return The member dimension (id "identifiers").
    [[nodiscard]] HydroCouple::IDimension *identifierDimension() const override
    {
        return const_cast<Dimension *>(&m_identifierDimension);
    }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return IdBasedComponentDataItem<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return IdBasedComponentDataItem<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override
    {
        return IdBasedComponentDataItem<T>::getSlab(destination, start, count,
                                                    message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!IdBasedComponentDataItem<T>::setSlab(source, start, count,
                                                  message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── IOutput ───────────────────────────────────────────────────────────

    /*!
     * \brief No-op — model components refresh the storage directly.
     */
    void updateValues() override {}

private:
    Dimension m_identifierDimension; //!< Identifier axis ("identifiers", role Entity).
};

using IdBasedInputInt     = IdBasedInput<int>;          //!< Id-based input of \c int.
using IdBasedInputDouble  = IdBasedInput<double>;       //!< Id-based input of \c double.
using IdBasedInputString  = IdBasedInput<std::string>;  //!< Id-based input of \c std::string.
using IdBasedOutputInt    = IdBasedOutput<int>;         //!< Id-based output of \c int.
using IdBasedOutputDouble = IdBasedOutput<double>;      //!< Id-based output of \c double.
using IdBasedOutputString = IdBasedOutput<std::string>; //!< Id-based output of \c std::string.

extern template class HYDROCOUPLESDK_EXPORT IdBasedInput<int>;
extern template class HYDROCOUPLESDK_EXPORT IdBasedInput<double>;
extern template class HYDROCOUPLESDK_EXPORT IdBasedInput<std::string>;
extern template class HYDROCOUPLESDK_EXPORT IdBasedOutput<int>;
extern template class HYDROCOUPLESDK_EXPORT IdBasedOutput<double>;
extern template class HYDROCOUPLESDK_EXPORT IdBasedOutput<std::string>;

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_IDBASEDEXCHANGEITEMS_H
