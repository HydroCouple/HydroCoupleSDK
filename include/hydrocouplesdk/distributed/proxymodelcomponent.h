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
 * explicit connectToPeer()/disconnectFromPeer()/isConnected(), ping() heartbeats,
 * per-request timeouts, and peer death surfacing as ComponentStatus::Failed
 * plus an ErrorEntry in the component's diagnostic queue.
 *
 * What the proxy does not do: it creates no arguments, inputs or outputs of
 * its own, so a workflow cannot connect exchange items to it; values move
 * through getRemoteValues() and setRemoteValues(). It does not require
 * connectToPeer() before a lifecycle call, and it ignores the
 * requiredOutputs of update().
 *
 * Every request polls for its reply every 200 microseconds until
 * requestTimeout() elapses. A reply that arrives after its request timed
 * out is never read; it stays queued on the transport under its unique
 * tag. Request tags count up from 1 per proxy; tag 0 is the control tag
 * the worker listens on.
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
 *
 * Construct one per remote component, on the rank that runs the workflow,
 * with the rank of the ComponentWorker that serves the real component.
 * Lifecycle calls set the proxy's own status around the round trip:
 *
 * | Call | Status while waiting | Status on Ack | On Error or timeout |
 * | --- | --- | --- | --- |
 * | initialize() | Initializing | Initialized | Failed, "remote initialize failed: ..." |
 * | validate() | Validating | Valid | Error reply: Invalid with the problems; timeout: Failed, "remote validate failed: ..." |
 * | prepare() | Preparing | Updated ("remote prepared") | Failed, "remote prepare failed: ..." |
 * | update() | Updating | Done, WaitingForData or Updated, mirroring the peer | Failed, "remote update failed: ..." |
 * | finish() | Finishing | Created when the peer reports Created, else Finished | Failed, "remote finish failed: ..." |
 *
 * Failed is set with AbstractModelComponent::setStatus(status, message),
 * which also queues a Severity::Fatal ErrorEntry, and marks the proxy
 * disconnected. A timeout's message reads "request timed out after
 * S s (op N)", N being the OpCode's number.
 */
