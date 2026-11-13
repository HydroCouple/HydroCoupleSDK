// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the distributed protocol foundations: the 32-byte Envelope,
// ValuePacker typed wire blocks (contiguous fast path and strided gather),
// and InProcTransport tagged blocking/async exchange across threads — the
// whole protocol unit-testable without mpirun (plan §5.1).

#include <gtest/gtest.h>

#include "hydrocouplesdk/core/diagnosticcodes.h"

#include "hydrocouplesdk/distributed/envelope.h"
#include "hydrocouplesdk/distributed/valuepacker.h"
#include "hydrocouplesdk/distributed/inproctransport.h"

#include <array>
#include <cstring>
#include <numeric>
#include <thread>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK::Distributed;

namespace {

BufferDescriptor hostBuffer(void *data, DataKind kind,
                            std::span<const int64_t> shape,
                            const int64_t *stridesBytes = nullptr)
{
    BufferDescriptor descriptor;
    descriptor.data         = data;
    descriptor.kind         = kind;
    descriptor.rank         = static_cast<int32_t>(shape.size());
    descriptor.shape        = shape.data();
    descriptor.stridesBytes = stridesBytes;
    descriptor.space        = MemorySpace::Host;
    return descriptor;
}

} // namespace

// ── Envelope ──────────────────────────────────────────────────────────────

TEST(EnvelopeTest, IsExactly32TriviallyCopyableBytes)
{
    EXPECT_EQ(sizeof(Envelope), 32u);
    EXPECT_TRUE(std::is_trivially_copyable_v<Envelope>);
}

TEST(EnvelopeTest, FactoryStampsFieldsAndValidates)
{
    const Envelope envelope =
        makeEnvelope(OpCode::Update, /*source=*/2, /*target=*/0,
                     /*tag=*/77, /*payloadBytes=*/128, /*epoch=*/5);
    EXPECT_TRUE(envelope.valid());
    EXPECT_EQ(envelope.op(), OpCode::Update);
    EXPECT_EQ(envelope.sourceRank, 2);
    EXPECT_EQ(envelope.targetRank, 0);
    EXPECT_EQ(envelope.tag, 77u);
    EXPECT_EQ(envelope.epoch, 5u);
    EXPECT_EQ(envelope.payloadBytes, 128u);
}

TEST(EnvelopeTest, RoundTripsAsRawBytes)
{
    const Envelope out = makeEnvelope(OpCode::GetValues, 1, 3, 42, 9);
    std::array<std::byte, sizeof(Envelope)> wire{};
    std::memcpy(wire.data(), &out, sizeof(out));
    Envelope in;
    std::memcpy(&in, wire.data(), sizeof(in));
    EXPECT_TRUE(in.valid());
    EXPECT_EQ(in.op(), OpCode::GetValues);
    EXPECT_EQ(in.tag, 42u);
}

TEST(EnvelopeTest, RejectsCorruptedMagic)
{
    Envelope envelope = makeEnvelope(OpCode::Ack, 0, 1, 0);
    envelope.magic = 0xDEADBEEF;
    EXPECT_FALSE(envelope.valid());
}

// ── ValuePacker ───────────────────────────────────────────────────────────

TEST(ValuePackerTest, ContiguousRoundTrip)
{
    std::vector<double> source(64);
    std::iota(source.begin(), source.end(), 0.0);
    const std::array<int64_t, 2> shape{8, 8};
    const auto descriptor =
        hostBuffer(source.data(), DataKind::Float64, shape);

    std::vector<std::byte> wire;
    std::string message;
    ASSERT_TRUE(ValuePacker::pack(descriptor, wire, message)) << message;
    EXPECT_EQ(static_cast<int64_t>(wire.size()),
              ValuePacker::packedSize(descriptor));

    DataKind kind = DataKind::Unknown;
    std::vector<int64_t> peekedShape;
    ASSERT_TRUE(ValuePacker::peek(wire, kind, peekedShape, message)) << message;
    EXPECT_EQ(kind, DataKind::Float64);
    EXPECT_EQ(peekedShape, (std::vector<int64_t>{8, 8}));

    std::vector<double> target(64, -1.0);
    const auto into = hostBuffer(target.data(), DataKind::Float64, shape);
    ASSERT_TRUE(ValuePacker::unpack(wire, into, message)) << message;
    EXPECT_EQ(target, source);
}

