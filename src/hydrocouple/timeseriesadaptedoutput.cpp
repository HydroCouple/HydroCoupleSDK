// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/timeseriesadaptedoutput.h"
#include "hydrocouple/dimension.h"

namespace HydroCouple::Temporal {

template<typename T>
TimeSeriesAdaptedOutput<T>::TimeSeriesAdaptedOutput(
    std::string_view id,
    std::shared_ptr<TimeSeries> timeSeries,
    HydroCouple::IOutput *provider)
    : AbstractAdaptedOutput(id, {}, nullptr, provider, nullptr),
      m_timeSeries(timeSeries),
      m_queryTime(0.0)
{
}

template<typename T>
void TimeSeriesAdaptedOutput<T>::refresh() {
    // Update the query time from the provider if it's a temporal output
    // For now, just emit that values have changed
    emitValueChanged(std::vector<int>{}, std::vector<int>{});
}

template<typename T>
void TimeSeriesAdaptedOutput<T>::getValue(
    HydroCouple::hydrocouple_variant &data,
    const std::initializer_list<int> &dimensionIndexes) const
{
    if (!m_timeSeries) return;

    // For a scalar time-series, dimensionIndexes should be empty
    if (dimensionIndexes.size() == 0) {
        double value = m_timeSeries->getValue(m_queryTime);
        data = static_cast<T>(value);
    }
}

template<typename T>
void TimeSeriesAdaptedOutput<T>::getValues(
    HydroCouple::hydrocouple_variant *data,
    const std::initializer_list<int> &dimensionIndexes,
    const std::initializer_list<int> &dimensionLengths) const
{
    if (!m_timeSeries || !data) return;

    if (dimensionIndexes.size() == 0 && dimensionLengths.size() == 0) {
        // Single value at query time
        double value = m_timeSeries->getValue(m_queryTime);
        *data = static_cast<T>(value);
    } else if (dimensionIndexes.size() == 0 && dimensionLengths.size() > 0) {
        // Range of times from a time dimension
        // This is more complex and would require knowing the query time range
        // For now, just return the single interpolated value
        double value = m_timeSeries->getValue(m_queryTime);
        *data = static_cast<T>(value);
    }
}

template<typename T>
int TimeSeriesAdaptedOutput<T>::dimensionLength(
    const std::initializer_list<int> &dims) const
{
    if (!m_timeSeries) return 0;
    // For scalar time-series, there's no spatial dimension
    // The time dimension length is the number of time steps
    if (dims.size() == 0) {
        return m_timeSeries->count();
    }
    return 0;
}

// Explicit template instantiations
template class TimeSeriesAdaptedOutput<int>;
template class TimeSeriesAdaptedOutput<double>;

} // namespace HydroCouple::Temporal
