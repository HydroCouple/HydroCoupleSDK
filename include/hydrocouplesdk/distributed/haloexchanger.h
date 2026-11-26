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
 * 1. begin(): gather owned values into per-peer staging and send them
 *    (with the transport's blocking send()); post nonblocking receives for
 *    incoming halos;
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
 * Tags: pattern negotiation uses baseTag (counts) and baseTag + 1 (index
 * lists); epoch e sends its halos on baseTag + 2 + (e mod 1024). One
 * exchanger therefore occupies tags baseTag to baseTag + 1025.
 *
 * Sends are posted before receives. InProcTransport::send() never blocks;
 * MpiTransport::send() is MPI_Send, which an MPI library may hold until the
 * peer receives once a halo exceeds its eager-message limit.
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
 *
 * PartitionedDataItem embeds one; use it directly when a component keeps
 * its own value arrays in the owned-then-virtual layout.
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
     * Reads the local rank from \p transport; sends nothing.
     *
     * \param[in] transport            Transport endpoint (borrowed; not null;
     *                                 must outlive the exchanger).
     * \param[in] ownedGlobalIndexes   Globals this rank owns (solves); each
     *                                 at most once.
     * \param[in] virtualGlobalIndexes Globals this rank mirrors.
     * \param[in] virtualOwners        Owner rank of each virtual entry
     *                                 (parallel to virtualGlobalIndexes; the
     *                                 lengths are checked by buildPattern()).
     * \param[in] baseTag              Tag base for this item's traffic
     *                                 (distinct per item on a transport).
     */
    HaloExchanger(HydroCouple::Distributed::ITransport *transport,
                  std::vector<int64_t> ownedGlobalIndexes,
                  std::vector<int64_t> virtualGlobalIndexes,
                  std::vector<int32_t> virtualOwners,
                  int32_t baseTag = 2000);

    //! Destroys the pattern and staging; pending receive requests are
    //! destroyed with it (see the transport for what that implies).
    ~HaloExchanger();

    HaloExchanger(const HaloExchanger &)            = delete;
    HaloExchanger &operator=(const HaloExchanger &) = delete;

    /*!
     * \brief Negotiates the send/receive pattern with every peer
     *        (collective: all ranks must call concurrently). Validates
     *        that every requested global is owned by exactly the asked
     *        rank; call once at prepare() time.
     *
     * Every rank sends each other rank the count of globals it needs from
     * it (0 included) and then the list, and receives the same from every
     * peer: 2 (P - 1) messages at most per rank for P ranks. The validation
     * runs on the owner: a rank asked for a global it does not own fails,
     * while the asking rank may still succeed.
     *
     * \param[out] message Failure description when returning false:
     *                     "HaloExchanger: virtualOwners must parallel
     *                     virtualGlobalIndexes", "HaloExchanger: virtual
     *                     global G lists this rank as its own owner",
     *                     "HaloExchanger: rank P requested global G, which
     *                     rank R does not own", or a transport message.
     *                     Cleared on success.
     * \return True when the pattern is built; patternBuilt() is then true.
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
     * \param[in]  localValues Base of the local value array (owned count +
     *                         virtual count elements; caller-owned).
     * \param[in]  elementSize Bytes per element (DataKind size, > 0).
     * \param[out] message     Failure description when returning false:
     *                         "HaloExchanger::begin: buildPattern() has not
     *                         run", "HaloExchanger::begin: previous epoch
     *                         not ended", or a send failure. Cleared on
     *                         success.
     * \return True when the epoch is open; end() or tryEnd() must close it.
     */
    [[nodiscard]] bool begin(void *localValues, int64_t elementSize,
                             std::string &message);

    /*!
     * \brief Completes the epoch: waits for incoming halos, scatters them
     *        into the virtual slots of the array given to begin(), and
     *        advances the epoch.
     *
     * Blocks with no timeout. A failure closes the epoch without advancing
     * it.
     *
     * \param[out] message Failure description when returning false:
     *                     "HaloExchanger::end: no active epoch" or
     *                     "HaloExchanger: halo from rank P failed: ...".
     *                     Cleared on success.
     * \return True when every halo is scattered and the epoch advanced.
     */
    [[nodiscard]] bool end(std::string &message);

    /*!
     * \brief Nonblocking completion probe: true once every incoming halo
     *        has arrived and been scattered (equivalent to end()).
     *
     * Scatters each halo as soon as it arrives; the call that finds the last
     * one advances the epoch and closes it.
     *
     * \param[out] complete True when the epoch completed in this call; false
     *                      while a halo is outstanding.
     * \param[out] message  Failure description when returning false
     *                      ("HaloExchanger::tryEnd: no active epoch", or a
     *                      halo failure as for end()).
     * \return False on failure (the epoch is closed); true otherwise, with
     *         the outcome in \p complete.
     */
    [[nodiscard]] bool tryEnd(bool &complete, std::string &message);

    /*!
     * \brief Completed synchronization epochs.
     * \return The number of epochs closed by a successful end() or tryEnd().
     */
    [[nodiscard]] uint64_t epoch() const noexcept;

    /*!
     * \brief True after a successful buildPattern().
     * \return Whether begin() may be called.
     */
    [[nodiscard]] bool patternBuilt() const noexcept;

    /*!
     * \brief Number of peers this rank sends halos to (diagnostics).
     * \return Peers that asked this rank for at least one global.
     */
    [[nodiscard]] size_t sendPeerCount() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl; //!< Pattern, staging and epoch state.
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_HALOEXCHANGER_H