TEST(ValuePackerTest, StridedSourceIsGatheredOnce)
{
    // Every other element of a 8-double scratch: logical shape {4}.
    std::array<double, 8> scratch{1, -9, 2, -9, 3, -9, 4, -9};
    const std::array<int64_t, 1> shape{4};
    const int64_t stride = 2 * static_cast<int64_t>(sizeof(double));
    const auto descriptor =
        hostBuffer(scratch.data(), DataKind::Float64, shape, &stride);

    std::vector<std::byte> wire;
    std::string message;
    ASSERT_TRUE(ValuePacker::pack(descriptor, wire, message)) << message;

    std::array<double, 4> target{};
    const auto into = hostBuffer(target.data(), DataKind::Float64, shape);
    ASSERT_TRUE(ValuePacker::unpack(wire, into, message)) << message;
    EXPECT_EQ(target, (std::array<double, 4>{1, 2, 3, 4}));
}

TEST(ValuePackerTest, UnpackRejectsKindMismatch)
{
    std::array<int32_t, 4> source{1, 2, 3, 4};
    const std::array<int64_t, 1> shape{4};
    std::vector<std::byte> wire;
    std::string message;
    ASSERT_TRUE(ValuePacker::pack(
        hostBuffer(source.data(), DataKind::Int32, shape), wire, message));

    std::array<double, 4> target{};
    EXPECT_FALSE(ValuePacker::unpack(
        wire, hostBuffer(target.data(), DataKind::Float64, shape), message));
    EXPECT_NE(message.find("kind"), std::string::npos);
}

TEST(ValuePackerTest, UnpackRejectsTruncatedBlock)
{
    std::array<double, 4> source{1, 2, 3, 4};
    const std::array<int64_t, 1> shape{4};
    std::vector<std::byte> wire;
    std::string message;
    ASSERT_TRUE(ValuePacker::pack(
        hostBuffer(source.data(), DataKind::Float64, shape), wire, message));
    wire.resize(wire.size() - 8);

    std::array<double, 4> target{};
    EXPECT_FALSE(ValuePacker::unpack(
        wire, hostBuffer(target.data(), DataKind::Float64, shape), message));
    EXPECT_FALSE(message.empty());
}

// ── InProcTransport ───────────────────────────────────────────────────────

class InProcTransportTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_hub = InProcHub::create(3);
        m_t0  = m_hub->createTransport(0);
        m_t1  = m_hub->createTransport(1);
        m_t2  = m_hub->createTransport(2);
    }

    std::shared_ptr<InProcHub>       m_hub;
    std::unique_ptr<InProcTransport> m_t0, m_t1, m_t2;
};

TEST_F(InProcTransportTest, EndpointsAndParticipants)
{
    EXPECT_EQ(m_hub->participantCount(), 3);
    EXPECT_EQ(m_t1->localEndpoint().rank, 1);
    EXPECT_EQ(m_t1->participantCount(), 3);
    EXPECT_EQ(m_hub->createTransport(99), nullptr);
}

TEST_F(InProcTransportTest, TaggedSendReceiveAcrossThreads)
{
    std::array<double, 4> payload{1.5, 2.5, 3.5, 4.5};
    const std::array<int64_t, 1> shape{4};

    std::thread producer([&] {
        std::string message;
        ASSERT_TRUE(m_t0->send({"inproc", 1}, /*tag=*/7,
                               hostBuffer(payload.data(), DataKind::Float64,
                                          shape),
                               &message)) << message;
    });

    std::array<double, 4> received{};
    std::string message;
    ASSERT_TRUE(m_t1->receive({"inproc", 0}, 7,
                              hostBuffer(received.data(), DataKind::Float64,
                                         shape),
                              &message)) << message;
    producer.join();
    EXPECT_EQ(received, payload);
}

