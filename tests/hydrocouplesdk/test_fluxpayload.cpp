// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the flux+derivative exchange payload convention (plan §5.4):
// the {2, N} Float64 block, typed row views, linearized boundary
// evaluation, convention validation of peer descriptors, and the proof
// that the payload travels through the ITransport seam like any other
// typed block.

#include <gtest/gtest.h>

#include "hydrocouplesdk/distributed/fluxpayload.h"
#include "hydrocouplesdk/distributed/inproctransport.h"

#include <array>
#include <thread>

using namespace HydroCouple;
using namespace HydroCouple::SDK::Distributed;

TEST(FluxPayloadTest, RowsViewOneContiguousBlock)
{
    FluxDerivativePayload payload(4);
    EXPECT_EQ(payload.count(), 4);

    for (int i = 0; i < 4; ++i)
    {
        payload.flux()[i]       = 10.0 * i;
        payload.derivative()[i] = 0.5 * i;
    }

    const BufferDescriptor descriptor = payload.descriptor();
    EXPECT_EQ(descriptor.kind, DataKind::Float64);
    EXPECT_EQ(descriptor.rank, 2);
    EXPECT_EQ(descriptor.shape[0], 2);
    EXPECT_EQ(descriptor.shape[1], 4);

    // Row-major {2, N}: flux row precedes derivative row in one block.
    const auto *values = static_cast<const double *>(descriptor.data);
    EXPECT_DOUBLE_EQ(values[1], 10.0); // flux[1]
    EXPECT_DOUBLE_EQ(values[5], 0.5);  // derivative[1]
}

TEST(FluxPayloadTest, LinearizedBoundaryEvaluation)
{
    FluxDerivativePayload payload(2);
    payload.flux()[0]       = 3.0;  // q0
    payload.derivative()[0] = 2.0;  // dq/dh

    // q(h) = q0 + dq/dh (h - h0)
    EXPECT_DOUBLE_EQ(payload.linearizedFlux(0, 1.5, 1.0), 3.0 + 2.0 * 0.5);
    EXPECT_DOUBLE_EQ(payload.linearizedFlux(0, 1.0, 1.0), 3.0);
    EXPECT_THROW((void)payload.linearizedFlux(5, 0.0, 0.0),
                 std::out_of_range);
}

TEST(FluxPayloadTest, ConventionValidation)
{
    FluxDerivativePayload payload(3);
    std::string message;

    EXPECT_TRUE(FluxDerivativePayload::conformsToConvention(
        payload.descriptor(), 3, message)) << message;
    EXPECT_TRUE(FluxDerivativePayload::conformsToConvention(
        payload.descriptor(), -1, message)) << message;

    // Wrong entity count.
    EXPECT_FALSE(FluxDerivativePayload::conformsToConvention(
        payload.descriptor(), 7, message));
    EXPECT_NE(message.find("expected 7"), std::string::npos);

    // Wrong kind.
    std::array<int32_t, 6> ints{};
    const std::array<int64_t, 2> shape{2, 3};
    BufferDescriptor wrongKind;
    wrongKind.data  = ints.data();
    wrongKind.kind  = DataKind::Int32;
    wrongKind.rank  = 2;
    wrongKind.shape = shape.data();
    wrongKind.space = MemorySpace::Host;
    EXPECT_FALSE(FluxDerivativePayload::conformsToConvention(wrongKind, 3,
                                                             message));

    // Wrong row dimension.
    const std::array<int64_t, 2> badShape{3, 3};
    wrongKind.kind  = DataKind::Float64;
    wrongKind.shape = badShape.data();
    EXPECT_FALSE(FluxDerivativePayload::conformsToConvention(wrongKind, 3,
                                                             message));
    EXPECT_NE(message.find("row dimension"), std::string::npos);
}

TEST(FluxPayloadTest, TravelsThroughTheTransportSeam)
{
    auto hub = InProcHub::create(2);
    auto t0  = hub->createTransport(0);
    auto t1  = hub->createTransport(1);

    FluxDerivativePayload sent(3);
    for (int i = 0; i < 3; ++i)
    {
        sent.flux()[i]       = 1.0 + i;
        sent.derivative()[i] = 0.1 * (i + 1);
    }

    std::thread producer([&] {
        std::string message;
        ASSERT_TRUE(t0->send({"inproc", 1}, 9, sent.descriptor(), &message))
            << message;
    });

    FluxDerivativePayload received(3);
    std::string message;
    ASSERT_TRUE(t1->receive({"inproc", 0}, 9, received.descriptor(),
                            &message)) << message;
    producer.join();

    ASSERT_TRUE(FluxDerivativePayload::conformsToConvention(
        received.descriptor(), 3, message)) << message;
    EXPECT_DOUBLE_EQ(received.flux()[2], 3.0);
    EXPECT_DOUBLE_EQ(received.derivative()[2], 0.3);
    EXPECT_DOUBLE_EQ(received.linearizedFlux(2, 2.0, 1.0),
                     3.0 + 0.3 * 1.0);
}

TEST(FluxPayloadTest, TransportRejectsShapeMismatch)
{
    auto hub = InProcHub::create(2);
    auto t0  = hub->createTransport(0);
    auto t1  = hub->createTransport(1);

    FluxDerivativePayload sent(3);
    std::thread producer([&] {
        std::string message;
        ASSERT_TRUE(t0->send({"inproc", 1}, 4, sent.descriptor(), &message))
            << message;
    });

    FluxDerivativePayload smaller(2); // wrong N — unpack must refuse
    std::string message;
    EXPECT_FALSE(t1->receive({"inproc", 0}, 4, smaller.descriptor(),
                             &message));
    EXPECT_FALSE(message.empty());
    producer.join();
}
