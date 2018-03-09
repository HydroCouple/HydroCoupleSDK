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
#include "core/abstractmodelcomponent.h"
#include "temporal/timeseriesidbasedcomponentdataitem.h"
#include "temporal/timedata.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Temporal;
using namespace SDKTemporal;

template<class T>
TimeSeriesIdBasedComponentDataItem<T>::TimeSeriesIdBasedComponentDataItem(const QString &id,
                                                                          const QStringList& identifiers,
                                                                          const QList<SDKTemporal::DateTime *> &times,
                                                                          const T& defaultValue)
  : ComponentDataItem2D<T>(id, identifiers.length(),times.length(),defaultValue),
    m_identifiers(identifiers),
    m_times(times)
{
  qSort(m_times.begin() , m_times.end() , &DateTime::compare);
  double duration = times[0]->modifiedJulianDay() - times[times.length() -1]->modifiedJulianDay();
  m_timeSpan = new TimeSpan(times[0]->dateTime(), duration, nullptr);
}

template<class T>
TimeSeriesIdBasedComponentDataItem<T>::~TimeSeriesIdBasedComponentDataItem()
{
  qDeleteAll(m_times);
  m_times.clear();

  delete m_timeSpan;
}

template<class T>
bool TimeSeriesIdBasedComponentDataItem<T>::addIdentifier(const QString& identifier)
{
  if(!m_identifiers.contains(identifier))
  {
    m_identifiers.append(identifier);
    ComponentDataItem2D<T>::resizeDataArrayILength(m_identifiers.length());
    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
bool TimeSeriesIdBasedComponentDataItem<T>::addIdentifiers(const QList<QString>& identifiers)
{
  bool added = false;

  for(const QString &id : identifiers)
  {
    if(!m_identifiers.contains(id))
    {
      m_identifiers.append(id);
      added = true;
    }
  }

  if(added)
  {
    ComponentDataItem2D<T>::resizeDataArrayILength(m_identifiers.length());
    return true;
  }

  return false;
}

template<class T>
bool TimeSeriesIdBasedComponentDataItem<T>::removeIdentifier(const QString& identifier)
{
  int index = m_identifiers.indexOf(identifier);

  if(index >= 0)
  {
    m_identifiers.removeAt(index);
    ComponentDataItem2D<T>::removeIItemAt(index);
    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
bool TimeSeriesIdBasedComponentDataItem<T>::addTime(SDKTemporal::DateTime* time)
{
  if(m_times.length() && time->dateTime() > m_times[m_times.length()]->dateTime())
  {
    m_times.append(time);
    resetTimeSpan();
    ComponentDataItem2D<T>::resizeDataArrayJLength(m_times.length());
    return true;
  }


  return false;
}

template<class T>
bool TimeSeriesIdBasedComponentDataItem<T>::addTimes(const QList<DateTime *> &times)
{
  if(times.length() > 1)
  {
    for(int i = 1 ; i < times.length() ; i++)
    {
      if(times[i]->dateTime() < times[i-1]->dateTime())
      {
        return false;
      }
    }


    if(m_times.length())
    {
      if(times[0]->dateTime() < m_times[m_times.length() -1]->dateTime())
      {
        return false;
      }
    }

    for(SDKTemporal::DateTime* ntime : times)
    {
      m_times.append(ntime);
    }

    resetTimeSpan();

    ComponentDataItem2D<T>::resizeDataArrayJLength(m_times.length());
    return true;
  }

  return false;
}

template<class T>
bool TimeSeriesIdBasedComponentDataItem<T>::removeTime(DateTime *time)
{
  int index = m_times.indexOf(time);

  if(index >= 0)
  {
    m_times.removeAt(index);
    resetTimeSpan();
    ComponentDataItem2D<T>::removeJItemAt(index);
    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::resetTimeSpan()
{
  if(m_times.length())
  {
    double duration = m_times[0]->modifiedJulianDay() - m_times[m_times.length() -1]->modifiedJulianDay();
    m_timeSpan->setModifiedJulianDay(m_times[0]->modifiedJulianDay());
    m_timeSpan->setDuration(duration);
  }
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::getValueT(const std::vector<int> &dimensionIndexes, void* data) const
{
  ComponentDataItem2D<T>::getValueT(dimensionIndexes, data);
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::getValueT(int idIndex, int timeIndex, void* data) const
{
  ComponentDataItem2D<T>::getValueT(idIndex,timeIndex, data);
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::getValuesT(int idIndex, int timeIndex, int idStride, int timeStride, void* data) const
{
  ComponentDataItem2D<T>::getValuesT(idIndex,timeIndex,idStride, timeStride, data);
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::setValueT(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem2D<T>::setValueT(dimensionIndexes, data);
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::setValueT(int idIndex, int timeIndex, const void *data)
{
  ComponentDataItem2D<T>::setValueT(idIndex, timeIndex, data);
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::setValuesT(int idIndex, int timeIndex, int idStride, int timeStride, const void *data)
{
  ComponentDataItem2D<T>::setValuesT(idIndex,timeIndex,idStride, timeStride, data);
}

template<class T>
QStringList TimeSeriesIdBasedComponentDataItem<T>::identifiersInternal() const
{
  return m_identifiers;
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::clearIdentifiers()
{
  m_identifiers.clear();
}

template<class T>
QList<DateTime *> TimeSeriesIdBasedComponentDataItem<T>::timesInternal() const
{
  return m_times;
}

template<class T>
DateTime *TimeSeriesIdBasedComponentDataItem<T>::timeInternal(int timeIndex) const
{
  return m_times[timeIndex];
}

template<class T>
int TimeSeriesIdBasedComponentDataItem<T>::timeCountInternal() const
{
  return m_times.length();
}

template<class T>
TimeSpan* TimeSeriesIdBasedComponentDataItem<T>::timeSpanInternal() const
{
  return m_timeSpan;
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::clearTimes()
{
  qDeleteAll(m_times);
  m_times.clear();
  ComponentDataItem2D<T>::resizeDataArray(0,0);
}

template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<QString>;