TEST_F(InProcTransportTest, TagsMatchOutOfOrder)
{
    std::array<double, 1> first{1.0}, second{2.0};
    const std::array<int64_t, 1> shape{1};
    std::string message;
    // Send tag 1 then tag 2; receive tag 2 first.
    ASSERT_TRUE(m_t0->send({"inproc", 1}, 1,
                           hostBuffer(first.data(), DataKind::Float64, shape),
                           &message));
    ASSERT_TRUE(m_t0->send({"inproc", 1}, 2,
                           hostBuffer(second.data(), DataKind::Float64, shape),
                           &message));

    std::array<double, 1> received{};
    ASSERT_TRUE(m_t1->receive({"inproc", 0}, 2,
                              hostBuffer(received.data(), DataKind::Float64,
                                         shape),
                              &message));
    EXPECT_DOUBLE_EQ(received[0], 2.0);
    ASSERT_TRUE(m_t1->receive({"inproc", 0}, 1,
                              hostBuffer(received.data(), DataKind::Float64,
                                         shape),
                              &message));
    EXPECT_DOUBLE_EQ(received[0], 1.0);
}

TEST_F(InProcTransportTest, AnySourceReceive)
{
    std::array<double, 1> payload{9.0};
    const std::array<int64_t, 1> shape{1};
    std::string message;
    ASSERT_TRUE(m_t2->send({"inproc", 0}, 5,
                           hostBuffer(payload.data(), DataKind::Float64,
                                      shape),
                           &message));

    std::array<double, 1> received{};
    // Source rank -1 = any sender.
    ASSERT_TRUE(m_t0->receive({"inproc", -1}, 5,
                              hostBuffer(received.data(), DataKind::Float64,
                                         shape),
                              &message));
    EXPECT_DOUBLE_EQ(received[0], 9.0);
}

TEST_F(InProcTransportTest, AsyncReceiveCompletesViaTestAndWait)
{
    const std::array<int64_t, 1> shape{2};
    std::array<double, 2> received{};
    auto request = m_t1->receiveAsync(
        {"inproc", 0}, 3,
        hostBuffer(received.data(), DataKind::Float64, shape));
    ASSERT_NE(request, nullptr);
    EXPECT_FALSE(request->test()); // nothing sent yet

    std::array<double, 2> payload{4.0, 8.0};
    std::string message;
    ASSERT_TRUE(m_t0->send({"inproc", 1}, 3,
                           hostBuffer(payload.data(), DataKind::Float64,
                                      shape),
                           &message));

    request->wait();
    std::string failure;
    EXPECT_FALSE(request->failed(failure)) << failure;
    EXPECT_DOUBLE_EQ(received[1], 8.0);
}

TEST_F(InProcTransportTest, WaitForIsBoundedThenCompletes)
{
    const std::array<int64_t, 1> shape{1};
    std::array<double, 1> received{};
    auto request = m_t1->receiveAsync(
        {"inproc", 0}, 4,
        hostBuffer(received.data(), DataKind::Float64, shape));
    ASSERT_NE(request, nullptr);
    EXPECT_FALSE(request->waitFor(0.01)); // nothing sent: returns, not a hang
    EXPECT_FALSE(request->waitFor(0.0));  // non-positive tests once

    std::array<double, 1> payload{5.0};
    std::string message;
    ASSERT_TRUE(m_t0->send({"inproc", 1}, 4,
                           hostBuffer(payload.data(), DataKind::Float64,
                                      shape),
                           &message));
    EXPECT_TRUE(request->waitFor(5.0));
    std::string failure;
    EXPECT_FALSE(request->failed(failure)) << failure;
    EXPECT_DOUBLE_EQ(received[0], 5.0);
}

TEST_F(InProcTransportTest, TypeMismatchSurfacesOnReceive)
{
    std::array<int32_t, 2> payload{1, 2};
    const std::array<int64_t, 1> shape{2};
    std::string message;
    ASSERT_TRUE(m_t0->send({"inproc", 1}, 4,
                           hostBuffer(payload.data(), DataKind::Int32, shape),
                           &message));

    std::array<double, 2> received{};
    EXPECT_FALSE(m_t1->receive({"inproc", 0}, 4,
                               hostBuffer(received.data(), DataKind::Float64,
                                          shape),
                               &message));
    EXPECT_FALSE(message.empty());
}

