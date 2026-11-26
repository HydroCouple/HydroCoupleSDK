// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timeseriesargument.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Time-series argument for model configuration.
 */

#ifndef HYDROCOUPLESDK_TIMESERIESARGUMENT_H
#define HYDROCOUPLESDK_TIMESERIESARGUMENT_H

#include "hydrocouplesdk/data/abstractargument.h"
#include "timedata.h"
#include "timeseries.h"

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "hydrocoupletemporal.h"

namespace HydroCouple::SDK::Temporal {

// Interface types from the standard's Temporal namespace.
using namespace HydroCouple::Temporal;

/*!
 * \brief Time-series argument holding a scalar double TimeSeries.
 *
 * Loads time-series data from CSV or JSON and provides interpolated values
 * at query times. The v2 data plane exposes the stored values as a rank-1
 * Float64 item of shape { count }; times are available via timeSeries().
 * The item is read-only through the data plane — values are loaded via
 * initialize().
 *
 * \details A forcing read from configuration: a boundary inflow, a rainfall
 * record. It answers ITimeSeriesComponentDataItem, so a host or configurator
 * can see that its axis is time. Input forms:
 * | Input type | \p value holds |
 * | --- | --- |
 * | String, JSON (and the other text types) | `{"times": [...], "values": [...]}`, optionally with "timeKind", "intervalLength", "timeInterpolation", "timeExtrapolation" as integers |
 * | File, URL ending in `.csv` (query string ignored) | CSV text with a header line and `time,value` rows, times as Julian days |
 * | File, URL otherwise | the JSON object above |
 *
 * The declared policies default to TimeInterpolation::Linear and
 * TimeExtrapolation::HoldLast, but getInterpolatedValue() uses the
 * TimeSeries' own method (Linear) and returns NaN outside the record: the
 * HoldLast declaration does not describe what the argument does, and setting
 * the policies does not change getInterpolatedValue().
 *
 * isValidArgType() comes from AbstractArgument and the constructor adds no
 * file filter, so it reports File and URL as unsupported although
 * initialize() reads both.
 *
 * \threadsafety Not thread-safe on its own.
 */
class HYDROCOUPLESDK_EXPORT TimeSeriesArgumentDouble
    : public HydroCouple::SDK::AbstractArgument,
      public virtual HydroCouple::Temporal::ITimeSeriesComponentDataItem
{
public:
    /*!
     * \brief Construct a time-series argument.
     * \param id               Identifier.
     * \param caption          Human-readable caption.
     * \param modelComponent   Owning component.
     * \param timeDim          Dimension for the time axis.
     * \details The argument has no value definition (nullptr) and an empty
     *          series. \p timeDim is borrowed; give it the id "time".
     */
    TimeSeriesArgumentDouble(std::string_view id,
                             std::string_view caption,
                             HydroCouple::IModelComponent *modelComponent,
                             HydroCouple::SDK::Dimension *timeDim);
    //! Construct with a borrowed physical value definition (must outlive item).
    //! \param id              Argument identifier.
    //! \param caption         Human-readable caption.
    //! \param modelComponent  Owning component (borrowed; may be nullptr).
    //! \param timeDim         Time axis (borrowed).
    //! \param valueDefinition Units of the values (borrowed; may be nullptr).
    TimeSeriesArgumentDouble(std::string_view id, std::string_view caption,
                             HydroCouple::IModelComponent *modelComponent,
                             HydroCouple::SDK::Dimension *timeDim,
                             HydroCouple::SDK::ValueDefinition *valueDefinition);

    //! Destroys the argument and its series.
    ~TimeSeriesArgumentDouble() override = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The data-plane shape: one value per time.
     * \return { count() of the series }. Rank 1, not { times, 1 }, so the
     *         SDK factory does not offer temporal_interpolation on it.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override;

    /*!
     * \brief The element type.
     * \return Always DataKind::Float64.
     */
    [[nodiscard]] DataKind dataKind() const override;

    /*!
     * \brief Copies a hyperslab of the stored values (not the times).
     * \param[in]  destination Host Float64 buffer.
     * \param[in]  start       First time index (one entry).
     * \param[in]  count       Number of values (one entry).
     * \param[out] message     Failure description; may be nullptr.
     * \return true when the values were copied.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override;

    /*!
     * \brief Always fails — the argument is read-only through the data
     *        plane; load values via initialize().
     * \param[in]  source  Ignored.
     * \param[in]  start   Ignored.
     * \param[in]  count   Ignored.
     * \param[out] message Receives "TimeSeriesArgumentDouble is read-only;
     *                     load values via initialize()"; may be nullptr.
     * \return false.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── ITimeSeriesComponentDataItem ──────────────────────────────────────
    //
    // An IArgument *is* an IComponentDataItem, so a time-series argument
    // can say so directly rather than leaving a consumer to infer it from
    // a rank-1 shape. A configurator reading this gets a time series; one
    // reading only the shape gets a column of numbers and has to guess
    // that the first axis is time.
    //
    // Nothing is stored twice for it. The Julian days live in the
    // TimeSeries already and are handed out as a span over that vector;
    // the IDateTime and ITimeSpan objects the interface requires are
    // built on demand and validated against those same values, so no
    // loader has to remember to keep a parallel cache in step. See
    // time() in the implementation for why that validation is exact
    // rather than approximate.

    /*!
     * \brief The time at \p timeIndex, or nullptr if out of range.
     * \details Built on demand and cached; a cached object is reused only
     *          while its Julian day still equals the series' time at that
     *          index, so a reload never hands out stale dates.
     * \param[in] timeIndex Zero-based time index.
     * \return A pointer owned by the argument, valid until the series is
     *         reloaded or the argument is destroyed.
     */
    [[nodiscard]] const HydroCouple::Temporal::IDateTime *time(
        int64_t timeIndex) const override;

    /*!
     * \brief The number of times in the series.
     * \return The series count.
     */
    [[nodiscard]] int64_t timeCount() const override;

    /*!
     * \brief The Julian days, as a span over the series' own storage.
     * \return A span valid until the series is reloaded.
     */
    [[nodiscard]] std::span<const double> times() const override;

    /*!
     * \brief The span the series covers, or a zero span when it is empty.
     * \return A span from the first to the last time (start 0 and duration 0
     *         when empty), owned by the argument and rebuilt when the series
     *         changes.
     */
    [[nodiscard]] HydroCouple::Temporal::ITimeSpan *timeSpan() const override;

    /*!
     * \brief The dimension the times are indexed by.
     * \return The time dimension given at construction (borrowed).
     */
    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override;

    // ── Temporal semantics ────────────────────────────────────────────────
    // Persisted with the series ("timeKind", "intervalLength",
    // "timeInterpolation", "timeExtrapolation" keys in the JSON form).

    //! What each value's time coordinate refers to.
    //! \return The declared kind; Unknown by default.
    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override;
    //! Declared interval length in days.
    //! \return 0 by default.
    [[nodiscard]] double intervalLength() const override;
    //! Declared interpolation policy.
    //! \return Linear by default.
    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override;
    //! Declared extrapolation policy.
    //! \return HoldLast by default (see the class notes: the argument does not hold).
    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const override;
    //! Declares the time kind and interval length (metadata, persisted).
    //! \param[in] kind               What the time coordinates refer to.
    //! \param[in] intervalLengthDays Interval length in days, negative for
    //!                               contiguous intervals, 0 when not declared.
    void setTimeKind(HydroCouple::Temporal::TimeKind kind, double intervalLengthDays = 0.0);
    //! Declares the interpolation policy (metadata, persisted).
    //! \param[in] policy The policy to report.
    void setTimeInterpolation(HydroCouple::Temporal::TimeInterpolation policy);
    //! Declares the extrapolation policy (metadata, persisted).
    //! \param[in] policy The policy to report.
    void setTimeExtrapolation(HydroCouple::Temporal::TimeExtrapolation policy);

    // ── IArgument ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns {"times": [...], "values": [...]} as a JSON string.
     * \return Compact JSON without the temporal declarations (writeData()
     *         carries those).
     */
    [[nodiscard]] std::string toString() const override;

    /*!
     * \brief Writes the series back to the input file (CSV for .csv paths,
     *        JSON otherwise), if one was set via File initialisation.
     * \details The CSV branch uses TimeSeries::saveToCSV(), which writes six
     *          significant digits and so destroys Julian-day times; the JSON
     *          branch writes toString() at full precision.
     */
    void saveData() override;

    /*!
     * \brief Parses \p value according to \p argType.
     *
     * String/JSON: a JSON object with "times" and "values" arrays.
     * File/URL: a .csv file (time,value rows) or a JSON file as above.
     *
     * \details Failure messages: those of IO::readReference(); `Failed to
     *          load CSV file: <reference>`; "Expected JSON object with
     *          "times" and "values" arrays"; "times/values must have equal
     *          length"; "TimeSeries: times must be strictly increasing"; and
     *          the JSON parser's. A load that fails part way leaves the
     *          series holding the samples read before the failure. A CSV
     *          load keeps the previous temporal declarations.
     * \param[in]  value   JSON text, or a path or URI for File and URL.
     * \param[in]  argType Representation of \p value.
     * \param[out] message Failure description.
     * \return true when the series was read.
     */
    [[nodiscard]] bool initialize(const std::string &value,
                                  HydroCouple::IArgument::ArgumentInputType argType,
                                  std::string &message) override;

    /*!
     * \brief Copies times and values from another time-series data item.
     * \details The source must be an ITimeSeriesComponentDataItem ("Source
     *          data item is not a time-series data item") of kind Float64
     *          ("Source data kind must be Float64") with shape { n } or
     *          { n, 1 } ("Source must be a scalar time series"). Its times()
     *          and values replace the series; the temporal declarations are
     *          not copied.
     * \warning A source of rank 3 or more passes the shape check and its
     *          whole extent is copied into a buffer sized for the first
     *          dimension only. Pass rank 1 or { n, 1 } sources.
     * \param[in]  componentDataItem Source item.
     * \param[out] message           Failure description; cleared on success.
     * \return true when the series was copied.
     */
    [[nodiscard]] bool initialize(const HydroCouple::IComponentDataItem &componentDataItem,
                                  std::string &message) override;

    // ── JSON persistence ──────────────────────────────────────────────────

    /*!
     * \brief Reads the base fields, then the series and declarations.
     * \details A document without "times" and "values" leaves the series as
     *          it was. Non-increasing times throw std::invalid_argument from
     *          TimeSeries::addValue() (not caught here).
     * \param[in] j JSON object as written by writeData().
     */
    void readData(const nlohmann::json &j) override;
    /*!
     * \brief Writes the base fields, "times", "values" and the four
     *        temporal declarations (as integers of their enumerations).
     * \return The JSON object.
     */
    [[nodiscard]] nlohmann::json writeData() const override;

    // ── Time-series access ────────────────────────────────────────────────

    /*!
     * \brief Load time-series from CSV file (time, value columns).
     * \details TimeSeries::loadFromCSV() with a header line; does not
     *          remember the file for saveData() and does not change
     *          currentArgumentInputType().
     * \param[in] filePath Local path of the CSV file.
     * \return true when every row was read.
     */
    bool loadFromCSV(const std::string &filePath);

    /*!
     * \brief Get interpolated value at query time.
     * \details Linear interpolation (the series' method); NaN before the
     *          first and after the last time.
     * \param[in] julianDay Query instant as a Julian day.
     * \return The interpolated value.
     */
    [[nodiscard]] double getInterpolatedValue(double julianDay) const;

    /*!
     * \brief Access the underlying TimeSeries.
     * \return The shared series; changing it changes the argument.
     */
    [[nodiscard]] std::shared_ptr<TimeSeries> timeSeries() const noexcept
    {
        return m_timeSeries;
    }

private:
    /*!
     * \brief Replaces the series and declarations from a JSON object.
     * \param[in]  j       Object with "times" and "values".
     * \param[out] message Why \p j was refused.
     * \return false when the keys are missing or the arrays differ in length.
     * \exception std::invalid_argument when the times are not increasing.
     */
    bool assignFromJson(const nlohmann::json &j, std::string &message);

    std::shared_ptr<TimeSeries> m_timeSeries; //!< The stored series.
    HydroCouple::SDK::Dimension     *m_timeDim; //!< Time axis (borrowed).
    //! Declared meaning of the time coordinates.
    HydroCouple::Temporal::TimeKind m_timeKind = HydroCouple::Temporal::TimeKind::Unknown;
    double m_intervalLength = 0.0; //!< Declared interval length in days.
    //! Declared interpolation policy.
    HydroCouple::Temporal::TimeInterpolation m_timeInterpolation = HydroCouple::Temporal::TimeInterpolation::Linear;
    //! Declared extrapolation policy.
    HydroCouple::Temporal::TimeExtrapolation m_timeExtrapolation = HydroCouple::Temporal::TimeExtrapolation::HoldLast;
    std::filesystem::path       m_inputFile; //!< Local file saveData() writes to, or empty.

    //! IDateTime objects the interface must hand out, built on demand.
    //! Mutable because the interface's accessors are const and the cache
    //! is an implementation detail of answering them, not state anyone
    //! outside can observe.
    mutable std::vector<std::shared_ptr<TimeData>> m_dateTimes;
    mutable std::shared_ptr<TimeSpan>              m_timeSpan; //!< Span handed out by timeSpan(), rebuilt when the series changes.
};

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESARGUMENT_H
