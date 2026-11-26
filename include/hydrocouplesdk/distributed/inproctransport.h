// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   inproctransport.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  InProcTransport — the in-process ITransport backend.
 *
 * Backend #0 of the transport layer (plan §5.1): a hub of rank-addressed
 * mailboxes inside one process, making the entire distributed protocol —
 * envelopes, value packing, proxy/worker loops, halo exchanges — fully
 * unit-testable without \c mpirun. The MPI and TCP backends implement the
 * same ITransport seam.
 *
 * Semantics mirror MPI's tagged point-to-point model: send() copies the
 * payload (gathering strided sources through ValuePacker) and never blocks;
 * receive() blocks until a message matching (source, tag) arrives — a
 * negative source rank (conventionally -1) matches any sender. Matching
 * takes the oldest queued message that fits, so messages between one
 * (sender, receiver, tag) triple arrive in send order, and messages on
 * different tags may be received in any order. Async variants return
 * IExchangeRequest handles with MPI-style completion semantics (the caller
 * keeps the destination buffer alive until wait()/test() completes).
 *
 * Lost peers cannot happen inside one process, and nothing times out: a
 * blocking receive() whose message is never sent blocks forever. Bound
 * waits with receiveAsync() and IExchangeRequest::waitFor().
 */

#ifndef HYDROCOUPLESDK_DISTRIBUTED_INPROCTRANSPORT_H
#define HYDROCOUPLESDK_DISTRIBUTED_INPROCTRANSPORT_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/core/identity.h"
#include "hydrocoupledistributed.h"

#include <memory>
#include <string>

namespace HydroCouple::SDK::Distributed {

class InProcTransport;

/*!
 * \brief Shared in-process message hub connecting a fixed set of ranks.
 *
 * The hub owns one mailbox (a queue of packed messages) per rank. It is
 * held through std::shared_ptr: every transport it creates, and every
 * pending receive request, keeps it alive, so the hub may be released
 * before its transports.
 *
 * Create one hub per simulated "communicator", then a transport per rank:
 * \code
 *   auto hub = InProcHub::create(4);
 *   auto t0  = hub->createTransport(0);
 *   auto t1  = hub->createTransport(1);
 * \endcode
 *
 * \threadsafety All operations are internally synchronized; transports on
 * different threads exchange freely.
 */
class HYDROCOUPLESDK_EXPORT InProcHub
    : public std::enable_shared_from_this<InProcHub>
{
public:
    /*!
     * \brief Creates a hub for \p participantCount ranks (≥ 1).
     * \param[in] participantCount Number of ranks; values below 1 are
     *                             raised to 1.
     * \return The new hub (never null).
     */
    [[nodiscard]] static std::shared_ptr<InProcHub> create(
        int32_t participantCount);

    //! Destroys the mailboxes; messages still queued are discarded.
    ~InProcHub();

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    InProcHub(const InProcHub &)            = delete;
    InProcHub &operator=(const InProcHub &) = delete;

    /*!
     * \brief Number of ranks this hub connects.
     * \return The count fixed at create() (at least 1).
     */
    [[nodiscard]] int32_t participantCount() const noexcept;

    /*!
     * \brief Creates the transport endpoint for \p rank
     *        (0 ≤ rank < participantCount()).
     *
     * Nothing stops a caller creating two transports for one rank; both
     * then read the same mailbox, so create exactly one per rank.
     *
     * \param[in] rank The rank the transport speaks for.
     * \return The transport (owned by the caller), or nullptr for an
     *         out-of-range rank.
     */
    [[nodiscard]] std::unique_ptr<InProcTransport> createTransport(
        int32_t rank);

    struct Impl; //!< Implementation detail (visible so request handles can
                 //!< reference it; members are internal).

private:
    friend class InProcTransport;
    //! Builds the mailboxes; reached only through create(), so the hub
    //! is always owned by a std::shared_ptr (createTransport() needs it).
    //! \param[in] participantCount Number of ranks (already clamped to ≥ 1).
    explicit InProcHub(int32_t participantCount);

    std::unique_ptr<Impl> m_impl; //!< Mailboxes and their locks.
};

/*!
 * \brief One rank's endpoint on an InProcHub.
 *
 * Implements the standard's HydroCouple::Distributed::ITransport for ranks
 * that share one process, typically one thread per rank. Its identity id is
 * "inproc-transport-<rank>" and its endpoint address is "inproc". Obtain it
 * from InProcHub::createTransport(); it keeps the hub alive.
 *
 * \threadsafety send()/receive() and the async variants may be called from
 * any thread; the hub synchronizes internally.
 */
class HYDROCOUPLESDK_EXPORT InProcTransport final
    : public HydroCouple::SDK::Identity,
      public virtual HydroCouple::Distributed::ITransport
{
public:
    //! A participant's address on this transport (only the rank is used).
    using Endpoint = HydroCouple::Distributed::ITransport::Endpoint;
    //! The standard's asynchronous request handle.
    using IExchangeRequest = HydroCouple::Distributed::IExchangeRequest;

    //! Releases this rank's hold on the hub; queued messages stay queued.
    ~InProcTransport() override;

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    InProcTransport(const InProcTransport &)            = delete;
    InProcTransport &operator=(const InProcTransport &) = delete;

    // ── ITransport ────────────────────────────────────────────────────────

    /*!
     * \brief This transport's rank-addressed endpoint.
     * \return Address "inproc" and this transport's rank.
     */
    [[nodiscard]] Endpoint localEndpoint() const override;

    /*!
     * \brief Ranks connected by the hub.
     * \return InProcHub::participantCount().
     */
    [[nodiscard]] int32_t participantCount() const override;

    /*!
     * \brief Copies \p payload into \p to's mailbox under \p tag.
     *        Never blocks (the in-process mailbox is unbounded).
     *
     * The payload is packed with ValuePacker before this returns, so the
     * caller may reuse its buffer at once.
     *
     * \param[in]  to      Destination; only \c to.rank is read
     *                     (0 ≤ rank < participantCount(); sending to one's
     *                     own rank is allowed).
     * \param[in]  tag     Stream tag the receiver matches on (any value).
     * \param[in]  payload Host-resident typed buffer of a fixed-width kind;
     *                     may be strided.
     * \param[out] message Optional; receives the failure description, and
     *                     is cleared on success.
     * \return False with "InProcTransport::send: invalid target rank N", or
     *         with ValuePacker::pack()'s message; true once the message is
     *         queued.
     */
    [[nodiscard]] bool send(const Endpoint &to, int32_t tag,
                            const BufferDescriptor &payload,
                            std::string *message = nullptr) override;

    /*!
     * \brief Blocks until a message matching (\p from, \p tag) arrives and
     *        unpacks it into \p into (kind and element count validated).
     *        A source rank of -1 matches any sender.
     *
     * The matched message is consumed even when unpacking it fails. There
     * is no timeout.
     *
     * \param[in]  from    Source; \c from.rank is a rank below
     *                     participantCount(), or negative for any sender.
     * \param[in]  tag     Stream tag to match.
     * \param[in]  into    Pre-allocated host buffer; its kind must equal the
     *                     sent kind and its element count the sent count.
     * \param[out] message Optional; receives the failure description, and
     *                     is cleared on success.
     * \return False with "InProcTransport::receive: invalid source rank N"
     *         (checked before waiting), or with ValuePacker::unpack()'s
     *         message; true when \p into holds the payload.
     */
    [[nodiscard]] bool receive(const Endpoint &from, int32_t tag,
                               const BufferDescriptor &into,
                               std::string *message = nullptr) override;

    /*!
     * \brief send() packaged as an immediately completed request.
     *
     * The payload is copied before this returns, so the buffer is free at
     * once; test() and wait() return immediately and failed() reports any
     * send() failure.
     *
     * \param[in] to      Destination (see send()).
     * \param[in] tag     Stream tag.
     * \param[in] payload Buffer to send (see send()).
     * \return A completed request (never null).
     */
    [[nodiscard]] std::unique_ptr<IExchangeRequest> sendAsync(
        const Endpoint &to, int32_t tag,
        const BufferDescriptor &payload) override;

    /*!
     * \brief Nonblocking receive: the returned request matches on test()
     *        or blocks on wait(). \p into must stay valid until completion.
     *
     * Nothing is taken from the mailbox until test(), wait() or waitFor()
     * finds a match, so two pending receives on one (source, tag) are served
     * in the order they are completed, not the order they were posted. The
     * source rank is not range-checked here: a rank that no transport uses
     * never matches. cancel() does not cancel; it returns test(). Destroying
     * a request that has not completed does not block and leaves any
     * unmatched message queued.
     *
     * \param[in] from Source; a negative rank matches any sender.
     * \param[in] tag  Stream tag to match.
     * \param[in] into Destination buffer (see receive()); the caller owns it
     *                 and keeps it valid until the request completes.
     * \return The pending request (never null); failed() reports an unpack
     *         failure after completion.
     */
    [[nodiscard]] std::unique_ptr<IExchangeRequest> receiveAsync(
        const Endpoint &from, int32_t tag,
        const BufferDescriptor &into) override;

private:
    friend class InProcHub;
    //! Reached only through InProcHub::createTransport().
    //! \param[in] hub  The hub (shared; kept alive by this transport).
    //! \param[in] rank This transport's rank (already range-checked).
    InProcTransport(std::shared_ptr<InProcHub> hub, int32_t rank);

    std::shared_ptr<InProcHub> m_hub;  //!< The hub holding every rank's mailbox.
    int32_t                    m_rank; //!< This transport's rank.
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_INPROCTRANSPORT_H