TEST_F(InProcTransportTest, InvalidTargetRankFails)
{
    std::array<double, 1> payload{1.0};
    const std::array<int64_t, 1> shape{1};
    std::string message;
    EXPECT_FALSE(m_t0->send({"inproc", 42}, 0,
                            hostBuffer(payload.data(), DataKind::Float64,
                                       shape),
                            &message));
    EXPECT_NE(message.find("42"), std::string::npos);
}

TEST_F(InProcTransportTest, EnvelopePlusPayloadPattern)
{
    // The protocol's two-phase pattern: fixed-size envelope receive, then
    // an exact-size payload receive.
    const std::array<int64_t, 1> envelopeShape{
        static_cast<int64_t>(sizeof(Envelope))};
    std::array<double, 3> values{1.0, 2.0, 3.0};
    const std::array<int64_t, 1> valueShape{3};

    std::thread worker([&] {
        std::string message;
        // Receive the envelope as raw bytes.
        Envelope envelope;
        ASSERT_TRUE(m_t1->receive(
            {"inproc", 0}, 100,
            hostBuffer(&envelope, DataKind::UInt8, envelopeShape), &message))
            << message;
        ASSERT_TRUE(envelope.valid());
        ASSERT_EQ(envelope.op(), OpCode::SetValues);
        ASSERT_EQ(envelope.payloadBytes, 3 * sizeof(double));
        // Then the payload under the same tag.
        std::array<double, 3> received{};
        ASSERT_TRUE(m_t1->receive(
            {"inproc", 0}, 100,
            hostBuffer(received.data(), DataKind::Float64, valueShape),
            &message)) << message;
        EXPECT_DOUBLE_EQ(received[2], 3.0);
    });

    Envelope envelope = makeEnvelope(OpCode::SetValues, 0, 1, 100,
                                     3 * sizeof(double));
    std::string message;
    ASSERT_TRUE(m_t0->send({"inproc", 1}, 100,
                           hostBuffer(&envelope, DataKind::UInt8,
                                      envelopeShape),
                           &message)) << message;
    ASSERT_TRUE(m_t0->send({"inproc", 1}, 100,
                           hostBuffer(values.data(), DataKind::Float64,
                                      valueShape),
                           &message)) << message;
    worker.join();
}

// ── Proxy / worker command loop (plan §5.3) ───────────────────────────────

#include "hydrocouplesdk/distributed/componentworker.h"
#include "hydrocouplesdk/distributed/proxymodelcomponent.h"
#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <memory>

namespace {

using SDK::AbstractModelComponent;
using SDK::Dimension;
using SDK::Quantity;
using SDK::Input1DDouble;
using SDK::Output1DDouble;

/*!
 * Remote-side component: doubles its "inflow" input into its "flow"
 * output on every update().
 */
class RemoteDoublerComponent : public AbstractModelComponent
{
public:
    explicit RemoteDoublerComponent(std::string_view id)
        : AbstractModelComponent(id)
    {}

    std::vector<std::string> validate() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Validating);
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Valid);
        return {};
    }
    void prepare() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Preparing);
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Updated);
    }
    void update(const std::vector<HydroCouple::IOutput *> & = {}) override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Updating);
        for (int i = 0; i < inflow->length(); ++i)
            (*flow)[i] = 2.0 * (*inflow)[i];
        ++updateCount;
        setStatus(stepsUntilDone > 0 && updateCount >= stepsUntilDone
                      ? HydroCouple::IModelComponent::ComponentStatus::Done
                      : HydroCouple::IModelComponent::ComponentStatus::Updated);
    }
    void finish() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Finishing);
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Finished);
    }

    Input1DDouble  *inflow = nullptr;
    Output1DDouble *flow   = nullptr;
    int stepsUntilDone = -1; //!< > 0: report Done after this many updates.
    int             updateCount = 0;

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        inflow = new Input1DDouble("inflow", &m_dim, 3, m_quantity.get(), this);
        addInput(inflow);
    }
    void createOutputs() override
    {
        m_quantity.reset(Quantity::unitLess("RemoteQ"));
        flow = new Output1DDouble("flow", &m_dim, 3, m_quantity.get(), this);
        addOutput(flow);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension m_dim{"i", "Index"};
    std::unique_ptr<Quantity> m_quantity;
};

} // namespace

class ProxyWorkerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_hub             = InProcHub::create(2);
        m_proxyTransport  = m_hub->createTransport(0);
        m_workerTransport = m_hub->createTransport(1);
        m_remote          = std::make_unique<RemoteDoublerComponent>("doubler");
        // Outputs are created during initialize(); pre-create so createOutputs
        // ordering (quantity before inputs) is respected via lazy arguments +
        // explicit initialize on the worker side driven remotely.
        m_worker = std::make_unique<ComponentWorker>(m_workerTransport.get(),
                                                     m_remote.get());
        m_workerThread = std::thread([this] { m_served = m_worker->run(); });

        m_proxy = std::make_unique<ProxyModelComponent>(
            "doubler-proxy", m_proxyTransport.get(), /*remoteRank=*/1,
            "doubler");
        m_proxy->setRequestTimeout(5.0);
    }

    void TearDown() override
    {
        if (m_workerThread.joinable())
        {
            // Orderly shutdown via the protocol.
            std::string message;
            Envelope shutdown = makeEnvelope(
                OpCode::Shutdown, 0, 1, /*tag=*/9999, 0);
            const int64_t shape = static_cast<int64_t>(sizeof(Envelope));
            BufferDescriptor descriptor;
            descriptor.data  = &shutdown;
            descriptor.kind  = DataKind::UInt8;
            descriptor.rank  = 1;
            descriptor.shape = &shape;
            descriptor.space = MemorySpace::Host;
            (void)m_proxyTransport->send({"inproc", 1}, 0, descriptor,
                                         &message);
            m_workerThread.join();
        }
    }

    std::shared_ptr<InProcHub>               m_hub;
    std::unique_ptr<InProcTransport>         m_proxyTransport;
    std::unique_ptr<InProcTransport>         m_workerTransport;
    std::unique_ptr<RemoteDoublerComponent>  m_remote;
    std::unique_ptr<ComponentWorker>         m_worker;
    std::unique_ptr<ProxyModelComponent>     m_proxy;
    std::thread                              m_workerThread;
    int                                      m_served = 0;
};

TEST_F(ProxyWorkerTest, ConnectHandshakeAndPing)
{
    std::string message;
    EXPECT_FALSE(m_proxy->isConnected());
    ASSERT_TRUE(m_proxy->connectToPeer(message)) << message;
    EXPECT_TRUE(m_proxy->isConnected());
    EXPECT_TRUE(m_proxy->ping(2.0));
    m_proxy->disconnectFromPeer();
    EXPECT_FALSE(m_proxy->isConnected());
}

TEST_F(ProxyWorkerTest, RemoteLifecycleMirrorsStatus)
{
    using Status = IModelComponent::ComponentStatus;
    std::string message;
    ASSERT_TRUE(m_proxy->connectToPeer(message)) << message;

    m_proxy->initialize();
    EXPECT_EQ(m_proxy->status(), Status::Initialized);
    EXPECT_EQ(m_remote->status(), Status::Initialized);

    EXPECT_TRUE(m_proxy->validate().empty());
    EXPECT_EQ(m_proxy->status(), Status::Valid);
    m_proxy->prepare();
    EXPECT_EQ(m_proxy->status(), Status::Updated);
    m_proxy->update();
    EXPECT_EQ(m_remote->updateCount, 1);
    EXPECT_EQ(m_proxy->status(), Status::Updated);
    m_proxy->finish();
    EXPECT_EQ(m_proxy->status(), Status::Finished);
    EXPECT_EQ(m_remote->status(), Status::Finished);

    // The proxy walked the normative table: nothing illegal was reported.
    for (const auto &entry : m_proxy->errors())
        EXPECT_NE(entry.code, HydroCouple::SDK::Diagnostic::IllegalStatusTransition)
            << entry.message;
}

