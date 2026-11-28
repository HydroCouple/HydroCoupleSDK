// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
/*!
 * \file   differentials.h
 * \brief  Buffers for the differentiation contract (Phase G2.1).
 *
 * The interface's DifferentialEntry pairs an item with a whole-item
 * BufferDescriptor. These helpers move such a buffer to and from
 * row-major host storage through the same hyperslab code the data plane
 * uses, so a strided or pitched cotangent buffer is honored exactly as a
 * strided data-plane buffer is. Float64 only in G2.1.
 *
 * A component author uses them inside vjp() and jvp(): readDifferential()
 * turns a seed entry into a row-major vector, writeDifferential() writes a
 * computed result into a result entry, and itemOf() gives the identity an
 * entry's \c item is compared with. All are header-only.
 */
#ifndef HYDROCOUPLESDK_DIFFERENTIALS_H
#define HYDROCOUPLESDK_DIFFERENTIALS_H

#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouple.h"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Differentiation {

/*!
 * \brief Number of elements in an item of the given shape.
 * \param[in] shape Extents (empty for a scalar).
 * \return The product of the extents (1 for an empty shape).
 */
inline int64_t elementCount(std::span<const int64_t> shape)
{
    int64_t n = 1;
    for (int64_t extent : shape)
        n *= extent;
    return n;
}

/*!
 * \brief A host Float64 BufferDescriptor over \p storage, shaped \p shape.
 *        Both must outlive the descriptor.
 * \param[in] storage Row-major values; its size should equal
 *                    elementCount(\p shape) (not checked).
 * \param[in] shape   Extents; empty gives a rank-0 (scalar) descriptor.
 * \return A contiguous host descriptor of kind Float64.
 */
inline BufferDescriptor hostBuffer(std::vector<double> &storage,
                                   const std::vector<int64_t> &shape)
{
    BufferDescriptor d;
    d.data = storage.data();
    d.kind = DataKind::Float64;
    d.rank = static_cast<int32_t>(shape.size());
    d.shape = shape.empty() ? nullptr : shape.data();
    d.stridesBytes = nullptr;
    d.space = MemorySpace::Host;
    return d;
}

/*!
 * \brief Copies a whole-item derivative buffer into row-major \p out.
 * \param[in]  entry     Seed or result entry; its buffer must be host,
 *                       Float64 and hold elementCount(\p itemShape)
 *                       elements (strides allowed).
 * \param[in]  itemShape Shape of the entry's item.
 * \param[out] out       Resized to the element count and filled.
 * \param[out] message   Optional; the hyperslab engine's reason on failure
 *                       ("kind mismatch", "unsupported memory space",
 *                       "buffer element count mismatch", ...).
 * \return True on success.
 */
inline bool readDifferential(const DifferentialEntry &entry,
                             const std::vector<int64_t> &itemShape,
                             std::vector<double> &out, std::string *message)
{
    out.assign(static_cast<size_t>(elementCount(itemShape)), 0.0);
    const std::vector<int64_t> start(itemShape.size(), 0);
    return detail::writeHyperslab<double>(out.data(), itemShape, entry.value,
                                          start, itemShape, message);
}

/*!
 * \brief Overwrites a whole-item derivative buffer with row-major \p values.
 * \param[in]  entry     Result entry whose buffer receives the values (host,
 *                       Float64, the item's element count; strides allowed).
 * \param[in]  itemShape Shape of the entry's item.
 * \param[in]  values    Row-major values, elementCount(\p itemShape) of
 *                       them.
 * \param[out] message   Optional; the hyperslab engine's reason on failure.
 * \return True on success.
 */
inline bool writeDifferential(const DifferentialEntry &entry,
                              const std::vector<int64_t> &itemShape,
                              const std::vector<double> &values,
                              std::string *message)
{
    const std::vector<int64_t> start(itemShape.size(), 0);
    return detail::readHyperslab<double>(values.data(), itemShape, entry.value,
                                         start, itemShape, message);
}

/*!
 * \brief The IComponentDataItem identity of any data item, as the
 *        interface's DifferentialEntry::item compares it.
 *
 * Compare an entry's \c item with itemOf(myInput) rather than with the
 * typed pointer: with virtual inheritance the two addresses can differ.
 *
 * \tparam T Any type derived from IComponentDataItem.
 * \param[in] item The item (may be null).
 * \return \p item converted to const IComponentDataItem*.
 */
template <typename T>
inline const IComponentDataItem *itemOf(const T *item)
{
    return static_cast<const IComponentDataItem *>(item);
}

} // namespace HydroCouple::SDK::Differentiation

#endif // HYDROCOUPLESDK_DIFFERENTIALS_H
