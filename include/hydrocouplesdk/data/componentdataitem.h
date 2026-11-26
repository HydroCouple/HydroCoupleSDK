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
 * For strided access wrap the loop in a `#pragma omp simd` at the call site.
 *
 * Two access paths
 * ----------------
 * The standard data plane (getSlab()/setSlab(), which the typed items expose
 * as getValuesInto()/setValuesFrom()) validates every request and is what
 * other components use. The direct accessors (operator[], operator(),
 * rawData(), bulkGet()/bulkSet()) are for the owning component's own code:
 * they bypass validation, and writes through them fire no value-changed
 * signal.
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
 * \c T needs a DataKind (Helpers::DataKindOf) and contiguous std::vector
 * storage, so \c bool does not compile (std::vector of bool has no data());
 * store \c uint8_t instead.
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
     * \details Every element starts at \p defaultValue. A negative length
     *          is not checked and makes the std::vector constructor throw.
     */
    explicit ComponentDataItem1D(int length = 0, const T &defaultValue = T{})
        : m_length(length), m_defaultValue(defaultValue), m_data(length, defaultValue)
    {}

    //! Destroys the storage.
    virtual ~ComponentDataItem1D() = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The DataKind of the stored element type.
     * \return Helpers::DataKindOf of \c T (Int32 for \c int, Float64 for
     *         \c double, String for \c std::string).
     */
    [[nodiscard]] static constexpr DataKind storageKind()
    {
        return Helpers::DataKindOf<T>::kind;
    }

    /*!
     * \brief The item's shape: { length }.
     * \return A one-element vector holding length().
     */
    [[nodiscard]] std::vector<int64_t> storageShape() const
    {
        return {static_cast<int64_t>(m_length)};
    }

    /*!
     * \brief Copies a hyperslab of the storage into \p destination
     *        (IComponentDataItem::getValuesInto semantics).
     * \param[in]  destination Caller's host buffer of kind storageKind().
     * \param[in]  start       First index (one entry).
     * \param[in]  count       Number of elements (one entry).
     * \param[out] message     Failure description (see
     *                         detail::validateSelection()); may be nullptr.
     * \return true when the values were copied.
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
     * \details Fires nothing: the mixin has no signal. The typed items call
     *          it and then emit value-changed.
     * \param[in]  source  Caller's host buffer of kind storageKind().
     * \param[in]  start   First index (one entry).
     * \param[in]  count   Number of elements (one entry).
     * \param[out] message Failure description; may be nullptr.
     * \return true when the values were stored.
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
     * \param[in] index Zero-based element index.
     * \return A reference into the storage, valid until the next resize.
     *         Writing through it fires no value-changed signal.
     * \exception std::out_of_range when \p index is outside [0, length()).
     */
    T &operator[](int index) { return m_data.at(index); }

    /*!
     * \brief Element access by index (bounds-checked, const).
     * \param[in] index Zero-based element index.
     * \return A const reference into the storage.
     * \exception std::out_of_range when \p index is outside [0, length()).
     */
    const T &operator[](int index) const { return m_data.at(index); }

    /*!
     * \brief Returns the current element count.
     * \return The length, as set by the constructor or resizeDataArray().
     */
    [[nodiscard]] int length() const noexcept { return m_length; }

    /*!
     * \brief Returns the default value used when resizing upward or resetting.
     * \return A copy of the default value.
     */
    [[nodiscard]] T defaultValue() const { return m_defaultValue; }

    /*!
     * \brief Changes the default value (does not re-fill existing elements).
     * \param[in] value New default for later resizes and resetDataArray().
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
     * \details Existing elements up to the new length are kept. With
     *          \p initializeWithDefault false, new elements are
     *          value-initialized (\c T{}). Fires no signal; shape() reports
     *          the new length at once.
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
     * \return The storage, length() elements in index order.
     */
    [[nodiscard]] const std::vector<T> &rawData() const noexcept { return m_data; }

    /*!
     * \brief Returns a mutable reference to the underlying vector.
     * \return The storage. Changing its size directly leaves length() stale;
     *         resize through resizeDataArray().
     */
    [[nodiscard]] std::vector<T> &rawData() noexcept { return m_data; }

    /*!
     * \brief Copies \p count elements starting at \p offset into \p out.
     *
     * Uses std::copy which the compiler auto-vectorizes under -O2/-O3.
     *
     * \details Not bounds-checked: \p offset + \p count must not exceed
     *          length().
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
     * \details Not bounds-checked, and fires no value-changed signal.
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
     * \param[in] i Zero-based index of the element to remove.
     * \exception std::out_of_range when \p i is outside [0, length()).
     */
    virtual void removeItemAt(int i)
    {
        if (i < 0 || i >= m_length)
            throw std::out_of_range("ComponentDataItem1D::removeItemAt: index out of range");
        m_data.erase(m_data.begin() + i);
        --m_length;
    }