TEST_F(ProxyWorkerTest, ProxyMirrorsTheRemoteReachingDone)
{
    // Before the status bit in the reply envelope the proxy reported Updated
    // after every remote step, so a time-stepped orchestrator waiting for
    // Done on a remote component never stopped.
    using Status = IModelComponent::ComponentStatus;
    std::string message;
    ASSERT_TRUE(m_proxy->connectToPeer(message)) << message;
    m_remote->stepsUntilDone = 2;

    m_proxy->initialize();
    ASSERT_TRUE(m_proxy->validate().empty());
    m_proxy->prepare();
    m_proxy->update();
    EXPECT_EQ(m_proxy->status(), Status::Updated);
    m_proxy->update();
    EXPECT_EQ(m_remote->status(), Status::Done);
    EXPECT_EQ(m_proxy->status(), Status::Done);
    m_proxy->finish();
    EXPECT_EQ(m_proxy->status(), Status::Finished);
}

TEST_F(ProxyWorkerTest, RemoteValueRoundTripThroughUpdate)
{
    std::string message;
    ASSERT_TRUE(m_proxy->connectToPeer(message)) << message;
    m_proxy->initialize();
    ASSERT_TRUE(m_proxy->validate().empty());
    m_proxy->prepare();

    // Push inflow {1,2,3} → remote update() doubles → pull flow {2,4,6}.
    std::array<double, 3> inflow{1.0, 2.0, 3.0};
    const std::array<int64_t, 1> shape{3};
    ASSERT_TRUE(m_proxy->setRemoteValues(
        "inflow", hostBuffer(inflow.data(), DataKind::Float64, shape),
        message)) << message;

    m_proxy->update();

    std::array<double, 3> flow{};
    ASSERT_TRUE(m_proxy->getRemoteValues(
        "flow", hostBuffer(flow.data(), DataKind::Float64, shape), message))
        << message;
    EXPECT_EQ(flow, (std::array<double, 3>{2.0, 4.0, 6.0}));
}

TEST_F(ProxyWorkerTest, UnknownItemSurfacesRemoteError)
{
    std::string message;
    ASSERT_TRUE(m_proxy->connectToPeer(message)) << message;
    m_proxy->initialize();

    std::array<double, 3> values{};
    const std::array<int64_t, 1> shape{3};
    EXPECT_FALSE(m_proxy->getRemoteValues(
        "no_such_item", hostBuffer(values.data(), DataKind::Float64, shape),
        message));
    EXPECT_NE(message.find("no_such_item"), std::string::npos);
}

TEST(ProxyWorkerDeath, DeadPeerLatchesFailedWithDiagnostic)
{
    // No worker running: every request times out; the proxy must latch
    // Failed and queue a Fatal diagnostic (kill-a-worker semantics).
    auto hub       = InProcHub::create(2);
    auto transport = hub->createTransport(0);
    ProxyModelComponent proxy("orphan-proxy", transport.get(), 1, "ghost");
    proxy.setRequestTimeout(0.2);

    std::string message;
    EXPECT_FALSE(proxy.connectToPeer(message));
    EXPECT_NE(message.find("timed out"), std::string::npos);

    proxy.initialize();
    EXPECT_EQ(proxy.status(), IModelComponent::ComponentStatus::Failed);
    const auto errors = proxy.errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors.back().severity, ErrorEntry::Severity::Fatal);
    EXPECT_NE(errors.back().message.find("timed out"), std::string::npos);
}

// ── PartitionedDataItem + HaloExchanger (plan §5.4) ───────────────────────

#include "hydrocouplesdk/distributed/partitioneddataitem.h"

namespace {

/*!
 * Two-rank decomposition of a 10-entity 1-D domain with one-deep halos:
 *   rank 0 owns 0..4 and mirrors {5} (owner 1)
 *   rank 1 owns 5..9 and mirrors {4} (owner 0)
 */
struct TwoRankFixture
{
    std::shared_ptr<InProcHub>       hub = InProcHub::create(2);
    std::unique_ptr<InProcTransport> t0  = hub->createTransport(0);
    std::unique_ptr<InProcTransport> t1  = hub->createTransport(1);

