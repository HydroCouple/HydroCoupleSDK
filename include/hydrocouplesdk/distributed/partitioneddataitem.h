// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   partitioneddataitem.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  PartitionedDataItem — the local virtual node representation of a
 *         globally partitioned field.
 *
 * Implements the standard's IPartitionedComponentDataItem: this rank
 * stores its owned entities followed by its virtual (ghost/halo) mirrors
 * in one contiguous typed array; owned/virtual global-index spans and the
 * virtual owner ranks expose the decomposition; synchronizeAsync() runs an
 * epoch-stamped halo exchange through the embedded HaloExchanger
 * (begin → overlap compute → end).
 *
 * SDK policy (the SWMM lessons): virtual entities carry no degrees of
 * freedom — solvers write owned slots only, and every epoch reconstructs
 * the mirrors from their owners.
 *
 * \threadsafety One item belongs to one rank's thread; concurrent ranks
 * synchronize through the transport.
 */

#ifndef HYDROCOUPLESDK_DISTRIBUTED_PARTITIONEDDATAITEM_H
#define HYDROCOUPLESDK_DISTRIBUTED_PARTITIONEDDATAITEM_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/distributed/haloexchanger.h"
#include "hydrocoupledistributed.h"
#include "hydrocouplehelpers.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace HydroCouple::SDK::Distributed {

/*!
 * \brief Typed partitioned data item with halo synchronization.
 *
 * Local layout: positions [0, ownedCount) hold owned values in
 * ownedGlobalIndexes order; positions [ownedCount, ownedCount+virtualCount)
 * hold the mirrors in virtualGlobalIndexes order. The v2 data plane spans
 * the whole local array; shape() = { ownedCount + virtualCount }.
 *
 * The item is constructed with no dimensions and no value definition, so
 * dimensions() is empty and valueDefinition() is null; it is a building
 * block for a component's decomposed field rather than an exchange item a
 * workflow connects. All values start as T{}. The read-only policy for
 * mirrors is not enforced: setValuesFrom() and the data plane can write
 * any position, and the next synchronization overwrites the mirrors.
 *
 * \tparam T Element type; DataKindOf<T> must name a fixed-width kind. The
 *           library instantiates double and int32_t (the aliases
 *           PartitionedDataItemDouble and PartitionedDataItemInt).
 */
template <typename T>
class PartitionedDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public virtual HydroCouple::Distributed::IPartitionedComponentDataItem
{
public:
    /*!
     * \brief Construct from decomposition metadata.
     *
     * Builds the embedded HaloExchanger (copying the index lists) but sends
     * nothing: the exchange pattern is negotiated by
     * exchanger().buildPattern() or by the first synchronizeAsync().
     * \p globalCount is reported by globalCount() and not checked against
     * the index lists.
     *
     * \param[in] id                   Item identifier.
     * \param[in] globalCount          Entities across all ranks.
     * \param[in] ownedGlobalIndexes   Globals this rank owns.
     * \param[in] virtualGlobalIndexes Globals this rank mirrors.
     * \param[in] virtualOwners        Owner rank per virtual entry.
     * \param[in] transport            Transport endpoint (borrowed; not null;
     *                                 must outlive the item).
     * \param[in] baseTag              Distinct tag base per item: the item
     *                                 uses tags baseTag to baseTag + 1025, so
     *                                 items on one transport need bases at
     *                                 least 1026 apart.
     */
    PartitionedDataItem(std::string_view id, int64_t globalCount,
                        std::vector<int64_t> ownedGlobalIndexes,
                        std::vector<int64_t> virtualGlobalIndexes,
                        std::vector<int32_t> virtualOwners,
                        HydroCouple::Distributed::ITransport *transport,
                        int32_t baseTag = 2000)
        : AbstractComponentDataItem(id, {}, nullptr, nullptr),
          m_globalCount(globalCount),
          m_ownedGlobals(std::move(ownedGlobalIndexes)),
          m_virtualGlobals(std::move(virtualGlobalIndexes)),
          m_virtualOwners(std::move(virtualOwners)),
          m_values(m_ownedGlobals.size() + m_virtualGlobals.size(), T{}),
          m_exchanger(std::make_unique<HaloExchanger>(
              transport, m_ownedGlobals, m_virtualGlobals, m_virtualOwners,
              baseTag))
    {
    }

    //! Destroys the values and the exchanger; an exchange still pending is
    //! abandoned.
    ~PartitionedDataItem() override = default;

