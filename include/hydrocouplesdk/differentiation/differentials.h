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

//! Number of elements in an item of the given shape.
inline int64_t elementCount(std::span<const int64_t> shape)
{
    int64_t n = 1;
    for (int64_t extent : shape)
        n *= extent;
    return n;
}

//! A host Float64 BufferDescriptor over \p storage, shaped \p shape. Both
//! must outlive the descriptor.
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

//! Copies a whole-item derivative buffer into row-major \p out.
inline bool readDifferential(const DifferentialEntry &entry,
                             const std::vector<int64_t> &itemShape,
                             std::vector<double> &out, std::string *message)
{
    out.assign(static_cast<size_t>(elementCount(itemShape)), 0.0);
    const std::vector<int64_t> start(itemShape.size(), 0);
    return detail::writeHyperslab<double>(out.data(), itemShape, entry.value,
                                          start, itemShape, message);
}

//! Overwrites a whole-item derivative buffer with row-major \p values.
inline bool writeDifferential(const DifferentialEntry &entry,
                              const std::vector<int64_t> &itemShape,
                              const std::vector<double> &values,
                              std::string *message)
{
    const std::vector<int64_t> start(itemShape.size(), 0);
    return detail::readHyperslab<double>(values.data(), itemShape, entry.value,
                                         start, itemShape, message);
}

//! The IComponentDataItem identity of any data item, as the interface's
//! DifferentialEntry::item compares it.
template <typename T>
inline const IComponentDataItem *itemOf(const T *item)
{
    return static_cast<const IComponentDataItem *>(item);
}

} // namespace HydroCouple::SDK::Differentiation

#endif // HYDROCOUPLESDK_DIFFERENTIALS_H
