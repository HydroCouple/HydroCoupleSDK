// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   timeseries.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Time series data storage with interpolation support.
 */

#ifndef HYDROCOUPLESDK_TIMESERIES_H
#define HYDROCOUPLESDK_TIMESERIES_H

#include "hydrocouple/identity.h"
#include "timedata.h"

#include <vector>
#include <memory>
#include <algorithm>

namespace HydroCouple::Temporal {

/*!
 * \brief Time series data: (time, value) pairs with interpolation.
 *
 * Stores time-value pairs and provides interpolation at arbitrary times.
 * Times must be monotonically increasing. Interpolation methods:
 * - Linear: linearly interpolates between values
 * - Nearest: returns nearest neighbor value
 * - Constant: uses previous value (step function)
 *
 * \threadsafety Not thread-safe on its own; caller synchronizes concurrent access.
 */
class HYDROCOUPLESDK_EXPORT TimeSeries : public HydroCouple::Identity
{
public:
    enum class InterpolationMethod {
        Linear,     // Linear interpolation between points
        Nearest,    // Nearest neighbor
        Constant    // Use value from left (previous) point
    };

    /*!
     * \brief Construct an empty time series.
     * \param id           Identifier
     * \param method       Interpolation method (default Linear)
     * \param expectedSize Hint for vector pre-allocation
     */
    TimeSeries(std::string_view id,
               InterpolationMethod method = InterpolationMethod::Linear,
               int expectedSize = 100);

    /*!
     * \brief Construct from time and value arrays.
     * \param id    Identifier
     * \param times Vector of monotonically increasing Julian Day values
     * \param values Vector of scalar values (must have same length as times)
     * \param method Interpolation method
     */
    TimeSeries(std::string_view id,
               const std::vector<double> &times,
               const std::vector<double> &values,
               InterpolationMethod method = InterpolationMethod::Linear);

    ~TimeSeries() override = default;

    // Data access
    [[nodiscard]] int count() const noexcept { return static_cast<int>(m_times.size()); }
    [[nodiscard]] const std::vector<double> &times() const noexcept { return m_times; }
    [[nodiscard]] const std::vector<double> &values() const noexcept { return m_values; }

    /*!
     * \brief Add (time, value) pair. Times must be strictly increasing.
     */
    void addValue(double time, double value);

    /*!
     * \brief Clear all data points.
     */
    void clear() { m_times.clear(); m_values.clear(); }

    /*!
     * \brief Get interpolated value at arbitrary time.
     * \param time Query time (as Julian Day)
     * \return Interpolated value, or NaN if time is outside data range
     */
    [[nodiscard]] double getValue(double time) const;

    /*!
     * \brief Get interpolated values for a time array.
     * \param times Array of query times
     * \return Vector of interpolated values (same length as times)
     */
    [[nodiscard]] std::vector<double> getValues(const std::vector<double> &times) const;

    // Accessors
    [[nodiscard]] InterpolationMethod interpolationMethod() const noexcept { return m_method; }
    void setInterpolationMethod(InterpolationMethod m) { m_method = m; }

    [[nodiscard]] double minTime() const noexcept { return m_times.empty() ? std::nan("") : m_times.front(); }
    [[nodiscard]] double maxTime() const noexcept { return m_times.empty() ? std::nan("") : m_times.back(); }

    // File I/O
    /*!
     * \brief Load from CSV file with columns: time, value.
     * \param filename Path to CSV file
     * \param skipHeader If true, skip first line
     * \return true on success
     */
    bool loadFromCSV(const std::string &filename, bool skipHeader = true);

    /*!
     * \brief Save to CSV file.
     * \param filename Path to output file
     * \return true on success
     */
    bool saveToCSV(const std::string &filename) const;

private:
    std::vector<double> m_times;
    std::vector<double> m_values;
    InterpolationMethod m_method;

    [[nodiscard]] int findLowerBound(double time) const;
    [[nodiscard]] double interpolateLinear(int i, double time) const;
    [[nodiscard]] double interpolateNearest(int i, double time) const;
    [[nodiscard]] double interpolateConstant(int i) const;
};

} // namespace HydroCouple::Temporal

#endif // HYDROCOUPLESDK_TIMESERIES_H
