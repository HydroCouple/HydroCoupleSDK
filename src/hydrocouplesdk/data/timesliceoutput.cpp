// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/timesliceoutput.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocoupletemporal.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace HydroCouple::SDK {

struct TimeSliceOutput::Impl
{
    HydroCouple::IComponentDataItem *source = nullptr;
    int64_t                          timeIndex = 0;

    std::vector<std::unique_ptr<Dimension>> dimensions;
    std::unique_ptr<ValueDefinition>        valueDefinition;

    /*!
     * Time levels available in the source (dimension 0).
     */
    [[nodiscard]] int64_t timeCount() const
    {
        const std::vector<int64_t> shape = source->shape();
        return shape.empty() ? 0 : shape.front();
    }
};

TimeSliceOutput::Prepared TimeSliceOutput::prepare(
    std::string_view id, HydroCouple::IComponentDataItem *source)
{
    // Dimensions of the slice: the source's, minus the leading time one.
    Prepared prepared;
    const std::vector<HydroCouple::IDimension *> sourceDimensions =
        source ? source->dimensions()
               : std::vector<HydroCouple::IDimension *>{};
    for (size_t d = 1; d < sourceDimensions.size(); ++d)
    {
        const std::string name = sourceDimensions[d]
                                     ? sourceDimensions[d]->id()
                                     : ("dim" + std::to_string(d));
        prepared.ownedDimensions.push_back(
            std::make_unique<Dimension>(name, name));
    }
    if (prepared.ownedDimensions.empty())
        prepared.ownedDimensions.push_back(
            std::make_unique<Dimension>("element", "Element"));

    for (const std::unique_ptr<Dimension> &dimension :
         prepared.ownedDimensions)
        prepared.dimensions.push_back(dimension.get());

    prepared.valueDefinition.reset(
        Quantity::unitLess(std::string(id) + "_slice"));
    return prepared;
}

TimeSliceOutput::TimeSliceOutput(std::string_view id,
                                 HydroCouple::IComponentDataItem *source,
                                 HydroCouple::IModelComponent *component)
    : TimeSliceOutput(id, source, component, prepare(id, source))
{
}

TimeSliceOutput::TimeSliceOutput(std::string_view id,
                                 HydroCouple::IComponentDataItem *source,
                                 HydroCouple::IModelComponent *component,
                                 Prepared prepared)
    : AbstractOutput(id, prepared.dimensions, prepared.valueDefinition.get(),
                     component),
      m_impl(std::make_unique<Impl>())
{
    m_impl->source          = source;
    m_impl->dimensions      = std::move(prepared.ownedDimensions);
    m_impl->valueDefinition = std::move(prepared.valueDefinition);
}

TimeSliceOutput::~TimeSliceOutput() = default;

int64_t TimeSliceOutput::timeIndex() const noexcept
{
    return m_impl->timeIndex;
}

void TimeSliceOutput::setTimeIndex(int64_t index)
{
    const int64_t levels = m_impl->timeCount();
    m_impl->timeIndex = std::clamp<int64_t>(index, 0,
                                            levels > 0 ? levels - 1 : 0);
}

bool TimeSliceOutput::advance()
{
    if (m_impl->timeIndex + 1 >= m_impl->timeCount())
        return false;
    ++m_impl->timeIndex;
    return true;
}

int64_t TimeSliceOutput::timeCount() const noexcept
{
    return m_impl->timeCount();
}

bool TimeSliceOutput::setTime(double time)
{
    auto *series =
        dynamic_cast<HydroCouple::Temporal::ITimeSeriesComponentDataItem *>(
            m_impl->source);
    if (!series)
        return false;

    const std::span<const double> times = series->times();
    if (times.empty())
        return false;

    int64_t nearest  = 0;
    double  distance = std::abs(times[0] - time);
    for (size_t i = 1; i < times.size(); ++i)
    {
        const double candidate = std::abs(times[i] - time);
        if (candidate < distance)
        {
            distance = candidate;
            nearest  = static_cast<int64_t>(i);
        }
    }
    m_impl->timeIndex = nearest;
    return true;
}

// ── v2 typed hyperslab data plane ─────────────────────────────────────────

std::vector<int64_t> TimeSliceOutput::shape() const
{
    std::vector<int64_t> shape = m_impl->source->shape();
    if (!shape.empty())
        shape.erase(shape.begin());
    return shape;
}

DataKind TimeSliceOutput::dataKind() const
{
    return m_impl->source->dataKind();
}

bool TimeSliceOutput::getValuesInto(const BufferDescriptor &destination,
                                    std::span<const int64_t> start,
                                    std::span<const int64_t> count,
                                    std::string *message) const
{
    // Re-express the slice request against the source: the current time
    // level, then the caller's selection over the remaining dimensions.
    std::vector<int64_t> sourceStart;
    std::vector<int64_t> sourceCount;
    sourceStart.reserve(start.size() + 1);
    sourceCount.reserve(count.size() + 1);
    sourceStart.push_back(m_impl->timeIndex);
    sourceCount.push_back(1);
    sourceStart.insert(sourceStart.end(), start.begin(), start.end());
    sourceCount.insert(sourceCount.end(), count.begin(), count.end());

    return m_impl->source->getValuesInto(destination, sourceStart, sourceCount,
                                         message);
}

bool TimeSliceOutput::setValuesFrom(const BufferDescriptor &,
                                    std::span<const int64_t>,
                                    std::span<const int64_t>,
                                    std::string *message)
{
    if (message)
        *message = "item \"" + id() +
                   "\" is a time slice of another item and is read-only";
    return false;
}

void TimeSliceOutput::updateValues() {}

} // namespace HydroCouple::SDK
