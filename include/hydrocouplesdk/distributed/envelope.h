// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   envelope.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  The fixed 32-byte wire envelope and op codes of the distributed
 *         coupling protocol.
 *
 * Every protocol message is a 32-byte Envelope optionally followed, on the
 * envelope's tag, by a payload message whose elements total
 * \c payloadBytes bytes (text and item ids travel as DataKind::UInt8; a
 * GetValues reply is the item's typed value block). A SetValues request
 * adds a third message, the value block, on the same tag. The fixed size
 * lets workers post one exact-size receive for the header, then a second
 * receive for the payload — no probing, no reallocation, transport-agnostic.
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
 * \brief Protocol operation codes carried in Envelope::opCode.
 *
 * A ProxyModelComponent sends the request codes; a ComponentWorker answers
 * each request with Ack or Error on the request's tag. The numeric values
 * are part of the wire format and must not change within one protocol
 * version (Envelope::kVersion).
 */
enum class OpCode : uint8_t
{
    Hello      = 0,  //!< Handshake request; the proxy sends it from connectToPeer() and the worker answers Ack.
    Ack        = 1,  //!< Reply: the tagged request succeeded (a GetValues Ack is followed by the value block).
    Initialize = 2,  //!< Request: call the remote component's initialize(); Error unless it reaches Initialized.
    Validate   = 3,  //!< Request: call validate(); Error carries the problems joined with "; ".
    Prepare    = 4,  //!< Request: call prepare(); Error when the component is then Failed.
    Update     = 5,  //!< Request: call update(); Error when the component is then Failed.
    Finish     = 6,  //!< Request: call finish(); always answered with Ack.
    GetValues  = 7,  //!< Request: the payload is the item id (UTF-8 bytes); the Ack is followed by the item's whole value block.
    SetValues  = 8,  //!< Request: the payload is the item id, followed by a second message holding the value block.
    Status     = 9,  //!< Reserved for status notifications; no SDK code sends it and ComponentWorker answers it with Error.
    Heartbeat  = 10, //!< Liveness request; the proxy sends it from ping() and the worker answers Ack.
    Error      = 11, //!< Reply: the request failed; a payload of Envelope::payloadBytes bytes carries the message text.
    Shutdown   = 12  //!< Request: the worker answers Ack and leaves ComponentWorker::run().
};

/*!
 * \brief The fixed 32-byte protocol message header.
 *
 * An envelope travels as a rank-1 DataKind::UInt8 payload of exactly
 * sizeof(Envelope) bytes. Requests go to the worker on tag 0 (the control
 * tag); the request's own payload, and every reply, travel on the tag
 * stamped in \c tag, which the proxy allocates fresh for each request.
 * Build one with makeEnvelope() so the constant fields are set.
 *
 * \threadsafety Plain data; instances are independent.
 */
struct Envelope
{
    static constexpr uint32_t kMagic   = 0x4843504C; //!< The bytes "HCPL"; valid() rejects any other value.
    static constexpr uint8_t  kVersion = 2;          //!< Protocol version; valid() rejects any other value.

    uint32_t magic        = kMagic;   //!< Constant marker for validation (kMagic).
    uint8_t  version      = kVersion; //!< Protocol version (kVersion).
    uint8_t  opCode       = 0;        //!< OpCode of this message, as its underlying byte; read it with op().
    uint16_t flags        = 0;        //!< Bit 15 (kFlagStatusPresent) set: the low byte carries the replier's ComponentStatus.
    int32_t  sourceRank   = -1;       //!< Sender's rank on the transport; the worker replies to this rank.
    int32_t  targetRank   = -1;       //!< Intended receiver's rank (informational; no SDK code reads it).
    uint32_t tag          = 0;        //!< Request tag: the request payload and every reply travel on this tag.
    uint32_t epoch        = 0;        //!< Synchronization epoch stamp (makeEnvelope() sets it; no SDK code reads it).
    uint64_t payloadBytes = 0;        //!< Size in bytes of the payload message that follows on \c tag (0: none).

    /*!
     * \brief The op code as its enum type.
     * \return \c opCode cast to OpCode (an unknown byte yields a value
     *         outside the named enumerators).
     */
    [[nodiscard]] OpCode op() const noexcept
    {
        return static_cast<OpCode>(opCode);
    }

    /*!
     * \brief Checks structural validity: the magic and version fields.
     * \return True when \c magic equals kMagic and \c version equals
     *         kVersion; the other fields are not checked.
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
 * \brief Builds an envelope with the magic and version fields stamped.
 *
 * \c flags is left 0; ComponentWorker sets kFlagStatusPresent on its
 * replies after calling this.
 *
 * \param[in] op           Operation code.
 * \param[in] sourceRank   Sender's rank on the transport.
 * \param[in] targetRank   Receiver's rank on the transport.
 * \param[in] tag          Request tag the payload and replies use.
 * \param[in] payloadBytes Size in bytes of the payload message that follows
 *                         (0 for none).
 * \param[in] epoch        Synchronization epoch stamp (0 by default).
 * \return The filled envelope; valid() is true for it.
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
