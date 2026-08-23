// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/device/spacetransfer.h"
#include "hydrocouplehelpers.h"

#include <cstring>
#include <vector>

namespace HydroCouple::SDK::Device {

namespace {

bool hostAccessible(const BufferDescriptor &descriptor)
{
    return descriptor.space == MemorySpace::Host ||
           descriptor.space == MemorySpace::HostPinned ||
           descriptor.space == MemorySpace::Unified;
}

int64_t elementCount(const BufferDescriptor &descriptor)
{
    int64_t count = 1;
    for (int32_t d = 0; d < descriptor.rank; ++d)
        count *= descriptor.shape[d];
    return count;
}

/*!
 * Element-wise odometer copy between two host descriptors (either or both
 * may be strided). \p elementSize bytes per element.
 */
void odometerCopy(const BufferDescriptor &source,
                  const BufferDescriptor &destination, int64_t elementSize)
{
    const int64_t count = elementCount(source);
    if (count == 0)
        return;

    // Packed row-major steps for a side without explicit strides.
    auto strideOf = [elementSize](const BufferDescriptor &d,
                                  std::vector<int64_t> &packedStorage)
        -> const int64_t * {
        if (d.stridesBytes)
            return d.stridesBytes;
        packedStorage.assign(static_cast<size_t>(d.rank), elementSize);
        for (int32_t k = d.rank - 2; k >= 0; --k)
            packedStorage[static_cast<size_t>(k)] =
                packedStorage[static_cast<size_t>(k + 1)] * d.shape[k + 1];
        return packedStorage.data();
    };

    std::vector<int64_t> sourcePacked, destinationPacked;
    const int64_t *sourceStrides      = strideOf(source, sourcePacked);
    const int64_t *destinationStrides =
        strideOf(destination, destinationPacked);

    // The destination is addressed through the source's logical shape:
    // negotiation guarantees equal element counts, and a flat index maps
    // row-major into whatever shape the destination declares.
    std::vector<int64_t> index(static_cast<size_t>(source.rank), 0);
    auto *sourceBytes = static_cast<const std::byte *>(source.data);
    auto *destinationBytes = static_cast<std::byte *>(destination.data);

    for (int64_t flat = 0; flat < count; ++flat)
    {
        int64_t sourceOffset = 0;
        for (int32_t k = 0; k < source.rank; ++k)
            sourceOffset += index[static_cast<size_t>(k)] * sourceStrides[k];

        int64_t destinationOffset = 0;
        int64_t remainder         = flat;
        for (int32_t k = destination.rank - 1; k >= 0; --k)
        {
            const int64_t extent = destination.shape[k];
            destinationOffset += (remainder % extent) * destinationStrides[k];
            remainder /= extent;
        }

        std::memcpy(destinationBytes + destinationOffset,
                    sourceBytes + sourceOffset,
                    static_cast<size_t>(elementSize));

        for (int32_t k = source.rank - 1; k >= 0; --k)
        {
            if (++index[static_cast<size_t>(k)] < source.shape[k])
                break;
            index[static_cast<size_t>(k)] = 0;
        }
    }
}

/*!
 * Gather a (possibly strided) host descriptor into packed bytes, or
 * scatter packed bytes into it.
 */
void gatherToPacked(const BufferDescriptor &source, std::byte *packed,
                    int64_t elementSize)
{
    BufferDescriptor packedDescriptor = source;
    packedDescriptor.data             = packed;
    packedDescriptor.stridesBytes     = nullptr;
    odometerCopy(source, packedDescriptor, elementSize);
}

void scatterFromPacked(const std::byte *packed,
                       const BufferDescriptor &destination,
                       int64_t elementSize)
{
    BufferDescriptor packedDescriptor = destination;
    packedDescriptor.data = const_cast<std::byte *>(packed);
    packedDescriptor.stridesBytes = nullptr;
    odometerCopy(packedDescriptor, destination, elementSize);
}

} // namespace

bool isContiguous(const BufferDescriptor &descriptor)
{
    if (!descriptor.stridesBytes)
        return true;
    const int64_t elementSize = Helpers::dataKindSize(descriptor.kind);
    int64_t expected = elementSize;
    for (int32_t k = descriptor.rank - 1; k >= 0; --k)
    {
        if (descriptor.shape[k] > 1 &&
            descriptor.stridesBytes[k] != expected)
            return false;
        expected *= descriptor.shape[k];
    }
    return true;
}

CopyPath negotiateCopyPath(const BufferDescriptor &source,
                           const BufferDescriptor &destination,
                           const DeviceBackend &backend, std::string &message)
{
    if (source.kind != destination.kind)
    {
        message = "space transfer: element kinds differ";
        return CopyPath::Unsupported;
    }
    const int64_t elementSize = Helpers::dataKindSize(source.kind);
    if (elementSize <= 0)
    {
        message = "space transfer: kind has no fixed width";
        return CopyPath::Unsupported;
    }
    if (elementCount(source) != elementCount(destination))
    {
        message = "space transfer: element counts differ";
        return CopyPath::Unsupported;
    }

    const bool sourceHost      = hostAccessible(source);
    const bool destinationHost = hostAccessible(destination);

    if (sourceHost && destinationHost)
    {
        message.clear();
        return CopyPath::HostHyperslab;
    }

    // A device endpoint is involved: the backend must support each
    // non-host space, and device views must be contiguous.
    if (!sourceHost && !backend.supportsSpace(source.space, source.deviceId))
    {
        message = "space transfer: backend '" + backend.name() +
                  "' does not support the source space";
        return CopyPath::Unsupported;
    }
    if (!destinationHost &&
        !backend.supportsSpace(destination.space, destination.deviceId))
    {
        message = "space transfer: backend '" + backend.name() +
                  "' does not support the destination space";
        return CopyPath::Unsupported;
    }
    if (!sourceHost && !isContiguous(source))
    {
        message = "space transfer: strided device source is not supported";
        return CopyPath::Unsupported;
    }
    if (!destinationHost && !isContiguous(destination))
    {
        message =
            "space transfer: strided device destination is not supported";
        return CopyPath::Unsupported;
    }

    const bool sourcePacked      = isContiguous(source);
    const bool destinationPacked = isContiguous(destination);
    message.clear();
    if (sourcePacked && destinationPacked)
        return CopyPath::BackendCopy;
    return CopyPath::StageThroughHost;
}

bool copyBuffer(DeviceBackend &backend, const BufferDescriptor &source,
                const BufferDescriptor &destination, std::string &message)
{
    const CopyPath path =
        negotiateCopyPath(source, destination, backend, message);
    if (path == CopyPath::Unsupported)
        return false;

    const int64_t elementSize = Helpers::dataKindSize(source.kind);
    const int64_t bytes       = elementCount(source) * elementSize;

    switch (path)
    {
        case CopyPath::HostHyperslab:
        {
            if (isContiguous(source) && isContiguous(destination))
                std::memcpy(destination.data, source.data,
                            static_cast<size_t>(bytes));
            else
                odometerCopy(source, destination, elementSize);
            return true;
        }
        case CopyPath::BackendCopy:
        {
            return backend.copy(destination.data, destination.space,
                                destination.deviceId, source.data,
                                source.space, source.deviceId,
                                static_cast<size_t>(bytes), &message);
        }
        case CopyPath::StageThroughHost:
        {
            std::vector<std::byte> staging(static_cast<size_t>(bytes));
            if (hostAccessible(source))
            {
                gatherToPacked(source, staging.data(), elementSize);
                return backend.copy(destination.data, destination.space,
                                    destination.deviceId, staging.data(),
                                    MemorySpace::Host, 0,
                                    static_cast<size_t>(bytes), &message);
            }
            if (!backend.copy(staging.data(), MemorySpace::Host, 0,
                              source.data, source.space, source.deviceId,
                              static_cast<size_t>(bytes), &message))
                return false;
            scatterFromPacked(staging.data(), destination, elementSize);
            return true;
        }
        case CopyPath::Unsupported:
            break;
    }
    return false;
}

} // namespace HydroCouple::SDK::Device