private:
    int            m_length;       //!< Element count.
    T              m_defaultValue; //!< Fill value for resizes and resets.
    std::vector<T> m_data;         //!< The elements, in index order.
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
     * \details Every element starts at \p defaultValue.
     */
    ComponentDataItem2D(int iLength = 0, int jLength = 0, const T &defaultValue = T{})
        : m_iLength(iLength), m_jLength(jLength),
          m_defaultValue(defaultValue),
          m_data(iLength * jLength, defaultValue)
    {}

    //! Destroys the storage.
    virtual ~ComponentDataItem2D() = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The DataKind of the stored element type.
     * \return Helpers::DataKindOf of \c T.
     */
    [[nodiscard]] static constexpr DataKind storageKind()
    {
        return Helpers::DataKindOf<T>::kind;
    }

    /*!
     * \brief The item's shape: { iLength, jLength }.
     * \return Rows then columns; dimension 0 is the row (i) index.
     */
    [[nodiscard]] std::vector<int64_t> storageShape() const
    {
        return {static_cast<int64_t>(m_iLength),
                static_cast<int64_t>(m_jLength)};
    }

    /*!
     * \brief Copies a hyperslab of the storage into \p destination
     *        (IComponentDataItem::getValuesInto semantics).
     * \param[in]  destination Caller's host buffer of kind storageKind();
     *                         receives the block in row-major order.
     * \param[in]  start       First row and column.
     * \param[in]  count       Number of rows and columns.
     * \param[out] message     Failure description; may be nullptr.
     * \return true when the values were copied.
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
     * \details Fires nothing: the mixin has no signal.
     * \param[in]  source  Caller's host buffer of kind storageKind(), in
     *                     row-major order.
     * \param[in]  start   First row and column.
     * \param[in]  count   Number of rows and columns.
     * \param[out] message Failure description; may be nullptr.
     * \return true when the values were stored.
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
     * \details Only the flat index i * jLength() + j is checked, so a column
     *          index past jLength() reaches the next row instead of throwing.
     * \param[in] i Row index.
     * \param[in] j Column index.
     * \return A reference into the storage; writes fire no signal.
     * \exception std::out_of_range when the flat index is outside the storage.
     */
    T &operator()(int i, int j) { return m_data.at(i * m_jLength + j); }

    /*!
     * \brief Element access by (i, j) (bounds-checked, const).
     * \param[in] i Row index.
     * \param[in] j Column index.
     * \return A const reference into the storage.
     * \exception std::out_of_range when the flat index is outside the storage.
     */
    const T &operator()(int i, int j) const { return m_data.at(i * m_jLength + j); }

    //! Number of rows (dimension 0).
    //! \return The row count.
    [[nodiscard]] int iLength() const noexcept { return m_iLength; }
    //! Number of columns (dimension 1).
    //! \return The column count.
    [[nodiscard]] int jLength() const noexcept { return m_jLength; }

    //! The value used when resizing or resetting.
    //! \return A copy of the default value.
    [[nodiscard]] T defaultValue() const { return m_defaultValue; }
    //! Changes the default value; existing elements are not re-filled.
    //! \param[in] value New default.
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
     * \param[in] iLength               New row count.
     * \param[in] initializeWithDefault If true, new rows get the default;
     *                                  otherwise \c T{}.
     */
    virtual void resizeDataArrayILength(int iLength, bool initializeWithDefault = true)
    {
        resizeDataArray(iLength, m_jLength, initializeWithDefault);
    }

    /*!
     * \brief Resizes the column dimension, optionally filling new columns.
     * \param[in] jLength               New column count; existing values stay
     *                                  at their (row, column) position.
     * \param[in] initializeWithDefault If true, new columns get the default;
     *                                  otherwise \c T{}.
     */
    virtual void resizeDataArrayJLength(int jLength, bool initializeWithDefault = true)
    {
        resizeDataArray(m_iLength, jLength, initializeWithDefault);
    }

    // ── Raw / SIMD-eligible bulk access ──────────────────────────────────

    //! The storage, row-major: element (i, j) at i * jLength() + j.
    //! \return A const reference to the vector.
    [[nodiscard]] const std::vector<T> &rawData() const noexcept { return m_data; }
    //! The storage, row-major, for direct writes (no signal).
    //! \return A mutable reference; do not change its size.
    [[nodiscard]] std::vector<T>       &rawData()       noexcept { return m_data; }

    /*!
     * \brief Copies one full row i into \p out (jLength elements).
     * \param[in]  i   Row index; not bounds-checked.
     * \param[out] out Buffer of at least jLength() elements.
     */
    void bulkGetRow(int i, T *out) const noexcept
    {
        std::copy(m_data.begin() + i * m_jLength,
                  m_data.begin() + (i + 1) * m_jLength, out);
    }

    /*!
     * \brief Copies \p in (jLength elements) into row i.
     * \details Not bounds-checked, and fires no signal.
     * \param[in] i  Row index.
     * \param[in] in Buffer of at least jLength() elements.
     */
    void bulkSetRow(int i, const T *in) noexcept
    {
        std::copy(in, in + m_jLength, m_data.begin() + i * m_jLength);
    }

