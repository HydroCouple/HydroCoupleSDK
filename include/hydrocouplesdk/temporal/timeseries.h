// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timeseries.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Time series data storage with interpolation support.
 */

#ifndef HYDROCOUPLESDK_TIMESERIES_H
#define HYDROCOUPLESDK_TIMESERIES_H

#include "hydrocouplesdk/core/identity.h"
#include "timedata.h"

#include <vector>
#include <memory>
#include <algorithm>

#include "hydrocoupletemporal.h"

namespace HydroCouple::SDK::Temporal {

// Interface types from the standard's Temporal namespace.
using namespace HydroCouple::Temporal;

/*!
 * \brief Time series data: (time, value) pairs with interpolation.
 *
 * Stores time-value pairs and provides interpolation at arbitrary times.
 * Times must be monotonically increasing. Interpolation methods:
 * - Linear: linearly interpolates between values
 * - Nearest: returns nearest neighbor value
 * - Constant: uses previous value (step function)
 *
 * \details What getValue() returns, for times \f$t_0 < \dots < t_{n-1}\f$ and
 * values \f$v_0, \dots, v_{n-1}\f$ (measured by the SDK's documentation
 * examples):
 * | Query \f$t\f$ | Linear | Nearest | Constant |
 * | --- | --- | --- | --- |
 * | \f$t < t_0\f$ | NaN | NaN | NaN |
 * | \f$t = t_0\f$ | \f$v_0\f$ | \f$v_0\f$ | \f$v_0\f$ |
 * | \f$t_{k-1} < t < t_k\f$ | \f$v_{k-1} + \frac{t - t_{k-1}}{t_k - t_{k-1}}(v_k - v_{k-1})\f$ | the nearer neighbor; \f$v_{k-1}\f$ on a tie | \f$v_{k-1}\f$ |
 * | \f$t = t_k\f$, \f$k \ge 1\f$ | \f$v_k\f$ | \f$v_k\f$ | \f$v_{k-1}\f$ (the value before the sample) |
 * | \f$t > t_{n-1}\f$ | NaN | \f$v_{n-1}\f$ | \f$v_{n-1}\f$ |
 *
 * Constant is left-continuous at every sample after the first: at exactly
 * \f$t_k\f$ it still returns \f$v_{k-1}\f$, and \f$v_{n-1}\f$ appears only
 * after \f$t_{n-1}\f$. Times are Julian days by convention, but any
 * increasing double coordinate works; no tolerance is applied.
 *
 * The class is an Identity but implements no HydroCouple interface: it is
 * the storage behind TimeSeriesArgumentDouble, TimeSeriesAdaptedOutput and
 * Temporal::TemporalInterpolationAdaptedOutput.
 *
 * \threadsafety Not thread-safe on its own; caller synchronizes concurrent access.
 */
class HYDROCOUPLESDK_EXPORT TimeSeries : public HydroCouple::SDK::Identity
{
public:
    //! How getValue() fills the gap between two samples (see the class table).
    enum class InterpolationMethod {
        Linear,     //!< Linear interpolation between points; NaN outside the samples.
        Nearest,    //!< Nearest neighbor; the earlier one on a tie; holds the last value after the end.
        Constant    //!< Use value from left (previous) point; holds the last value after the end.
    };

    /*!
     * \brief Construct an empty time series.
     * \param id           Identifier
     * \param method       Interpolation method (default Linear)
     * \param expectedSize Hint for vector pre-allocation
     * \details A negative \p expectedSize makes reserve() throw std::length_error.
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
     * \details Neither requirement is checked: unordered times give wrong
     *          lookups, and \p values shorter than \p times makes getValue()
     *          read past the end of the values. Build a checked series
     *          with addValue() instead when the input is not trusted.
     */
    TimeSeries(std::string_view id,
               const std::vector<double> &times,
               const std::vector<double> &values,
               InterpolationMethod method = InterpolationMethod::Linear);

    ~TimeSeries() override = default;

    // Data access
    //! Number of samples.
    //! \return The length of times().
    [[nodiscard]] int count() const noexcept { return static_cast<int>(m_times.size()); }
    //! Sample times, in increasing order.
    //! \return A reference valid until the series changes.
    [[nodiscard]] const std::vector<double> &times() const noexcept { return m_times; }
    //! Sample values, parallel to times().
    //! \return A reference valid until the series changes.
    [[nodiscard]] const std::vector<double> &values() const noexcept { return m_values; }

    /*!
     * \brief Add (time, value) pair. Times must be strictly increasing.
     * \param[in] time  Sample time; must exceed the last time.
     * \param[in] value Sample value.
     * \exception std::invalid_argument "TimeSeries: times must be strictly
     *            increasing" when \p time is not after the last time; the
     *            series is then unchanged.
     */
    void addValue(double time, double value);

