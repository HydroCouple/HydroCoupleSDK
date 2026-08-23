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

#include <array>
#include <memory>
#include <span>
#include <string>
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
 * \tparam T Element type (double, int, …).
 */
template <typename T>
class PartitionedDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public virtual HydroCouple::Distributed::IPartitionedComponentDataItem
{
public:
    /*!
     * \brief Construct from decomposition metadata.
     * \param[in] id                   Item identifier.
     * \param[in] globalCount          Entities across all ranks.
     * \param[in] ownedGlobalIndexes   Globals this rank owns.
     * \param[in] virtualGlobalIndexes Globals this rank mirrors.
     * \param[in] virtualOwners        Owner rank per virtual entry.
     * \param[in] transport            Transport endpoint (borrowed).
     * \param[in] baseTag              Distinct tag base per item.
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

    ~PartitionedDataItem() override = default;

    // ── IPartitionedComponentDataItem ────────────────────────────────────

    [[nodiscard]] int64_t globalCount() const override
    {
        return m_globalCount;
    }

    [[nodiscard]] std::span<const int64_t> ownedGlobalIndexes() const override
    {
        return {m_ownedGlobals.data(), m_ownedGlobals.size()};
    }

    [[nodiscard]] std::span<const int64_t> virtualGlobalIndexes()
        const override
    {
        return {m_virtualGlobals.data(), m_virtualGlobals.size()};
    }

    [[nodiscard]] std::span<const int32_t> virtualOwners() const override
    {
        return {m_virtualOwners.data(), m_virtualOwners.size()};
    }

    [[nodiscard]] uint64_t synchronizationEpoch() const override
    {
        return m_exchanger->epoch();
    }

    /*!
     * \brief Starts an epoch exchange; the returned request completes it
     *        (wait() = scatter halos; test() polls). Overlap computation on
     *        owned values between this call and completion.
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

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return {static_cast<int64_t>(m_values.size())};
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return Helpers::DataKindOf<T>::kind;
    }

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
     */
    [[nodiscard]] std::span<T> ownedValues() noexcept
    {
        return {m_values.data(), m_ownedGlobals.size()};
    }

    /*!
     * \brief Virtual mirrors (reconstructed each epoch — read-only policy).
     */
    [[nodiscard]] std::span<const T> virtualValues() const noexcept
    {
        return {m_values.data() + m_ownedGlobals.size(),
                m_virtualGlobals.size()};
    }

    /*!
     * \brief The embedded exchanger (pattern building at prepare() time).
     */
    [[nodiscard]] HaloExchanger &exchanger() noexcept { return *m_exchanger; }

private:
    int64_t              m_globalCount;
    std::vector<int64_t> m_ownedGlobals;
    std::vector<int64_t> m_virtualGlobals;
    std::vector<int32_t> m_virtualOwners;
    std::vector<T>       m_values; //!< Owned-then-virtual contiguous storage.
    std::unique_ptr<HaloExchanger> m_exchanger;
};

using PartitionedDataItemDouble = PartitionedDataItem<double>;
using PartitionedDataItemInt    = PartitionedDataItem<int32_t>;

extern template class HYDROCOUPLESDK_EXPORT PartitionedDataItem<double>;
extern template class HYDROCOUPLESDK_EXPORT PartitionedDataItem<int32_t>;

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_PARTITIONEDDATAITEM_H
