// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   componentdataitem.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Typed 1-D and 2-D data-storage mixins for component data items.
 *
 * ComponentDataItem1D<T> and ComponentDataItem2D<T> are header-only template
 * helpers that provide contiguous std::vector storage for concrete component
 * data-item subclasses (Argument1D<T>, exchange items, etc.).
 *
 * They are intentionally *not* derived from IComponentDataItem — they are
 * pure storage/accessor mixins. Concrete classes multiply-inherit from both
 * AbstractComponentDataItem (which provides the HydroCouple interface) and
 * ComponentDataItem1D<T> (which provides the typed data array).
 *
 * Data layout
 * -----------
 * - ComponentDataItem1D<T>: flat std::vector<T> of length \c m_length.
 * - ComponentDataItem2D<T>: row-major std::vector<T> of size i×j;
 *   element (i,j) is at index i*jLength + j.
 *
 * SIMD notes
 * ----------
 * bulkGet/bulkSet use std::copy which auto-vectorizes under -O2 or higher.
 * For strided access wrap the loop in a #pragma omp simd at the call site.
 */

#ifndef HYDROCOUPLESDK_COMPONENTDATAITEM_H
#define HYDROCOUPLESDK_COMPONENTDATAITEM_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"
#include "hydrocouplesdk/data/bufferio.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace HydroCouple::SDK {

// ── ComponentDataItem1D<T> ────────────────────────────────────────────────

/*!
 * \brief 1-D typed data storage mixin.
 *
 * Stores a flat contiguous array of \c T with a configurable default value
 * and implements the v2 typed hyperslab data plane over it.
 *
 * \tparam T  Element type (double, float, int, std::string, …).
 */
template<typename T>
class ComponentDataItem1D
{
public:
    /*!
     * \brief Construct with a given initial length and default value.
     * \param length        Initial element count (≥ 0).
     * \param defaultValue  Value used when resizing upward or resetting.
     */
    explicit ComponentDataItem1D(int length = 0, const T &defaultValue = T{})
        : m_length(length), m_defaultValue(defaultValue), m_data(length, defaultValue)
    {}

    virtual ~ComponentDataItem1D() = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The DataKind of the stored element type.
     */
    [[nodiscard]] static constexpr DataKind storageKind()
    {
        return Helpers::DataKindOf<T>::kind;
    }

    /*!
     * \brief The item's shape: { length }.
     */
    [[nodiscard]] std::vector<int64_t> storageShape() const
    {
        return {static_cast<int64_t>(m_length)};
    }

    /*!
     * \brief Copies a hyperslab of the storage into \p destination
     *        (IComponentDataItem::getValuesInto semantics).
     */
    [[nodiscard]] bool getSlab(const BufferDescriptor &destination,
                               std::span<const int64_t> start,
                               std::span<const int64_t> count,
                               std::string *message = nullptr) const
    {
        const std::array<int64_t, 1> shape{static_cast<int64_t>(m_length)};
        return detail::readHyperslab<T>(m_data.data(), shape, destination,
                                        start, count, message);
    }

    /*!
     * \brief Copies values from \p source into a hyperslab of the storage
     *        (IComponentDataItem::setValuesFrom semantics).
     */
    [[nodiscard]] bool setSlab(const BufferDescriptor &source,
                               std::span<const int64_t> start,
                               std::span<const int64_t> count,
                               std::string *message = nullptr)
    {
        const std::array<int64_t, 1> shape{static_cast<int64_t>(m_length)};
        return detail::writeHyperslab<T>(m_data.data(), shape, source,
                                         start, count, message);
    }

    // ── Direct typed access ───────────────────────────────────────────────

    /*!
     * \brief Element access by index (bounds-checked).
     */
    T &operator[](int index) { return m_data.at(index); }

    /*!
     * \brief Element access by index (bounds-checked, const).
     */
    const T &operator[](int index) const { return m_data.at(index); }

    /*!
     * \brief Returns the current element count.
     */
    [[nodiscard]] int length() const noexcept { return m_length; }

    /*!
     * \brief Returns the default value used when resizing upward or resetting.
     */
    [[nodiscard]] T defaultValue() const { return m_defaultValue; }

    /*!
     * \brief Changes the default value (does not re-fill existing elements).
     */
    void setDefaultValue(const T &value) { m_defaultValue = value; }

