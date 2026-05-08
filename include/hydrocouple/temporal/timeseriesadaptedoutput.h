// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   timeseriesadaptedoutput.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Time-series adapted outputs for interpolated data access.
 */

#ifndef HYDROCOUPLESDK_TIMESERIESADAPTEDOUTPUT_H
#define HYDROCOUPLESDK_TIMESERIESADAPTEDOUTPUT_H

#include "hydrocouple/abstractadaptedoutput.h"
#include "timeseries.h"

#include <memory>
#include <vector>

namespace HydroCouple::Temporal {

/*!
 * \brief Adapted output that interpolates a TimeSeries at query times.
 *
 * Wraps a TimeSeries or TimeSeriesOutput and provides interpolated values
 * at any query time using the configured interpolation method (linear,
 * nearest, constant). Enables consumers to sample time-varying data at
 * arbitrary times without storing the entire time-series in memory.
 *
 * \tparam T Scalar value type (double, int, string)
 * \threadsafety Not thread-safe on its own; caller synchronizes access.
 */
template<typename T>
class HYDROCOUPLESDK_EXPORT TimeSeriesAdaptedOutput
    : public AbstractAdaptedOutput
{
public:
    /*!
     * \brief Construct an adapted output from a time-series.
     * \param id           Identifier for this adapted output
     * \param timeSeries   Time-series to interpolate
     * \param provider     Provider output (ITimeSeriesComponentDataItem)
     * \param consumers    Vector of consumer inputs
     */
    TimeSeriesAdaptedOutput(std::string_view id,
                            std::shared_ptr<TimeSeries> timeSeries,
                            HydroCouple::IOutput *provider);

    ~TimeSeriesAdaptedOutput() override = default;

    // ── IAdaptedOutput ────────────────────────────────────────────────────

    /*!
     * \brief Refresh adapted output values from provider.
     * Called by framework when provider updates.
     */
    void refresh() override;

    // ── IComponentDataItem ────────────────────────────────────────────────

    /*!
     * \brief Get interpolated value at query time.
     */
    void getValue(HydroCouple::hydrocouple_variant &data,
                  const std::initializer_list<int> &dimensionIndexes) const override;

    /*!
     * \brief Get interpolated values for a time range.
     */
    void getValues(HydroCouple::hydrocouple_variant *data,
                   const std::initializer_list<int> &dimensionIndexes,
                   const std::initializer_list<int> &dimensionLengths) const override;

    /*!
     * \brief Not supported for adapted output.
     */
    void setValue(const HydroCouple::hydrocouple_variant &data,
                  const std::initializer_list<int> &dimensionIndexes) override
    {
        // Adapted outputs are read-only
    }

    /*!
     * \brief Not supported for adapted output.
     */
    void setValues(const HydroCouple::hydrocouple_variant *data,
                   const std::initializer_list<int> &dimensionIndexes,
                   const std::initializer_list<int> &dimensionLengths) override
    {
        // Adapted outputs are read-only
    }

    int dimensionLength(const std::initializer_list<int> &dims) const override;

    // ── Time-series specific ──────────────────────────────────────────────

    /*!
     * \brief Get interpolated scalar value at a specific time.
     * \param time Query time (as Julian Day)
     * \return Interpolated value
     */
    T getInterpolatedValue(double time) const {
        return static_cast<T>(m_timeSeries->getValue(time));
    }

    /*!
     * \brief Set the query time for interpolation.
     * Used by consumers to request values at a specific time.
     */
    void setQueryTime(double julianDay) noexcept { m_queryTime = julianDay; }

    /*!
     * \brief Get the current query time.
     */
    double queryTime() const noexcept { return m_queryTime; }

    /*!
     * \brief Access the underlying TimeSeries.
     */
    std::shared_ptr<TimeSeries> timeSeries() const noexcept { return m_timeSeries; }

private:
    std::shared_ptr<TimeSeries> m_timeSeries;
    double m_queryTime;  // Julian Day for interpolation queries
};

// ── Template specializations for common types ──────────────────────────

using TimeSeriesAdaptedOutputDouble = TimeSeriesAdaptedOutput<double>;
using TimeSeriesAdaptedOutputInt = TimeSeriesAdaptedOutput<int>;

} // namespace HydroCouple::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESADAPTEDOUTPUT_H
