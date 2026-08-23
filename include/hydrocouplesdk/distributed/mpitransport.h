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
 *        examples). Initializes MPI with MPI_THREAD_MULTIPLE when not
 *        already initialized; finalizes only what it initialized.
 */
class HYDROCOUPLESDK_EXPORT MpiEnvironment final
{
public:
    MpiEnvironment(int &argc, char **&argv);
    ~MpiEnvironment();

    MpiEnvironment(const MpiEnvironment &)            = delete;
    MpiEnvironment &operator=(const MpiEnvironment &) = delete;

private:
    bool m_ownsInit = false;
};

/*!
 * \brief MPI implementation of the standard's ITransport.
 *
 * Wire format: every message is one ValuePacker block (kind + shape header
 * followed by raw elements) sent as MPI bytes, identical to
 * InProcTransport — the two backends are interchangeable behind the seam.
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
     */
    MpiTransport();

    ~MpiTransport() override;

    MpiTransport(const MpiTransport &)            = delete;
    MpiTransport &operator=(const MpiTransport &) = delete;

    /*!
     * \brief True when MPI is initialized and the transport is usable.
     */
    [[nodiscard]] static bool available();

    // ── ITransport ────────────────────────────────────────────────────────

    [[nodiscard]] Endpoint localEndpoint() const override;

    [[nodiscard]] int32_t participantCount() const override;

    /*!
     * \brief Packs \p payload into a wire block and sends it (blocking).
     */
    [[nodiscard]] bool send(const Endpoint &to, int32_t tag,
                            const BufferDescriptor &payload,
                            std::string *message = nullptr) override;

    /*!
     * \brief Probes for the matching message, receives its wire block, and
     *        unpacks it into \p into (kind/shape validated). \p from.rank of
     *        -1 receives from any source.
     */
    [[nodiscard]] bool receive(const Endpoint &from, int32_t tag,
                               const BufferDescriptor &into,
                               std::string *message = nullptr) override;

    /*!
     * \brief Nonblocking send over MPI_Isend; the request owns the packed
     *        wire block, so \p payload is reusable immediately on return.
     */
    [[nodiscard]] std::unique_ptr<HydroCouple::Distributed::IExchangeRequest>
    sendAsync(const Endpoint &to, int32_t tag,
              const BufferDescriptor &payload) override;

    /*!
     * \brief Nonblocking receive via matched probe (MPI_Improbe/MPI_Imrecv);
     *        \p into must stay valid until the request completes.
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
