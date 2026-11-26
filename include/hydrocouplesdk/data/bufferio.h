// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   bufferio.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Shared hyperslab copy engine for the v2 typed data plane.
 *
 * Implements the IComponentDataItem::getValuesInto()/setValuesFrom()
 * contract once, for any element type and rank, between a data item's
 * row-major contiguous storage and a caller-described BufferDescriptor
 * (which may be strided).
 *
 * Fast path: when the descriptor is C-contiguous each innermost run is
 * copied with std::copy (memcpy for trivially copyable types), which the
 * compiler auto-vectorizes. Strided destinations fall back to per-element
 * addressing via Helpers::byteOffset.
 *
 * DataKind::String buffers point to arrays of std::string and are host-only,
 * per the standard; they use element-wise assignment.
 */

#ifndef HYDROCOUPLESDK_BUFFERIO_H
#define HYDROCOUPLESDK_BUFFERIO_H

#include "hydrocouple.h"
#include "hydrocouplehelpers.h"

#include <algorithm>
#include <cstring>
#include <span>
#include <string>
#include <vector>

namespace HydroCouple::SDK {
namespace detail {

/*!
 * \brief Validates a hyperslab selection against an item's shape and a
 *        caller buffer. Returns an empty string on success.
 * \details Checks, in this order, and returns the first failure: the
 *          buffer's kind equals \p itemKind ("kind mismatch"); the buffer is
 *          in MemorySpace::Host ("unsupported memory space"); \p start and
 *          \p count each have one entry per dimension of \p itemShape
 *          ("selection rank mismatch"); every start and count is
 *          non-negative and start + count stays within the extent
 *          ("selection out of bounds"); the buffer's element count (the
 *          product of its shape, 1 for rank 0) equals the product of
 *          \p count ("buffer element count mismatch"). The buffer's rank
 *          and itemSizeBytes are not checked, so a destination may have any
 *          rank whose element count matches.
 * \param[in] itemShape Extent of each dimension of the item's storage.
 * \param[in] buffer    The caller's descriptor.
 * \param[in] itemKind  The item's element kind.
 * \param[in] start     First index of the selection per dimension.
 * \param[in] count     Extent of the selection per dimension.
 * \return An empty string when the selection is valid, otherwise one of
 *         the messages above.
 */
inline std::string validateSelection(std::span<const int64_t> itemShape,
                                     const BufferDescriptor &buffer,
                                     DataKind itemKind,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count)
{
    if (buffer.kind != itemKind)
        return "kind mismatch";
    if (buffer.space != MemorySpace::Host)
        return "unsupported memory space";
    if (start.size() != itemShape.size() || count.size() != itemShape.size())
        return "selection rank mismatch";
    int64_t selected = 1;
    for (size_t k = 0; k < itemShape.size(); ++k)
    {
        if (start[k] < 0 || count[k] < 0 || start[k] + count[k] > itemShape[k])
            return "selection out of bounds";
        selected *= count[k];
    }
    if (Helpers::elementCount(buffer) != selected)
        return "buffer element count mismatch";
    return {};
}

/*!
 * \brief Builds a C-contiguous host BufferDescriptor over \p data with the
 *        given element kind and shape. \p shape must outlive the descriptor.
 * \details itemSizeBytes is left 0 (the kind's natural size), strides null.
 *          Helpers::makeContiguous() in hydrocouplehelpers.h builds the same
 *          descriptor with itemSizeBytes set.
 * \param[in] data  Base address of the caller's buffer (not owned).
 * \param[in] kind  Element kind of the buffer.
 * \param[in] shape Extent per dimension; the descriptor points into it.
 * \return The descriptor.
 */
inline BufferDescriptor hostDescriptor(void *data, DataKind kind,
                                       std::span<const int64_t> shape)
{
    BufferDescriptor d;
    d.data         = data;
    d.kind         = kind;
    d.rank         = static_cast<int32_t>(shape.size());
    d.shape        = shape.data();
    d.stridesBytes = nullptr; // C-contiguous
    d.space        = MemorySpace::Host;
    return d;
}

/*!
 * \brief Row-major linear offset of \p index within \p shape.
 * \param[in] shape Extent per dimension.
 * \param[in] index Element index per dimension; same length as \p shape,
 *                  not range-checked.
 * \return The element offset (not bytes) in C order.
 */
inline int64_t linearOffset(std::span<const int64_t> shape,
                            std::span<const int64_t> index)
{
    int64_t offset = 0;
    for (size_t k = 0; k < shape.size(); ++k)
        offset = offset * shape[k] + index[k];
    return offset;
}

/*!
 * \brief Copies a hyperslab between row-major contiguous item storage and a
 *        caller buffer, in either direction.
 *
 * \details Validates with validateSelection() first and copies nothing on
 *          failure. A rank-0 item copies its single element. An empty
 *          selection (a zero count) succeeds without touching memory. A
 *          C-contiguous buffer is filled one innermost run at a time with
 *          std::copy; a strided buffer is addressed element by element
 *          through Helpers::byteOffset(). The buffer is indexed by the
 *          selection's own coordinates, so element (0, ..., 0) of the
 *          buffer receives element \p start of the item.
 *
 * \tparam T        Element type; DataKindOf<T>::kind must equal the item's kind.
 * \tparam ToBuffer true = read from storage into the buffer;
 *                  false = write from the buffer into storage.
 * \param[in,out] storage   The item's row-major storage, at least the
 *                          product of \p itemShape elements.
 * \param[in]     itemShape Extent of each dimension of the storage.
 * \param[in]     buffer    The caller's descriptor (host memory).
 * \param[in]     start     First index of the selection per dimension.
 * \param[in]     count     Extent of the selection per dimension.
 * \param[out]    message   Receives the validation message on failure; may
 *                          be nullptr.
 * \return true when the selection was copied.
 */
template <typename T, bool ToBuffer>
bool copyHyperslab(T *storage,
                   std::span<const int64_t> itemShape,
                   const BufferDescriptor &buffer,
                   std::span<const int64_t> start,
                   std::span<const int64_t> count,
                   std::string *message)
{
    if (std::string error = validateSelection(
            itemShape, buffer, Helpers::DataKindOf<T>::kind, start, count);
        !error.empty())
    {
        if (message) *message = std::move(error);
        return false;
    }

    const size_t rank = itemShape.size();
    T *bufferBase = static_cast<T *>(buffer.data);

    if (rank == 0)
    {
        if constexpr (ToBuffer)
            *bufferBase = *storage;
        else
            *storage = *bufferBase;
        return true;
    }

    const int64_t innerCount = count[rank - 1];
    int64_t rows = 1;
    for (size_t k = 0; k + 1 < rank; ++k)
        rows *= count[k];
    if (rows == 0 || innerCount == 0)
        return true; // empty selection

    const bool bufferContiguous = Helpers::isContiguous(buffer);

    std::vector<int64_t> cursor(rank, 0);   // offsets within the selection
    std::vector<int64_t> itemIndex(rank, 0);
    int64_t bufferLinear = 0;

    for (int64_t row = 0; row < rows; ++row)
    {
        for (size_t k = 0; k < rank; ++k)
            itemIndex[k] = start[k] + cursor[k];
        T *storageRow = storage + linearOffset(itemShape, itemIndex);

        if (bufferContiguous)
        {
            if constexpr (ToBuffer)
                std::copy(storageRow, storageRow + innerCount,
                          bufferBase + bufferLinear);
            else
                std::copy(bufferBase + bufferLinear,
                          bufferBase + bufferLinear + innerCount, storageRow);
            bufferLinear += innerCount;
        }
        else
        {
            for (int64_t j = 0; j < innerCount; ++j)
            {
                cursor[rank - 1] = j;
                const int64_t byte = Helpers::byteOffset(
                    buffer, std::span<const int64_t>(cursor.data(), rank));
                T *element = reinterpret_cast<T *>(
                    reinterpret_cast<char *>(buffer.data) + byte);
                if constexpr (ToBuffer)
                    *element = storageRow[j];
                else
                    storageRow[j] = *element;
            }
            cursor[rank - 1] = 0;
        }

        // Advance the outer-dimension odometer (dimensions [0, rank-1)).
        for (size_t k = rank - 1; k-- > 0;)
        {
            if (++cursor[k] < count[k])
                break;
            cursor[k] = 0;
        }
    }
    return true;
}

/*!
 * \brief Copies a hyperslab from row-major contiguous \p storage into the
 *        caller-described \p destination buffer. Returns true on success.
 * \details The common implementation of getValuesInto() for every typed
 *          SDK item; see copyHyperslab() for the copy and validateSelection()
 *          for the failures.
 * \tparam T Element type of the storage.
 * \param[in]  storage     The item's row-major storage (read only).
 * \param[in]  itemShape   Extent of each dimension of the storage.
 * \param[in]  destination The caller's buffer descriptor.
 * \param[in]  start       First index of the selection per dimension.
 * \param[in]  count       Extent of the selection per dimension.
 * \param[out] message     Failure description; may be nullptr.
 * \return true when the values were copied.
 */
template <typename T>
bool readHyperslab(const T *storage,
                   std::span<const int64_t> itemShape,
                   const BufferDescriptor &destination,
                   std::span<const int64_t> start,
                   std::span<const int64_t> count,
                   std::string *message)
{
    return copyHyperslab<T, true>(const_cast<T *>(storage), itemShape,
                                  destination, start, count, message);
}

/*!
 * \brief Copies a hyperslab from the caller-described \p source buffer into
 *        row-major contiguous \p storage. Returns true on success.
 * \details The common implementation of setValuesFrom() for every typed
 *          SDK item; it fires no signal (the item does that after success).
 * \tparam T Element type of the storage.
 * \param[in,out] storage   The item's row-major storage.
 * \param[in]     itemShape Extent of each dimension of the storage.
 * \param[in]     source    The caller's buffer descriptor.
 * \param[in]     start     First index of the selection per dimension.
 * \param[in]     count     Extent of the selection per dimension.
 * \param[out]    message   Failure description; may be nullptr.
 * \return true when the values were stored.
 */
template <typename T>
bool writeHyperslab(T *storage,
                    std::span<const int64_t> itemShape,
                    const BufferDescriptor &source,
                    std::span<const int64_t> start,
                    std::span<const int64_t> count,
                    std::string *message)
{
    return copyHyperslab<T, false>(storage, itemShape, source, start, count,
                                   message);
}

} // namespace detail
} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_BUFFERIO_H
