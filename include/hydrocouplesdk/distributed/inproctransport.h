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
 * source rank of -1 matches any sender. Async variants return
 * IExchangeRequest handles with MPI-style completion semantics (the caller
 * keeps the destination buffer alive until wait()/test() completes).
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
     */
    [[nodiscard]] static std::shared_ptr<InProcHub> create(
        int32_t participantCount);

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
     */
    [[nodiscard]] int32_t participantCount() const noexcept;

    /*!
     * \brief Creates the transport endpoint for \p rank
     *        (0 ≤ rank < participantCount()).
     * \return nullptr for an out-of-range rank.
     */
    [[nodiscard]] std::unique_ptr<InProcTransport> createTransport(
        int32_t rank);

    struct Impl; //!< Implementation detail (visible so request handles can
                 //!< reference it; members are internal).

private:
    friend class InProcTransport;
    explicit InProcHub(int32_t participantCount);

    std::unique_ptr<Impl> m_impl;
};

/*!
 * \brief One rank's endpoint on an InProcHub.
 *
 * \threadsafety send()/receive() and the async variants may be called from
 * any thread; the hub synchronizes internally.
 */
class HYDROCOUPLESDK_EXPORT InProcTransport final
    : public HydroCouple::SDK::Identity,
      public virtual HydroCouple::Distributed::ITransport
{
public:
    using Endpoint = HydroCouple::Distributed::ITransport::Endpoint;
    using IExchangeRequest = HydroCouple::Distributed::IExchangeRequest;

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
     */
    [[nodiscard]] Endpoint localEndpoint() const override;

    /*!
     * \brief Ranks connected by the hub.
     */
    [[nodiscard]] int32_t participantCount() const override;

    /*!
     * \brief Copies \p payload into \p to's mailbox under \p tag.
     *        Never blocks (the in-process mailbox is unbounded).
     */
    [[nodiscard]] bool send(const Endpoint &to, int32_t tag,
                            const BufferDescriptor &payload,
                            std::string *message = nullptr) override;

    /*!
     * \brief Blocks until a message matching (\p from, \p tag) arrives and
     *        unpacks it into \p into (kind and element count validated).
     *        A source rank of -1 matches any sender.
     */
    [[nodiscard]] bool receive(const Endpoint &from, int32_t tag,
                               const BufferDescriptor &into,
                               std::string *message = nullptr) override;

    /*!
     * \brief send() packaged as an immediately completed request.
     */
    [[nodiscard]] std::unique_ptr<IExchangeRequest> sendAsync(
        const Endpoint &to, int32_t tag,
        const BufferDescriptor &payload) override;

    /*!
     * \brief Nonblocking receive: the returned request matches on test()
     *        or blocks on wait(). \p into must stay valid until completion.
     */
    [[nodiscard]] std::unique_ptr<IExchangeRequest> receiveAsync(
        const Endpoint &from, int32_t tag,
        const BufferDescriptor &into) override;

private:
    friend class InProcHub;
    InProcTransport(std::shared_ptr<InProcHub> hub, int32_t rank);

    std::shared_ptr<InProcHub> m_hub;
    int32_t                    m_rank;
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_INPROCTRANSPORT_H
