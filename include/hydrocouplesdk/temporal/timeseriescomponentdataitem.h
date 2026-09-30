// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timeseriescomponentdataitem.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Time-series data storage mixin for temporal component data items.
 *
 * Pure storage/accessor mixin (like ComponentDataItem1D) providing time
 * management and flat row-major value storage indexed by
 * (timeIndex, elementIndex). Implements the v2 typed hyperslab data plane
 * with time as dimension 0, per the v2 standard's canonical ordering, and
 * keeps a contiguous Julian-day cache for ITimeSeriesComponentDataItem::times().
 */

#ifndef HYDROCOUPLESDK_TIMESERIESCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_TIMESERIESCOMPONENTDATAITEM_H

#include "hydrocouple.h"
#include "hydrocouplesdk/data/bufferio.h"
#include "timedata.h"

#include <array>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include "hydrocoupletemporal.h"

namespace HydroCouple::SDK::Temporal {

// Interface types from the standard's Temporal namespace.
using namespace HydroCouple::Temporal;

/*!
 * \brief Time-series data storage mixin providing temporal indexing.
 *
 * Concrete classes multiply-inherit from an Input/Output/Argument base AND
 * this mixin.
 *
 * Data layout (for element dimension of size N):
 * - m_times[0..T-1]: shared_ptr<IDateTime> per time step
 * - m_timesJulian[0..T-1]: contiguous Julian-day cache (for times() span)
 * - m_values[t * N + i]: value at time t, element index i
 *
 * \tparam T Scalar value type (int, double, std::string, …)
 * \threadsafety Not thread-safe on its own; caller synchronizes concurrent access.
 */
template<typename T>
class TimeSeriesComponentDataItem
{
public:
    /*!
     * \brief Construct a time-series storage mixin.
     * \param expectedTimeSteps Hint for vector pre-allocation.
     * \param elementSize Number of elements at each time step (1 for scalar series).
     */
    explicit TimeSeriesComponentDataItem(int expectedTimeSteps = 100, int elementSize = 1)
        : m_elementSize(elementSize),
          m_timeSpan(std::make_shared<TimeSpan>("TimeSpan", 0.0, 0.0))
    {
        m_times.reserve(expectedTimeSteps);
        m_timesJulian.reserve(expectedTimeSteps);
        m_values.reserve(static_cast<size_t>(expectedTimeSteps) * elementSize);
    }

    virtual ~TimeSeriesComponentDataItem() = default;

    // ── Time management ──────────────────────────────────────────────────

    /*!
     * \brief Add a time step with initial element values.
     * \param time   DateTime for this time step (should be strictly increasing).
     * \param values Values for this time step (must match elementSize).
     */
    void addTimeStep(std::shared_ptr<IDateTime> time, const std::vector<T> &values)
    {
        if (values.size() != static_cast<size_t>(m_elementSize))
            throw std::invalid_argument(
                "TimeSeriesComponentDataItem: values size must match elementSize");
        m_timesJulian.push_back(time->julianDay());
        m_times.push_back(std::move(time));
        m_values.insert(m_values.end(), values.begin(), values.end());

        const double start = m_timesJulian.front();
        const double end   = m_timesJulian.back();
        m_timeSpan = std::make_shared<TimeSpan>("TimeSpan", start, end - start);
    }

    /*!
     * \brief ITimeSeriesComponentDataItem::time implementation.
     */
    [[nodiscard]] const IDateTime *time(int64_t timeIndex) const
    {
        if (timeIndex < 0 || timeIndex >= static_cast<int64_t>(m_times.size()))
            return nullptr;
        return m_times[static_cast<size_t>(timeIndex)].get();
    }

    /*!
     * \brief ITimeSeriesComponentDataItem::timeCount implementation.
     */
    [[nodiscard]] int64_t timeCount() const noexcept
    {
        return static_cast<int64_t>(m_times.size());
    }

    /*!
     * \brief ITimeSeriesComponentDataItem::times implementation — contiguous
     *        Julian-day values, one per time step.
     */
    [[nodiscard]] std::span<const double> times() const noexcept
    {
        return {m_timesJulian.data(), m_timesJulian.size()};
    }

    // ── Temporal semantics (ITimeSeriesComponentDataItem) ────────────────
    //
    // What a time coordinate refers to and what the item does between/outside
    // its held instants. Defaults are the honest "not declared" values; a
    // component that knows (every component does) sets them at construction.

    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const noexcept { return m_timeKind; }
    [[nodiscard]] double intervalLength() const noexcept { return m_intervalLength; }
    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const noexcept { return m_timeInterpolation; }
    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const noexcept { return m_timeExtrapolation; }

