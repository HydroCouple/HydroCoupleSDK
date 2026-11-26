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
 * \details The values come from the TimeSeries handed to the constructor,
 * not from the provider: the provider is only the adaptee it is registered
 * on (and so the component its errors go to). No factory creates it and a
 * composition document cannot name it; build it in code. It has no
 * arguments, no dimensions and no value definition, and its factory is
 * nullptr. The query time starts at 0 (a Julian day in 4713 BC), so set it
 * before reading. Temporal::TemporalInterpolationAdaptedOutput is the
 * adapter that samples its adaptee instead.
 *
 * For \c int, a NaN result (outside the series under Linear) is converted
 * with static_cast, which is undefined behavior; use \c double.
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
     * \details \p timeSeries is shared, so later changes to it are seen. The
     *          adapter is not registered with \p provider; call
     *          provider->addAdaptedOutput() if refreshes should reach it.
     */
    TimeSeriesAdaptedOutput(std::string_view id,
                            std::shared_ptr<TimeSeries> timeSeries,
                            HydroCouple::IOutput *provider);

    //! Destroys the adapter and deregisters it from its adaptee.
    ~TimeSeriesAdaptedOutput() override = default;

    // ── IAdaptedOutput ────────────────────────────────────────────────────

    /*!
     * \brief One-time initialization before use. No-op.
     */
    void initialize() override;

    /*!
     * \brief Refresh adapted output values from provider; fires valueChanged.
     * \details Reads nothing: it only fires value-changed for {0}, {1}. It
     *          does not refresh chained adapters.
     */
    void refresh() override;

    /*!
     * \brief IOutput::updateValues — no-op; interpolation happens on read.
     */
    void updateValues() override;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief A single value.
     * \return { 1 }.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override;

    /*!
     * \brief The element type.
     * \return Helpers::DataKindOf of \c T.
     */
    [[nodiscard]] DataKind dataKind() const override;

    /*!
     * \brief Writes the value interpolated at queryTime() into \p destination
     *        (selection must be start {0}, count {1}).
     * \details Fails with "No time series attached" when constructed with a
     *          null series. The value can be NaN (Linear outside the series),
     *          and the read still succeeds.
     * \param[in]  destination Host buffer of kind dataKind(), one element.
     * \param[in]  start       {0}.
     * \param[in]  count       {1}.
     * \param[out] message     Failure description; may be nullptr.
     * \return true when the value was written.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override;

    /*!
     * \brief Always fails — adapted outputs are read-only.
     * \param[in]  source  Ignored.
     * \param[in]  start   Ignored.
     * \param[in]  count   Ignored.
     * \param[out] message Receives "TimeSeriesAdaptedOutput is read-only".
     * \return false.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── Time-series specific ──────────────────────────────────────────────

    /*!
     * \brief Get interpolated scalar value at a specific time.
     * \param time Query time (as Julian Day).
     * \return TimeSeries::getValue() at \p time, converted to \c T.
     */
    [[nodiscard]] T getInterpolatedValue(double time) const
    {
        return static_cast<T>(m_timeSeries->getValue(time));
    }

    /*!
     * \brief Set the query time for interpolation.
     * \details Fires nothing; the next read uses it.
     * \param[in] julianDay Query instant as a Julian day.
     */
    void setQueryTime(double julianDay) noexcept { m_queryTime = julianDay; }

    /*!
     * \brief Get the current query time.
     * \return The query instant as a Julian day; 0 initially.
     */
    [[nodiscard]] double queryTime() const noexcept { return m_queryTime; }

    /*!
     * \brief Access the underlying TimeSeries.
     * \return The shared series (may be nullptr).
     */
    [[nodiscard]] std::shared_ptr<TimeSeries> timeSeries() const noexcept
    {
        return m_timeSeries;
    }

private:
    std::shared_ptr<TimeSeries> m_timeSeries; //!< Series the values come from (shared).
    double m_queryTime; //!< Julian Day for interpolation queries.
};

using TimeSeriesAdaptedOutputDouble = TimeSeriesAdaptedOutput<double>; //!< Interpolating adapter over a \c double series.
using TimeSeriesAdaptedOutputInt    = TimeSeriesAdaptedOutput<int>;    //!< Interpolating adapter returning \c int.

extern template class HYDROCOUPLESDK_EXPORT TimeSeriesAdaptedOutput<double>;
extern template class HYDROCOUPLESDK_EXPORT TimeSeriesAdaptedOutput<int>;

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESADAPTEDOUTPUT_H
