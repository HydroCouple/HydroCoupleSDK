/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#include "stdafx.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "temporal/timedata.h"
#include "temporal/timeseriesexchangeitem.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Temporal;

TimeSeriesOutputDouble::TimeSeriesOutputDouble(const QString& id,
                                               const std::list<SDKTemporal::DateTime *> &times,
                                               Dimension* timeDimension,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent* modelComponent)
  : AbstractOutput(id, QList<Dimension*>({timeDimension}), valueDefinition, modelComponent),
    TimeSeriesComponentDataItem<double>(id, times, valueDefinition->defaultValue().toDouble()),
    m_timeDimension(timeDimension)
{
}

TimeSeriesOutputDouble::~TimeSeriesOutputDouble()
{
}

QList<IDateTime*> TimeSeriesOutputDouble::times() const
{
  QList<IDateTime*> otimes;
  std::vector<SDKTemporal::DateTime*> itimes = TimeSeriesComponentDataItem<double>::timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeSeriesOutputDouble::time(int timeIndex) const
{
  return timeInternal(timeIndex);
}

ITimeSpan* TimeSeriesOutputDouble::timeSpan() const
{
  return TimeSeriesComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesOutputDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesOutputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void TimeSeriesOutputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesOutputDouble::getValue(int timeIndex, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(timeIndex,data);
}

void TimeSeriesOutputDouble::getValues(int timeIndex, int stride, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesOutputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesOutputDouble::setValue(int timeIndex, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValueT(timeIndex,data);
}

void TimeSeriesOutputDouble::setValues(int timeIndex, int stride, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex, stride , data);
}

//==============================================================================================================================

TimeSeriesInputDouble::TimeSeriesInputDouble(const QString& id,
                                               const std::list<SDKTemporal::DateTime*>& times,
                                               Dimension* timeDimension,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent* modelComponent)
  : AbstractInput(id, QList<Dimension*>({timeDimension}), valueDefinition, modelComponent),
    TimeSeriesComponentDataItem<double>(id, times, valueDefinition->defaultValue().toDouble()),
    m_timeDimension(timeDimension)
{
}

TimeSeriesInputDouble::~TimeSeriesInputDouble()
{
}

QList<IDateTime*> TimeSeriesInputDouble::times() const
{
  QList<IDateTime*> otimes;
  std::vector<SDKTemporal::DateTime*> itimes = TimeSeriesComponentDataItem<double>::timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeSeriesInputDouble::time(int timeIndex) const
{
  return timeInternal(timeIndex);
}

ITimeSpan* TimeSeriesInputDouble::timeSpan() const
{
  return TimeSeriesComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesInputDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesInputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void TimeSeriesInputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesInputDouble::getValue(int timeIndex, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(timeIndex,data);
}

void TimeSeriesInputDouble::getValues(int timeIndex, int stride, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesInputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesInputDouble::setValue(int timeIndex, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValueT(timeIndex,data);
}

void TimeSeriesInputDouble::setValues(int timeIndex, int stride, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex, stride , data);
}

//==============================================================================================================================

TimeSeriesMultiInputDouble::TimeSeriesMultiInputDouble(const QString& id,
                                               const std::list<SDKTemporal::DateTime *> &times,
                                               Dimension* timeDimension,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent* modelComponent)
  : AbstractMultiInput(id, QList<Dimension*>({timeDimension}), valueDefinition, modelComponent),
    TimeSeriesComponentDataItem<double>(id, times, valueDefinition->defaultValue().toDouble())
{
}

TimeSeriesMultiInputDouble::~TimeSeriesMultiInputDouble()
{
}

QList<IDateTime*> TimeSeriesMultiInputDouble::times() const
{
  QList<IDateTime*> otimes;
  std::vector<SDKTemporal::DateTime*> itimes = TimeSeriesComponentDataItem<double>::timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeSeriesMultiInputDouble::time(int timeIndex) const
{
  return timeInternal(timeIndex);
}

int TimeSeriesMultiInputDouble::timeCount() const
{
  return timeCountInternal();
}

ITimeSpan* TimeSeriesMultiInputDouble::timeSpan() const
{
  return TimeSeriesComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesMultiInputDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesMultiInputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void TimeSeriesMultiInputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesMultiInputDouble::getValue(int timeIndex, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(timeIndex,data);
}

void TimeSeriesMultiInputDouble::getValues(int timeIndex, int stride, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesMultiInputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesMultiInputDouble::setValue(int timeIndex, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValueT(timeIndex,data);
}

void TimeSeriesMultiInputDouble::setValues(int timeIndex, int stride, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex, stride , data);
}

template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<QString>;
