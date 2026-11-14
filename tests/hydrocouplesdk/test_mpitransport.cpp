// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// MPI-launched tests for MpiTransport (plan §5.1 backend #2) — a separate
// executable with its own main (MpiEnvironment) because gtest_main cannot
// initialize MPI. Run under `mpirun -n 3`; every test skips gracefully
// with fewer ranks. Proves the protocol seam: the same ValuePacker wire
// blocks, tag matching, and HaloExchanger/PartitionedDataItem machinery
// verified over InProcTransport run unchanged over MPI.

#include <gtest/gtest.h>
#include <mpi.h>

#include "hydrocouplesdk/distributed/mpitransport.h"
#include "hydrocouplesdk/distributed/partitioneddataitem.h"

#include <array>
#include <numeric>
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

/*!
 * Keeps ranks in lockstep between tests so tag traffic cannot bleed.
 */
class MpiTransportTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        MPI_Barrier(MPI_COMM_WORLD);
        m_transport = std::make_unique<MpiTransport>();
        m_rank      = m_transport->localEndpoint().rank;
        m_size      = m_transport->participantCount();
    }

    void TearDown() override { MPI_Barrier(MPI_COMM_WORLD); }

    std::unique_ptr<MpiTransport> m_transport;
    int32_t                       m_rank = -1;
    int32_t                       m_size = 0;
};

} // namespace

TEST_F(MpiTransportTest, EndpointsMatchTheLauncher)
{
    int worldRank = -1, worldSize = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
    MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
    EXPECT_EQ(m_rank, worldRank);
    EXPECT_EQ(m_size, worldSize);
    EXPECT_EQ(m_transport->localEndpoint().address, "mpi");
    EXPECT_TRUE(MpiTransport::available());
}

TEST_F(MpiTransportTest, BlockingTypedRoundTrip)
{
    if (m_size < 2)
        GTEST_SKIP() << "needs 2 ranks";

    const std::array<int64_t, 1> shape{4};
    if (m_rank == 0)
    {
        std::array<double, 4> payload{1.5, 2.5, 3.5, 4.5};
        std::string message;
        ASSERT_TRUE(m_transport->send(
            {"mpi", 1}, 10,
            hostBuffer(payload.data(), DataKind::Float64, shape), &message))
            << message;
    }
    else if (m_rank == 1)
    {
        std::array<double, 4> received{};
        std::string message;
        ASSERT_TRUE(m_transport->receive(
            {"mpi", 0}, 10,
            hostBuffer(received.data(), DataKind::Float64, shape), &message))
            << message;
        EXPECT_EQ(received, (std::array<double, 4>{1.5, 2.5, 3.5, 4.5}));
    }
}

TEST_F(MpiTransportTest, AnySourceAndOutOfOrderTags)
{
    if (m_size < 3)
        GTEST_SKIP() << "needs 3 ranks";

    const std::array<int64_t, 1> shape{1};
    if (m_rank == 1 || m_rank == 2)
    {
        std::array<double, 1> value{static_cast<double>(m_rank)};
        std::string message;
        ASSERT_TRUE(m_transport->send(
            {"mpi", 0}, 20 + m_rank,
            hostBuffer(value.data(), DataKind::Float64, shape), &message))
            << message;
    }
    else if (m_rank == 0)
    {
        // Receive tag 22 (from rank 2) before tag 21, each from any source.
        std::array<double, 1> fromTwo{}, fromOne{};
        std::string message;
        ASSERT_TRUE(m_transport->receive(
            {"mpi", -1}, 22,
            hostBuffer(fromTwo.data(), DataKind::Float64, shape), &message))
            << message;
        ASSERT_TRUE(m_transport->receive(
            {"mpi", -1}, 21,
            hostBuffer(fromOne.data(), DataKind::Float64, shape), &message))
            << message;
        EXPECT_DOUBLE_EQ(fromTwo[0], 2.0);
        EXPECT_DOUBLE_EQ(fromOne[0], 1.0);
    }
}