    /*!
     * \brief Clear all data points.
     * \details The interpolation method is kept.
     */
    void clear() { m_times.clear(); m_values.clear(); }

    /*!
     * \brief Get interpolated value at arbitrary time.
     * \details See the class table: Linear returns NaN outside the samples,
     *          while Nearest and Constant hold the last value after the end.
     *          An empty series returns NaN. The lookup is a binary search.
     * \param time Query time (as Julian Day)
     * \return Interpolated value, or NaN if time is outside data range
     */
    [[nodiscard]] double getValue(double time) const;

    /*!
     * \brief Get interpolated values for a time array.
     * \details getValue() for each time, in order; the query times need not
     *          be sorted.
     * \param times Array of query times
     * \return Vector of interpolated values (same length as times)
     */
    [[nodiscard]] std::vector<double> getValues(const std::vector<double> &times) const;

    // Accessors
    //! The interpolation method getValue() uses.
    //! \return The method set at construction or by setInterpolationMethod().
    [[nodiscard]] InterpolationMethod interpolationMethod() const noexcept { return m_method; }
    //! Changes the interpolation method; applies to the next getValue().
    //! \param[in] m New method.
    void setInterpolationMethod(InterpolationMethod m) { m_method = m; }

    //! First sample time.
    //! \return times().front(), or NaN for an empty series.
    [[nodiscard]] double minTime() const noexcept { return m_times.empty() ? std::nan("") : m_times.front(); }
    //! Last sample time.
    //! \return times().back(), or NaN for an empty series.
    [[nodiscard]] double maxTime() const noexcept { return m_times.empty() ? std::nan("") : m_times.back(); }

    // File I/O
    /*!
     * \brief Load from CSV file with columns: time, value.
     * \param filename Path to CSV file
     * \param skipHeader If true, skip first line
     * \return true on success
     * \details Reads the whole file and calls loadFromCSVText(). A file that
     *          cannot be opened returns false and leaves the series unchanged.
     */
    bool loadFromCSV(const std::string &filename, bool skipHeader = true);

    /*!
     * \brief Parses CSV text that has already been read.
     * \details The parse half of loadFromCSV(), split out because a series
     * fetched from a service arrives as bytes and never as a path.
     * \param text CSV text, one "time,value" pair per line.
     * \param skipHeader Whether the first line is a header.
     * \details Clears the series first. Each line is split at its first
     *          comma; a line with no comma is skipped. Both fields are read
     *          with std::stod, so leading spaces are allowed and the time is a
     *          plain number (a Julian day, not a date string). A field that
     *          does not parse, or a time not after the previous one, stops the
     *          load and returns false with the rows read so far kept.
     * \return true when every line was read.
     */
    bool loadFromCSVText(const std::string &text, bool skipHeader = true);

    /*!
     * \brief Save to CSV file.
     * \details Writes a "Time,Value" header and one row per sample with the
     *          stream's default precision of six significant digits. A
     *          Julian day such as 2460000.5 is written as 2.46e+06, so a
     *          saved series of real dates cannot be read back (its times
     *          collapse and loadFromCSV() fails); only small time coordinates
     *          survive the round trip.
     * \param filename Path to output file
     * \return true on success
     */
    bool saveToCSV(const std::string &filename) const;

private:
    std::vector<double> m_times;  //!< Sample times, strictly increasing.
    std::vector<double> m_values; //!< Sample values, parallel to m_times.
    InterpolationMethod m_method; //!< Method used by getValue().

    //! Index of the sample at or before \p time used to interpolate.
    //! \param[in] time Query time.
    //! \return -1 before the first sample (or when empty); the last index
    //!         after the last sample; otherwise the index k - 1 of the
    //!         first sample time t_k not less than \p time (0 at t_0).
    [[nodiscard]] int findLowerBound(double time) const;
    //! Linear interpolation between samples \p i and \p i + 1.
    //! \param[in] i    Lower sample index from findLowerBound().
    //! \param[in] time Query time.
    //! \return The interpolated value, or NaN when \p i is -1 or the last index.
    [[nodiscard]] double interpolateLinear(int i, double time) const;
    //! The nearer of samples \p i and \p i + 1 (the earlier on a tie).
    //! \param[in] i    Lower sample index from findLowerBound().
    //! \param[in] time Query time.
    //! \return The value, or NaN when \p i is -1.
    [[nodiscard]] double interpolateNearest(int i, double time) const;
    //! The value of sample \p i.
    //! \param[in] i Lower sample index from findLowerBound().
    //! \return The value, or NaN when \p i is -1.
    [[nodiscard]] double interpolateConstant(int i) const;
};

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMESERIES_H
