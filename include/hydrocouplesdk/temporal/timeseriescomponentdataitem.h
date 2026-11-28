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
 * The time axis grows: addTimeStep() appends a row, so shape() is
 * { timeCount(), elementSize() } and changes as the component records.
 * Values of existing rows can be overwritten with setValueAt() or the data
 * plane; no member removes a single row.
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
     * \details Starts with no time steps, a zero span at Julian day 0, and
     *          the temporal semantics "not declared" (Unknown, interval 0).
     */
    explicit TimeSeriesComponentDataItem(int expectedTimeSteps = 100, int elementSize = 1)
        : m_elementSize(elementSize),
          m_timeSpan(std::make_shared<TimeSpan>("TimeSpan", 0.0, 0.0))
    {
        m_times.reserve(expectedTimeSteps);
        m_timesJulian.reserve(expectedTimeSteps);
        m_values.reserve(static_cast<size_t>(expectedTimeSteps) * elementSize);
    }

    //! Destroys the storage and the IDateTime objects it shares.
    virtual ~TimeSeriesComponentDataItem() = default;

    // ── Time management ──────────────────────────────────────────────────

    /*!
     * \brief Add a time step with initial element values.
     * \param time   DateTime for this time step (should be strictly increasing).
     * \param values Values for this time step (must match elementSize).
     * \details The order of times is not checked, and \p time must not be
     *          nullptr. The item shares ownership of \p time. Each call
     *          replaces the TimeSpan object, so a pointer from an earlier
     *          timeSpan() no longer refers to the item's span (and dangles if
     *          nothing else holds it); it also may invalidate the times()
     *          span and rawData() references. Fires no signal.
     * \exception std::invalid_argument "TimeSeriesComponentDataItem: values
     *            size must match elementSize" when \p values has the wrong
     *            length; nothing is added.
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
     * \param[in] timeIndex Zero-based time step.
     * \return The IDateTime given to addTimeStep() (non-owning), or nullptr
     *         when \p timeIndex is out of range.
     */
    [[nodiscard]] const IDateTime *time(int64_t timeIndex) const
    {
        if (timeIndex < 0 || timeIndex >= static_cast<int64_t>(m_times.size()))
            return nullptr;
        return m_times[static_cast<size_t>(timeIndex)].get();
    }

    /*!
     * \brief ITimeSeriesComponentDataItem::timeCount implementation.
     * \return The number of time steps recorded.
     */
    [[nodiscard]] int64_t timeCount() const noexcept
    {
        return static_cast<int64_t>(m_times.size());
    }

    /*!
     * \brief ITimeSeriesComponentDataItem::times implementation — contiguous
     *        Julian-day values, one per time step.
     * \return A span over the cached Julian days; valid until the next
     *         addTimeStep() or clearTimeSteps().
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

    //! What each value's time coordinate refers to.
    //! \return The kind set by setTimeKind(); TimeKind::Unknown by default.
    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const noexcept { return m_timeKind; }
    //! Length of the averaging or accumulation interval, in days.
    //! \return The length set by setTimeKind(); 0 (not declared) by default.
    [[nodiscard]] double intervalLength() const noexcept { return m_intervalLength; }
    //! What the item does, or accepts, between held instants.
    //! \return The declared policy; Unknown by default (TimeSeriesInput and
    //!         TimeSeriesOutput set None).
    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const noexcept { return m_timeInterpolation; }
    //! What the item does, or accepts, outside its held range.
    //! \return The declared policy; Unknown by default (TimeSeriesInput and
    //!         TimeSeriesOutput set Refuse).
    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const noexcept { return m_timeExtrapolation; }

    /*!
     * \brief Declares what the time coordinates mean.
     * \details Metadata only; the storage does not change. validateConnection()
     *          compares it across a link.
     * \param[in] kind               Instantaneous, an interval statistic, or
     *                               Accumulated.
     * \param[in] intervalLengthDays Interval length in days for an interval
     *                               kind; INTERVAL_LENGTH_CONTIGUOUS (any
     *                               negative value) for contiguous intervals
     *                               of varying length; 0 when not declared.
     */
    void setTimeKind(HydroCouple::Temporal::TimeKind kind, double intervalLengthDays = 0.0) noexcept
    {
        m_timeKind = kind;
        m_intervalLength = intervalLengthDays;
    }
    //! Declares the interpolation policy. Metadata only: the storage serves
    //! held instants by index and never interpolates.
    //! \param[in] policy The policy to report.
    void setTimeInterpolation(HydroCouple::Temporal::TimeInterpolation policy) noexcept { m_timeInterpolation = policy; }
    //! Declares the extrapolation policy. Metadata only.
    //! \param[in] policy The policy to report.
    void setTimeExtrapolation(HydroCouple::Temporal::TimeExtrapolation policy) noexcept { m_timeExtrapolation = policy; }

    /*!
     * \brief ITimeSeriesComponentDataItem::timeSpan implementation.
     * \return The span from the first to the last recorded time (start 0,
     *         duration 0 when empty). Owned by the item and replaced by every
     *         addTimeStep() and clearTimeSteps(): do not keep the pointer.
     */
    [[nodiscard]] ITimeSpan *timeSpan() const noexcept { return m_timeSpan.get(); }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The DataKind of the stored element type.
     * \return Helpers::DataKindOf of \c T.
     */
    [[nodiscard]] static constexpr DataKind storageKind()
    {
        return HydroCouple::Helpers::DataKindOf<T>::kind;
    }

    /*!
     * \brief The item's shape: { timeCount, elementSize }. Time is
     *        dimension 0 per the v2 standard's canonical ordering.
     * \return The two extents; the first grows with addTimeStep().
     */
    [[nodiscard]] std::vector<int64_t> storageShape() const
    {
        return {static_cast<int64_t>(m_times.size()),
                static_cast<int64_t>(m_elementSize)};
    }

    /*!
     * \brief Copies a hyperslab of the storage into \p destination
     *        (IComponentDataItem::getValuesInto semantics).
     * \details start {t, e} and count {nt, ne} select time steps t to
     *          t + nt - 1 and elements e to e + ne - 1; "time step t, all
     *          elements" is a contiguous row.
     * \param[in]  destination Caller's host buffer of kind storageKind().
     * \param[in]  start       First time step and element.
     * \param[in]  count       Number of time steps and elements.
     * \param[out] message     Failure description; may be nullptr.
     * \return true when the values were copied.
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
     * \details Overwrites existing time steps only; it cannot add one.
     * \param[in]  source  Caller's host buffer of kind storageKind().
     * \param[in]  start   First time step and element.
     * \param[in]  count   Number of time steps and elements.
     * \param[out] message Failure description; may be nullptr.
     * \return true when the values were stored.
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
     * \param[in] timeIndex    Zero-based time step.
     * \param[in] elementIndex Zero-based element.
     * \return The value, or \c T{} when either index is out of range (no
     *         error is reported).
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
     * \details Out-of-range indices are ignored silently. Fires no signal.
     * \param[in] timeIndex    Zero-based time step.
     * \param[in] elementIndex Zero-based element.
     * \param[in] value        New value.
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
     * \return The element count fixed at construction.
     */
    [[nodiscard]] int elementSize() const noexcept { return m_elementSize; }

    // ── Raw / SIMD-eligible bulk access ──────────────────────────────────

    //! The values, time-major: value (t, e) at t * elementSize() + e.
    //! \return A const reference to the vector.
    [[nodiscard]] const std::vector<T> &rawData() const noexcept { return m_values; }
    //! The values for direct writes (no signal).
    //! \return A mutable reference; do not change its size.
    [[nodiscard]] std::vector<T>       &rawData()       noexcept { return m_values; }

    /*!
     * \brief Removes all time steps and values.
     * \details Resets the span to start 0, duration 0. The temporal
     *          semantics are kept.
     */
    void clearTimeSteps()
    {
        m_times.clear();
        m_timesJulian.clear();
        m_values.clear();
        m_timeSpan = std::make_shared<TimeSpan>("TimeSpan", 0.0, 0.0);
    }

protected:
    std::vector<std::shared_ptr<IDateTime>> m_times; //!< One IDateTime per time step (shared).
    //! Declared meaning of the time coordinates.
    HydroCouple::Temporal::TimeKind m_timeKind = HydroCouple::Temporal::TimeKind::Unknown;
    double m_intervalLength = 0.0; //!< Declared interval length in days.
    //! Declared interpolation policy.
    HydroCouple::Temporal::TimeInterpolation m_timeInterpolation = HydroCouple::Temporal::TimeInterpolation::Unknown;
    //! Declared extrapolation policy.
    HydroCouple::Temporal::TimeExtrapolation m_timeExtrapolation = HydroCouple::Temporal::TimeExtrapolation::Unknown;
    std::vector<double> m_timesJulian; //!< Julian-day cache backing times().
    std::vector<T>      m_values;      //!< Flat: [time * elementSize + element].
    int                 m_elementSize; //!< Elements per time step.
    std::shared_ptr<TimeSpan> m_timeSpan; //!< First-to-last span, rebuilt by addTimeStep().
};

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESCOMPONENTDATAITEM_H
