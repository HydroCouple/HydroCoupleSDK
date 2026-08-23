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
 * \tparam T  Element type (int, double, std::string, …).
 */
template <typename T>
class Input1D
    : public AbstractInput,
      public ComponentDataItem1D<T>
{
public:
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
    Dimension *m_dimension;
};

using Input1DInt    = Input1D<int>;
using Input1DDouble = Input1D<double>;
using Input1DString = Input1D<std::string>;

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
 * \tparam T  Element type (int, double, std::string, …).
 */
template <typename T>
class Output1D
    : public AbstractOutput,
      public ComponentDataItem1D<T>
{
public:
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

    void updateValues() override {}

    // ── JSON persistence ──────────────────────────────────────────────────

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
    Dimension *m_dimension;
};

using Output1DInt    = Output1D<int>;
using Output1DDouble = Output1D<double>;
using Output1DString = Output1D<std::string>;

extern template class HYDROCOUPLESDK_EXPORT Output1D<int>;
extern template class HYDROCOUPLESDK_EXPORT Output1D<double>;
extern template class HYDROCOUPLESDK_EXPORT Output1D<std::string>;

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_EXCHANGEITEMS1D_H