    PartitionedDataItemDouble item0{
        "field", 10, {0, 1, 2, 3, 4}, {5}, {1}, t0.get()};
    PartitionedDataItemDouble item1{
        "field", 10, {5, 6, 7, 8, 9}, {4}, {0}, t1.get()};
};

} // namespace

TEST(PartitionedDataItemTest, ExposesDecompositionMetadata)
{
    TwoRankFixture fixture;
    EXPECT_EQ(fixture.item0.globalCount(), 10);
    EXPECT_EQ(fixture.item0.ownedGlobalIndexes().size(), 5u);
    EXPECT_EQ(fixture.item0.virtualGlobalIndexes()[0], 5);
    EXPECT_EQ(fixture.item0.virtualOwners()[0], 1);
    EXPECT_EQ(fixture.item0.shape(), (std::vector<int64_t>{6}));
    EXPECT_EQ(fixture.item0.dataKind(), DataKind::Float64);
    EXPECT_EQ(fixture.item0.synchronizationEpoch(), 0u);
}

TEST(PartitionedDataItemTest, DataPlaneSpansOwnedThenVirtual)
{
    TwoRankFixture fixture;
    auto owned = fixture.item0.ownedValues();
    for (size_t i = 0; i < owned.size(); ++i)
        owned[i] = static_cast<double>(i + 1);

    std::array<double, 6> block{};
    const std::array<int64_t, 1> shape{6}, zero{0};
    std::string message;
    ASSERT_TRUE(fixture.item0.getValuesInto(
        hostBuffer(block.data(), DataKind::Float64, shape), zero, shape,
        &message)) << message;
    EXPECT_DOUBLE_EQ(block[4], 5.0);
    EXPECT_DOUBLE_EQ(block[5], 0.0); // virtual mirror, not yet exchanged
}

TEST(HaloExchangerTest, TwoRankExchangeFillsMirrors)
{
    TwoRankFixture fixture;
    // rank 0 values: global g → 100+g; rank 1: global g → 200+g.
    for (size_t i = 0; i < 5; ++i)
    {
        fixture.item0.ownedValues()[i] = 100.0 + static_cast<double>(i);
        fixture.item1.ownedValues()[i] = 200.0 + 5.0 + static_cast<double>(i);
    }

    // Both ranks run pattern + exchange concurrently (collective).
    std::string message0, message1;
    std::thread rank1([&] {
        auto request = fixture.item1.synchronizeAsync();
        request->wait();
        std::string failure;
        ASSERT_FALSE(request->failed(failure)) << failure;
    });
    auto request = fixture.item0.synchronizeAsync();
    request->wait();
    std::string failure;
    ASSERT_FALSE(request->failed(failure)) << failure;
    rank1.join();

    // rank 0's mirror of global 5 came from rank 1 (200+5); vice versa.
    EXPECT_DOUBLE_EQ(fixture.item0.virtualValues()[0], 205.0);
    EXPECT_DOUBLE_EQ(fixture.item1.virtualValues()[0], 104.0);
    EXPECT_EQ(fixture.item0.synchronizationEpoch(), 1u);
    EXPECT_EQ(fixture.item1.synchronizationEpoch(), 1u);
}

TEST(HaloExchangerTest, GatherHappensAtBeginOverlapIsSafe)
{
    TwoRankFixture fixture;
    fixture.item0.ownedValues()[4] = 42.0; // global 4, mirrored by rank 1
    fixture.item1.ownedValues()[0] = 7.0;  // global 5, mirrored by rank 0

    std::thread rank1([&] {
        auto request = fixture.item1.synchronizeAsync();
        // Overlap window: overwrite the owned value AFTER begin. The halo
        // received by rank 0 must carry the value as of begin (7.0).
        fixture.item1.ownedValues()[0] = -999.0;
        request->wait();
        std::string failure;
        ASSERT_FALSE(request->failed(failure)) << failure;
    });
    auto request = fixture.item0.synchronizeAsync();
    fixture.item0.ownedValues()[4] = -999.0; // same on rank 0
    request->wait();
    std::string failure;
    ASSERT_FALSE(request->failed(failure)) << failure;
    rank1.join();

    EXPECT_DOUBLE_EQ(fixture.item0.virtualValues()[0], 7.0);
    EXPECT_DOUBLE_EQ(fixture.item1.virtualValues()[0], 42.0);
}

