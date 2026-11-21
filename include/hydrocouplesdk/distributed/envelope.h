// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   envelope.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  The fixed 32-byte wire envelope and op codes of the distributed
 *         coupling protocol.
 *
 * Every protocol message is a 32-byte Envelope optionally followed by a
 * payload of exactly \c payloadBytes bytes. The fixed size lets workers
 * post one exact-size receive for the header, then a second receive for
 * the payload — no probing, no reallocation, transport-agnostic.
 *
 * The layout is intentionally trivially copyable plain data (validated by
 * static_asserts) so it crosses process and transport boundaries as raw
 * bytes. Multi-byte fields are native-endian; heterogeneous-endian
 * deployments are out of scope for the MPI and in-process backends and a
 * TCP backend must byte-swap at its boundary.
 */

#ifndef HYDROCOUPLESDK_DISTRIBUTED_ENVELOPE_H
#define HYDROCOUPLESDK_DISTRIBUTED_ENVELOPE_H

#include "hydrocouplesdk/export.h"

#include <cstdint>
#include <type_traits>

namespace HydroCouple::SDK::Distributed {

/*!
 * \brief Protocol operation codes.
 */
enum class OpCode : uint8_t
{
    Hello      = 0,  //!< Handshake: worker announces readiness.
    Ack        = 1,  //!< Positive acknowledgment of the tagged request.
    Initialize = 2,  //!< Drive the remote component's initialize().
    Validate   = 3,  //!< Drive validate().
    Prepare    = 4,  //!< Drive prepare().
    Update     = 5,  //!< Drive update().
    Finish     = 6,  //!< Drive finish().
    GetValues  = 7,  //!< Request a data item's values (payload follows reply).
    SetValues  = 8,  //!< Push values into a data item (payload follows).
    Status     = 9,  //!< Status-propagation notification.
    Heartbeat  = 10, //!< Liveness probe (cloud hardening).
    Error      = 11, //!< Remote failure; payload carries the message text.
    Shutdown   = 12  //!< Orderly worker termination.
};

/*!
 * \brief The fixed 32-byte protocol message header.
 *
 * \threadsafety Plain data; instances are independent.
 */
struct Envelope
{
    static constexpr uint32_t kMagic   = 0x4843504C; //!< "HCPL".
    static constexpr uint8_t  kVersion = 2;          //!< Protocol version.

    uint32_t magic        = kMagic;   //!< Constant marker for validation.
    uint8_t  version      = kVersion; //!< Protocol version.
    uint8_t  opCode       = 0;        //!< OpCode of this message.
    uint16_t flags        = 0;        //!< Bit 15 (kFlagStatusPresent): low byte carries the replier's ComponentStatus.
    int32_t  sourceRank   = -1;       //!< Sender's rank.
    int32_t  targetRank   = -1;       //!< Intended receiver's rank.
    uint32_t tag          = 0;        //!< Request/route tag.
    uint32_t epoch        = 0;        //!< Synchronization epoch stamp.
    uint64_t payloadBytes = 0;        //!< Bytes that follow this header.

    /*!
     * \brief The op code as its enum type.
     */
    [[nodiscard]] OpCode op() const noexcept
    {
        return static_cast<OpCode>(opCode);
    }

    /*!
     * \brief Structural validity (magic + version).
     */
    [[nodiscard]] bool valid() const noexcept
    {
        return magic == kMagic && version == kVersion;
    }
};

/*!
 * \brief Set on a reply whose low flags byte is the replying component's
 *        ComponentStatus after the request was served.
 *
 * Lets a proxy mirror the peer's status (Updated vs Done vs WaitingForData)
 * instead of assuming one, without a protocol version bump: a peer that
 * predates the bit sends flags == 0, and a proxy that sees no bit keeps its
 * own inference.
 */
inline constexpr uint16_t kFlagStatusPresent = 0x8000;

static_assert(sizeof(Envelope) == 32, "protocol envelope must be 32 bytes");
static_assert(std::is_trivially_copyable_v<Envelope>,
              "protocol envelope must cross transports as raw bytes");

/*!
 * \brief Convenience factory stamping the constant fields.
 */
[[nodiscard]] inline Envelope makeEnvelope(OpCode op, int32_t sourceRank,
                                           int32_t targetRank, uint32_t tag,
                                           uint64_t payloadBytes = 0,
                                           uint32_t epoch = 0) noexcept
{
    Envelope envelope;
    envelope.opCode       = static_cast<uint8_t>(op);
    envelope.sourceRank   = sourceRank;
    envelope.targetRank   = targetRank;
    envelope.tag          = tag;
    envelope.epoch        = epoch;
    envelope.payloadBytes = payloadBytes;
    return envelope;
}

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_ENVELOPE_H
