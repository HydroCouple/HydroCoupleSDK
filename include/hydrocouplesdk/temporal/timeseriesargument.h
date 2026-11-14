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
     */
    TimeSeriesArgumentDouble(std::string_view id,
                             std::string_view caption,
                             HydroCouple::IModelComponent *modelComponent,
                             HydroCouple::SDK::Dimension *timeDim);

    ~TimeSeriesArgumentDouble() override = default;

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override;

    [[nodiscard]] DataKind dataKind() const override;

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override;

    /*!
     * \brief Always fails — the argument is read-only through the data
     *        plane; load values via initialize().
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
     */
    [[nodiscard]] const HydroCouple::Temporal::IDateTime *time(
        int64_t timeIndex) const override;

    /*!
     * \brief The number of times in the series.
     */
    [[nodiscard]] int64_t timeCount() const override;

    /*!
     * \brief The Julian days, as a span over the series' own storage.
     */
    [[nodiscard]] std::span<const double> times() const override;

    /*!
     * \brief The span the series covers, or a zero span when it is empty.
     */
    [[nodiscard]] HydroCouple::Temporal::ITimeSpan *timeSpan() const override;

    /*!
     * \brief The dimension the times are indexed by.
     */
    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override;

    // ── Temporal semantics ────────────────────────────────────────────────
    // Persisted with the series ("timeKind", "intervalLength",
    // "timeInterpolation", "timeExtrapolation" keys in the JSON form).

    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override;
    [[nodiscard]] double intervalLength() const override;
    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override;
    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const override;
    void setTimeKind(HydroCouple::Temporal::TimeKind kind, double intervalLengthDays = 0.0);
    void setTimeInterpolation(HydroCouple::Temporal::TimeInterpolation policy);
    void setTimeExtrapolation(HydroCouple::Temporal::TimeExtrapolation policy);

    // ── IArgument ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns {"times": [...], "values": [...]} as a JSON string.
     */
    [[nodiscard]] std::string toString() const override;

    /*!
     * \brief Writes the series back to the input file (CSV for .csv paths,
     *        JSON otherwise), if one was set via File initialisation.
     */
    void saveData() override;

    /*!
     * \brief Parses \p value according to \p argType.
     *
     * String/JSON: a JSON object with "times" and "values" arrays.
     * File/URL: a .csv file (time,value rows) or a JSON file as above.
     */
    [[nodiscard]] bool initialize(const std::string &value,
                                  HydroCouple::IArgument::ArgumentInputType argType,
                                  std::string &message) override;

    /*!
     * \brief Copies times and values from another time-series data item.
     */
    [[nodiscard]] bool initialize(const HydroCouple::IComponentDataItem &componentDataItem,
                                  std::string &message) override;

    // ── JSON persistence ──────────────────────────────────────────────────

    void readData(const nlohmann::json &j) override;
    [[nodiscard]] nlohmann::json writeData() const override;

    // ── Time-series access ────────────────────────────────────────────────

    /*!
     * \brief Load time-series from CSV file (time, value columns).
     */
    bool loadFromCSV(const std::string &filePath);

    /*!
     * \brief Get interpolated value at query time.
     */
    [[nodiscard]] double getInterpolatedValue(double julianDay) const;

    /*!
     * \brief Access the underlying TimeSeries.
     */
    [[nodiscard]] std::shared_ptr<TimeSeries> timeSeries() const noexcept
    {
        return m_timeSeries;
    }

private:
    bool assignFromJson(const nlohmann::json &j, std::string &message);

    std::shared_ptr<TimeSeries> m_timeSeries;
    HydroCouple::SDK::Dimension     *m_timeDim;
    HydroCouple::Temporal::TimeKind m_timeKind = HydroCouple::Temporal::TimeKind::Unknown;
    double m_intervalLength = 0.0;
    HydroCouple::Temporal::TimeInterpolation m_timeInterpolation = HydroCouple::Temporal::TimeInterpolation::Linear;
    HydroCouple::Temporal::TimeExtrapolation m_timeExtrapolation = HydroCouple::Temporal::TimeExtrapolation::HoldLast;
    std::filesystem::path       m_inputFile;

    //! IDateTime objects the interface must hand out, built on demand.
    //! Mutable because the interface's accessors are const and the cache
    //! is an implementation detail of answering them, not state anyone
    //! outside can observe.
    mutable std::vector<std::shared_ptr<TimeData>> m_dateTimes;
    mutable std::shared_ptr<TimeSpan>              m_timeSpan;
};

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESARGUMENT_H
