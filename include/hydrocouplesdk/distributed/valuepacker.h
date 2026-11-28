// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   valuepacker.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  ValuePacker — typed contiguous wire form for BufferDescriptor
 *         payloads.
 *
 * Wire layout: a small fixed header (DataKind, rank) followed by the shape
 * (rank × int64) and the raw contiguous element bytes, all native-endian.
 * Payloads are always typed contiguous buffers — never per-element
 * variants — so efficiency on the wire equals efficiency in memory (plan
 * §5.2). Strided sources are gathered once through the hyperslab engine
 * (bufferio.h); contiguous sources take its memcpy path. Both transports,
 * InProcTransport and MpiTransport, move every payload in this form, which
 * is why a receiver's kind and element count are checked on arrival.
 *
 * Only fixed-width kinds pack: Float64, Float32, the signed and unsigned
 * 8-, 16-, 32- and 64-bit integers, and Boolean (one byte). String, Opaque
 * and Unknown are refused. BufferDescriptor::itemSizeBytes is not read;
 * the element size comes from the kind.
 */

#ifndef HYDROCOUPLESDK_DISTRIBUTED_VALUEPACKER_H
#define HYDROCOUPLESDK_DISTRIBUTED_VALUEPACKER_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"

#include <cstddef>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Distributed {

/*!
 * \brief Packs and unpacks typed value blocks for the wire.
 *
 * Use it when a typed buffer must cross a byte channel: a transport packs
 * on send and unpacks on receive. Every function reports failure through
 * its return value and a message; none throws.
 *
 * \threadsafety Stateless free functions packaged as a class for
 * discoverability; safe from any thread.
 */
class HYDROCOUPLESDK_EXPORT ValuePacker
{
public:
    /*!
     * \brief Serialized header preceding the element bytes.
     */
    struct WireHeader
    {
        int32_t kind = 0; //!< DataKind of the elements, as its integer value.
        int32_t rank = 0; //!< Number of dimensions (shape entries that follow).
    };

    /*!
     * \brief Bytes needed to pack \p source (header + shape + elements).
     * \param[in] source Descriptor whose kind, rank and shape are read; its
     *                   data pointer and memory space are not used.
     * \return sizeof(WireHeader) + 8 × rank + element count × element size,
     *         or 0 for a kind without a fixed width (String, Opaque,
     *         Unknown).
     */
    [[nodiscard]] static int64_t packedSize(const BufferDescriptor &source)
        noexcept;

    /*!
     * \brief Packs \p source into \p wire (resized to fit).
     *
     * Contiguous sources are copied with one memcpy; strided sources are
     * gathered through the hyperslab engine into the wire buffer directly.
     *
     * \param[in]  source  Typed buffer to pack; must be MemorySpace::Host
     *                     and of a fixed-width kind; may be strided.
     * \param[out] wire    Destination byte vector, resized to
     *                     packedSize(\p source) on success (left unchanged
     *                     when the kind or space is refused).
     * \param[out] message Failure description when returning false; cleared
     *                     on success.
     * \return False with "ValuePacker::pack: source has a non-fixed-width
     *         DataKind" or "ValuePacker::pack: source must be host-resident",
     *         or with the hyperslab engine's reason; true otherwise.
     */
    [[nodiscard]] static bool pack(const BufferDescriptor &source,
                                   std::vector<std::byte> &wire,
                                   std::string &message);

    /*!
     * \brief Reads the header and shape of a packed block.
     *
     * Use it to size a destination before unpack() when the receiver does
     * not know the sender's shape. The element bytes are not checked.
     *
     * \param[in]  wire    Packed bytes.
     * \param[out] kind    Element kind from the header (not checked to be a
     *                     known DataKind).
     * \param[out] shape   Extents, one per dimension (rank entries).
     * \param[out] message Failure description when returning false; cleared
     *                     on success.
     * \return False with "ValuePacker::peek: block shorter than the wire
     *         header" or "ValuePacker::peek: truncated shape" (also for a
     *         negative rank); true otherwise.
     */
    [[nodiscard]] static bool peek(const std::vector<std::byte> &wire,
                                   DataKind &kind,
                                   std::vector<int64_t> &shape,
                                   std::string &message);

    /*!
     * \brief Unpacks a packed block into \p destination, which must match
     *        the packed kind and element count (strides honored).
     *
     * The destination's shape may differ from the packed shape as long as
     * the element counts agree; elements are written in row-major order of
     * the packed shape.
     *
     * \param[in]  wire        Packed bytes; its size must equal the size its
     *                         own header implies.
     * \param[in]  destination Host-resident typed buffer to fill; the caller
     *                         owns its memory, which may be strided.
     * \param[out] message     Failure description when returning false;
     *                         cleared on success.
     * \return False when peek() fails, with "ValuePacker::unpack: block size
     *         N does not match its header (M)", with "ValuePacker::unpack:
     *         destination must be host-resident", or with the hyperslab
     *         engine's reason prefixed by "ValuePacker::unpack: " ("kind
     *         mismatch", "buffer element count mismatch"); true otherwise.
     */
    [[nodiscard]] static bool unpack(const std::vector<std::byte> &wire,
                                     const BufferDescriptor &destination,
                                     std::string &message);
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_VALUEPACKER_H
