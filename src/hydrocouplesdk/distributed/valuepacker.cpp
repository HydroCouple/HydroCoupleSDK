// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/distributed/valuepacker.h"
#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplehelpers.h"

#include <cstring>

namespace HydroCouple::SDK::Distributed {

namespace {

/*!
 * Dispatches a hyperslab copy between a typed array and a descriptor for
 * every fixed-width DataKind. \p ToWire selects direction.
 */
template <bool ToWire>
bool dispatchCopy(DataKind kind, std::byte *wireElements,
                  std::span<const int64_t> shape,
                  const BufferDescriptor &buffer, std::string *message)
{
    const std::vector<int64_t> start(shape.size(), 0);
    switch (kind)
    {
#define HC_PACK_CASE(K, T)                                                   \
    case DataKind::K:                                                        \
        if constexpr (ToWire)                                                \
            return HydroCouple::SDK::detail::writeHyperslab<T>(              \
                reinterpret_cast<T *>(wireElements), shape, buffer, start,   \
                shape, message);                                             \
        else                                                                 \
            return HydroCouple::SDK::detail::readHyperslab<T>(               \
                reinterpret_cast<const T *>(wireElements), shape, buffer,    \
                start, shape, message)
        HC_PACK_CASE(Float64, double);
        HC_PACK_CASE(Float32, float);
        HC_PACK_CASE(Int64, int64_t);
        HC_PACK_CASE(UInt64, uint64_t);
        HC_PACK_CASE(Int32, int32_t);
        HC_PACK_CASE(UInt32, uint32_t);
        HC_PACK_CASE(Int16, int16_t);
        HC_PACK_CASE(UInt16, uint16_t);
        HC_PACK_CASE(Int8, int8_t);
        HC_PACK_CASE(UInt8, uint8_t);
        HC_PACK_CASE(Boolean, bool);
#undef HC_PACK_CASE
    default:
        if (message)
            *message = "ValuePacker: unsupported DataKind";
        return false;
    }
}

} // namespace

int64_t ValuePacker::packedSize(const BufferDescriptor &source) noexcept
{
    const int64_t elementSize = HydroCouple::Helpers::dataKindSize(source.kind);
    if (elementSize <= 0)
        return 0;
    return static_cast<int64_t>(sizeof(WireHeader)) +
           source.rank * static_cast<int64_t>(sizeof(int64_t)) +
           HydroCouple::Helpers::elementCount(source) * elementSize;
}

bool ValuePacker::pack(const BufferDescriptor &source,
                       std::vector<std::byte> &wire, std::string &message)
{
    const int64_t size = packedSize(source);
    if (size <= 0)
    {
        message = "ValuePacker::pack: source has a non-fixed-width DataKind";
        return false;
    }
    if (source.space != MemorySpace::Host)
    {
        message = "ValuePacker::pack: source must be host-resident";
        return false;
    }

    wire.resize(static_cast<size_t>(size));
    std::byte *cursor = wire.data();

    const WireHeader header{static_cast<int32_t>(source.kind), source.rank};
    std::memcpy(cursor, &header, sizeof(header));
    cursor += sizeof(header);
    std::memcpy(cursor, source.shape,
                static_cast<size_t>(source.rank) * sizeof(int64_t));
    cursor += static_cast<size_t>(source.rank) * sizeof(int64_t);

    const std::span<const int64_t> shape(source.shape,
                                         static_cast<size_t>(source.rank));
    std::string copyMessage;
    // The wire block is the contiguous "storage"; gather the (possibly
    // strided) source into it. Contiguous sources hit the memcpy fast path
    // inside the hyperslab engine.
    if (!dispatchCopy</*ToWire=*/true>(source.kind, cursor, shape, source,
                                       &copyMessage))
    {
        message = "ValuePacker::pack: " + copyMessage;
        return false;
    }
    message.clear();
    return true;
}

bool ValuePacker::peek(const std::vector<std::byte> &wire, DataKind &kind,
                       std::vector<int64_t> &shape, std::string &message)
{
    if (wire.size() < sizeof(WireHeader))
    {
        message = "ValuePacker::peek: block shorter than the wire header";
        return false;
    }
    WireHeader header;
    std::memcpy(&header, wire.data(), sizeof(header));
    if (header.rank < 0 ||
        wire.size() < sizeof(WireHeader) +
                          static_cast<size_t>(header.rank) * sizeof(int64_t))
    {
        message = "ValuePacker::peek: truncated shape";
        return false;
    }
    kind = static_cast<DataKind>(header.kind);
    shape.resize(static_cast<size_t>(header.rank));
    std::memcpy(shape.data(), wire.data() + sizeof(WireHeader),
                static_cast<size_t>(header.rank) * sizeof(int64_t));
    message.clear();
    return true;
}

bool ValuePacker::unpack(const std::vector<std::byte> &wire,
                         const BufferDescriptor &destination,
                         std::string &message)
{
    DataKind kind = DataKind::Unknown;
    std::vector<int64_t> shape;
    if (!peek(wire, kind, shape, message))
        return false;

    const int64_t elementSize = HydroCouple::Helpers::dataKindSize(kind);
    int64_t count = 1;
    for (int64_t extent : shape)
        count *= extent;
    const size_t expected = sizeof(WireHeader) +
                            shape.size() * sizeof(int64_t) +
                            static_cast<size_t>(count * elementSize);
    if (wire.size() != expected)
    {
        message = "ValuePacker::unpack: block size " +
                  std::to_string(wire.size()) +
                  " does not match its header (" + std::to_string(expected) +
                  ")";
        return false;
    }
    if (destination.space != MemorySpace::Host)
    {
        message = "ValuePacker::unpack: destination must be host-resident";
        return false;
    }

    const std::byte *elements =
        wire.data() + sizeof(WireHeader) + shape.size() * sizeof(int64_t);
    std::string copyMessage;
    if (!dispatchCopy</*ToWire=*/false>(
            kind, const_cast<std::byte *>(elements), shape, destination,
            &copyMessage))
    {
        message = "ValuePacker::unpack: " + copyMessage;
        return false;
    }
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Distributed
