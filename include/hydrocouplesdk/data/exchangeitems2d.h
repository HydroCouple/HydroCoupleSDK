// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   exchangeitems2d.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Concrete 2-D typed IInput and IOutput implementations.
 *
 * Input2D<T> / Output2D<T> combine AbstractInput / AbstractOutput with
 * ComponentDataItem2D<T> and implement the v2 typed hyperslab data plane by
 * delegating to the storage mixin.
 *
 * Input2D<T> provides a default canConsume() that accepts any 2-D provider
 * of the same DataKind, and a retrieveValuesFromProvider() that pulls the
 * overlapping leading region of the provider's values into local storage.
 */

#ifndef HYDROCOUPLESDK_EXCHANGEITEMS2D_H
#define HYDROCOUPLESDK_EXCHANGEITEMS2D_H

#include "hydrocouplesdk/data/abstractinput.h"
#include "hydrocouplesdk/data/abstractoutput.h"
#include "hydrocouplesdk/data/componentdataitem.h"

#include <algorithm>
#include <array>
#include <string>

namespace HydroCouple::SDK {

// ── Input2D<T> ────────────────────────────────────────────────────────────

/*!
 * \brief Concrete 2-D typed input exchange item (row-major storage).
 * \tparam T  Element type (int, double, std::string, …).
 */
template <typename T>
class Input2D
    : public AbstractInput,
      public ComponentDataItem2D<T>
{
public:
    Input2D(std::string_view id, Dimension *dimensionX, Dimension *dimensionY,
            int lengthX, int lengthY,
            ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
        : AbstractInput(id, {dimensionX, dimensionY}, valueDef, mc),
          ComponentDataItem2D<T>(lengthX, lengthY),
          m_dimensionX(dimensionX),
          m_dimensionY(dimensionY)
    {}

    ~Input2D() override = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return ComponentDataItem2D<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return ComponentDataItem2D<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return ComponentDataItem2D<T>::getSlab(destination, start, count, message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!ComponentDataItem2D<T>::setSlab(source, start, count, message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── IInput ────────────────────────────────────────────────────────────

    /*!
     * \brief Accepts any 2-D provider of the same DataKind.
     */
    [[nodiscard]] bool canConsume(HydroCouple::IOutput *provider,
                                  std::string &message) const override
    {
        if (!provider)
        {
            message = "Provider is null";
            return false;
        }
        if (provider->dataKind() != ComponentDataItem2D<T>::storageKind())
        {
            message = "Provider data kind mismatch";
            return false;
        }
        if (provider->shape().size() != 2)
        {
            message = "Provider is not two-dimensional";
            return false;
        }
        return true;
    }

    /*!
     * \brief Pulls the overlapping leading region from the connected provider.
     */
    void retrieveValuesFromProvider() override
    {
        HydroCouple::IOutput *p = provider();
        if (!p)
            return;
        const auto srcShape = p->shape();
        if (srcShape.size() != 2 ||
            p->dataKind() != ComponentDataItem2D<T>::storageKind())
            return;
        const std::array<int64_t, 2> count{
            std::min<int64_t>(srcShape[0], this->iLength()),
            std::min<int64_t>(srcShape[1], this->jLength())};
        if (count[0] <= 0 || count[1] <= 0)
            return;
        // Pull into a dense staging block, then write it into our storage so
        // partial overlaps land at the right row offsets.
        std::vector<T> staging(static_cast<size_t>(count[0] * count[1]));
        const BufferDescriptor stagingBuf = detail::hostDescriptor(
            staging.data(), ComponentDataItem2D<T>::storageKind(),
            std::span<const int64_t>(count.data(), 2));
        const std::array<int64_t, 2> zero{0, 0};
        std::string message;
        if (!p->getValuesInto(stagingBuf,
                              std::span<const int64_t>(zero.data(), 2),
                              std::span<const int64_t>(count.data(), 2),
                              &message))
            return;
        if (this->setSlab(stagingBuf, std::span<const int64_t>(zero.data(), 2),
                          std::span<const int64_t>(count.data(), 2), &message))
        {
            emitValueChanged({0, 0}, {count[0], count[1]});
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
            assignRows(j["values"]);
    }

    [[nodiscard]] nlohmann::json writeData() const override
    {
        auto j = AbstractComponentDataItem::writeData();
        j["values"] = rowsToJson();
        return j;
    }

private:
    [[nodiscard]] nlohmann::json rowsToJson() const
    {
        auto rows = nlohmann::json::array();
        for (int i = 0; i < this->iLength(); ++i)
        {
            auto row = nlohmann::json::array();
            for (int jj = 0; jj < this->jLength(); ++jj)
                row.push_back((*this)(i, jj));
            rows.push_back(std::move(row));
        }
        return rows;
    }

    void assignRows(const nlohmann::json &j)
    {
        if (!j.is_array())
            return;
        const int rows = static_cast<int>(j.size());
        const int cols = rows > 0 && j[0].is_array()
                             ? static_cast<int>(j[0].size()) : 0;
        this->resizeDataArray(rows, cols);
        for (int i = 0; i < rows; ++i)
            for (int jj = 0; jj < cols && jj < static_cast<int>(j[i].size()); ++jj)
                (*this)(i, jj) = j[i][jj].get<T>();
    }

    Dimension *m_dimensionX;
    Dimension *m_dimensionY;
};

using Input2DInt    = Input2D<int>;
using Input2DDouble = Input2D<double>;
using Input2DString = Input2D<std::string>;

extern template class HYDROCOUPLESDK_EXPORT Input2D<int>;
extern template class HYDROCOUPLESDK_EXPORT Input2D<double>;
extern template class HYDROCOUPLESDK_EXPORT Input2D<std::string>;


// ── Output2D<T> ───────────────────────────────────────────────────────────

/*!
 * \brief Concrete 2-D typed output exchange item (row-major storage).
 *
 * updateValues() is a no-op by default; model components typically override
 * it (or write directly via setValuesFrom / the typed mixin accessors).
 *
 * \tparam T  Element type (int, double, std::string, …).
 */
template <typename T>
class Output2D
    : public AbstractOutput,
      public ComponentDataItem2D<T>
{
public:
    Output2D(std::string_view id, Dimension *dimensionX, Dimension *dimensionY,
             int lengthX, int lengthY,
             ValueDefinition *valueDef, HydroCouple::IModelComponent *mc)
        : AbstractOutput(id, {dimensionX, dimensionY}, valueDef, mc),
          ComponentDataItem2D<T>(lengthX, lengthY),
          m_dimensionX(dimensionX),
          m_dimensionY(dimensionY)
    {}

    ~Output2D() override = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return ComponentDataItem2D<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return ComponentDataItem2D<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return ComponentDataItem2D<T>::getSlab(destination, start, count, message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!ComponentDataItem2D<T>::setSlab(source, start, count, message))
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
        if (!j.contains("values") || !j["values"].is_array())
            return;
        const auto &rows = j["values"];
        const int r = static_cast<int>(rows.size());
        const int c = r > 0 && rows[0].is_array()
                          ? static_cast<int>(rows[0].size()) : 0;
        this->resizeDataArray(r, c);
        for (int i = 0; i < r; ++i)
            for (int jj = 0; jj < c && jj < static_cast<int>(rows[i].size()); ++jj)
                (*this)(i, jj) = rows[i][jj].get<T>();
    }

    [[nodiscard]] nlohmann::json writeData() const override
    {
        auto j = AbstractComponentDataItem::writeData();
        auto rows = nlohmann::json::array();
        for (int i = 0; i < this->iLength(); ++i)
        {
            auto row = nlohmann::json::array();
            for (int jj = 0; jj < this->jLength(); ++jj)
                row.push_back((*this)(i, jj));
            rows.push_back(std::move(row));
        }
        j["values"] = rows;
        return j;
    }

private:
    Dimension *m_dimensionX;
    Dimension *m_dimensionY;
};

using Output2DInt    = Output2D<int>;
using Output2DDouble = Output2D<double>;
using Output2DString = Output2D<std::string>;

extern template class HYDROCOUPLESDK_EXPORT Output2D<int>;
extern template class HYDROCOUPLESDK_EXPORT Output2D<double>;
extern template class HYDROCOUPLESDK_EXPORT Output2D<std::string>;

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_EXCHANGEITEMS2D_H
