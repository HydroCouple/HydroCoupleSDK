// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/temporal/temporalinterpolationadaptedoutput.h"
#include "hydrocouplesdk/data/bufferio.h"

#include "hydrocoupletemporal.h"

namespace HydroCouple::SDK::Temporal {

TemporalInterpolationAdaptedOutput::TemporalInterpolationAdaptedOutput(
    std::string_view id, HydroCouple::IOutput *adaptee,
    HydroCouple::IAdaptedOutputFactory *factory)
    : AbstractAdaptedOutput(id, {}, nullptr, adaptee, factory),
      m_method("method", &m_argDimension, 1, m_methodQuality.get(), nullptr),
      m_series(id)
{
    m_method[0] = "linear";
    addArgument(&m_method);
    m_sampleDimension.setRole(HydroCouple::IDimension::DimensionRole::Time);
    m_fieldDimension.setRole(HydroCouple::IDimension::DimensionRole::Component);
    m_record = std::make_unique<SampleRecord>(
        std::string(id) + ".record", m_series,
        std::vector<Dimension *>{&m_sampleDimension, &m_fieldDimension});
}

std::vector<HydroCouple::IComponentDataItem *>
TemporalInterpolationAdaptedOutput::states() const
{
    return {m_record.get()};
}

// ── The recorded samples ────────────────────────────────────────────────────

TemporalInterpolationAdaptedOutput::SampleRecord::SampleRecord(
    std::string_view id, const TimeSeries &series,
    const std::vector<Dimension *> &dimensions)
    : AbstractComponentDataItem(id, dimensions, nullptr, nullptr), m_series(series)
{
}

std::vector<int64_t> TemporalInterpolationAdaptedOutput::SampleRecord::shape() const
{
    return {static_cast<int64_t>(m_series.count()), 2};
}

DataKind TemporalInterpolationAdaptedOutput::SampleRecord::dataKind() const
{
    return DataKind::Float64;
}

bool TemporalInterpolationAdaptedOutput::SampleRecord::getValuesInto(
    const BufferDescriptor &destination, std::span<const int64_t> start,
    std::span<const int64_t> count, std::string *message) const
{
    const std::vector<double> &times = m_series.times();
    const std::vector<double> &values = m_series.values();
    std::vector<double> rows(times.size() * 2);
    for (size_t i = 0; i < times.size(); ++i)
    {
        rows[2 * i] = times[i];
        rows[2 * i + 1] = values[i];
    }
    const std::vector<int64_t> extent = shape();
    return detail::readHyperslab<double>(rows.data(), extent, destination, start,
                                         count, message);
}

bool TemporalInterpolationAdaptedOutput::SampleRecord::setValuesFrom(
    const BufferDescriptor &, std::span<const int64_t>, std::span<const int64_t>,
    std::string *message)
{
    if (message)
        *message = "the sample record of a TemporalInterpolationAdaptedOutput is read-only";
    return false;
}

void TemporalInterpolationAdaptedOutput::initialize()
{
    const std::string &method = m_method[0];
    if (method == "nearest")
        m_series.setInterpolationMethod(TimeSeries::InterpolationMethod::Nearest);
    else if (method == "previous")
        m_series.setInterpolationMethod(TimeSeries::InterpolationMethod::Constant);
    else
        m_series.setInterpolationMethod(TimeSeries::InterpolationMethod::Linear);

    m_series.clear();

    auto *timeSeries =
        dynamic_cast<HydroCouple::Temporal::ITimeSeriesComponentDataItem *>(
            adaptee());
    if (!timeSeries)
        return;

    for (int64_t index = 0; index < timeSeries->timeCount(); ++index)
        appendSample(index);
}

void TemporalInterpolationAdaptedOutput::refresh()
{
    auto *timeSeries =
        dynamic_cast<HydroCouple::Temporal::ITimeSeriesComponentDataItem *>(
            adaptee());
    if (!timeSeries || timeSeries->timeCount() < 1)
        return;

    const int64_t last = timeSeries->timeCount() - 1;
    const std::span<const double> times = timeSeries->times();

    // Same-time dedupe: the second refresh of one step (a host that both
    // pushes and sweeps) must not corrupt the record — and TimeSeries
    // demands strictly increasing times anyway.
    if (m_series.count() > 0 &&
        times[static_cast<size_t>(last)] <= m_series.times().back())
        return;

    appendSample(last);
    emitValueChanged({0}, {1});
    refreshAdaptedOutputs();
}

void TemporalInterpolationAdaptedOutput::updateValues()
{
    refresh();
}

std::vector<int64_t> TemporalInterpolationAdaptedOutput::shape() const
{
    return {1};
}

DataKind TemporalInterpolationAdaptedOutput::dataKind() const
{
    return DataKind::Float64;
}

bool TemporalInterpolationAdaptedOutput::getValuesInto(
    const BufferDescriptor &destination, std::span<const int64_t> start,
    std::span<const int64_t> count, std::string *message) const
{
    if (m_series.count() == 0)
    {
        if (message)
            *message = "TemporalInterpolationAdaptedOutput \"" + id() +
                       "\" holds no samples; initialize() has not run or "
                       "the adaptee has no time steps";
        return false;
    }

    const double value = m_series.getValue(m_queryTime);
    const int64_t one = 1;
    return detail::readHyperslab<double>(
        &value, std::span<const int64_t>(&one, 1), destination, start, count,
        message);
}

bool TemporalInterpolationAdaptedOutput::setValuesFrom(
    const BufferDescriptor &, std::span<const int64_t>,
    std::span<const int64_t>, std::string *message)
{
    if (message)
        *message = "TemporalInterpolationAdaptedOutput is read-only";
    return false;
}

void TemporalInterpolationAdaptedOutput::appendSample(int64_t index)
{
    auto *timeSeries =
        dynamic_cast<HydroCouple::Temporal::ITimeSeriesComponentDataItem *>(
            adaptee());
    HydroCouple::IOutput *provider = adaptee();
    if (!timeSeries || !provider)
        return;

    double value = 0.0;
    const int64_t rowShape[2] = {1, 1};
    HydroCouple::BufferDescriptor destination;
    destination.data  = &value;
    destination.kind  = DataKind::Float64;
    destination.rank  = 2;
    destination.shape = rowShape;

    const int64_t start[2] = {index, 0};
    const int64_t count[2] = {1, 1};
    if (!provider->getValuesInto(destination,
                                 std::span<const int64_t>(start, 2),
                                 std::span<const int64_t>(count, 2), nullptr))
        return;

    m_series.addValue(timeSeries->times()[static_cast<size_t>(index)], value);
}

} // namespace HydroCouple::SDK::Temporal