protected:
    /*!
     * \brief Removes row \p i and shrinks the row count by one.
     * \param[in] i Row index.
     * \exception std::out_of_range when \p i is outside [0, iLength()).
     */
    virtual void removeIItemAt(int i)
    {
        if (i < 0 || i >= m_iLength)
            throw std::out_of_range("ComponentDataItem2D::removeIItemAt: index out of range");
        m_data.erase(m_data.begin() + i * m_jLength,
                     m_data.begin() + (i + 1) * m_jLength);
        --m_iLength;
    }

    /*!
     * \brief Removes column \p j from every row and shrinks the column count by one.
     * \param[in] j Column index.
     * \exception std::out_of_range when \p j is outside [0, jLength()).
     */
    virtual void removeJItemAt(int j)
    {
        if (j < 0 || j >= m_jLength)
            throw std::out_of_range("ComponentDataItem2D::removeJItemAt: index out of range");
        for (int i = m_iLength - 1; i >= 0; --i)
            m_data.erase(m_data.begin() + i * m_jLength + j);
        --m_jLength;
    }

    /*!
     * \brief Resizes both dimensions, keeping the overlapping block in place.
     * \details Builds new storage filled with the default (or \c T{}) and
     *          copies the rows and columns common to the old and new shapes.
     * \param[in] iLength               New row count.
     * \param[in] jLength               New column count.
     * \param[in] initializeWithDefault Fill value choice for new elements.
     */
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
    int            m_iLength;      //!< Row count.
    int            m_jLength;      //!< Column count.
    T              m_defaultValue; //!< Fill value for resizes and resets.
    std::vector<T> m_data;         //!< Row-major elements.
};

// The common specializations live in the library. On Windows its exported
// items (Argument, Input and Output, 1-D and 2-D) export these bases with
// them, so every translation unit has to see them declared here: one that
// includes only this header would otherwise compile its own copy of a member
// that another one imports from the DLL, and the link fails (LNK2005).
extern template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<int>;
extern template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<double>;
extern template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<std::string>;
extern template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<int>;
extern template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<double>;
extern template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<std::string>;

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_COMPONENTDATAITEM_H
