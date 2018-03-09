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
#include "temporal/timeseriesidbasedexchangeitem.h"
#include "temporal/timedata.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Temporal;

TimeSeriesIdBasedOutputDouble::TimeSeriesIdBasedOutputDouble(const QString& id,
                                                             const QStringList& identifiers,
                                                             Dimension* identifierDimension,
                                                             const QList<SDKTemporal::DateTime *> &times,
                                                             Dimension* timeDimension,
                                                             ValueDefinition* valueDefinition,
                                                             AbstractModelComponent* modelComponent)
  : AbstractOutput(id,QList<Dimension*>({identifierDimension, timeDimension}),valueDefinition, modelComponent),
    TimeSeriesIdBasedComponentDataItem<double>(id, identifiers, times,valueDefinition->defaultValue().toDouble()),
    m_identifierDimension(identifierDimension),
    m_timeDimension(timeDimension)
{

}

TimeSeriesIdBasedOutputDouble::~TimeSeriesIdBasedOutputDouble()
{
}

QStringList TimeSeriesIdBasedOutputDouble::identifiers() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::identifiersInternal();
}

IDimension* TimeSeriesIdBasedOutputDouble::identifierDimension() const
{
  return m_identifierDimension;
}

QList<IDateTime*> TimeSeriesIdBasedOutputDouble::times() const
{
  QList<IDateTime*> otimes;
  QList<SDKTemporal::DateTime*> itimes = TimeSeriesIdBasedComponentDataItem<double>::timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeSeriesIdBasedOutputDouble::time(int timeIndex) const
{
  return timeInternal(timeIndex);
}

int TimeSeriesIdBasedOutputDouble::timeCount() const
{
  return timeCountInternal();
}

ITimeSpan* TimeSeriesIdBasedOutputDouble::timeSpan() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesIdBasedOutputDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesIdBasedOutputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert((int)dimensionIndexes.size() < dimensions().length());

  if(dimensionIndexes.size() == 0)
  {
    return iLength();
  }
  else
  {
    return jLength();
  }
}

void TimeSeriesIdBasedOutputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem2D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedOutputDouble::getValue(int timeIndex, int idIndex,  void *data) const
{
  ComponentDataItem2D<double>::getValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedOutputDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride,  void *data) const
{
  ComponentDataItem2D<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedOutputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem2D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedOutputDouble::setValue(int timeIndex, int idIndex, const void *data)
{
  ComponentDataItem2D<double>::setValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedOutputDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data)
{
  ComponentDataItem2D<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

//==============================================================================================================================

TimeSeriesIdBasedInputDouble::TimeSeriesIdBasedInputDouble(const QString& id,
                                                           const QStringList& identifiers,
                                                           Dimension* identifierDimension,
                                                           const QList<SDKTemporal::DateTime*> &times,
                                                           Dimension* timeDimension,
                                                           ValueDefinition* valueDefinition,
                                                           AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({identifierDimension, timeDimension}),valueDefinition, modelComponent),
    TimeSeriesIdBasedComponentDataItem<double>(id, identifiers, times,valueDefinition->defaultValue().toDouble()),
    m_identifierDimension(identifierDimension),
    m_timeDimension(timeDimension)
{

}

TimeSeriesIdBasedInputDouble::~TimeSeriesIdBasedInputDouble()
{
}


QStringList TimeSeriesIdBasedInputDouble::identifiers() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::identifiersInternal();
}

IDimension* TimeSeriesIdBasedInputDouble::identifierDimension() const
{
  return m_identifierDimension;
}

QList<IDateTime *> TimeSeriesIdBasedInputDouble::times() const
{
  QList<IDateTime*> otimes;
  QList<SDKTemporal::DateTime*> itimes = TimeSeriesIdBasedComponentDataItem<double>::timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeSeriesIdBasedInputDouble::time(int timeIndex) const
{
  return timeInternal(timeIndex);
}

int TimeSeriesIdBasedInputDouble::timeCount() const
{
  return timeCountInternal();
}

ITimeSpan* TimeSeriesIdBasedInputDouble::timeSpan() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesIdBasedInputDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesIdBasedInputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert((int)dimensionIndexes.size() < dimensions().length());

  if((int)dimensionIndexes.size() == 0)
  {
    return iLength();
  }
  else
  {
    return jLength();
  }
}

void TimeSeriesIdBasedInputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem2D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedInputDouble::getValue(int timeIndex, int idIndex,  void *data) const
{
  ComponentDataItem2D<double>::getValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedInputDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride,  void *data) const
{
  ComponentDataItem2D<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedInputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem2D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedInputDouble::setValue(int timeIndex, int idIndex, const void *data)
{
  ComponentDataItem2D<double>::setValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedInputDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data)
{
  ComponentDataItem2D<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

//==============================================================================================================================

TimeSeriesIdBasedMultiInputDouble::TimeSeriesIdBasedMultiInputDouble(const QString& id,
                                                                     const QStringList& identifiers,
                                                                     Dimension* identifierDimension,
                                                                     const QList<SDKTemporal::DateTime*>& times,
                                                                     Dimension* timeDimension,
                                                                     ValueDefinition* valueDefinition,
                                                                     AbstractModelComponent* modelComponent)
  : AbstractMultiInput(id,QList<Dimension*>({identifierDimension, timeDimension}),valueDefinition, modelComponent),
    TimeSeriesIdBasedComponentDataItem<double>(id, identifiers, times,valueDefinition->defaultValue().toDouble()),
    m_identifierDimension(identifierDimension),
    m_timeDimension(timeDimension)
{

}

TimeSeriesIdBasedMultiInputDouble::~TimeSeriesIdBasedMultiInputDouble()
{
}

QStringList TimeSeriesIdBasedMultiInputDouble::identifiers() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::identifiersInternal();
}

IDimension* TimeSeriesIdBasedMultiInputDouble::identifierDimension() const
{
  return m_identifierDimension;
}

QList<IDateTime*> TimeSeriesIdBasedMultiInputDouble::times() const
{
  QList<IDateTime*> otimes;
  QList<SDKTemporal::DateTime*> itimes = TimeSeriesIdBasedComponentDataItem<double>::timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeSeriesIdBasedMultiInputDouble::time(int timeIndex) const
{
  return timeInternal(timeIndex);
}

int TimeSeriesIdBasedMultiInputDouble::timeCount() const
{
  return timeCountInternal();
}

ITimeSpan* TimeSeriesIdBasedMultiInputDouble::timeSpan() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesIdBasedMultiInputDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesIdBasedMultiInputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert((int)dimensionIndexes.size() < dimensions().length());

  if((int)dimensionIndexes.size() == 0)
  {
    return iLength();
  }
  else
  {
    return jLength();
  }
}

void TimeSeriesIdBasedMultiInputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem2D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedMultiInputDouble::getValue(int timeIndex, int idIndex,  void *data) const
{
  ComponentDataItem2D<double>::getValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedMultiInputDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride,  void *data) const
{
  ComponentDataItem2D<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedMultiInputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem2D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedMultiInputDouble::setValue(int timeIndex, int idIndex, const void *data)
{
  ComponentDataItem2D<double>::setValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedMultiInputDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data)
{
  ComponentDataItem2D<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<QString>;
