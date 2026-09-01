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
    : AbstractArgument(id, caption, {timeDim}, nullptr, modelComponent),
      m_timeSeries(std::make_shared<TimeSeries>(std::string(id))),
      m_timeDim(timeDim)
{
}

// ── v2 typed hyperslab data plane ─────────────────────────────────────────

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
    return true;
}

} // namespace HydroCouple::SDK::Temporal
