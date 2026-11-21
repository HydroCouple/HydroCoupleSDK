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
 * \tparam T        Element type; DataKindOf<T>::kind must equal the item's kind.
 * \tparam ToBuffer true = read from storage into the buffer;
 *                  false = write from the buffer into storage.
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