    /*!
     * \brief Resets all elements to the default value without changing length.
     */
    void resetDataArray()
    {
        std::fill(m_data.begin(), m_data.end(), m_defaultValue);
    }

    /*!
     * \brief Resizes the array, optionally filling new elements with the default.
     * \param length                New element count.
     * \param initializeWithDefault If true, new slots get \c m_defaultValue.
     */
    virtual void resizeDataArray(int length, bool initializeWithDefault = true)
    {
        int old = m_length;
        m_length = length;
        m_data.resize(length);
        if (initializeWithDefault && length > old)
            std::fill(m_data.begin() + old, m_data.end(), m_defaultValue);
    }

    // ── Raw / SIMD-eligible bulk access ──────────────────────────────────

    /*!
     * \brief Returns a const reference to the underlying vector.
     */
    [[nodiscard]] const std::vector<T> &rawData() const noexcept { return m_data; }

    /*!
     * \brief Returns a mutable reference to the underlying vector.
     */
    [[nodiscard]] std::vector<T> &rawData() noexcept { return m_data; }

    /*!
     * \brief Copies \p count elements starting at \p offset into \p out.
     *
     * Uses std::copy which the compiler auto-vectorizes under -O2/-O3.
     *
     * \param offset  Start index in the internal array.
     * \param count   Number of elements to copy.
     * \param out     Pre-allocated destination buffer of at least \p count elements.
     */
    void bulkGet(int offset, int count, T *out) const noexcept
    {
        std::copy(m_data.begin() + offset, m_data.begin() + offset + count, out);
    }

    /*!
     * \brief Copies \p count elements from \p in into the array starting at \p offset.
     * \param offset  Start index in the internal array.
     * \param count   Number of elements to copy.
     * \param in      Source buffer of at least \p count elements.
     */
    void bulkSet(int offset, int count, const T *in) noexcept
    {
        std::copy(in, in + count, m_data.begin() + offset);
    }

protected:
    /*!
     * \brief Removes the element at index \p i and shrinks the array by one.
     */
    virtual void removeItemAt(int i)
    {
        if (i < 0 || i >= m_length)
            throw std::out_of_range("ComponentDataItem1D::removeItemAt: index out of range");
        m_data.erase(m_data.begin() + i);
        --m_length;
    }

private:
    int            m_length;
    T              m_defaultValue;
    std::vector<T> m_data;
};


// ── ComponentDataItem2D<T> ────────────────────────────────────────────────

/*!
 * \brief 2-D typed data storage mixin (row-major flat vector).
 *
 * Element (i, j) is at index \c i*jLength + j in the flat array, giving
 * cache-friendly sequential access along the j (inner) dimension.
 * Implements the v2 typed hyperslab data plane over the flat storage.
 *
 * \tparam T  Element type (double, float, int, std::string, …).
 */
template<typename T>
class ComponentDataItem2D
{
public:
    /*!
     * \brief Construct with given dimensions and default value.
     * \param iLength       Row count (≥ 0).
     * \param jLength       Column count (≥ 0).
     * \param defaultValue  Value used when resizing or resetting.
     */
    ComponentDataItem2D(int iLength = 0, int jLength = 0, const T &defaultValue = T{})
        : m_iLength(iLength), m_jLength(jLength),
          m_defaultValue(defaultValue),
          m_data(iLength * jLength, defaultValue)
    {}

    virtual ~ComponentDataItem2D() = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The DataKind of the stored element type.
     */
    [[nodiscard]] static constexpr DataKind storageKind()
    {
        return Helpers::DataKindOf<T>::kind;
    }

    /*!
     * \brief The item's shape: { iLength, jLength }.
     */
    [[nodiscard]] std::vector<int64_t> storageShape() const
    {
        return {static_cast<int64_t>(m_iLength),
                static_cast<int64_t>(m_jLength)};
    }

    /*!
     * \brief Copies a hyperslab of the storage into \p destination
     *        (IComponentDataItem::getValuesInto semantics).
     */
    [[nodiscard]] bool getSlab(const BufferDescriptor &destination,
                               std::span<const int64_t> start,
                               std::span<const int64_t> count,
                               std::string *message = nullptr) const
    {
        const std::array<int64_t, 2> shape{static_cast<int64_t>(m_iLength),
                                           static_cast<int64_t>(m_jLength)};
        return detail::readHyperslab<T>(m_data.data(), shape, destination,
                                        start, count, message);
    }

