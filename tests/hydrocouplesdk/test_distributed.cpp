// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the distributed protocol foundations: the 32-byte Envelope,
// ValuePacker typed wire blocks (contiguous fast path and strided gather),
// and InProcTransport tagged blocking/async exchange across threads — the
// whole protocol unit-testable without mpirun (plan §5.1).

#include <gtest/gtest.h>

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
