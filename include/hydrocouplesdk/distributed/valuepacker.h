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
 * (rank × int64) and the raw contiguous element bytes. Payloads are always
 * typed contiguous buffers — never per-element variants — so efficiency on
 * the wire equals efficiency in memory (plan §5.2). Strided sources are
 * gathered once through the hyperslab engine; contiguous sources memcpy.
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
        int32_t kind = 0; //!< DataKind of the elements.
        int32_t rank = 0; //!< Number of dimensions.
    };

    /*!
     * \brief Bytes needed to pack \p source (header + shape + elements).
     * \return 0 for unsupported (non-fixed-width) kinds.
     */
    [[nodiscard]] static int64_t packedSize(const BufferDescriptor &source)
        noexcept;

    /*!
     * \brief Packs \p source into \p wire (resized to fit).
     *
     * Contiguous sources are copied with one memcpy; strided sources are
     * gathered through the hyperslab engine into the wire buffer directly.
     *
     * \param[in]  source  Host-resident typed buffer.
     * \param[out] wire    Destination byte vector.
     * \param[out] message Failure description when returning false.
     */
    [[nodiscard]] static bool pack(const BufferDescriptor &source,
                                   std::vector<std::byte> &wire,
                                   std::string &message);

    /*!
     * \brief Reads the header and shape of a packed block.
     * \param[in]  wire    Packed bytes.
     * \param[out] kind    Element kind.
     * \param[out] shape   Extents (rank entries).
     * \param[out] message Failure description when returning false.
     */
    [[nodiscard]] static bool peek(const std::vector<std::byte> &wire,
                                   DataKind &kind,
                                   std::vector<int64_t> &shape,
                                   std::string &message);

    /*!
     * \brief Unpacks a packed block into \p destination, which must match
     *        the packed kind and element count (strides honored).
     * \param[in]  wire        Packed bytes.
     * \param[in]  destination Host-resident typed buffer to fill.
     * \param[out] message     Failure description when returning false.
     */
    [[nodiscard]] static bool unpack(const std::vector<std::byte> &wire,
                                     const BufferDescriptor &destination,
                                     std::string &message);
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_VALUEPACKER_H