    /*!
     * \brief Copies values from \p source into a hyperslab of the storage
     *        (IComponentDataItem::setValuesFrom semantics).
     */
    [[nodiscard]] bool setSlab(const BufferDescriptor &source,
                               std::span<const int64_t> start,
                               std::span<const int64_t> count,
                               std::string *message = nullptr)
    {
        const std::array<int64_t, 2> shape{static_cast<int64_t>(m_iLength),
                                           static_cast<int64_t>(m_jLength)};
        return detail::writeHyperslab<T>(m_data.data(), shape, source,
                                         start, count, message);
    }

    // ── Direct typed access ───────────────────────────────────────────────

    /*!
     * \brief Element access by (i, j) (bounds-checked).
     */
    T &operator()(int i, int j) { return m_data.at(i * m_jLength + j); }

    /*!
     * \brief Element access by (i, j) (bounds-checked, const).
     */
    const T &operator()(int i, int j) const { return m_data.at(i * m_jLength + j); }

    [[nodiscard]] int iLength() const noexcept { return m_iLength; }
    [[nodiscard]] int jLength() const noexcept { return m_jLength; }

    [[nodiscard]] T defaultValue() const { return m_defaultValue; }
    void setDefaultValue(const T &value) { m_defaultValue = value; }

    /*!
     * \brief Resets all elements to the default value.
     */
    void resetDataArray()
    {
        std::fill(m_data.begin(), m_data.end(), m_defaultValue);
    }

    /*!
     * \brief Resizes the row dimension, optionally filling new rows.
     */
    virtual void resizeDataArrayILength(int iLength, bool initializeWithDefault = true)
    {
        resizeDataArray(iLength, m_jLength, initializeWithDefault);
    }

    /*!
     * \brief Resizes the column dimension, optionally filling new columns.
     */
    virtual void resizeDataArrayJLength(int jLength, bool initializeWithDefault = true)
    {
        resizeDataArray(m_iLength, jLength, initializeWithDefault);
    }

    // ── Raw / SIMD-eligible bulk access ──────────────────────────────────

    [[nodiscard]] const std::vector<T> &rawData() const noexcept { return m_data; }
    [[nodiscard]] std::vector<T>       &rawData()       noexcept { return m_data; }

    /*!
     * \brief Copies one full row i into \p out (jLength elements).
     */
    void bulkGetRow(int i, T *out) const noexcept
    {
        std::copy(m_data.begin() + i * m_jLength,
                  m_data.begin() + (i + 1) * m_jLength, out);
    }

    /*!
     * \brief Copies \p in (jLength elements) into row i.
     */
    void bulkSetRow(int i, const T *in) noexcept
    {
        std::copy(in, in + m_jLength, m_data.begin() + i * m_jLength);
    }

protected:
    virtual void removeIItemAt(int i)
    {
        if (i < 0 || i >= m_iLength)
            throw std::out_of_range("ComponentDataItem2D::removeIItemAt: index out of range");
        m_data.erase(m_data.begin() + i * m_jLength,
                     m_data.begin() + (i + 1) * m_jLength);
        --m_iLength;
    }

    virtual void removeJItemAt(int j)
    {
        if (j < 0 || j >= m_jLength)
            throw std::out_of_range("ComponentDataItem2D::removeJItemAt: index out of range");
        for (int i = m_iLength - 1; i >= 0; --i)
            m_data.erase(m_data.begin() + i * m_jLength + j);
        --m_jLength;
    }

    virtual void resizeDataArray(int iLength, int jLength,
                                 bool initializeWithDefault = true)
    {
        std::vector<T> newData(iLength * jLength,
                               initializeWithDefault ? m_defaultValue : T{});
        int copyI = std::min(m_iLength, iLength);
        int copyJ = std::min(m_jLength, jLength);
        for (int i = 0; i < copyI; ++i)
            std::copy(m_data.begin() + i * m_jLength,
                      m_data.begin() + i * m_jLength + copyJ,
                      newData.begin() + i * jLength);
        m_iLength = iLength;
        m_jLength = jLength;
        m_data    = std::move(newData);
    }

private:
    int            m_iLength;
    int            m_jLength;
    T              m_defaultValue;
    std::vector<T> m_data;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_COMPONENTDATAITEM_H