TEST(HaloExchangerTest, RepeatedEpochsCarryFreshValues)
{
    TwoRankFixture fixture;
    for (int step = 1; step <= 3; ++step)
    {
        fixture.item0.ownedValues()[4] = 10.0 * step;
        fixture.item1.ownedValues()[0] = 20.0 * step;
        std::thread rank1([&] {
            auto request = fixture.item1.synchronizeAsync();
            request->wait();
        });
        auto request = fixture.item0.synchronizeAsync();
        request->wait();
        rank1.join();
        EXPECT_DOUBLE_EQ(fixture.item0.virtualValues()[0], 20.0 * step);
        EXPECT_DOUBLE_EQ(fixture.item1.virtualValues()[0], 10.0 * step);
    }
    EXPECT_EQ(fixture.item0.synchronizationEpoch(), 3u);
}

TEST(HaloExchangerTest, ThreeRankRingExchange)
{
    // 12 entities over 3 ranks (4 each); each rank mirrors its right
    // neighbor's first owned entity (periodic ring).
    auto hub = InProcHub::create(3);
    auto t0  = hub->createTransport(0);
    auto t1  = hub->createTransport(1);
    auto t2  = hub->createTransport(2);

    PartitionedDataItemDouble item0("f", 12, {0, 1, 2, 3}, {4}, {1}, t0.get());
    PartitionedDataItemDouble item1("f", 12, {4, 5, 6, 7}, {8}, {2}, t1.get());
    PartitionedDataItemDouble item2("f", 12, {8, 9, 10, 11}, {0}, {0},
                                    t2.get());

    item0.ownedValues()[0] = 1000.0; // global 0
    item1.ownedValues()[0] = 1004.0; // global 4
    item2.ownedValues()[0] = 1008.0; // global 8

    std::thread w1([&] { item1.synchronizeAsync()->wait(); });
    std::thread w2([&] { item2.synchronizeAsync()->wait(); });
    item0.synchronizeAsync()->wait();
    w1.join();
    w2.join();

    EXPECT_DOUBLE_EQ(item0.virtualValues()[0], 1004.0); // from rank 1
    EXPECT_DOUBLE_EQ(item1.virtualValues()[0], 1008.0); // from rank 2
    EXPECT_DOUBLE_EQ(item2.virtualValues()[0], 1000.0); // from rank 0
}

TEST(HaloExchangerTest, MisattributedOwnershipFailsPatternBuild)
{
    auto hub = InProcHub::create(2);
    auto t0  = hub->createTransport(0);
    auto t1  = hub->createTransport(1);

    // rank 0 asks rank 1 for global 3 — which rank 1 does not own.
    HaloExchanger wrong(t0.get(), {0, 1, 2, 3, 4}, {3}, {1});
    HaloExchanger other(t1.get(), {5, 6, 7, 8, 9}, {}, {});

    std::string message0, message1;
    bool ok0 = false, ok1 = false;
    std::thread rank1([&] { ok1 = other.buildPattern(message1); });
    ok0 = wrong.buildPattern(message0);
    rank1.join();

    // The owner-side validation catches the bad request.
    EXPECT_FALSE(ok1);
    EXPECT_NE(message1.find("does not own"), std::string::npos);
    (void)ok0;
}

TEST(HaloExchangerTest, SelfOwnedVirtualIsRejectedUpFront)
{
    auto hub = InProcHub::create(2);
    auto t0  = hub->createTransport(0);
    HaloExchanger exchanger(t0.get(), {0, 1}, {2}, {0}); // owner == self
    std::string message;
    EXPECT_FALSE(exchanger.buildPattern(message));
    EXPECT_NE(message.find("own"), std::string::npos);
}
