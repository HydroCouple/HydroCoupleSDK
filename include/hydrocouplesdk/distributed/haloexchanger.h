// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   haloexchanger.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  HaloExchanger — epoch-stamped nonblocking halo (ghost/virtual
 *         node) exchange over any ITransport.
 *
 * Implements the local virtual node representation's communication core
 * (plan §5.4). The exchange pattern is built once from ownership metadata
 * at prepare() time — each rank tells every owner which global indexes it
 * mirrors, and owners record the matching local positions to gather — then
 * every synchronization epoch runs the classic three-step:
 *
 * 1. begin(): gather owned values into per-peer staging and post
 *    nonblocking sends; post nonblocking receives for incoming halos;
 * 2. ... overlap computation on owned entities ...
 * 3. end(): wait for the receives, scatter into the virtual slots, and
 *    advance the epoch.
 *
 * SDK policy encoded here (the SWMM lessons): virtual entities carry no
 * degrees of freedom — they are mirrors, reconstructed on every epoch,
 * never solved; values gathered at begin() belong to that epoch even if
 * owned values are overwritten during the overlap window; and the pattern
 * is validated once, in one place, before any exchange runs.
 *
 * The exchange is kind-agnostic: values move as raw bytes of the item's
 * fixed-width DataKind, gathered/scattered with per-element memcpy into
 * reused per-peer staging buffers (no steady-state allocation).
 *
 * \threadsafety One exchanger belongs to one rank's thread; different
 * ranks' exchangers may run concurrently (the transport synchronizes).
 */

#ifndef HYDROCOUPLESDK_DISTRIBUTED_HALOEXCHANGER_H
#define HYDROCOUPLESDK_DISTRIBUTED_HALOEXCHANGER_H

#include "hydrocouplesdk/export.h"
#include "hydrocoupledistributed.h"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace HydroCouple::SDK::Distributed {

/*!
 * \brief Ownership-driven halo exchange engine for one partitioned item.
 */
class HYDROCOUPLESDK_EXPORT HaloExchanger
{
public:
    /*!
     * \brief Construct from ownership metadata.
     *
     * The local storage convention is owned-then-virtual: local position i
     * of an owned entity is its index in \p ownedGlobalIndexes, and local
     * position of virtual entity j is ownedGlobalIndexes.size() + j.
     *
     * \param[in] transport            Transport endpoint (borrowed).
     * \param[in] ownedGlobalIndexes   Globals this rank owns (solves).
     * \param[in] virtualGlobalIndexes Globals this rank mirrors.
     * \param[in] virtualOwners        Owner rank of each virtual entry
     *                                 (parallel to virtualGlobalIndexes).
     * \param[in] baseTag              Tag base for this item's traffic
     *                                 (distinct per item on a transport).
     */
    HaloExchanger(HydroCouple::Distributed::ITransport *transport,
                  std::vector<int64_t> ownedGlobalIndexes,
                  std::vector<int64_t> virtualGlobalIndexes,
                  std::vector<int32_t> virtualOwners,
                  int32_t baseTag = 2000);

    ~HaloExchanger();

    HaloExchanger(const HaloExchanger &)            = delete;
    HaloExchanger &operator=(const HaloExchanger &) = delete;

    /*!
     * \brief Negotiates the send/receive pattern with every peer
     *        (collective: all ranks must call concurrently). Validates
     *        that every requested global is owned by exactly the asked
     *        rank; call once at prepare() time.
     * \param[out] message Failure description when returning false.
     */
    [[nodiscard]] bool buildPattern(std::string &message);

    /*!
     * \brief Starts one exchange epoch: gathers owned values from
     *        \p localValues (owned-then-virtual layout, contiguous,
     *        \p elementSize bytes per element), posts sends and receives.
     *
     * \p localValues must stay valid until end(); owned entries may be
     * overwritten during the overlap window — the halo carries the values
     * as of this call.
     *
     * \param[in]  localValues Base of the local value array.
     * \param[in]  elementSize Bytes per element (DataKind size).
     * \param[out] message     Failure description when returning false.
     */
    [[nodiscard]] bool begin(void *localValues, int64_t elementSize,
                             std::string &message);

    /*!
     * \brief Completes the epoch: waits for incoming halos, scatters them
     *        into the virtual slots of the array given to begin(), and
     *        advances the epoch.
     * \param[out] message Failure description when returning false.
     */
    [[nodiscard]] bool end(std::string &message);

    /*!
     * \brief Nonblocking completion probe: true once every incoming halo
     *        has arrived and been scattered (equivalent to end()).
     */
    [[nodiscard]] bool tryEnd(bool &complete, std::string &message);

    /*!
     * \brief Completed synchronization epochs.
     */
    [[nodiscard]] uint64_t epoch() const noexcept;

    /*!
     * \brief True after a successful buildPattern().
     */
    [[nodiscard]] bool patternBuilt() const noexcept;

    /*!
     * \brief Number of peers this rank sends halos to (diagnostics).
     */
    [[nodiscard]] size_t sendPeerCount() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_HALOEXCHANGER_H
