// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   temporalinterpolationadaptedoutput.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Adapted output interpolating a time-series adaptee at a query time.
 */

#ifndef HYDROCOUPLESDK_TEMPORALINTERPOLATIONADAPTEDOUTPUT_H
#define HYDROCOUPLESDK_TEMPORALINTERPOLATIONADAPTEDOUTPUT_H

#include "hydrocouplesdk/data/abstractadaptedoutput.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/temporal/timeseries.h"

#include <memory>

namespace HydroCouple::SDK::Temporal {

/*!
 * \brief Serves a single-column time-series adaptee's value interpolated at
 *        an arbitrary query time.
 *
 * Unlike TimeSeriesAdaptedOutput — factory-less, constructed around a
 * TimeSeries handed in from outside, unreachable from a composition spec —
 * this adapter is constructible from the provider alone: initialize() seeds
 * an internal series from the adaptee's whole history, and refresh()
 * appends the adaptee's latest sample. A refresh at an already-recorded
 * time is skipped (the dedupe is what makes a host that refreshes twice per
 * step harmless). getValuesInto() serves the value interpolated at
 * queryTime() as a rank-1 {1} Double.
 *
 * Argument \c method: "linear" (default), "nearest", or "previous"
 * (hold-from-left); an unrecognized value falls back to linear —
 * initialize() has no error channel.
 *
 * Stateful: what it serves depends on every sample it has recorded, so
 * states() lists that record as a read-only [samples, 2] Float64 item of
 * (time, value) rows. The record grows by a row per refresh, so the adapter
 * is neither differentiable nor checkpointable (its state changes shape); an
 * orchestrator that must return a composition to an earlier step refuses a
 * connection through it rather than replay it with the wrong record.
 *
 * \details Argument (an Argument1DString of length 1, value set from the
 * Quality "Interpolation method" with categories linear, nearest, previous):
 * | Id | Default | Values |
 * | --- | --- | --- |
 * | `method` | `"linear"` | `"linear"`, `"nearest"`, `"previous"`; read once, by initialize() |
 *
 * What a read returns at query time \f$t\f$ is TimeSeries::getValue() with
 * the matching method (previous is TimeSeries::InterpolationMethod::Constant):
 * linear gives NaN before the first and after the last sample; nearest and
 * previous hold the last sample after the end; previous at exactly a sample
 * time after the first returns the sample before it. A NaN is served as a
 * successful read.
 *
 * The query time is the adapter's own setting: nothing in the SDK sets it
 * from the consumer's times() or the workflow clock, so the host (or a
 * consuming component) calls setQueryTime() before each read. It starts at
 * 0. The adapter has no dimensions and no value definition.
 *
 * Samples are read from the adaptee as element (index, 0) of a
 * { times, 1 } Float64 item; an adaptee with times out of order makes
 * initialize() or refresh() throw std::invalid_argument from
 * TimeSeries::addValue().
 */
class HYDROCOUPLESDK_EXPORT TemporalInterpolationAdaptedOutput
    : public AbstractAdaptedOutput
{
public:
    /*!
     * \brief Builds the adapter with method "linear", no samples and query time 0.
     * \param id       Instance identifier.
     * \param adaptee  Single-column Double ITimeSeriesComponentDataItem
     *                 output (borrowed).
     * \param factory  Creating factory; registration with the adaptee is
     *                 the factory's postcondition.
     */
    TemporalInterpolationAdaptedOutput(
        std::string_view id, HydroCouple::IOutput *adaptee,
        HydroCouple::IAdaptedOutputFactory *factory);

    //! Destroys the adapter and deregisters it from its adaptee.
    ~TemporalInterpolationAdaptedOutput() override = default;

    // ── IAdaptedOutput ────────────────────────────────────────────────────

    //! Latches the method and seeds the series from the adaptee's history.
    //! \details Clears the record first, so calling it again restarts from
    //! the adaptee's current history. An unrecognized method means linear.
    //! An adaptee that is not a time series leaves the record empty. Fires
    //! no signal and does not refresh chained adapters.
    void initialize() override;

    //! Appends the adaptee's latest (time, value); same-time refreshes skip.
    //! \details Skips when the adaptee has no time step or its last time is
    //! not after the last recorded one. After an append it fires
    //! value-changed and refreshes the chained adapters. Only the last
    //! adaptee sample is taken: steps the adaptee added between two
    //! refreshes, other than the last, are not recorded.
    void refresh() override;

    //! Delegates to refresh().
    void updateValues() override;

    //! The recorded samples: the state refresh() appends to.
    //! \return One read-only item of shape { samples, 2 } holding (time, value) rows.
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> states() const override;

    // ── v2 typed hyperslab data plane ─────────────────────────────────────

    /*!
     * \brief One value: the interpolated value at queryTime().
     * \return { 1 }.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override;
    /*!
     * \brief The element type.
     * \return Always DataKind::Float64.
     */
    [[nodiscard]] DataKind dataKind() const override;

    //! Serves the value interpolated at queryTime(); fails while empty.
    //! \details The failure message is `TemporalInterpolationAdaptedOutput "<id>"
    //! holds no samples; initialize() has not run or the adaptee has no time steps`.
    //! \param[in]  destination Host Float64 buffer of one element.
    //! \param[in]  start       {0}.
    //! \param[in]  count       {1}.
    //! \param[out] message     Failure description; may be nullptr.
    //! \return true when the value (possibly NaN) was written.
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override;

    //! Always fails — adapted outputs are read-only.
    //! \param[in]  source  Ignored.
    //! \param[in]  start   Ignored.
    //! \param[in]  count   Ignored.
    //! \param[out] message Receives "TemporalInterpolationAdaptedOutput is read-only".
    //! \return false.
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── Query time ────────────────────────────────────────────────────────

    //! Sets the instant the next read interpolates at. Fires nothing.
    //! \param[in] julianDay Query instant, in the adaptee's time units (Julian days).
    void setQueryTime(double julianDay) noexcept { m_queryTime = julianDay; }
    //! The instant reads interpolate at.
    //! \return The query time; 0 initially.
    [[nodiscard]] double queryTime() const noexcept { return m_queryTime; }

    //! Samples accumulated so far (initialize seed + refreshes).
    //! \return The record's row count.
    [[nodiscard]] int sampleCount() const noexcept { return m_series.count(); }

private:
    //! The recorded samples as a read-only [count, 2] item of (time, value).
    class SampleRecord final : public AbstractComponentDataItem
    {
    public:
        /*!
         * \brief A view of \p series; it copies nothing.
         * \param[in] id         Item id ("<adapter id>.record").
         * \param[in] series     The adapter's series (borrowed).
         * \param[in] dimensions "sample" (role Time) and "field" (role Component).
         */
        SampleRecord(std::string_view id, const TimeSeries &series,
                     const std::vector<Dimension *> &dimensions);
        //! Rows by two columns.
        //! \return { samples, 2 }.
        [[nodiscard]] std::vector<int64_t> shape() const override;
        //! The element type.
        //! \return Always DataKind::Float64.
        [[nodiscard]] DataKind dataKind() const override;
        //! Copies a hyperslab of the (time, value) rows; builds the rows on every call.
        //! \param[in]  destination Host Float64 buffer.
        //! \param[in]  start       First row and column.
        //! \param[in]  count       Number of rows and columns.
        //! \param[out] message     Failure description; may be nullptr.
        //! \return true when the values were copied.
        [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                         std::span<const int64_t> start,
                                         std::span<const int64_t> count,
                                         std::string *message = nullptr) const override;
        //! Always fails: the record is read-only.
        //! \param[in]  source  Ignored.
        //! \param[in]  start   Ignored.
        //! \param[in]  count   Ignored.
        //! \param[out] message Receives the refusal.
        //! \return false.
        [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                         std::span<const int64_t> start,
                                         std::span<const int64_t> count,
                                         std::string *message = nullptr) override;

    private:
        const TimeSeries &m_series; //!< The adapter's series (borrowed).
    };

    //! Copies sample \p index of the adaptee into the internal series.
    //! \details A failed read is skipped silently.
    //! \param[in] index Zero-based adaptee time step.
    //! \exception std::invalid_argument when its time is not after the last recorded one.
    void appendSample(int64_t index);

    Dimension m_argDimension{"scalar"}; //!< Length axis of the method argument.
    //! Categories of the method argument.
    std::unique_ptr<Quality> m_methodQuality{
        new Quality("Interpolation method",
                    {"linear", "nearest", "previous"})};

    Argument1DString m_method; //!< Argument "method".

    TimeSeries m_series;             //!< Recorded (time, value) samples.
    double m_queryTime = 0.0;        //!< Instant reads interpolate at.
    Dimension m_sampleDimension{"sample"}; //!< Record dimension 0 (role Time).
    Dimension m_fieldDimension{"field"};   //!< Record dimension 1 (role Component).
    std::unique_ptr<SampleRecord> m_record; //!< The state item states() returns.
};

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TEMPORALINTERPOLATIONADAPTEDOUTPUT_H
