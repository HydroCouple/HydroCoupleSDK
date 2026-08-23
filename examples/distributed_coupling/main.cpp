// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Distributed coupling example — the serial composition's channel
 *         domain decomposed across MPI ranks with a partitioned data item
 *         and epoch-stamped halo exchange.
 *
 * Demonstrates the distributed spine:
 *
 * 1. MpiTransport behind the standard's ITransport seam (the same code
 *    runs over InProcTransport in the test suite — no protocol change);
 * 2. PartitionedDataItem<double> holding this rank's owned reaches
 *    followed by its virtual (halo) mirrors of neighbouring ranks'
 *    reaches, in one contiguous typed array;
 * 3. HaloExchanger building the exchange pattern once at prepare() and
 *    running begin → overlap compute → end each step, so communication
 *    hides behind computation;
 * 4. SDK policy in practice: virtual entities carry no degrees of freedom
 *    — the solver writes owned slots only, and every epoch reconstructs
 *    the mirrors from their owners.
 *
 * The domain is a chain of reaches split contiguously across ranks: each
 * rank mirrors its upstream and downstream neighbours' boundary reaches.
 *
 * Run:
 * \code
 *   mpirun -n 4 ./distributed_coupling
 * \endcode
 */

#include "hydrocouplesdk/distributed/mpitransport.h"
#include "hydrocouplesdk/distributed/partitioneddataitem.h"
#include "hydrocouplesdk/distributed/fluxpayload.h"

#include <cmath>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK::Distributed;

namespace {

constexpr int64_t kGlobalReachCount = 24;
constexpr int     kTimeSteps        = 10;

/*!
 * \brief Contiguous block decomposition: [first, last) reaches of a rank.
 */
struct Block
{
    int64_t first;
    int64_t last;
};

Block blockFor(int32_t rank, int32_t size)
{
    const int64_t base      = kGlobalReachCount / size;
    const int64_t remainder = kGlobalReachCount % size;
    const int64_t first =
        rank * base + std::min<int64_t>(rank, remainder);
    const int64_t count = base + (rank < remainder ? 1 : 0);
    return {first, first + count};
}

int32_t ownerOf(int64_t reach, int32_t size)
{
    for (int32_t rank = 0; rank < size; ++rank)
    {
        const Block block = blockFor(rank, size);
        if (reach >= block.first && reach < block.last)
            return rank;
    }
    return -1;
}

} // namespace

int main(int argc, char **argv)
{
    MpiEnvironment environment(argc, argv);
    MpiTransport   transport;

    const int32_t rank = transport.localEndpoint().rank;
    const int32_t size = transport.participantCount();

    if (kGlobalReachCount < size)
    {
        if (rank == 0)
            std::cerr << "need at least " << kGlobalReachCount
                      << " ranks or fewer\n";
        return 1;
    }

    // ── 1. Decomposition: owned reaches, plus halo mirrors of the
    //       neighbours' boundary reaches. ───────────────────────────────
    const Block block = blockFor(rank, size);

    std::vector<int64_t> owned;
    for (int64_t reach = block.first; reach < block.last; ++reach)
        owned.push_back(reach);

    std::vector<int64_t> virtualReaches;
    std::vector<int32_t> virtualOwners;
    if (block.first > 0)
    {
        virtualReaches.push_back(block.first - 1); // upstream neighbour
        virtualOwners.push_back(ownerOf(block.first - 1, size));
    }
    if (block.last < kGlobalReachCount)
    {
        virtualReaches.push_back(block.last); // downstream neighbour
        virtualOwners.push_back(ownerOf(block.last, size));
    }

    PartitionedDataItem<double> flow("flow", kGlobalReachCount, owned,
                                     virtualReaches, virtualOwners,
                                     &transport, /*baseTag=*/900);

    // ── 2. Build the exchange pattern once (collective). ─────────────────
    std::string message;
    if (!flow.exchanger().buildPattern(message))
    {
        std::cerr << "rank " << rank << ": pattern failed: " << message
                  << '\n';
        return 1;
    }

    // Initial condition: flow proportional to the global reach index.
    for (size_t i = 0; i < owned.size(); ++i)
        flow.ownedValues()[i] = static_cast<double>(owned[i]);

    // ── 3. Time stepping with communication/computation overlap. ─────────
    for (int step = 0; step < kTimeSteps; ++step)
    {
        // begin(): gather owned values and post sends/receives.
        auto exchange = flow.synchronizeAsync();

        // ... overlap: work that touches only interior owned reaches ...
        double interiorSum = 0.0;
        for (size_t i = 1; i + 1 < owned.size(); ++i)
            interiorSum += flow.ownedValues()[i];

        // end(): wait, scatter halos into the virtual slots, advance epoch.
        exchange->wait();
        if (exchange->failed(message))
        {
            std::cerr << "rank " << rank << ": exchange failed: " << message
                      << '\n';
            return 1;
        }

        // Diffusion-style update using halo values at the boundaries.
        // Virtual entities are never written — mirrors only.
        std::vector<double> updated(owned.size());
        const auto mirrors = flow.virtualValues();
        size_t     mirror  = 0;
        const double upstreamHalo =
            block.first > 0 ? mirrors[mirror++] : flow.ownedValues().front();
        const double downstreamHalo =
            block.last < kGlobalReachCount ? mirrors[mirror]
                                           : flow.ownedValues().back();

        for (size_t i = 0; i < owned.size(); ++i)
        {
            const double left =
                i == 0 ? upstreamHalo : flow.ownedValues()[i - 1];
            const double right = i + 1 == owned.size()
                                     ? downstreamHalo
                                     : flow.ownedValues()[i + 1];
            updated[i] = flow.ownedValues()[i] +
                         0.25 * (left - 2.0 * flow.ownedValues()[i] + right);
        }
        for (size_t i = 0; i < owned.size(); ++i)
            flow.ownedValues()[i] = updated[i];

        (void)interiorSum;
    }

    // ── 4. Flux + derivative payload: the boundary exchange convention. ──
    FluxDerivativePayload boundaryFlux(2);
    boundaryFlux.flux()[0]       = flow.ownedValues().front();
    boundaryFlux.derivative()[0] = -0.5; // ∂q/∂h at the upstream face
    boundaryFlux.flux()[1]       = flow.ownedValues().back();
    boundaryFlux.derivative()[1] = -0.5;
    const double linearized = boundaryFlux.linearizedFlux(0, /*state=*/1.2,
                                                          /*reference=*/1.0);

    // ── 5. Report (rank 0 collects nothing — each rank prints its own). ──
    const double localSum =
        std::accumulate(flow.ownedValues().begin(),
                        flow.ownedValues().end(), 0.0);
    std::cout << "rank " << rank << '/' << size << ": owns ["
              << block.first << ", " << block.last << ") · epochs "
              << flow.synchronizationEpoch() << " · halo peers "
              << flow.exchanger().sendPeerCount() << " · local flow sum "
              << localSum << " · linearized upstream flux " << linearized
              << '\n';

    return 0;
}
