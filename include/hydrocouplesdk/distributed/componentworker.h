// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   componentworker.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  ComponentWorker — the remote-side event loop that drives a local
 *         model component from protocol messages.
 *
 * A worker owns one transport endpoint and one IModelComponent. run()
 * receives Envelope messages (two-phase: 32-byte header, then an optional
 * exact-size payload under the same tag), dispatches the op to the
 * component, and replies on the request's tag:
 *
 * - lifecycle ops (Initialize/Validate/Prepare/Update/Finish) drive the
 *   component and reply Ack on success or Error with a text payload;
 * - GetValues (payload = item id) replies Ack whose payload is the
 *   ValuePacker block of the output's full value set;
 * - SetValues (payload = item id, then a value block) writes into the
 *   named input through the typed data plane;
 * - Hello and Heartbeat reply Ack immediately (liveness);
 * - Shutdown replies Ack and exits the loop.
 *
 * Requests arrive on tag 0 from any rank; the reply goes to the
 * envelope's sourceRank on the envelope's tag. Every reply sent by reply()
 * carries kFlagStatusPresent with the component's status after the request
 * (the Ack of a GetValues does not). Item ids are matched against the
 * component's outputs first, then its inputs, so GetValues and SetValues
 * work on either kind of item.
 *
 * | Request | Ack when | Error text |
 * | --- | --- | --- |
 * | Initialize | status() is Initialized afterwards | "remote initialize() did not reach Initialized" |
 * | Validate | validate() returns no problems | the problems joined with "; " |
 * | Prepare | status() is not Failed afterwards | "remote prepare() failed" |
 * | Update | status() is not Failed afterwards | "remote update() failed" |
 * | Finish | always | none |
 * | GetValues | the item exists and its values copy out | "remote component has no data item 'ID'", "item 'ID' has a non-fixed-width DataKind", or the item's getValuesInto() message |
 * | SetValues | the item exists and accepts the block | "remote component has no data item 'ID'", "SetValues value block rejected: ..." or the item's setValuesFrom() message |
 * | Hello, Heartbeat, Shutdown | always | none |
 * | any other code | never | "unsupported op code N" |
 *
 * A payload that fails to arrive is answered with "worker failed to
 * receive the request payload: ...". An envelope that fails to arrive or
 * is not valid() is dropped without a reply.
 *
 * \threadsafety run() executes on one thread (typically a dedicated worker
 * thread or a worker process's main); the component must not be touched
 * concurrently by others.
 */

#ifndef HYDROCOUPLESDK_DISTRIBUTED_COMPONENTWORKER_H
#define HYDROCOUPLESDK_DISTRIBUTED_COMPONENTWORKER_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/distributed/envelope.h"
#include "hydrocoupledistributed.h"

#include <string>

namespace HydroCouple::SDK::Distributed {

/*!
 * \brief Remote-side command loop bridging a transport and a component.
 *
 * Run one in the process (or thread) that owns the real component; the
 * host talks to it through a ProxyModelComponent on another rank of the
 * same transport. The worker holds no state between messages.
 */
class HYDROCOUPLESDK_EXPORT ComponentWorker
{
public:
    /*!
     * \brief Construct with the worker's transport endpoint and the
     *        component it serves (both borrowed; must outlive the worker).
     * \param[in] transport This rank's transport (not null).
     * \param[in] component The component to drive (not null); it should be
     *                      in the Created status, so a remote Initialize
     *                      can start its lifecycle.
     */
    ComponentWorker(HydroCouple::Distributed::ITransport *transport,
                    HydroCouple::IModelComponent *component);

    /*!
     * \brief Runs the event loop until a Shutdown op arrives.
     *
     * Blocks the calling thread; the transport's receive has no timeout,
     * so a worker that is never sent Shutdown never returns.
     *
     * \return Messages served, counting the Shutdown and any dropped
     *         malformed envelopes (diagnostics/tests).
     */
    int run();

    /*!
     * \brief Serves exactly one message (test hook).
     *
     * Blocks until an envelope arrives on tag 0, then receives its payload,
     * dispatches it and sends the reply.
     *
     * \return false when the served op was Shutdown; true otherwise,
     *         including for a malformed envelope or a request answered with
     *         Error.
     */
    bool serveOne();

private:
    /*!
     * \brief Sends a reply envelope, then \p text as a UInt8 payload when
     *        it is not empty.
     *
     * Sets kFlagStatusPresent and the component's status in the envelope
     * flags. Send failures are ignored.
     *
     * \param[in] toRank Requester's rank.
     * \param[in] tag    The request's tag.
     * \param[in] op     OpCode::Ack or OpCode::Error.
     * \param[in] text   Error text (empty: no payload).
     */
    void reply(int32_t toRank, uint32_t tag, OpCode op,
               const std::string &text = {});
    /*!
     * \brief Answers a GetValues request: copies the item's whole value set
     *        and sends an Ack envelope followed by the typed block.
     *
     * Replies Error when the item's kind has no fixed width or its
     * getValuesInto() fails. Send failures are ignored.
     *
     * \param[in] toRank Requester's rank.
     * \param[in] tag    The request's tag.
     * \param[in] item   The requested item.
     */
    void replyValues(int32_t toRank, uint32_t tag,
                     const HydroCouple::IComponentDataItem &item);

    HydroCouple::Distributed::ITransport *m_transport; //!< This rank's transport (borrowed).
    HydroCouple::IModelComponent         *m_component; //!< The served component (borrowed).
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_COMPONENTWORKER_H
