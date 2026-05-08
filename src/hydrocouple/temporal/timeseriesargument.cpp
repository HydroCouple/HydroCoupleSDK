// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/temporal/timeseriesargument.h"
#include "hydrocouple/dimension.h"

#include <fstream>
#include <sstream>

namespace HydroCouple::Temporal {

TimeSeriesArgumentDouble::TimeSeriesArgumentDouble(
    std::string_view id,
    std::string_view caption,
    HydroCouple::IModelComponent *modelComponent,
    HydroCouple::Dimension *timeDim)
    : AbstractArgument(id, caption, {timeDim}, nullptr, modelComponent),
      m_timeSeries(std::make_shared<TimeSeries>(std::string(id))),
      m_timeDim(timeDim)
{
}

bool TimeSeriesArgumentDouble::loadFromCSV(const std::string &filePath) {
    return m_timeSeries->loadFromCSV(filePath, true);
}

double TimeSeriesArgumentDouble::getInterpolatedValue(double julianDay) const {
    if (!m_timeSeries) return 0.0;
    return m_timeSeries->getValue(julianDay);
}

void TimeSeriesArgumentDouble::getValue(
    HydroCouple::hydrocouple_variant &data,
    const std::initializer_list<int> &dimensionIndexes) const
{
    if (!m_timeSeries || dimensionIndexes.size() == 0) return;

    // Return the value at the requested time index
    int timeIndex = *dimensionIndexes.begin();
    if (timeIndex >= 0 && timeIndex < (int)m_timeSeries->count()) {
        data = m_timeSeries->values()[timeIndex];
    }
}

void TimeSeriesArgumentDouble::getValues(
    HydroCouple::hydrocouple_variant *data,
    const std::initializer_list<int> &dimensionIndexes,
    const std::initializer_list<int> &dimensionLengths) const
{
    if (!m_timeSeries || !data) return;

    // Return a range of values
    if (dimensionIndexes.size() == 0 && dimensionLengths.size() > 0) {
        int count = std::min(*dimensionLengths.begin(), (int)m_timeSeries->count());
        for (int i = 0; i < count; ++i) {
            data[i] = m_timeSeries->values()[i];
        }
    }
}

int TimeSeriesArgumentDouble::dimensionLength(
    const std::initializer_list<int> &dims) const
{
    if (!m_timeSeries) return 0;
    return m_timeSeries->count();
}

} // namespace HydroCouple::Temporal
