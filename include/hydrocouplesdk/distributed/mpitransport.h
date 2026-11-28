// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   mpitransport.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  MpiTransport — the MPI backend behind the standard's ITransport
 *         seam (plan §5.1, backend #2).
 *
 * Carries the same ValuePacker wire blocks as InProcTransport, so every
 * protocol layer built on the seam (Envelope dispatch, ComponentWorker,
 * ProxyModelComponent, HaloExchanger, PartitionedDataItem) runs unchanged
 * over MPI: kind and shape travel with every payload and are validated on
 * unpack, strided descriptors are gathered once on send and scattered once
 * on receive, and message sizes are discovered with matched probes — no
 * out-of-band size negotiation.
 *
 * MPI specifics kept behind the pimpl (this header installs without MPI):
 * the transport duplicates its communicator at construction so its tag
 * space cannot collide with the application's, any-source receives map to
 * MPI_ANY_SOURCE, and asynchronous receives use MPI_Improbe/MPI_Imrecv so
 * a request can be tested before the payload size is known.
 *
 * The transport neither initializes nor finalizes MPI — the launcher owns
 * that (an MpiEnvironment RAII helper is provided for test mains).
 *
 * Built only when the SDK is configured with USE_MPI=ON (the source is
 * compiled into HydroCoupleSDK, and consumers link MPI::MPI_CXX). The
 * header itself names no MPI type.
 *
 * \threadsafety One transport belongs to one rank (process). Calls from
 * multiple threads additionally require MPI_THREAD_MULTIPLE from the MPI
 * library.
 */

#ifndef HYDROCOUPLESDK_DISTRIBUTED_MPITRANSPORT_H
#define HYDROCOUPLESDK_DISTRIBUTED_MPITRANSPORT_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/core/identity.h"
#include "hydrocoupledistributed.h"

#include <memory>
#include <string>

namespace HydroCouple::SDK::Distributed {

/*!
 * \brief RAII MPI initialization for executables the SDK owns (test mains,
 *        examples). Requests MPI_THREAD_SERIALIZED through MPI_Init_thread
 *        when MPI is not yet initialized; finalizes only what it initialized.
 *
 * Create one at the top of main(), before any MpiTransport, and let it go
 * out of scope after every transport is destroyed.
 */
class HYDROCOUPLESDK_EXPORT MpiEnvironment final
{
public:
    /*!
     * \brief Initializes MPI unless it already is.
     *
     * When MPI is already initialized, records the level it was started
     * with (MPI_Query_thread) and takes no ownership.
     *
     * \param[in,out] argc Argument count from main(); MPI may remove its own
     *                     arguments.
     * \param[in,out] argv Argument vector from main(); MPI may rewrite it.
     */
    MpiEnvironment(int &argc, char **&argv);
    //! Calls MPI_Finalize() when this object initialized MPI and MPI has not
    //! been finalized already.
    ~MpiEnvironment();

    MpiEnvironment(const MpiEnvironment &)            = delete;
    MpiEnvironment &operator=(const MpiEnvironment &) = delete;

    /*!
     * \brief The threading level MPI actually provided.
     *
     * The SDK's own requirement is only MPI_THREAD_FUNNELED: nothing in the
     * distributed layer starts a thread, and the IO thread never touches
     * MPI. SERIALIZED is *requested* so a host may drive components from a
     * worker thread, but a lower level is not fatal — implementations
     * differ, and Microsoft MPI in particular does not offer the full range
     * that OpenMPI and MPICH do.
     *
     * A host that intends to call MPI from several threads concurrently
     * must check this rather than assume; the previous code requested
     * MPI_THREAD_MULTIPLE and discarded the answer, which reads as a
     * guarantee that was never actually obtained.
     *
     * \return The MPI_THREAD_* level MPI reported (an int as MPI defines it).
     */
    [[nodiscard]] int providedThreadLevel() const noexcept
    {
        return m_provided;
    }

    /*!
     * \brief Whether MPI met the SDK's minimum (MPI_THREAD_FUNNELED).
     * \return True when providedThreadLevel() ≥ MPI_THREAD_FUNNELED.
     */
    [[nodiscard]] bool threadLevelSufficient() const noexcept;

private:
    bool m_ownsInit = false; //!< True when this object called MPI_Init_thread.
    int  m_provided = 0;     //!< Thread level MPI provided.
};

/*!
 * \brief MPI implementation of the standard's ITransport.
 *
 * One transport per process: its rank and participant count are the
 * process's rank and size in MPI_COMM_WORLD, its identity caption is
 * "MPI transport (rank R of N)", and its endpoint address is "mpi". MPI
 * error codes are reported through MPI_Error_string, prefixed with the
 * operation ("MpiTransport::send: ...").
 *
 * Wire format: every message is one ValuePacker block (kind + shape header
 * followed by raw elements) sent as MPI bytes, identical to
 * InProcTransport — the two backends are interchangeable behind the seam.
 *
 * Lost peers: this transport cannot detect them. Under MPI's default error
 * handler a lost rank aborts the job; a pending operation with it is never
 * completed as failed. Callers that must not hang bound their waits with the
 * asynchronous operations and IExchangeRequest::waitFor(), which polls.
 */
class HYDROCOUPLESDK_EXPORT MpiTransport final
    : public Identity,
      public virtual HydroCouple::Distributed::ITransport
{
public:
    /*!
     * \brief Constructs over a duplicate of MPI_COMM_WORLD.
     *
     * MPI must already be initialized (see MpiEnvironment). The duplicate
     * isolates this transport's tag space from the application's.
     * MPI_Comm_dup is collective: every rank must construct its transport.
     */
    MpiTransport();

    //! Frees the duplicated communicator unless MPI is already finalized.
    ~MpiTransport() override;

    MpiTransport(const MpiTransport &)            = delete;
    MpiTransport &operator=(const MpiTransport &) = delete;

    /*!
     * \brief True when MPI is initialized and the transport is usable.
     * \return True when MPI_Initialized is true and MPI_Finalized is false.
     */
    [[nodiscard]] static bool available();

    // ── ITransport ────────────────────────────────────────────────────────

    /*!
     * \brief This process's rank-addressed endpoint.
     * \return Address "mpi" and this process's rank.
     */
    [[nodiscard]] Endpoint localEndpoint() const override;

    /*!
     * \brief The number of MPI processes.
     * \return The size of the duplicated communicator.
     */
    [[nodiscard]] int32_t participantCount() const override;

    /*!
     * \brief Packs \p payload into a wire block and sends it (blocking).
     *
     * Uses MPI_Send, which may wait until the receiver posts a matching
     * receive for large messages.
     *
     * \param[in]  to      Destination; 0 ≤ \c to.rank < participantCount().
     * \param[in]  tag     MPI tag (0 ≤ tag ≤ the MPI_TAG_UB of the MPI library).
     * \param[in]  payload Host-resident buffer of a fixed-width kind; may be
     *                     strided.
     * \param[out] message Optional; receives the failure description, and
     *                     is cleared on success.
     * \return False with "MpiTransport::send: invalid target rank N", with
     *         ValuePacker::pack()'s message, or with the MPI error text;
     *         true once the send has completed locally.
     */
    [[nodiscard]] bool send(const Endpoint &to, int32_t tag,
                            const BufferDescriptor &payload,
                            std::string *message = nullptr) override;

    /*!
     * \brief Probes for the matching message, receives its wire block, and
     *        unpacks it into \p into (kind/shape validated). \p from.rank of
     *        -1 receives from any source.
     *
     * Blocks in MPI_Mprobe with no timeout.
     *
     * \param[in]  from    Source rank below participantCount(), or negative
     *                     for any source (MPI_ANY_SOURCE).
     * \param[in]  tag     MPI tag to match.
     * \param[in]  into    Pre-allocated host buffer of the sent kind and
     *                     element count.
     * \param[out] message Optional; receives the failure description, and
     *                     is cleared on success.
     * \return False with "MpiTransport::receive: invalid source rank N", the
     *         MPI error text, or ValuePacker::unpack()'s message; true when
     *         \p into holds the payload.
     */
    [[nodiscard]] bool receive(const Endpoint &from, int32_t tag,
                               const BufferDescriptor &into,
                               std::string *message = nullptr) override;

    /*!
     * \brief Nonblocking send over MPI_Isend; the request owns the packed
     *        wire block, so \p payload is reusable immediately on return.
     *
     * The request's destructor waits for an incomplete send (MPI_Wait).
     * cancel() does not cancel; it returns test(). waitFor() polls test()
     * every 50 microseconds.
     *
     * \param[in] to      Destination (see send()).
     * \param[in] tag     MPI tag.
     * \param[in] payload Buffer to send (see send()).
     * \return The request (never null); an invalid rank or a packing failure
     *         yields an already completed request whose failed() is true.
     */
    [[nodiscard]] std::unique_ptr<HydroCouple::Distributed::IExchangeRequest>
    sendAsync(const Endpoint &to, int32_t tag,
              const BufferDescriptor &payload) override;

    /*!
     * \brief Nonblocking receive via matched probe (MPI_Improbe/MPI_Imrecv);
     *        \p into must stay valid until the request completes.
     *
     * The message is claimed only when test(), wait() or waitFor() first
     * finds it, so an unclaimed request holds nothing. A destroyed request
     * waits only for a receive it has already started. cancel() does not
     * cancel; it returns test().
     *
     * \param[in] from Source rank, or negative for any source.
     * \param[in] tag  MPI tag to match.
     * \param[in] into Destination buffer (see receive()); the caller owns it.
     * \return The request (never null); an out-of-range source yields an
     *         already completed request whose failed() is true.
     */
    [[nodiscard]] std::unique_ptr<HydroCouple::Distributed::IExchangeRequest>
    receiveAsync(const Endpoint &from, int32_t tag,
                 const BufferDescriptor &into) override;

private:
    struct Impl; //!< Hides all MPI types from the public surface.
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_MPITRANSPORT_H
