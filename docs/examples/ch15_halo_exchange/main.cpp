// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 15: a 12-cell diffusion chain decomposed over three ranks
 *         with PartitionedDataItem and its HaloExchanger, checked bit for bit
 *         against the same computation on one rank; then the
 *         flux+derivative payload and the wire format.
 */

#include "hydrocouplesdk/distributed/envelope.h"
#include "hydrocouplesdk/distributed/fluxpayload.h"
#include "hydrocouplesdk/distributed/inproctransport.h"
#include "hydrocouplesdk/distributed/partitioneddataitem.h"
#include "hydrocouplesdk/distributed/valuepacker.h"

#include <array>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK::Distributed;

namespace {

constexpr int64_t kCells = 12;
constexpr int     kRanks = 3;
constexpr int     kSteps = 5;
constexpr double  kAlpha = 0.25; // diffusion number

//! The initial condition: a step at the middle of the chain.
double initial(int64_t cell) { return cell < kCells / 2 ? 1.0 : 0.0; }

//! One explicit diffusion update; the chain's ends reflect (zero gradient).
double diffuse(double left, double center, double right)
{
    return center + kAlpha * (left - 2.0 * center + right);
}

//! The reference: every cell on one rank.
std::vector<double> serialRun()
{
    std::vector<double> u(kCells);
    for (int64_t c = 0; c < kCells; ++c)
        u[c] = initial(c);
    for (int step = 0; step < kSteps; ++step)
    {
        std::vector<double> next(kCells);
        for (int64_t c = 0; c < kCells; ++c)
            next[c] = diffuse(u[c == 0 ? 0 : c - 1], u[c], u[c + 1 == kCells ? c : c + 1]);
        u = next;
    }
    return u;
}

//! [rank]
/*!
 * What one rank runs: it owns cells [4r, 4r + 4) and mirrors the cell on
 * each side that a neighbor owns.
 */
void runRank(InProcTransport *transport, std::vector<double> &result,
             std::string &summary, std::string &error)
{
    const int32_t rank  = transport->localEndpoint().rank;
    const int64_t first = rank * (kCells / kRanks);
    const int64_t last  = first + kCells / kRanks;

    std::vector<int64_t> owned, mirrored;
    std::vector<int32_t> owners;
    for (int64_t c = first; c < last; ++c)
        owned.push_back(c);
    if (first > 0)
    {
        mirrored.push_back(first - 1);
        owners.push_back(rank - 1);
    }
    if (last < kCells)
    {
        mirrored.push_back(last);
        owners.push_back(rank + 1);
    }
    PartitionedDataItemDouble u("u", kCells, owned, mirrored, owners, transport);

    // Collective: every rank negotiates the pattern once, before stepping.
    if (!u.exchanger().buildPattern(error))
        return;
    for (size_t i = 0; i < owned.size(); ++i)
        u.ownedValues()[i] = initial(owned[i]);

    for (int step = 0; step < kSteps; ++step)
    {
        // begin: owned values are copied out now and sent to the neighbors.
        auto exchange = u.synchronizeAsync();
        // ... work on interior cells could overlap the exchange here ...
        exchange->wait(); // end: mirrors are filled and the epoch advances
        if (exchange->failed(error))
            return;

        const auto values = u.ownedValues();
        const auto halo   = u.virtualValues();
        size_t h = 0;
        const double left  = first > 0 ? halo[h++] : values.front();
        const double right = last < kCells ? halo[h] : values.back();
        std::vector<double> next(values.size());
        for (size_t i = 0; i < values.size(); ++i)
            next[i] = diffuse(i == 0 ? left : values[i - 1], values[i],
                              i + 1 == values.size() ? right : values[i + 1]);
        for (size_t i = 0; i < values.size(); ++i)
            values[i] = next[i]; // owned slots only: mirrors are never solved
    }
    result.assign(u.ownedValues().begin(), u.ownedValues().end());
    summary = "rank " + std::to_string(rank) + " owns [" + std::to_string(first) + ", " +
              std::to_string(last) + "), mirrors " + std::to_string(mirrored.size()) +
              ", sends to " + std::to_string(u.exchanger().sendPeerCount()) +
              " peer(s), epochs " + std::to_string(u.synchronizationEpoch());
}
//! [rank]

int fail(const std::string &what)
{
    std::fprintf(stderr, "FAILED: %s\n", what.c_str());
    return 1;
}

} // namespace

int main()
{
    //! [threads]
    std::shared_ptr<InProcHub> hub = InProcHub::create(kRanks);
    std::array<std::unique_ptr<InProcTransport>, kRanks> transports;
    for (int r = 0; r < kRanks; ++r)
        transports[r] = hub->createTransport(r);

    std::array<std::vector<double>, kRanks> results;
    std::array<std::string, kRanks> summaries, errors;
    std::vector<std::thread> threads;
    for (int r = 0; r < kRanks; ++r)
        threads.emplace_back(runRank, transports[r].get(), std::ref(results[r]),
                             std::ref(summaries[r]), std::ref(errors[r]));
    for (std::thread &thread : threads)
        thread.join();
    //! [threads]
    for (const std::string &summary : summaries)
        std::printf("%s\n", summary.c_str());

    const std::vector<double> reference = serialRun();
    std::printf("cell  serial      distributed\n");
    for (int r = 0; r < kRanks; ++r)
    {
        if (!errors[r].empty())
            return fail("rank " + std::to_string(r) + ": " + errors[r]);
        for (size_t i = 0; i < results[r].size(); ++i)
        {
            const size_t cell = r * (kCells / kRanks) + i;
            std::printf("%4zu  %.9f %.9f\n", cell, reference[cell], results[r][i]);
            if (results[r][i] != reference[cell])
                return fail("the decomposed run differs from the serial run");
        }
    }

    //! [misowned]
    // Rank 0 claims global 3 is owned by rank 1, which does not own it: the
    // owner refuses while building the pattern.
    {
        std::shared_ptr<InProcHub> pair = InProcHub::create(2);
        auto t0 = pair->createTransport(0);
        auto t1 = pair->createTransport(1);
        HaloExchanger asking(t0.get(), {0, 1, 2}, {3}, {1});
        HaloExchanger owner(t1.get(), {4, 5, 6}, {}, {});
        std::string askingMessage, ownerMessage;
        bool ownerOk = true;
        std::thread rank1([&] { ownerOk = owner.buildPattern(ownerMessage); });
        (void)asking.buildPattern(askingMessage);
        rank1.join();
        std::printf("owner's buildPattern: %s\n", ownerMessage.c_str());
        if (ownerOk || ownerMessage.find("does not own") == std::string::npos)
            return fail("a misattributed owner should be refused");
    }
    //! [misowned]

    //! [flux]
    // A boundary flux and its derivative with respect to head travel as one
    // Float64 {2, N} block.
    {
        std::shared_ptr<InProcHub> pair = InProcHub::create(2);
        auto t0 = pair->createTransport(0);
        auto t1 = pair->createTransport(1);

        FluxDerivativePayload sent(2);
        sent.flux()[0] = 1.20;  sent.derivative()[0] = -0.40; // m³/s, m²/s
        sent.flux()[1] = 0.80;  sent.derivative()[1] = -0.25;
        std::string message;
        if (!t0->send({"inproc", 1}, /*tag=*/7, sent.descriptor(), &message))
            return fail(message);

        FluxDerivativePayload received(2);
        if (!t1->receive({"inproc", 0}, 7, received.descriptor(), &message) ||
            !FluxDerivativePayload::conformsToConvention(received.descriptor(), 2, message))
            return fail(message);
        // The receiver's head moved from 2.00 m to 2.10 m since the flux was computed.
        const double q = received.linearizedFlux(0, /*state=*/2.10, /*reference=*/2.00);
        std::printf("linearized flux at h = 2.10 m: %.4f m3/s\n", q);
        if (q != 1.20 + (-0.40) * (2.10 - 2.00))
            return fail("linearized flux");

        FluxDerivativePayload wrong(3);
        const bool refused = !t0->send({"inproc", 1}, 8, sent.descriptor(), &message) ||
                             !t1->receive({"inproc", 0}, 8, wrong.descriptor(), &message);
        std::printf("receiving 2 entities into 3: %s\n", message.c_str());
        if (!refused)
            return fail("a {2, 2} block should not unpack into {2, 3}");
    }
    //! [flux]

    //! [wire]
    // What travels: a 32-byte envelope for protocol messages, and every
    // payload as a ValuePacker block (header, shape, elements).
    {
        std::array<double, 6> block{};
        const std::array<int64_t, 2> shape{2, 3};
        BufferDescriptor descriptor;
        descriptor.data  = block.data();
        descriptor.kind  = DataKind::Float64;
        descriptor.rank  = 2;
        descriptor.shape = shape.data();
        descriptor.space = MemorySpace::Host;
        std::printf("sizeof(Envelope) = %zu, packed {2, 3} Float64 block = %lld bytes\n",
                    sizeof(Envelope),
                    static_cast<long long>(ValuePacker::packedSize(descriptor)));
        if (ValuePacker::packedSize(descriptor) != 8 + 2 * 8 + 6 * 8)
            return fail("packed size");
    }
    //! [wire]
    return 0;
}
