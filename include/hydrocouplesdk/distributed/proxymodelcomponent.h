// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   proxymodelcomponent.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  ProxyModelComponent — the local stand-in for a component served
 *         by a remote ComponentWorker.
 *
 * The proxy implements the standard's IProxyModelComponent: compositions
 * treat it exactly like a local IModelComponent, while every lifecycle call
 * becomes a protocol request (Envelope + optional payload) answered by the
 * worker with Ack or Error. Cloud hardening per the interface review:
 * explicit connect()/disconnect()/isConnected(), ping() heartbeats,
 * per-request timeouts, and peer death surfacing as ComponentStatus::Failed
 * plus an ErrorEntry in the component's diagnostic queue.
 *
 * \threadsafety Drive from one thread (like any model component).
 */

#ifndef HYDROCOUPLESDK_DISTRIBUTED_PROXYMODELCOMPONENT_H
#define HYDROCOUPLESDK_DISTRIBUTED_PROXYMODELCOMPONENT_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/distributed/envelope.h"
#include "hydrocoupledistributed.h"

#include <atomic>
#include <string>

namespace HydroCouple::SDK::Distributed {

/*!
 * \brief Local proxy for a remote component behind any ITransport.
 */
class HYDROCOUPLESDK_EXPORT ProxyModelComponent
    : public HydroCouple::SDK::AbstractModelComponent,
      public virtual HydroCouple::Distributed::IProxyModelComponent
{
public:
    using Endpoint = HydroCouple::Distributed::ITransport::Endpoint;

    /*!
     * \brief Construct a proxy.
     * \param[in] id         Local identifier for this proxy.
     * \param[in] transport  Transport endpoint (borrowed; must outlive).
     * \param[in] remoteRank The worker's rank on the transport.
     * \param[in] remoteId   The remote component's id (informational).
     */
    ProxyModelComponent(std::string_view id,
                        HydroCouple::Distributed::ITransport *transport,
                        int32_t remoteRank, std::string remoteId);

    ~ProxyModelComponent() override = default;

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    ProxyModelComponent(const ProxyModelComponent &)            = delete;
    ProxyModelComponent &operator=(const ProxyModelComponent &) = delete;

    // ── IDistributedModelComponent ───────────────────────────────────────

    [[nodiscard]] HydroCouple::Distributed::ITransport *transport()
        const override;
    [[nodiscard]] int32_t partitionCount() const override;
    [[nodiscard]] int32_t partitionRank() const override;

    // ── IProxyModelComponent ─────────────────────────────────────────────

    [[nodiscard]] Endpoint remoteEndpoint() const override;
    [[nodiscard]] std::string remoteId() const override;

    /*!
     * \brief Hello/Ack handshake within the request timeout.
     */
    [[nodiscard]] bool connect(std::string &message) override;

    void disconnect() override;

    [[nodiscard]] bool isConnected() const override;

    /*!
     * \brief Heartbeat round trip within \p timeoutSeconds.
     */
    [[nodiscard]] bool ping(double timeoutSeconds) override;

    [[nodiscard]] double requestTimeout() const override;
    void setRequestTimeout(double seconds) override;

    // ── IModelComponent lifecycle (remoted) ──────────────────────────────

    /*!
     * \brief Drives the remote initialize(); mirrors Initialized locally or
     *        latches Failed + a diagnostic on error/timeout.
     */
    void initialize() override;

    /*!
     * \brief Drives the remote validate(); problems come back as the list.
     */
    std::vector<std::string> validate() override;

    void prepare() override;

    void update(const std::vector<HydroCouple::IOutput *> &requiredOutputs =
                    {}) override;

    void finish() override;

    // ── Remote data access ────────────────────────────────────────────────

    /*!
     * \brief Fetches the full value block of the remote item \p itemId into
     *        \p destination (kind and count validated by the transport).
     */
    [[nodiscard]] bool getRemoteValues(const std::string &itemId,
                                       const BufferDescriptor &destination,
                                       std::string &message);

    /*!
     * \brief Pushes \p source into the remote item \p itemId.
     */
    [[nodiscard]] bool setRemoteValues(const std::string &itemId,
                                       const BufferDescriptor &source,
                                       std::string &message);

protected:
    // AbstractModelComponent hooks — a proxy holds no local state to build.
    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override {}
    void createOutputs() override {}
    void initializeFailureCleanUp() override {}

private:
    /*!
     * \brief One request/response round trip.
     * \param[in]  op             Operation to send.
     * \param[in]  requestPayload Optional payload descriptor (nullptr = none).
     * \param[out] replyInto      Optional destination for an Ack payload.
     * \param[out] message        Error/diagnostic text.
     * \param[in]  timeoutSeconds Overrides requestTimeout() when > 0.
     * \return true on Ack within the timeout.
     */
    bool roundTrip(OpCode op, const BufferDescriptor *requestPayload,
                   const BufferDescriptor *replyInto, std::string &message,
                   double timeoutSeconds = -1.0);

    /*!
     * \brief Latches Failed + a Fatal diagnostic for a dead/failed peer.
     */
    void peerFailure(const std::string &message);

    HydroCouple::Distributed::ITransport *m_transport;
    int32_t                               m_remoteRank;
    std::string                           m_remoteId;
    std::atomic<uint32_t>                 m_nextTag{1};
    bool                                  m_connected = false;
    double                                m_requestTimeout = 30.0;
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_PROXYMODELCOMPONENT_H
