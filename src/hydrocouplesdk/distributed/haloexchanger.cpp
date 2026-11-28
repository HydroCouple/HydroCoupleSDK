// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/distributed/haloexchanger.h"

#include <algorithm>
#include <cstring>

namespace HydroCouple::SDK::Distributed {

namespace {

using Endpoint = HydroCouple::Distributed::ITransport::Endpoint;

/*! Pattern-negotiation tag offsets (relative to the item's base tag). */
constexpr int32_t kCountTagOffset = 0;
constexpr int32_t kListTagOffset  = 1;
/*! Data epochs cycle through this many tags above the base. */
constexpr int32_t kDataTagOffset  = 2;
constexpr int32_t kEpochTagSpan   = 1024;

/*! Rank-1 Int64 host descriptor over \p data; \p shapeStorage must
 *  outlive it. */
BufferDescriptor int64Buffer(void *data, const int64_t &shapeStorage)
{
    BufferDescriptor descriptor;
    descriptor.data  = data;
    descriptor.kind  = DataKind::Int64;
    descriptor.rank  = 1;
    descriptor.shape = &shapeStorage;
    descriptor.space = MemorySpace::Host;
    return descriptor;
}

/*! Rank-1 UInt8 host descriptor over \p data; \p shapeStorage must
 *  outlive it. */
BufferDescriptor byteBuffer(void *data, const int64_t &shapeStorage)
{
    BufferDescriptor descriptor;
    descriptor.data  = data;
    descriptor.kind  = DataKind::UInt8;
    descriptor.rank  = 1;
    descriptor.shape = &shapeStorage;
    descriptor.space = MemorySpace::Host;
    return descriptor;
}

} // namespace

/*!
 * \brief The exchanger's pattern, staging and epoch state.
 */
struct HaloExchanger::Impl
{
    HydroCouple::Distributed::ITransport *transport = nullptr; //!< Borrowed transport.
    int32_t                               myRank    = -1;      //!< This rank on the transport.
    int32_t                               baseTag   = 2000;    //!< First of the 1026 tags this exchanger uses.

    std::vector<int64_t> ownedGlobals;   //!< Global index of each owned local position.
    std::vector<int64_t> virtualGlobals; //!< Global index of each mirrored entity.
    std::vector<int32_t> virtualOwners;  //!< Owner rank of each mirrored entity.

    std::unordered_map<int64_t, int64_t> ownedGlobalToLocal; //!< Owned global index → local position.

    bool     patternBuilt = false; //!< Set by a successful buildPattern().
    uint64_t epoch        = 0;     //!< Completed epochs.

    /*! Per-peer gather list: local owned positions to send, in the
     *  requester's order. */
    std::map<int32_t, std::vector<int64_t>> sendLists;
    /*! Per-peer scatter list: local virtual positions to fill, in the
     *  order requested from that owner. */
    std::map<int32_t, std::vector<int64_t>> recvLists;

    /*! Reused staging (no steady-state allocation). */
    std::map<int32_t, std::vector<std::byte>> sendStage;
    std::map<int32_t, std::vector<std::byte>> recvStage; //!< Per-owner receive staging, reused every epoch.
    /*! Shape storage for in-flight descriptors (stable addresses). */
    std::map<int32_t, int64_t> sendShapes;
    std::map<int32_t, int64_t> recvShapes; //!< Byte count of each owner's receive staging (stable address).

    /*!
     * \brief One posted halo receive of the open epoch.
     */
    struct PendingReceive
    {
        int32_t peer = -1; //!< Owner rank the halo comes from.
        std::unique_ptr<HydroCouple::Distributed::IExchangeRequest> request; //!< The transport's receive.
        bool scattered = false; //!< The halo has been copied into the virtual slots.
    };
    std::vector<PendingReceive> pending; //!< Receives of the open epoch, one per owner peer.

    void   *exchangeValues     = nullptr; //!< Array given to begin().
    int64_t exchangeElementSize = 0;      //!< Element size given to begin(), in bytes.
    bool    exchangeActive      = false;  //!< An epoch is open (begin() succeeded, not yet ended).

    /*!
     * \brief The tag the current epoch's halos travel on.
     * \return baseTag + 2 + (epoch mod 1024).
     */
    [[nodiscard]] int32_t dataTag() const noexcept
    {
        return baseTag + kDataTagOffset +
               static_cast<int32_t>(epoch % kEpochTagSpan);
    }

