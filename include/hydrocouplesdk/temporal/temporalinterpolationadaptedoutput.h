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
 */
class HYDROCOUPLESDK_EXPORT TemporalInterpolationAdaptedOutput
    : public AbstractAdaptedOutput
{
public:
    /*!
     * \param id       Instance identifier.
     * \param adaptee  Single-column Double ITimeSeriesComponentDataItem
     *                 output (borrowed).
     * \param factory  Creating factory; registration with the adaptee is
     *                 the factory's postcondition.
     */
    TemporalInterpolationAdaptedOutput(
        std::string_view id, HydroCouple::IOutput *adaptee,
        HydroCouple::IAdaptedOutputFactory *factory);

    ~TemporalInterpolationAdaptedOutput() override = default;

    // ── IAdaptedOutput ────────────────────────────────────────────────────

    //! Latches the method and seeds the series from the adaptee's history.
    void initialize() override;

    //! Appends the adaptee's latest (time, value); same-time refreshes skip.
    void refresh() override;

    //! Delegates to refresh().
    void updateValues() override;

    //! The recorded samples: the state refresh() appends to.
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> states() const override;

    // ── v2 typed hyperslab data plane ─────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override;
    [[nodiscard]] DataKind dataKind() const override;

    //! Serves the value interpolated at queryTime(); fails while empty.
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override;

    //! Always fails — adapted outputs are read-only.
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── Query time ────────────────────────────────────────────────────────

    void setQueryTime(double julianDay) noexcept { m_queryTime = julianDay; }
    [[nodiscard]] double queryTime() const noexcept { return m_queryTime; }

    //! Samples accumulated so far (initialize seed + refreshes).
    [[nodiscard]] int sampleCount() const noexcept { return m_series.count(); }

private:
    //! The recorded samples as a read-only [count, 2] item of (time, value).
    class SampleRecord final : public AbstractComponentDataItem
    {
    public:
        SampleRecord(std::string_view id, const TimeSeries &series,
                     const std::vector<Dimension *> &dimensions);
        [[nodiscard]] std::vector<int64_t> shape() const override;
        [[nodiscard]] DataKind dataKind() const override;
        [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                         std::span<const int64_t> start,
                                         std::span<const int64_t> count,
                                         std::string *message = nullptr) const override;
        [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                         std::span<const int64_t> start,
                                         std::span<const int64_t> count,
                                         std::string *message = nullptr) override;

    private:
        const TimeSeries &m_series;
    };

    //! Copies sample \p index of the adaptee into the internal series.
    void appendSample(int64_t index);

    Dimension m_argDimension{"scalar"};
    std::unique_ptr<Quality> m_methodQuality{
        new Quality("Interpolation method",
                    {"linear", "nearest", "previous"})};

    Argument1DString m_method;

    TimeSeries m_series;
    double m_queryTime = 0.0;
    Dimension m_sampleDimension{"sample"};
    Dimension m_fieldDimension{"field"};
    std::unique_ptr<SampleRecord> m_record;
};

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TEMPORALINTERPOLATIONADAPTEDOUTPUT_H