    void setTimeKind(HydroCouple::Temporal::TimeKind kind, double intervalLengthDays = 0.0) noexcept
    {
        m_timeKind = kind;
        m_intervalLength = intervalLengthDays;
    }
    void setTimeInterpolation(HydroCouple::Temporal::TimeInterpolation policy) noexcept { m_timeInterpolation = policy; }
    void setTimeExtrapolation(HydroCouple::Temporal::TimeExtrapolation policy) noexcept { m_timeExtrapolation = policy; }

    /*!
     * \brief ITimeSeriesComponentDataItem::timeSpan implementation.
     */
    [[nodiscard]] ITimeSpan *timeSpan() const noexcept { return m_timeSpan.get(); }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The DataKind of the stored element type.
     */
    [[nodiscard]] static constexpr DataKind storageKind()
    {
        return HydroCouple::Helpers::DataKindOf<T>::kind;
    }

    /*!
     * \brief The item's shape: { timeCount, elementSize }. Time is
     *        dimension 0 per the v2 standard's canonical ordering.
     */
    [[nodiscard]] std::vector<int64_t> storageShape() const
    {
        return {static_cast<int64_t>(m_times.size()),
                static_cast<int64_t>(m_elementSize)};
    }

    /*!
     * \brief Copies a hyperslab of the storage into \p destination
     *        (IComponentDataItem::getValuesInto semantics).
     */
    [[nodiscard]] bool getSlab(const BufferDescriptor &destination,
                               std::span<const int64_t> start,
                               std::span<const int64_t> count,
                               std::string *message = nullptr) const
    {
        const std::array<int64_t, 2> shape{
            static_cast<int64_t>(m_times.size()),
            static_cast<int64_t>(m_elementSize)};
        return HydroCouple::SDK::detail::readHyperslab<T>(
            m_values.data(), shape, destination, start, count, message);
    }

    /*!
     * \brief Copies values from \p source into a hyperslab of the storage
     *        (IComponentDataItem::setValuesFrom semantics).
     */
    [[nodiscard]] bool setSlab(const BufferDescriptor &source,
                               std::span<const int64_t> start,
                               std::span<const int64_t> count,
                               std::string *message = nullptr)
    {
        const std::array<int64_t, 2> shape{
            static_cast<int64_t>(m_times.size()),
            static_cast<int64_t>(m_elementSize)};
        return HydroCouple::SDK::detail::writeHyperslab<T>(
            m_values.data(), shape, source, start, count, message);
    }

    // ── Direct typed access ───────────────────────────────────────────────

    /*!
     * \brief Get value at specific time and element index.
     */
    [[nodiscard]] T getValueAt(int64_t timeIndex, int64_t elementIndex) const
    {
        const int64_t idx = timeIndex * m_elementSize + elementIndex;
        if (timeIndex < 0 || timeIndex >= timeCount() ||
            elementIndex < 0 || elementIndex >= m_elementSize)
            return T{};
        return m_values[static_cast<size_t>(idx)];
    }

    /*!
     * \brief Set value at specific time and element index.
     */
    void setValueAt(int64_t timeIndex, int64_t elementIndex, const T &value)
    {
        if (timeIndex < 0 || timeIndex >= timeCount() ||
            elementIndex < 0 || elementIndex >= m_elementSize)
            return;
        m_values[static_cast<size_t>(timeIndex * m_elementSize + elementIndex)] = value;
    }

    /*!
     * \brief Number of elements at each time step.
     */
    [[nodiscard]] int elementSize() const noexcept { return m_elementSize; }

    // ── Raw / SIMD-eligible bulk access ──────────────────────────────────

    [[nodiscard]] const std::vector<T> &rawData() const noexcept { return m_values; }
    [[nodiscard]] std::vector<T>       &rawData()       noexcept { return m_values; }

    /*!
     * \brief Removes all time steps and values.
     */
    void clearTimeSteps()
    {
        m_times.clear();
        m_timesJulian.clear();
        m_values.clear();
        m_timeSpan = std::make_shared<TimeSpan>("TimeSpan", 0.0, 0.0);
    }

protected:
    std::vector<std::shared_ptr<IDateTime>> m_times;
    HydroCouple::Temporal::TimeKind m_timeKind = HydroCouple::Temporal::TimeKind::Unknown;
    double m_intervalLength = 0.0;
    HydroCouple::Temporal::TimeInterpolation m_timeInterpolation = HydroCouple::Temporal::TimeInterpolation::Unknown;
    HydroCouple::Temporal::TimeExtrapolation m_timeExtrapolation = HydroCouple::Temporal::TimeExtrapolation::Unknown;
    std::vector<double> m_timesJulian; //!< Julian-day cache backing times().
    std::vector<T>      m_values;      //!< Flat: [time * elementSize + element].
    int                 m_elementSize;
    std::shared_ptr<TimeSpan> m_timeSpan;
};

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESCOMPONENTDATAITEM_H
