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
 */
class HYDROCOUPLESDK_EXPORT ComponentWorker
{
public:
    /*!
     * \brief Construct with the worker's transport endpoint and the
     *        component it serves (both borrowed; must outlive the worker).
     */
    ComponentWorker(HydroCouple::Distributed::ITransport *transport,
                    HydroCouple::IModelComponent *component);

    /*!
     * \brief Runs the event loop until a Shutdown op arrives.
     * \return Messages served (diagnostics/tests).
     */
    int run();

    /*!
     * \brief Serves exactly one message (test hook).
     * \return false when the served op was Shutdown.
     */
    bool serveOne();

private:
    void reply(int32_t toRank, uint32_t tag, OpCode op,
               const std::string &text = {});
    void replyValues(int32_t toRank, uint32_t tag,
                     const HydroCouple::IComponentDataItem &item);

    HydroCouple::Distributed::ITransport *m_transport;
    HydroCouple::IModelComponent         *m_component;
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_COMPONENTWORKER_H
