// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   exchangeitems1d.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Concrete 1-D typed IInput and IOutput implementations.
 *
 * Input1D<T> / Output1D<T> combine AbstractInput / AbstractOutput with
 * ComponentDataItem1D<T> and implement the v2 typed hyperslab data plane by
 * delegating to the storage mixin.
 *
 * Input1D<T> provides a default canConsume() that accepts any 1-D provider
 * of the same DataKind, and a retrieveValuesFromProvider() that pulls the
 * provider's values into local storage. Subclasses override these for
 * model-specific rules; applyData() is a no-op hook for subclasses.
 */

#ifndef HYDROCOUPLESDK_EXCHANGEITEMS1D_H
#define HYDROCOUPLESDK_EXCHANGEITEMS1D_H

#include "hydrocouplesdk/data/abstractinput.h"
#include "hydrocouplesdk/data/abstractoutput.h"
#include "hydrocouplesdk/data/componentdataitem.h"

#include <algorithm>
#include <string>

namespace HydroCouple::SDK {

// ── Input1D<T> ────────────────────────────────────────────────────────────

/*!
 * \brief Concrete 1-D typed input exchange item.
 *
 * \details The storage holds the values last retrieved from the provider,
 * or written with setValuesFrom(). Its length is fixed by the component (it
 * does not follow the provider): retrieveValuesFromProvider() copies the
 * first min(provider length, length()) values and leaves the rest.
 * canConsume() checks only the kind and the rank; it does not compare
 * lengths, units or dimensions (validateConnection() does the unit and
 * dimension checks).
 *
 * \tparam T  Element type (int, double, std::string, …).
 */
template <typename T>
class Input1D
    : public AbstractInput,
      public ComponentDataItem1D<T>
{
public:
    /*!
     * \brief Construct an unconnected input with \p length elements of \c T{}.
     * \param id        Input identifier, unique within the component.
     * \param dimension The input's axis (borrowed).
     * \param length    Element count.
     * \param valueDef  Value definition (borrowed; may be nullptr).
     * \param mc        Owning component (borrowed; may be nullptr).
     */
    Input1D(std::string_view id, Dimension *dimension, int length,
            ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
        : AbstractInput(id, {dimension}, valueDef, mc),
          ComponentDataItem1D<T>(length),
          m_dimension(dimension)
    {}

    ~Input1D() override = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return ComponentDataItem1D<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return ComponentDataItem1D<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return ComponentDataItem1D<T>::getSlab(destination, start, count, message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!ComponentDataItem1D<T>::setSlab(source, start, count, message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── IInput ────────────────────────────────────────────────────────────

    /*!
     * \brief Accepts any 1-D provider of the same DataKind.
     * \param[in]  provider Candidate output.
     * \param[out] message  "Provider is null", "Provider data kind
     *                      mismatch" or "Provider is not one-dimensional".
     * \return true when \p provider has this input's kind and rank 1.
     */
    [[nodiscard]] bool canConsume(HydroCouple::IOutput *provider,
                                  std::string &message) const override
    {
        if (!provider)
        {
            message = "Provider is null";
            return false;
        }
        if (provider->dataKind() != ComponentDataItem1D<T>::storageKind())
        {
            message = "Provider data kind mismatch";
            return false;
        }
        if (provider->shape().size() != 1)
        {
            message = "Provider is not one-dimensional";
            return false;
        }
        return true;
    }

    /*!
     * \brief Pulls up to length() leading values from the connected provider.
     * \details Copies elements [0, n) with n = min(provider length,
     *          length()) and fires value-changed for them. Returns without a
     *          change or a message when unconnected, when the provider's
     *          rank or kind does not fit, when n is 0, or when the read fails.
     */
    void retrieveValuesFromProvider() override
    {
        HydroCouple::IOutput *p = provider();
        if (!p)
            return;
        const auto srcShape = p->shape();
        if (srcShape.size() != 1 ||
            p->dataKind() != ComponentDataItem1D<T>::storageKind())
            return;
        const int64_t n = std::min<int64_t>(srcShape[0], this->length());
        if (n <= 0)
            return;
        const BufferDescriptor dest = detail::hostDescriptor(
            this->rawData().data(), ComponentDataItem1D<T>::storageKind(),
            std::span<const int64_t>(&n, 1));
        const int64_t zero = 0;
        std::string message;
        if (p->getValuesInto(dest, std::span<const int64_t>(&zero, 1),
                             std::span<const int64_t>(&n, 1), &message))
        {
            emitValueChanged({0}, {n});
        }
    }

    /*!
     * \brief Hook for subclasses to apply retrieved values to model state.
     */
    void applyData() override {}

    // ── JSON persistence ──────────────────────────────────────────────────

    /*!
     * \brief Reads the base fields and, when present, the "values" array.
     * \details Resizes to the array; fires no signal. An element of the
     *          wrong type throws nlohmann::json::type_error.
     * \param[in] j JSON object as written by writeData().
     */
    void readData(const nlohmann::json &j) override
    {
        AbstractComponentDataItem::readData(j);
        if (j.contains("values"))
        {
            const auto &arr = j["values"];
            this->resizeDataArray(static_cast<int>(arr.size()));
            for (int i = 0; i < static_cast<int>(arr.size()); ++i)
                (*this)[i] = arr[i].get<T>();
        }
    }

    /*!
     * \brief Writes the base fields and the values.
     * \return The base object with "values" holding the array.
     */
    [[nodiscard]] nlohmann::json writeData() const override
    {
        auto j = AbstractComponentDataItem::writeData();
        auto arr = nlohmann::json::array();
        for (int i = 0; i < this->length(); ++i)
            arr.push_back((*this)[i]);
        j["values"] = arr;
        return j;
    }

private:
    Dimension *m_dimension; //!< The input's axis (borrowed).
};

using Input1DInt    = Input1D<int>;         //!< 1-D input of \c int (DataKind::Int32).
using Input1DDouble = Input1D<double>;      //!< 1-D input of \c double (DataKind::Float64).
using Input1DString = Input1D<std::string>; //!< 1-D input of \c std::string (DataKind::String).

extern template class HYDROCOUPLESDK_EXPORT Input1D<int>;
extern template class HYDROCOUPLESDK_EXPORT Input1D<double>;
extern template class HYDROCOUPLESDK_EXPORT Input1D<std::string>;


// ── Output1D<T> ───────────────────────────────────────────────────────────

/*!
 * \brief Concrete 1-D typed output exchange item.
 *
 * updateValues() is a no-op by default; model components typically override
 * it (or write directly via setValuesFrom / the typed mixin accessors).
 *
 * Writes through setValuesFrom() fire value-changed; writes through
 * operator[] or rawData() do not. Neither refreshes adapted outputs: the
 * workflow (or AbstractOutput::updateValues(query)) does that.
 *
 * \tparam T  Element type (int, double, std::string, …).
 */
template <typename T>
class Output1D
    : public AbstractOutput,
      public ComponentDataItem1D<T>
{
public:
    /*!
     * \brief Construct an output with \p length elements of \c T{} and no consumers.
     * \param id        Output identifier, unique within the component.
     * \param dimension The output's axis (borrowed).
     * \param length    Element count.
     * \param valueDef  Value definition (borrowed; may be nullptr).
     * \param mc        Producing component (borrowed; may be nullptr).
     */
    Output1D(std::string_view id, Dimension *dimension, int length,
             ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
        : AbstractOutput(id, {dimension}, valueDef, mc),
          ComponentDataItem1D<T>(length),
          m_dimension(dimension)
    {}

    ~Output1D() override = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return ComponentDataItem1D<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return ComponentDataItem1D<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return ComponentDataItem1D<T>::getSlab(destination, start, count, message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!ComponentDataItem1D<T>::setSlab(source, start, count, message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── IOutput ───────────────────────────────────────────────────────────

    //! No-op: the component writes the storage in its own update().
    void updateValues() override {}

    // ── JSON persistence ──────────────────────────────────────────────────

    /*!
     * \brief Reads the base fields and, when present, the "values" array.
     * \details Resizes to the array; fires no signal.
     * \param[in] j JSON object as written by writeData().
     */
    void readData(const nlohmann::json &j) override
    {
        AbstractComponentDataItem::readData(j);
        if (j.contains("values"))
        {
            const auto &arr = j["values"];
            this->resizeDataArray(static_cast<int>(arr.size()));
            for (int i = 0; i < static_cast<int>(arr.size()); ++i)
                (*this)[i] = arr[i].get<T>();
        }
    }

    /*!
     * \brief Writes the base fields and the values.
     * \return The base object with "values" holding the array.
     */
    [[nodiscard]] nlohmann::json writeData() const override
    {
        auto j = AbstractComponentDataItem::writeData();
        auto arr = nlohmann::json::array();
        for (int i = 0; i < this->length(); ++i)
            arr.push_back((*this)[i]);
        j["values"] = arr;
        return j;
    }

private:
    Dimension *m_dimension; //!< The output's axis (borrowed).
};

using Output1DInt    = Output1D<int>;         //!< 1-D output of \c int (DataKind::Int32).
using Output1DDouble = Output1D<double>;      //!< 1-D output of \c double (DataKind::Float64).
using Output1DString = Output1D<std::string>; //!< 1-D output of \c std::string (DataKind::String).

extern template class HYDROCOUPLESDK_EXPORT Output1D<int>;
extern template class HYDROCOUPLESDK_EXPORT Output1D<double>;
extern template class HYDROCOUPLESDK_EXPORT Output1D<std::string>;

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_EXCHANGEITEMS1D_H
