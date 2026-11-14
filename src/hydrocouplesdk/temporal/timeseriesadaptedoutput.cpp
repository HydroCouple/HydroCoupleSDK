// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/temporal/timeseriesadaptedoutput.h"
#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/core/dimension.h"

#include "hydrocoupletemporal.h"

namespace HydroCouple::SDK::Temporal {

// Interface types from the standard's Temporal namespace.
using namespace HydroCouple::Temporal;

template<typename T>
TimeSeriesAdaptedOutput<T>::TimeSeriesAdaptedOutput(
    std::string_view id,
    std::shared_ptr<TimeSeries> timeSeries,
    HydroCouple::IOutput *provider)
    : AbstractAdaptedOutput(id, {}, nullptr, provider, nullptr),
      m_timeSeries(std::move(timeSeries)),
      m_queryTime(0.0)
{
}

template<typename T>
void TimeSeriesAdaptedOutput<T>::initialize()
{
}

template<typename T>
void TimeSeriesAdaptedOutput<T>::refresh()
{
    emitValueChanged({0}, {1});
}

template<typename T>
void TimeSeriesAdaptedOutput<T>::updateValues()
{
}

template<typename T>
std::vector<int64_t> TimeSeriesAdaptedOutput<T>::shape() const
{
    return {1};
}

template<typename T>
DataKind TimeSeriesAdaptedOutput<T>::dataKind() const
{
    return HydroCouple::Helpers::DataKindOf<T>::kind;
}

template<typename T>
bool TimeSeriesAdaptedOutput<T>::getValuesInto(const BufferDescriptor &destination,
                                               std::span<const int64_t> start,
                                               std::span<const int64_t> count,
                                               std::string *message) const
{
    if (!m_timeSeries)
    {
        if (message) *message = "No time series attached";
        return false;
    }
    const T value = static_cast<T>(m_timeSeries->getValue(m_queryTime));
    const int64_t one = 1;
    return HydroCouple::SDK::detail::readHyperslab<T>(
        &value, std::span<const int64_t>(&one, 1), destination, start, count,
        message);
}

template<typename T>
bool TimeSeriesAdaptedOutput<T>::setValuesFrom(const BufferDescriptor &,
                                               std::span<const int64_t>,
                                               std::span<const int64_t>,
                                               std::string *message)
{
    if (message)
        *message = "TimeSeriesAdaptedOutput is read-only";
    return false;
}

// Explicit template instantiations
template class HYDROCOUPLESDK_EXPORT TimeSeriesAdaptedOutput<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesAdaptedOutput<double>;

} // namespace HydroCouple::SDK::Temporal