TEST_F(MpiTransportTest, AsyncSendAndMatchedProbeReceive)
{
    if (m_size < 2)
        GTEST_SKIP() << "needs 2 ranks";

    const std::array<int64_t, 1> shape{8};
    if (m_rank == 0)
    {
        std::vector<double> payload(8);
        std::iota(payload.begin(), payload.end(), 0.0);
        auto request = m_transport->sendAsync(
            {"mpi", 1}, 30,
            hostBuffer(payload.data(), DataKind::Float64, shape));
        request->wait();
        std::string message;
        EXPECT_FALSE(request->failed(message)) << message;
    }
    else if (m_rank == 1)
    {
        std::vector<double> received(8, -1.0);
        auto request = m_transport->receiveAsync(
            {"mpi", 0}, 30,
            hostBuffer(received.data(), DataKind::Float64, shape));
        while (!request->test())
        {
        }
        std::string message;
        EXPECT_FALSE(request->failed(message)) << message;
        EXPECT_DOUBLE_EQ(received[7], 7.0);
    }
}

TEST_F(MpiTransportTest, StridedSourceGatheredOnSend)
{
    if (m_size < 2)
        GTEST_SKIP() << "needs 2 ranks";

    const std::array<int64_t, 1> shape{4};
    if (m_rank == 0)
    {
        std::array<double, 8> scratch{1, -9, 2, -9, 3, -9, 4, -9};
        const int64_t stride = 2 * static_cast<int64_t>(sizeof(double));
        std::string message;
        ASSERT_TRUE(m_transport->send(
            {"mpi", 1}, 40,
            hostBuffer(scratch.data(), DataKind::Float64, shape, &stride),
            &message)) << message;
    }
    else if (m_rank == 1)
    {
        std::array<double, 4> received{};
        std::string message;
        ASSERT_TRUE(m_transport->receive(
            {"mpi", 0}, 40,
            hostBuffer(received.data(), DataKind::Float64, shape), &message))
            << message;
        EXPECT_EQ(received, (std::array<double, 4>{1, 2, 3, 4}));
    }
}

TEST_F(MpiTransportTest, KindMismatchRejectedOnUnpack)
{
    if (m_size < 2)
        GTEST_SKIP() << "needs 2 ranks";

    const std::array<int64_t, 1> shape{4};
    if (m_rank == 0)
    {
        std::array<int32_t, 4> ints{1, 2, 3, 4};
        std::string message;
        ASSERT_TRUE(m_transport->send(
            {"mpi", 1}, 50,
            hostBuffer(ints.data(), DataKind::Int32, shape), &message))
            << message;
    }
    else if (m_rank == 1)
    {
        std::array<double, 4> doubles{};
        std::string message;
        EXPECT_FALSE(m_transport->receive(
            {"mpi", 0}, 50,
            hostBuffer(doubles.data(), DataKind::Float64, shape), &message));
        EXPECT_NE(message.find("kind"), std::string::npos);
    }
}

TEST_F(MpiTransportTest, HaloExchangeRingRunsUnchangedOverMpi)
{
    if (m_size < 3)
        GTEST_SKIP() << "needs 3 ranks";
    if (m_rank > 2)
        GTEST_SKIP() << "ring uses ranks 0-2";

    // Global entities {0,1,2}; rank r owns r and mirrors the other two —
    // the same decomposition proven over InProcTransport.
    std::vector<int64_t> owned{m_rank};
    std::vector<int64_t> virtuals;
    std::vector<int32_t> owners;
    for (int32_t peer = 0; peer < 3; ++peer)
    {
        if (peer != m_rank)
        {
            virtuals.push_back(peer);
            owners.push_back(peer);
        }
    }

    PartitionedDataItem<double> item("halo", 3, owned, virtuals, owners,
                                     m_transport.get(), /*baseTag=*/600);

    std::string message;
    ASSERT_TRUE(item.exchanger().buildPattern(message)) << message;

    for (uint64_t epoch = 0; epoch < 2; ++epoch)
    {
        item.ownedValues()[0] = 100.0 * m_rank + static_cast<double>(epoch);
        auto request = item.synchronizeAsync();
        request->wait();
        ASSERT_FALSE(request->failed(message)) << message;

        const auto mirrors = item.virtualValues();
        for (size_t v = 0; v < virtuals.size(); ++v)
        {
            EXPECT_DOUBLE_EQ(mirrors[v],
                             100.0 * owners[v] + static_cast<double>(epoch))
                << "rank " << m_rank << " epoch " << epoch;
        }
        EXPECT_EQ(item.synchronizationEpoch(), epoch + 1);
    }
}

int main(int argc, char **argv)
{
    MpiEnvironment environment(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