    /*!
     * \brief Scatters one peer's received halo block into the virtual slots.
     * \param[in] receive A completed receive whose staging holds the halo.
     */
    void scatter(const PendingReceive &receive)
    {
        const std::vector<int64_t> &locals = recvLists[receive.peer];
        const std::vector<std::byte> &stage = recvStage[receive.peer];
        auto *bytes = static_cast<std::byte *>(exchangeValues);
        for (size_t k = 0; k < locals.size(); ++k)
            std::memcpy(bytes + locals[k] * exchangeElementSize,
                        stage.data() +
                            static_cast<int64_t>(k) * exchangeElementSize,
                        static_cast<size_t>(exchangeElementSize));
    }
};

// ── Construction ──────────────────────────────────────────────────────────

HaloExchanger::HaloExchanger(
    HydroCouple::Distributed::ITransport *transport,
    std::vector<int64_t> ownedGlobalIndexes,
    std::vector<int64_t> virtualGlobalIndexes,
    std::vector<int32_t> virtualOwners, int32_t baseTag)
    : m_impl(std::make_unique<Impl>())
{
    Impl &impl          = *m_impl;
    impl.transport      = transport;
    impl.myRank         = transport->localEndpoint().rank;
    impl.baseTag        = baseTag;
    impl.ownedGlobals   = std::move(ownedGlobalIndexes);
    impl.virtualGlobals = std::move(virtualGlobalIndexes);
    impl.virtualOwners  = std::move(virtualOwners);

    impl.ownedGlobalToLocal.reserve(impl.ownedGlobals.size());
    for (size_t i = 0; i < impl.ownedGlobals.size(); ++i)
        impl.ownedGlobalToLocal[impl.ownedGlobals[i]] =
            static_cast<int64_t>(i);
}

HaloExchanger::~HaloExchanger() = default;

uint64_t HaloExchanger::epoch() const noexcept
{
    return m_impl->epoch;
}

bool HaloExchanger::patternBuilt() const noexcept
{
    return m_impl->patternBuilt;
}

size_t HaloExchanger::sendPeerCount() const noexcept
{
    return m_impl->sendLists.size();
}

// ── Pattern negotiation (collective, once at prepare) ─────────────────────

bool HaloExchanger::buildPattern(std::string &message)
{
    Impl &impl = *m_impl;
    if (impl.virtualOwners.size() != impl.virtualGlobals.size())
    {
        message = "HaloExchanger: virtualOwners must parallel "
                  "virtualGlobalIndexes";
        return false;
    }

    // Group this rank's needs by owner: owner → (global, local virtual pos).
    std::map<int32_t, std::vector<std::pair<int64_t, int64_t>>> needs;
    const int64_t ownedCount =
        static_cast<int64_t>(impl.ownedGlobals.size());
    for (size_t j = 0; j < impl.virtualGlobals.size(); ++j)
    {
        const int32_t owner = impl.virtualOwners[j];
        if (owner == impl.myRank)
        {
            message = "HaloExchanger: virtual global " +
                      std::to_string(impl.virtualGlobals[j]) +
                      " lists this rank as its own owner";
            return false;
        }
        needs[owner].emplace_back(impl.virtualGlobals[j],
                                  ownedCount + static_cast<int64_t>(j));
    }

    const int32_t participants = impl.transport->participantCount();

    // Phase A: tell every peer what this rank needs from it (count, list).
    // Sends are posted before any receive, so no rank blocks another.
    for (int32_t peer = 0; peer < participants; ++peer)
    {
        if (peer == impl.myRank)
            continue;
        std::vector<int64_t> globals;
        if (auto it = needs.find(peer); it != needs.end())
            for (const auto &[global, local] : it->second)
                globals.push_back(global);

        int64_t count = static_cast<int64_t>(globals.size());
        const int64_t one = 1;
        if (!impl.transport->send({"", peer},
                                  impl.baseTag + kCountTagOffset,
                                  int64Buffer(&count, one), &message))
            return false;
        if (count > 0)
        {
            const int64_t length = count;
            if (!impl.transport->send({"", peer},
                                      impl.baseTag + kListTagOffset,
                                      int64Buffer(globals.data(), length),
                                      &message))
                return false;
        }

        // Record the scatter list in exactly the order requested.
        if (auto it = needs.find(peer); it != needs.end())
        {
            std::vector<int64_t> &locals = impl.recvLists[peer];
            locals.clear();
            for (const auto &[global, local] : it->second)
                locals.push_back(local);
        }
    }

    // Phase B: learn what every peer needs from this rank.
    for (int32_t peer = 0; peer < participants; ++peer)
    {
        if (peer == impl.myRank)
            continue;
        int64_t count = 0;
        const int64_t one = 1;
        if (!impl.transport->receive({"", peer},
                                     impl.baseTag + kCountTagOffset,
                                     int64Buffer(&count, one), &message))
            return false;
        if (count <= 0)
            continue;
        std::vector<int64_t> globals(static_cast<size_t>(count));
        const int64_t length = count;
        if (!impl.transport->receive({"", peer},
                                     impl.baseTag + kListTagOffset,
                                     int64Buffer(globals.data(), length),
                                     &message))
            return false;

        std::vector<int64_t> &locals = impl.sendLists[peer];
        locals.clear();
        locals.reserve(globals.size());
        for (int64_t global : globals)
        {
            const auto it = impl.ownedGlobalToLocal.find(global);
            if (it == impl.ownedGlobalToLocal.end())
            {
                message = "HaloExchanger: rank " + std::to_string(peer) +
                          " requested global " + std::to_string(global) +
                          ", which rank " + std::to_string(impl.myRank) +
                          " does not own";
                return false;
            }
            locals.push_back(it->second);
        }
    }

    impl.patternBuilt = true;
    message.clear();
    return true;
}

// ── Epoch execution ───────────────────────────────────────────────────────

bool HaloExchanger::begin(void *localValues, int64_t elementSize,
                          std::string &message)
{
    Impl &impl = *m_impl;
    if (!impl.patternBuilt)
    {
        message = "HaloExchanger::begin: buildPattern() has not run";
        return false;
    }
    if (impl.exchangeActive)
    {
        message = "HaloExchanger::begin: previous epoch not ended";
        return false;
    }
    impl.exchangeValues      = localValues;
    impl.exchangeElementSize = elementSize;
    impl.exchangeActive      = true;

    const int32_t tag = impl.dataTag();
    auto *bytes = static_cast<const std::byte *>(localValues);

    // Gather + post sends (values captured now belong to this epoch).
    for (const auto &[peer, locals] : impl.sendLists)
    {
        std::vector<std::byte> &stage = impl.sendStage[peer];
        stage.resize(locals.size() * static_cast<size_t>(elementSize));
        for (size_t k = 0; k < locals.size(); ++k)
            std::memcpy(stage.data() +
                            static_cast<int64_t>(k) * elementSize,
                        bytes + locals[k] * elementSize,
                        static_cast<size_t>(elementSize));
        impl.sendShapes[peer] = static_cast<int64_t>(stage.size());
        if (!impl.transport->send({"", peer}, tag,
                                  byteBuffer(stage.data(),
                                             impl.sendShapes[peer]),
                                  &message))
        {
            impl.exchangeActive = false;
            return false;
        }
    }

    // Post nonblocking receives for incoming halos.
    impl.pending.clear();
    for (const auto &[peer, locals] : impl.recvLists)
    {
        std::vector<std::byte> &stage = impl.recvStage[peer];
        stage.resize(locals.size() * static_cast<size_t>(elementSize));
        impl.recvShapes[peer] = static_cast<int64_t>(stage.size());
        Impl::PendingReceive receive;
        receive.peer    = peer;
        receive.request = impl.transport->receiveAsync(
            {"", peer}, tag,
            byteBuffer(stage.data(), impl.recvShapes[peer]));
        impl.pending.push_back(std::move(receive));
    }
    message.clear();
    return true;
}

bool HaloExchanger::end(std::string &message)
{
    Impl &impl = *m_impl;
    if (!impl.exchangeActive)
    {
        message = "HaloExchanger::end: no active epoch";
        return false;
    }
    for (Impl::PendingReceive &receive : impl.pending)
    {
        receive.request->wait();
        std::string failure;
        if (receive.request->failed(failure))
        {
            message = "HaloExchanger: halo from rank " +
                      std::to_string(receive.peer) + " failed: " + failure;
            impl.exchangeActive = false;
            return false;
        }
        if (!receive.scattered)
        {
            impl.scatter(receive);
            receive.scattered = true;
        }
    }
    impl.pending.clear();
    impl.exchangeActive = false;
    ++impl.epoch;
    message.clear();
    return true;
}

bool HaloExchanger::tryEnd(bool &complete, std::string &message)
{
    Impl &impl = *m_impl;
    if (!impl.exchangeActive)
    {
        message = "HaloExchanger::tryEnd: no active epoch";
        return false;
    }
    for (Impl::PendingReceive &receive : impl.pending)
    {
        if (receive.scattered)
            continue;
        if (!receive.request->test())
        {
            complete = false;
            message.clear();
            return true;
        }
        std::string failure;
        if (receive.request->failed(failure))
        {
            message = "HaloExchanger: halo from rank " +
                      std::to_string(receive.peer) + " failed: " + failure;
            impl.exchangeActive = false;
            return false;
        }
        impl.scatter(receive);
        receive.scattered = true;
    }
    impl.pending.clear();
    impl.exchangeActive = false;
    ++impl.epoch;
    complete = true;
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Distributed
