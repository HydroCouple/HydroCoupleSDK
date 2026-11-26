// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/temporal/timeseriesargument.h"
#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/core/dimension.h"

#include <fstream>
#include <sstream>

#include "hydrocoupletemporal.h"
#include "hydrocouplesdk/io/uriresolver.h"

namespace HydroCouple::SDK::Temporal {

// Interface types from the standard's Temporal namespace.
using namespace HydroCouple::Temporal;

TimeSeriesArgumentDouble::TimeSeriesArgumentDouble(
    std::string_view id,
    std::string_view caption,
    HydroCouple::IModelComponent *modelComponent,
    HydroCouple::SDK::Dimension *timeDim)
    : TimeSeriesArgumentDouble(id, caption, modelComponent, timeDim, nullptr)
{
}

TimeSeriesArgumentDouble::TimeSeriesArgumentDouble(
    std::string_view id, std::string_view caption,
    HydroCouple::IModelComponent *modelComponent,
    HydroCouple::SDK::Dimension *timeDim,
    HydroCouple::SDK::ValueDefinition *valueDefinition)
    : AbstractArgument(id, caption, {timeDim}, valueDefinition, modelComponent),
      m_timeSeries(std::make_shared<TimeSeries>(std::string(id))),
      m_timeDim(timeDim)
{
}

// ── v2 typed hyperslab data plane ─────────────────────────────────────────

// ── ITimeSeriesComponentDataItem ─────────────────────────────────────────

const IDateTime *TimeSeriesArgumentDouble::time(int64_t timeIndex) const
{
    const std::vector<double> &julian = m_timeSeries->times();

    if (timeIndex < 0 ||
        static_cast<size_t>(timeIndex) >= julian.size())
    {
        return nullptr;
    }

    // The cache is validated per slot against the series' own value, and
    // that validation is exact rather than a heuristic: an IDateTime here
    // *is* its Julian day, so a slot whose day still matches is still the
    // right answer no matter what else was reloaded around it. A size
    // change drops the whole cache.
    //
    // Doing it this way rather than rebuilding the cache wherever the
    // series is loaded is deliberate. There are four such places today —
    // CSV text, CSV file, JSON, and copying from another data item — and
    // a fifth would silently hand out stale dates. This cannot go stale
    // because it never trusts itself.
    if (m_dateTimes.size() != julian.size())
    {
        m_dateTimes.assign(julian.size(), nullptr);
    }

    const size_t index = static_cast<size_t>(timeIndex);
    const double julianDay = julian[index];

    std::shared_ptr<TimeData> &slot = m_dateTimes[index];

    if (!slot || slot->julianDay() != julianDay)
    {
        slot = std::make_shared<TimeData>(
            id() + "_t" + std::to_string(timeIndex), julianDay);
    }

    return slot.get();
}

int64_t TimeSeriesArgumentDouble::timeCount() const
{
    return static_cast<int64_t>(m_timeSeries->times().size());
}

std::span<const double> TimeSeriesArgumentDouble::times() const
{
    // Straight over the series' storage: the Julian days are already
    // contiguous there, and a second copy would be one more thing that
    // could disagree with the first.
    const std::vector<double> &julian = m_timeSeries->times();

    return {julian.data(), julian.size()};
}

ITimeSpan *TimeSeriesArgumentDouble::timeSpan() const
{
    const std::vector<double> &julian = m_timeSeries->times();

    // A zero span rather than nullptr for an empty series: a caller asking
    // what a series covers before it is loaded should get "nothing", not a
    // pointer it has to test.
    const double start = julian.empty() ? 0.0 : julian.front();
    const double duration =
        julian.empty() ? 0.0 : julian.back() - julian.front();

    // Validated the same way as the dates, and exactly for the same
    // reason: a TimeSpan is its start and its duration, so one that
    // matches both is the right answer however it came to be cached.
    if (!m_timeSpan || m_timeSpan->startTime() != start ||
        m_timeSpan->duration() != duration)
    {
        m_timeSpan = std::make_shared<TimeSpan>(id() + "_span", start,
                                                duration);
    }

    return m_timeSpan.get();
}

IDimension *TimeSeriesArgumentDouble::timeDimension() const
{
    return m_timeDim;
}

HydroCouple::Temporal::TimeKind TimeSeriesArgumentDouble::timeKind() const
{
    return m_timeKind;
}

double TimeSeriesArgumentDouble::intervalLength() const
{
    return m_intervalLength;
}

HydroCouple::Temporal::TimeInterpolation TimeSeriesArgumentDouble::timeInterpolation() const
{
    // The declared defaults are Linear and HoldLast. getInterpolatedValue()
    // interpolates linearly but returns NaN outside the record, so the
    // HoldLast default does not describe what the argument does.
    return m_timeInterpolation;
}

HydroCouple::Temporal::TimeExtrapolation TimeSeriesArgumentDouble::timeExtrapolation() const
{
    return m_timeExtrapolation;
}

void TimeSeriesArgumentDouble::setTimeKind(HydroCouple::Temporal::TimeKind kind,
                                           double intervalLengthDays)
{
    m_timeKind = kind;
    m_intervalLength = intervalLengthDays;
}

void TimeSeriesArgumentDouble::setTimeInterpolation(HydroCouple::Temporal::TimeInterpolation policy)
{
    m_timeInterpolation = policy;
}

void TimeSeriesArgumentDouble::setTimeExtrapolation(HydroCouple::Temporal::TimeExtrapolation policy)
{
    m_timeExtrapolation = policy;
}

std::vector<int64_t> TimeSeriesArgumentDouble::shape() const
{
    return {static_cast<int64_t>(m_timeSeries ? m_timeSeries->count() : 0)};
}

DataKind TimeSeriesArgumentDouble::dataKind() const
{
    return DataKind::Float64;
}

bool TimeSeriesArgumentDouble::getValuesInto(const BufferDescriptor &destination,
                                             std::span<const int64_t> start,
                                             std::span<const int64_t> count,
                                             std::string *message) const
{
    const int64_t n = m_timeSeries ? m_timeSeries->count() : 0;
    return HydroCouple::SDK::detail::readHyperslab<double>(
        m_timeSeries ? m_timeSeries->values().data() : nullptr,
        std::span<const int64_t>(&n, 1), destination, start, count, message);
}

bool TimeSeriesArgumentDouble::setValuesFrom(const BufferDescriptor &,
                                             std::span<const int64_t>,
                                             std::span<const int64_t>,
                                             std::string *message)
{
    if (message)
        *message = "TimeSeriesArgumentDouble is read-only; load values via initialize()";
    return false;
}

// ── IArgument ─────────────────────────────────────────────────────────────

std::string TimeSeriesArgumentDouble::toString() const
{
    nlohmann::json j;
    j["times"]  = m_timeSeries->times();
    j["values"] = m_timeSeries->values();
    return j.dump();
}

void TimeSeriesArgumentDouble::saveData()
{
    if (m_inputFile.empty())
        return;
    if (m_inputFile.extension() == ".csv")
    {
        m_timeSeries->saveToCSV(m_inputFile.string());
    }
    else
    {
        std::ofstream f(m_inputFile);
        if (f)
            f << toString();
    }
}

namespace
{
    /*!
     * \brief The extension of what a reference names, query string aside.
     *
     * `std::filesystem::path("https://host/flow.csv?token=x").extension()`
     * is ".csv?token=x", so a served series would be parsed as JSON.
     *
     * \param[in] reference A path or URI.
     * \return The extension of the part before the first "?" or "#",
     *         with its dot (".csv"), or empty.
     */
    std::string referenceExtension(const std::string &reference)
    {
        std::string path = reference.substr(0, reference.find_first_of("?#"));

        return std::filesystem::path(path).extension().string();
    }
} // namespace

bool TimeSeriesArgumentDouble::initialize(
    const std::string &value,
    HydroCouple::IArgument::ArgumentInputType argType,
    std::string &message)
{
    using AT = HydroCouple::IArgument::ArgumentInputType;
    try
    {
        if (argType == AT::File || argType == AT::URL)
        {
            std::string text;
            if (!IO::readReference(value, text, message))
                return false;

            // Only something on disk can be written back to; remembering a
            // URL here would make saveData() create a file named after a
            // service. By the same split, only a remote reference is worth
            // recording: a file's values can be inlined, a service's cannot.
            const bool local = IO::isWritableReference(value);
            m_inputFile = local ? std::filesystem::path(value)
                                : std::filesystem::path();
            setSourceUri(local ? std::string() : value);

            if (referenceExtension(value) == ".csv")
            {
                if (!m_timeSeries->loadFromCSVText(text, true))
                {
                    message = "Failed to load CSV file: " + value;
                    return false;
                }
                setCurrentArgumentInputType(argType);
                return true;
            }

            if (!assignFromJson(nlohmann::json::parse(text), message))
                return false;
        }
        else if (std::string uri; IO::jsonUriReference(value, uri))
        {
            // A payload that names a URI is that URI, read now.
            return initialize(uri, AT::URL, message);
        }
        else
        {
            setSourceUri({});

            if (!assignFromJson(nlohmann::json::parse(value), message))
                return false;
        }
        setCurrentArgumentInputType(argType);
        return true;
    }
    catch (const std::exception &e)
    {
        message = e.what();
        return false;
    }
}

bool TimeSeriesArgumentDouble::initialize(
    const HydroCouple::IComponentDataItem &componentDataItem,
    std::string &message)
{
    const auto *series =
        dynamic_cast<const ITimeSeriesComponentDataItem *>(&componentDataItem);
    if (!series)
    {
        message = "Source data item is not a time-series data item";
        return false;
    }
    if (componentDataItem.dataKind() != DataKind::Float64)
    {
        message = "Source data kind must be Float64";
        return false;
    }
    const auto srcShape = componentDataItem.shape();
    if (srcShape.empty() || (srcShape.size() == 2 && srcShape[1] != 1))
    {
        message = "Source must be a scalar time series";
        return false;
    }
    const int64_t n = srcShape[0];
    std::vector<double> values(static_cast<size_t>(n));
    if (n > 0)
    {
        const BufferDescriptor dest = HydroCouple::SDK::detail::hostDescriptor(
            values.data(), DataKind::Float64, srcShape);
        const std::vector<int64_t> zero(srcShape.size(), 0);
        if (!componentDataItem.getValuesInto(dest, zero, srcShape, &message))
            return false;
    }
    const auto srcTimes = series->times();
    m_timeSeries->clear();
    for (int64_t i = 0; i < n; ++i)
        m_timeSeries->addValue(srcTimes[static_cast<size_t>(i)],
                               values[static_cast<size_t>(i)]);
    message.clear();
    return true;
}

// ── JSON persistence ──────────────────────────────────────────────────────

void TimeSeriesArgumentDouble::readData(const nlohmann::json &j)
{
    AbstractArgument::readData(j);
    std::string msg;
    (void)assignFromJson(j, msg);
}

nlohmann::json TimeSeriesArgumentDouble::writeData() const
{
    auto j = AbstractArgument::writeData();
    j["times"]  = m_timeSeries->times();
    j["values"] = m_timeSeries->values();
    j["timeKind"]          = static_cast<int>(m_timeKind);
    j["intervalLength"]    = m_intervalLength;
    j["timeInterpolation"] = static_cast<int>(m_timeInterpolation);
    j["timeExtrapolation"] = static_cast<int>(m_timeExtrapolation);
    return j;
}

// ── Time-series access ────────────────────────────────────────────────────

bool TimeSeriesArgumentDouble::loadFromCSV(const std::string &filePath)
{
    return m_timeSeries->loadFromCSV(filePath, true);
}

double TimeSeriesArgumentDouble::getInterpolatedValue(double julianDay) const
{
    if (!m_timeSeries)
        return 0.0;
    return m_timeSeries->getValue(julianDay);
}

bool TimeSeriesArgumentDouble::assignFromJson(const nlohmann::json &j,
                                              std::string &message)
{
    if (!j.is_object() || !j.contains("times") || !j.contains("values"))
    {
        message = "Expected JSON object with \"times\" and \"values\" arrays";
        return false;
    }
    const auto times  = j["times"].get<std::vector<double>>();
    const auto values = j["values"].get<std::vector<double>>();
    if (times.size() != values.size())
    {
        message = "times/values must have equal length";
        return false;
    }
    m_timeSeries->clear();
    for (size_t i = 0; i < times.size(); ++i)
        m_timeSeries->addValue(times[i], values[i]);
    if (j.contains("timeKind"))
        m_timeKind = static_cast<HydroCouple::Temporal::TimeKind>(j["timeKind"].get<int>());
    if (j.contains("intervalLength"))
        m_intervalLength = j["intervalLength"].get<double>();
    if (j.contains("timeInterpolation"))
        m_timeInterpolation = static_cast<HydroCouple::Temporal::TimeInterpolation>(j["timeInterpolation"].get<int>());
    if (j.contains("timeExtrapolation"))
        m_timeExtrapolation = static_cast<HydroCouple::Temporal::TimeExtrapolation>(j["timeExtrapolation"].get<int>());
    return true;
}

} // namespace HydroCouple::SDK::Temporal
