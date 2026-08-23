// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timeseriesadaptedoutput.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Time-series adapted output for interpolated data access.
 */

#ifndef HYDROCOUPLESDK_TIMESERIESADAPTEDOUTPUT_H
#define HYDROCOUPLESDK_TIMESERIESADAPTEDOUTPUT_H

#include "hydrocouplesdk/data/abstractadaptedoutput.h"
#include "timeseries.h"

#include <memory>

#include "hydrocoupletemporal.h"

namespace HydroCouple::SDK::Temporal {

// Interface types from the standard's Temporal namespace.
using namespace HydroCouple::Temporal;

/*!
 * \brief Adapted output that interpolates a TimeSeries at a query time.
 *
 * Wraps a TimeSeries and provides the interpolated value at the current
 * query time using the configured interpolation method (linear, nearest,
 * constant). The v2 data plane exposes the interpolated result as a rank-1
 * item of shape { 1 }; consumers set the query time via setQueryTime().
 *
 * \tparam T Scalar value type (double, int).
 * \threadsafety Not thread-safe on its own; caller synchronizes access.
 */
template<typename T>
class TimeSeriesAdaptedOutput
    : public HydroCouple::SDK::AbstractAdaptedOutput
{
public:
    /*!
     * \brief Construct an adapted output from a time-series.
     * \param id           Identifier for this adapted output.
     * \param timeSeries   Time-series to interpolate.
     * \param provider     Provider output.
     */
    TimeSeriesAdaptedOutput(std::string_view id,
                            std::shared_ptr<TimeSeries> timeSeries,
                            HydroCouple::IOutput *provider);

    ~TimeSeriesAdaptedOutput() override = default;

    // ── IAdaptedOutput ────────────────────────────────────────────────────

    /*!
     * \brief One-time initialisation before use. No-op.
     */
    void initialize() override;

    /*!
     * \brief Refresh adapted output values from provider; fires valueChanged.
     */
    void refresh() override;

    /*!
     * \brief IOutput::updateValues — no-op; interpolation happens on read.
     */
    void updateValues() override;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override;

    [[nodiscard]] DataKind dataKind() const override;

    /*!
     * \brief Writes the value interpolated at queryTime() into \p destination
     *        (selection must be start {0}, count {1}).
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override;

    /*!
     * \brief Always fails — adapted outputs are read-only.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── Time-series specific ──────────────────────────────────────────────

    /*!
     * \brief Get interpolated scalar value at a specific time.
     * \param time Query time (as Julian Day).
     */
    [[nodiscard]] T getInterpolatedValue(double time) const
    {
        return static_cast<T>(m_timeSeries->getValue(time));
    }

    /*!
     * \brief Set the query time for interpolation.
     */
    void setQueryTime(double julianDay) noexcept { m_queryTime = julianDay; }

    /*!
     * \brief Get the current query time.
     */
    [[nodiscard]] double queryTime() const noexcept { return m_queryTime; }

    /*!
     * \brief Access the underlying TimeSeries.
     */
    [[nodiscard]] std::shared_ptr<TimeSeries> timeSeries() const noexcept
    {
        return m_timeSeries;
    }

private:
    std::shared_ptr<TimeSeries> m_timeSeries;
    double m_queryTime; //!< Julian Day for interpolation queries.
};

using TimeSeriesAdaptedOutputDouble = TimeSeriesAdaptedOutput<double>;
using TimeSeriesAdaptedOutputInt    = TimeSeriesAdaptedOutput<int>;

extern template class HYDROCOUPLESDK_EXPORT TimeSeriesAdaptedOutput<double>;
extern template class HYDROCOUPLESDK_EXPORT TimeSeriesAdaptedOutput<int>;

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESADAPTEDOUTPUT_H
