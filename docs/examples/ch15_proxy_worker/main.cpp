// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 15: drive a component that lives on another rank through a
 *         ProxyModelComponent and a ComponentWorker, over the in-process
 *         transport, and check every value against a local computation.
 */

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/distributed/componentworker.h"
#include "hydrocouplesdk/distributed/envelope.h"
#include "hydrocouplesdk/distributed/inproctransport.h"
#include "hydrocouplesdk/distributed/proxymodelcomponent.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <thread>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Distributed;
using Status = IModelComponent::ComponentStatus;

namespace {

constexpr int    kReaches = 3;
constexpr int    kSteps   = 4;
constexpr double kDt      = 60.0; // s
constexpr double kK       = 0.01; // 1/s

//! [remote]
/*!
 * The component the worker serves: three reach storages S (m³) fed by an
 * inflow I (m³/s), S <- S + dt (I - k S), outflow Q = k S. It reports Done
 * after kSteps updates.
 */
class ReachStorage : public AbstractModelComponent
{
public:
    explicit ReachStorage(std::string_view id)
        : AbstractModelComponent(id), m_flow(Quantity::flowInCMS())
    {
    }

    std::vector<std::string> validate() override
    {
        setStatus(Status::Validating);
        setStatus(Status::Valid);
        return {};
    }
    void prepare() override
    {
        setStatus(Status::Preparing);
        for (int i = 0; i < kReaches; ++i)
            m_storage[i] = 100.0 * (i + 1);
        setStatus(Status::Updated);
    }
    void update(const std::vector<IOutput *> & = {}) override
    {
        setStatus(Status::Updating);
        for (int i = 0; i < kReaches; ++i)
        {
            m_storage[i] += kDt * ((*m_inflow)[i] - kK * m_storage[i]);
            (*m_outflow)[i] = kK * m_storage[i];
        }
        ++m_steps;
        setStatus(m_steps >= kSteps ? Status::Done : Status::Updated);
    }
    void finish() override
    {
        setStatus(Status::Finishing);
        setStatus(Status::Finished);
    }

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        m_inflow = std::make_unique<Input1DDouble>("inflow", &m_reach, kReaches,
                                                   m_flow.get(), this);
        addInput(m_inflow.get());
    }
    void createOutputs() override
    {
        m_outflow = std::make_unique<Output1DDouble>("outflow", &m_reach, kReaches,
                                                     m_flow.get(), this);
        addOutput(m_outflow.get());
    }
    void initializeFailureCleanUp() override {}

private:
    // Metadata first: the items point to it, and members are destroyed in
    // reverse order. The component owns its items; addInput() and addOutput()
    // only register them with the base class, which never deletes them.
    Dimension m_reach{"reach", "Reach"};
    std::unique_ptr<Quantity> m_flow;
    std::unique_ptr<Input1DDouble> m_inflow;
    std::unique_ptr<Output1DDouble> m_outflow;
    std::array<double, kReaches> m_storage{};
    int m_steps = 0;
};
//! [remote]

//! Host descriptor over a three-element array.
BufferDescriptor reachBuffer(std::array<double, kReaches> &values,
                             const std::array<int64_t, 1> &shape)
{
    BufferDescriptor descriptor;
    descriptor.data  = values.data();
    descriptor.kind  = DataKind::Float64;
    descriptor.rank  = 1;
    descriptor.shape = shape.data();
    descriptor.space = MemorySpace::Host;
    return descriptor;
}

int fail(const std::string &what)
{
    std::fprintf(stderr, "FAILED: %s\n", what.c_str());
    return 1;
}

} // namespace

int main()
{
    //! [ranks]
    // One hub stands for one communicator; each rank gets its own transport.
    std::shared_ptr<InProcHub> hub = InProcHub::create(2);
    std::unique_ptr<InProcTransport> hostRank   = hub->createTransport(0);
    std::unique_ptr<InProcTransport> workerRank = hub->createTransport(1);

    // Rank 1: the real component and the worker that serves it, on its own
    // thread (it would be its own process under MPI).
    ReachStorage remote("reaches");
    ComponentWorker worker(workerRank.get(), &remote);
    int served = 0;
    std::thread workerThread([&] { served = worker.run(); });

    // Rank 0: the proxy a workflow would hold in place of the component.
    ProxyModelComponent proxy("reaches-proxy", hostRank.get(), /*remoteRank=*/1,
                              "reaches");
    proxy.setRequestTimeout(5.0);
    //! [ranks]

    auto stopWorker = [&] {
        //! [shutdown]
        // The proxy has no shutdown call: the host sends the Shutdown request
        // itself, on the control tag 0, and waits for the worker to return.
        Envelope shutdown = makeEnvelope(OpCode::Shutdown, 0, 1, /*tag=*/0xFFFF);
        const int64_t bytes = sizeof(Envelope);
        BufferDescriptor descriptor;
        descriptor.data  = &shutdown;
        descriptor.kind  = DataKind::UInt8;
        descriptor.rank  = 1;
        descriptor.shape = &bytes;
        descriptor.space = MemorySpace::Host;
        std::string message;
        (void)hostRank->send({"inproc", 1}, /*tag=*/0, descriptor, &message);
        workerThread.join();
        //! [shutdown]
    };

    //! [lifecycle]
    std::string message;
    if (!proxy.connectToPeer(message) || !proxy.ping(1.0))
    {
        stopWorker();
        return fail("handshake: " + message);
    }
    proxy.initialize();
    const bool valid = proxy.validate().empty();
    proxy.prepare();
    std::printf("after prepare: proxy %s, remote %s\n",
                AbstractModelComponent::statusToString(proxy.status()).c_str(),
                AbstractModelComponent::statusToString(remote.status()).c_str());
    if (!valid || proxy.status() != Status::Updated)
    {
        stopWorker();
        return fail("remote lifecycle did not reach Updated");
    }
    //! [lifecycle]

    //! [values]
    const std::array<int64_t, 1> shape{kReaches};
    std::array<double, kReaches> inflow{0.5, 1.0, 2.0};       // m³/s
    std::array<double, kReaches> expected{100.0, 200.0, 300.0}; // S, m³
    int step = 0;
    while (proxy.status() == Status::Updated)
    {
        // Push this step's inflow, step the remote component, pull its outflow.
        if (!proxy.setRemoteValues("inflow", reachBuffer(inflow, shape), message))
            break;
        proxy.update();
        std::array<double, kReaches> outflow{};
        if (!proxy.getRemoteValues("outflow", reachBuffer(outflow, shape), message))
            break;
        ++step;
        std::printf("step %d  proxy %-8s Q =", step,
                    AbstractModelComponent::statusToString(proxy.status()).c_str());
        for (int i = 0; i < kReaches; ++i)
        {
            expected[i] += kDt * (inflow[i] - kK * expected[i]);
            std::printf(" %.6f", outflow[i]);
            if (std::abs(outflow[i] - kK * expected[i]) > 1e-12)
            {
                stopWorker();
                return fail("remote outflow differs from the local computation");
            }
        }
        std::printf("\n");
        for (double &q : inflow)
            q *= 1.5; // a new inflow for the next step
    }
    if (proxy.status() != Status::Done || step != kSteps)
    {
        stopWorker();
        return fail("expected the proxy to mirror Done after the last step: " + message);
    }
    //! [values]

    //! [errors]
    // An unknown item is refused by the worker; the peer is alive, so the
    // proxy's status does not change.
    std::array<double, kReaches> unused{};
    const bool found = proxy.getRemoteValues("stage", reachBuffer(unused, shape), message);
    std::printf("getRemoteValues(\"stage\"): %s, \"%s\"\n", found ? "true" : "false",
                message.c_str());
    if (found || message.find("no data item 'stage'") == std::string::npos)
    {
        stopWorker();
        return fail("an unknown item should be refused");
    }
    //! [errors]

    proxy.finish();
    std::printf("after finish: proxy %s, remote %s\n",
                AbstractModelComponent::statusToString(proxy.status()).c_str(),
                AbstractModelComponent::statusToString(remote.status()).c_str());
    stopWorker();
    std::printf("worker served %d messages\n", served);
    if (proxy.status() != Status::Finished || remote.status() != Status::Finished)
        return fail("finish() was not mirrored");

    //! [deadpeer]
    // A proxy whose worker never answers: every request times out, and a
    // lifecycle call latches Failed with a Fatal entry on the error queue.
    std::shared_ptr<InProcHub> lonely = InProcHub::create(2);
    std::unique_ptr<InProcTransport> alone = lonely->createTransport(0);
    ProxyModelComponent orphan("orphan", alone.get(), 1, "nobody");
    orphan.setRequestTimeout(0.2);
    orphan.initialize();
    const std::vector<ErrorEntry> errors = orphan.errors();
    std::printf("orphan: %s, %zu error(s)\n",
                AbstractModelComponent::statusToString(orphan.status()).c_str(),
                errors.size());
    for (const ErrorEntry &entry : errors)
        std::printf("  %s: %s\n",
                    entry.severity == ErrorEntry::Severity::Fatal ? "fatal" : "error",
                    entry.message.c_str());
    if (orphan.status() != Status::Failed || errors.empty() ||
        errors.back().severity != ErrorEntry::Severity::Fatal)
        return fail("a dead peer should latch Failed with a Fatal diagnostic");
    //! [deadpeer]
    return 0;
}