class HYDROCOUPLESDK_EXPORT ProxyModelComponent
    : public HydroCouple::SDK::AbstractModelComponent,
      public virtual HydroCouple::Distributed::IProxyModelComponent
{
public:
    //! A participant's address on the transport (only the rank is used).
    using Endpoint = HydroCouple::Distributed::ITransport::Endpoint;

    /*!
     * \brief Construct a proxy.
     *
     * Sends nothing; the request timeout starts at 30 seconds.
     *
     * \param[in] id         Local identifier for this proxy.
     * \param[in] transport  Transport endpoint (borrowed; must outlive; not
     *                       null).
     * \param[in] remoteRank The worker's rank on the transport.
     * \param[in] remoteId   The remote component's id (informational).
     */
    ProxyModelComponent(std::string_view id,
                        HydroCouple::Distributed::ITransport *transport,
                        int32_t remoteRank, std::string remoteId);

    //! Destroys the proxy; sends nothing to the peer (no Shutdown).
    ~ProxyModelComponent() override = default;

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    ProxyModelComponent(const ProxyModelComponent &)            = delete;
    ProxyModelComponent &operator=(const ProxyModelComponent &) = delete;

    // ── IDistributedModelComponent ───────────────────────────────────────

    /*!
     * \brief The transport the proxy sends its requests through.
     * \return The borrowed transport given to the constructor.
     */
    [[nodiscard]] HydroCouple::Distributed::ITransport *transport()
        const override;
    /*!
     * \brief The transport's participant count.
     * \return transport()->participantCount(): the number of ranks on the
     *         transport, not a partition count of the remote component's
     *         domain.
     */
    [[nodiscard]] int32_t partitionCount() const override;
    /*!
     * \brief The proxy's own rank on the transport.
     * \return transport()->localEndpoint().rank (the local rank, not the
     *         worker's).
     */
    [[nodiscard]] int32_t partitionRank() const override;

    // ── IProxyModelComponent ─────────────────────────────────────────────

    /*!
     * \brief The worker's endpoint.
     * \return The local endpoint's address with the worker's rank.
     */
    [[nodiscard]] Endpoint remoteEndpoint() const override;
    /*!
     * \brief The remote component's id as given to the constructor.
     * \return The informational id (not checked against the peer).
     */
    [[nodiscard]] std::string remoteId() const override;

    /*!
     * \brief Hello/Ack handshake within the request timeout.
     *
     * Returns true at once, sending nothing, when already connected.
     *
     * \param[out] message Failure description (for example "request timed
     *                     out after 30.000000 s (op 0)"); cleared on success.
     * \return True when the worker answered Ack.
     */
    [[nodiscard]] bool connectToPeer(std::string &message) override;

    /*!
     * \brief Marks the proxy disconnected; sends nothing to the peer.
     */
    void disconnectFromPeer() override;

    /*!
     * \brief Whether the last handshake succeeded and no failure followed.
     * \return True after a successful connectToPeer() until
     *         disconnectFromPeer() or a peer failure.
     */
    [[nodiscard]] bool isConnected() const override;

    /*!
     * \brief Heartbeat round trip within \p timeoutSeconds.
     *
     * Does not change the status or the connection flag on failure.
     *
     * \param[in] timeoutSeconds Longest wait for the reply, in seconds; a
     *                           non-positive value uses requestTimeout().
     * \return True when the worker answered Ack in time.
     */
    [[nodiscard]] bool ping(double timeoutSeconds) override;

    /*!
     * \brief The timeout applied to every forwarded request.
     * \return Seconds (30 by default); always positive.
     */
    [[nodiscard]] double requestTimeout() const override;
    /*!
     * \brief Sets the timeout applied to every forwarded request.
     *
     * A non-positive value is ignored and the current timeout kept; the
     * proxy has no wait-forever mode, although the interface allows one.
     *
     * \param[in] seconds New timeout in seconds (> 0).
     */
    void setRequestTimeout(double seconds) override;

    // ── IModelComponent lifecycle (remoted) ──────────────────────────────

    /*!
     * \brief Drives the remote initialize(); mirrors Initialized locally or
     *        latches Failed + a diagnostic on error/timeout.
     */
    void initialize() override;

    /*!
     * \brief Drives the remote validate(); problems come back as the list.
     *
     * An Error reply means the peer is alive and found problems: the proxy
     * becomes Invalid, queues a Severity::Error entry with the joined
     * problems, and returns them as one string. A timeout or transport
     * failure makes it Failed and returns the failure text.
     *
     * \return Empty when the peer validated; otherwise one element holding
     *         the message.
     */
    std::vector<std::string> validate() override;

    /*!
     * \brief Drives the remote prepare(); the proxy becomes Updated on Ack.
     */
    void prepare() override;

    /*!
     * \brief Drives one remote update() and mirrors the status the peer
     *        reports (Done, WaitingForData, otherwise Updated).
     *
     * A peer that sends no status flag is taken as Updated.
     *
     * \param[in] requiredOutputs Ignored: the worker always calls the remote
     *                            update() with no outputs.
     */
    void update(const std::vector<HydroCouple::IOutput *> &requiredOutputs =
                    {}) override;

    /*!
     * \brief Drives the remote finish(); the proxy becomes Finished, or
     *        Created when the peer reports Created.
     */
    void finish() override;

    // ── Remote data access ────────────────────────────────────────────────

    /*!
     * \brief Fetches the full value block of the remote item \p itemId into
     *        \p destination (kind and count validated by the transport).
     *
     * The remote item may be an output or an input. Does not change the
     * proxy's status on failure.
     *
     * \param[in]  itemId      Id of the remote data item.
     * \param[in]  destination Host buffer of the item's kind and element
     *                         count; filled on success.
     * \param[out] message     Failure description: a timeout, the remote
     *                         error text (for example "remote component has
     *                         no data item 'flow'"), or the transport's
     *                         unpack message; cleared on success.
     * \return True when \p destination holds the remote values.
     */
    [[nodiscard]] bool getRemoteValues(const std::string &itemId,
                                       const BufferDescriptor &destination,
                                       std::string &message);

    /*!
     * \brief Pushes \p source into the remote item \p itemId.
     *
     * Sends the request envelope, the item id, and the value block, then
     * waits for Ack within requestTimeout(). The remote item receives the
     * block through setValuesFrom() over its whole extent, so \p source must
     * have the item's kind and element count. The reply's status flag is not
     * recorded, and the status is not changed on failure.
     *
     * \param[in]  itemId  Id of the remote data item.
     * \param[in]  source  Host buffer of the item's kind and element count.
     * \param[out] message Failure description ("request timed out after
     *                     S s (op SetValues)", the remote error text, or a
     *                     send failure). Not cleared on success.
     * \return True when the worker answered Ack.
     */
    [[nodiscard]] bool setRemoteValues(const std::string &itemId,
                                       const BufferDescriptor &source,
                                       std::string &message);

protected:
    // AbstractModelComponent hooks — a proxy holds no local state to build.
    //! Creates nothing: the proxy has no arguments.
    void createArguments() override {}
    /*!
     * \brief Accepts the (absent) arguments.
     * \param[out] message Cleared.
     * \return Always true.
     */
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    //! Creates nothing: the proxy has no inputs.
    void createInputs() override {}
    //! Creates nothing: the proxy has no outputs.
    void createOutputs() override {}
    //! Nothing to clean up.
    void initializeFailureCleanUp() override {}

private:
    /*!
     * \brief One request/response round trip.
     *
     * Sends the envelope on tag 0 and the payload on a fresh tag, waits for
     * the reply envelope on that tag, records the peer's status from its
     * flags, then receives an Ack payload into \p replyInto or the Error
     * text.
     *
     * \param[in]  op             Operation to send.
     * \param[in]  requestPayload Optional payload descriptor (nullptr = none).
     * \param[out] replyInto      Optional destination for an Ack payload.
     * \param[out] message        Error/diagnostic text.
     * \param[in]  timeoutSeconds Overrides requestTimeout() when > 0.
     * \param[out] rejected       When non-null, set true if the peer answered
     *                            with an Error reply (it is alive and said
     *                            no), false for a transport failure/timeout.
     * \return true on Ack within the timeout; false on a send failure, a
     *         timeout, a failed receive, "malformed reply envelope", or an
     *         Error reply (its text, or "remote error" when it has none).
     */
    bool roundTrip(OpCode op, const BufferDescriptor *requestPayload,
                   const BufferDescriptor *replyInto, std::string &message,
                   double timeoutSeconds = -1.0, bool *rejected = nullptr);

    /*!
     * \brief The peer's status as of its last reply, or \p fallback when the
     *        peer did not report one (it predates kFlagStatusPresent).
     * \param[in] fallback Status to assume when none was reported.
     * \return The reported or assumed status.
     */
    [[nodiscard]] HydroCouple::IModelComponent::ComponentStatus
    remoteStatusOr(HydroCouple::IModelComponent::ComponentStatus fallback) const;

    /*!
     * \brief Latches Failed + a Fatal diagnostic for a dead/failed peer.
     *
     * Also marks the proxy disconnected.
     *
     * \param[in] message Status message and diagnostic text.
     */
    void peerFailure(const std::string &message);

    HydroCouple::Distributed::ITransport *m_transport;  //!< Borrowed transport.
    int32_t                               m_remoteRank; //!< The worker's rank.
    std::string                           m_remoteId;   //!< Informational remote id.
    std::atomic<uint32_t>                 m_nextTag{1}; //!< Next request tag (tag 0 is the control tag).
    bool                                  m_connected = false; //!< Set by a successful handshake.
    double                                m_requestTimeout = 30.0; //!< Seconds per request (> 0).
    std::atomic<int>                      m_remoteStatus{-1}; //!< -1: not reported.
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_PROXYMODELCOMPONENT_H