    // ── IPartitionedComponentDataItem ────────────────────────────────────

    //! This item has a single (entity) dimension, so axis 0 is the decomposed one.
    //! \return 0.
    [[nodiscard]] int32_t partitionedDimension() const override
    {
        return 0;
    }

    /*!
     * \brief The global entity count given to the constructor.
     * \return Entities across all ranks.
     */
    [[nodiscard]] int64_t globalCount() const override
    {
        return m_globalCount;
    }

    /*!
     * \brief Global indexes of the owned entities, in local order.
     * \return A view valid for the item's lifetime.
     */
    [[nodiscard]] std::span<const int64_t> ownedGlobalIndexes() const override
    {
        return {m_ownedGlobals.data(), m_ownedGlobals.size()};
    }

    /*!
     * \brief Global indexes of the mirrored entities, in local order.
     * \return A view valid for the item's lifetime.
     */
    [[nodiscard]] std::span<const int64_t> virtualGlobalIndexes()
        const override
    {
        return {m_virtualGlobals.data(), m_virtualGlobals.size()};
    }

    /*!
     * \brief Owner rank of each mirrored entity.
     * \return A view parallel to virtualGlobalIndexes(), valid for the
     *         item's lifetime.
     */
    [[nodiscard]] std::span<const int32_t> virtualOwners() const override
    {
        return {m_virtualOwners.data(), m_virtualOwners.size()};
    }

    /*!
     * \brief Completed halo synchronizations.
     * \return HaloExchanger::epoch(): 0 before the first exchange completes.
     */
    [[nodiscard]] uint64_t synchronizationEpoch() const override
    {
        return m_exchanger->epoch();
    }

    /*!
     * \brief Starts an epoch exchange; the returned request completes it
     *        (wait() = scatter halos; test() polls). Overlap computation on
     *        owned values between this call and completion.
     *
     * Collective: every rank of the transport must call it for the same
     * epoch. The first call also builds the exchange pattern, which blocks
     * until every peer has sent its needs. Owned values are copied out
     * before this returns, so they may change during the overlap; mirrors
     * are defined only after the request completes, and the epoch then
     * advances. A second call before the previous request completed fails
     * with "HaloExchanger::begin: previous epoch not ended".
     *
     * The request's waitFor() polls test() every 50 microseconds; cancel()
     * does not cancel and returns test(); destroying an incomplete request
     * does not wait, leaving the exchanger's epoch open.
     *
     * \return A request (never null). A pattern or begin failure returns a
     *         request that is already complete with failed() true and the
     *         HaloExchanger message.
     */
    [[nodiscard]] std::unique_ptr<HydroCouple::Distributed::IExchangeRequest>
    synchronizeAsync() override
    {
        class Request final : public HydroCouple::Distributed::IExchangeRequest
        {
        public:
            explicit Request(HaloExchanger *exchanger, std::string failure)
                : m_exchanger(exchanger), m_failure(std::move(failure))
            {
            }
            bool test() override
            {
                if (m_complete || !m_failure.empty())
                    return true;
                bool complete = false;
                std::string message;
                if (!m_exchanger->tryEnd(complete, message))
                {
                    m_failure  = message;
                    m_complete = true;
                    return true;
                }
                m_complete = complete;
                return m_complete;
            }
            void wait() override
            {
                if (m_complete || !m_failure.empty())
                    return;
                std::string message;
                if (!m_exchanger->end(message))
                    m_failure = message;
                m_complete = true;
            }
            [[nodiscard]] bool waitFor(double timeoutSeconds) override
            {
                // The exchanger has no timed end(): poll test() to the deadline.
                const auto deadline =
                    std::chrono::steady_clock::now() +
                    std::chrono::duration<double>(std::max(0.0, timeoutSeconds));
                for (;;)
                {
                    if (test())
                        return true;
                    if (std::chrono::steady_clock::now() >= deadline)
                        return false;
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
            }
            // Cancellation is not supported by this request: report whether the
            // operation has already completed (true) or is still pending (false).
            [[nodiscard]] bool cancel() override { return test(); }
            bool failed(std::string &message) const override
            {
                message = m_failure;
                return !m_failure.empty();
            }

        private:
            HaloExchanger *m_exchanger;
            std::string    m_failure;
            bool           m_complete = false;
        };

        std::string message;
        if (!m_exchanger->patternBuilt() &&
            !m_exchanger->buildPattern(message))
            return std::make_unique<Request>(m_exchanger.get(), message);
        if (!m_exchanger->begin(m_values.data(),
                                HydroCouple::Helpers::dataKindSize(
                                    Helpers::DataKindOf<T>::kind),
                                message))
            return std::make_unique<Request>(m_exchanger.get(), message);
        return std::make_unique<Request>(m_exchanger.get(), std::string{});
    }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The local extent: owned plus mirrored entities.
     * \return { ownedCount + virtualCount }.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return {static_cast<int64_t>(m_values.size())};
    }

    /*!
     * \brief The element kind of T.
     * \return Helpers::DataKindOf<T>::kind.
     */
    [[nodiscard]] DataKind dataKind() const override
    {
        return Helpers::DataKindOf<T>::kind;
    }

    /*!
     * \brief Copies a hyperslab of the local array (owned then mirrored)
     *        into \p destination.
     * \param[in]  destination Host buffer of kind dataKind() whose element
     *                         count equals the product of \p count.
     * \param[in]  start       One start index (0 ≤ start ≤ extent).
     * \param[in]  count       One count (start + count ≤ extent).
     * \param[out] message     Optional; the hyperslab engine's reason on
     *                         failure ("kind mismatch", "selection out of
     *                         bounds", ...).
     * \return True on success.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override
    {
        const std::array<int64_t, 1> localShape{
            static_cast<int64_t>(m_values.size())};
        return HydroCouple::SDK::detail::readHyperslab<T>(
            m_values.data(), localShape, destination, start, count, message);
    }

    /*!
     * \brief Writes a hyperslab of the local array from \p source and
     *        emits the value-changed event for that selection.
     *
     * Writing mirrored positions is allowed but against the item's policy:
     * the next synchronization overwrites them.
     *
     * \param[in]  source  Host buffer of kind dataKind() whose element count
     *                     equals the product of \p count.
     * \param[in]  start   One start index.
     * \param[in]  count   One count.
     * \param[out] message Optional; the hyperslab engine's reason on failure.
     * \return True on success.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        const std::array<int64_t, 1> localShape{
            static_cast<int64_t>(m_values.size())};
        if (!HydroCouple::SDK::detail::writeHyperslab<T>(
                m_values.data(), localShape, source, start, count, message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── Typed local access ────────────────────────────────────────────────

    /*!
     * \brief Owned values (solver-writable region).
     * \return A writable view of the first ownedGlobalIndexes().size()
     *         values; valid for the item's lifetime. Writes do not emit the
     *         value-changed event.
     */
    [[nodiscard]] std::span<T> ownedValues() noexcept
    {
        return {m_values.data(), m_ownedGlobals.size()};
    }

    /*!
     * \brief Virtual mirrors (reconstructed each epoch — read-only policy).
     * \return A view of the last virtualGlobalIndexes().size() values;
     *         defined after the first synchronization completes (T{} before).
     */
    [[nodiscard]] std::span<const T> virtualValues() const noexcept
    {
        return {m_values.data() + m_ownedGlobals.size(),
                m_virtualGlobals.size()};
    }

    /*!
     * \brief The embedded exchanger (pattern building at prepare() time).
     * \return The exchanger owned by this item; call buildPattern() on it
     *         from every rank at prepare() time to keep the first
     *         synchronizeAsync() from negotiating.
     */
    [[nodiscard]] HaloExchanger &exchanger() noexcept { return *m_exchanger; }

private:
    int64_t              m_globalCount;    //!< Entities across all ranks.
    std::vector<int64_t> m_ownedGlobals;   //!< Global index of each owned position.
    std::vector<int64_t> m_virtualGlobals; //!< Global index of each mirrored position.
    std::vector<int32_t> m_virtualOwners;  //!< Owner rank of each mirrored position.
    std::vector<T>       m_values; //!< Owned-then-virtual contiguous storage.
    std::unique_ptr<HaloExchanger> m_exchanger; //!< Pattern and epoch engine over \c m_values.
};

//! A partitioned Float64 field (instantiated in the library).
using PartitionedDataItemDouble = PartitionedDataItem<double>;
//! A partitioned Int32 field (instantiated in the library).
using PartitionedDataItemInt    = PartitionedDataItem<int32_t>;

//! \cond EXPLICIT_INSTANTIATIONS
extern template class HYDROCOUPLESDK_EXPORT PartitionedDataItem<double>;
extern template class HYDROCOUPLESDK_EXPORT PartitionedDataItem<int32_t>;
//! \endcond

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_PARTITIONEDDATAITEM_H
