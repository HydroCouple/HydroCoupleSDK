/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
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
#include "temporal/timedata.h"
#include "spatiotemporal/timegeometrycomponentdataitem.h"
#include "spatial/geometry.h"
#include "spatial/envelope.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

template<class T>
TimeGeometryComponentDataItem<T>::TimeGeometryComponentDataItem(const QString &id,
                                                                IGeometry::GeometryType geometryType,
                                                                const T &defaultValue)
  : m_geometryType(geometryType),
    m_defaultValue(defaultValue),
    m_id(id)
{
  m_envelope = new Envelope();
  m_timeCursor = new DataCursor();
  m_timeSpan = new SDKTemporal::TimeSpan(nullptr);
}

template<class T>
TimeGeometryComponentDataItem<T>::~TimeGeometryComponentDataItem()
{
  delete m_envelope;
  delete m_timeCursor;
  delete m_timeSpan;

  for(SDKTemporal::DateTime *time : m_times)
  {
    delete time;
  }

  m_times.clear();
  m_data.clear();

}

template<class T>
QString TimeGeometryComponentDataItem<T>::getId() const
{
  return m_id;
}

template<class T>
bool TimeGeometryComponentDataItem<T>::addTime(SDKTemporal::DateTime *time)
{
  if(m_times.size() && m_times[m_times.size() - 1]->julianDay() >= time->julianDay())
  {
    return false;
  }

  m_times.push_back(time);
  m_data.push_back(std::vector<T>(m_geometries.size() , m_defaultValue));
  resetTimeSpan();

  m_timeCursor->setMin(0);
  m_timeCursor->setMax(m_times.size() - 1);

  return true;
}

template<class T>
bool TimeGeometryComponentDataItem<T>::addTimes(const QList<SDKTemporal::DateTime*>& times)
{
  if(m_times.size())
  {
    std::vector<SDKTemporal::DateTime*> timesAdded ;
    timesAdded.push_back(m_times[m_times.size() - 1]);


    for(SDKTemporal::DateTime* ntime : times)
    {
      if(timesAdded[timesAdded.size() - 1]->julianDay() >= ntime->julianDay())
      {
        return false;
      }
      else
      {
        timesAdded.push_back(ntime);
      }
    }
  }

  for(SDKTemporal::DateTime* ntime : times)
  {
    m_times.push_back(ntime);
  }

  for(int i = 0 ; i < times.length() ; i++)
  {
    m_data.push_back(std::vector<T>(m_geometries.size() , m_defaultValue));
  }

  resetTimeSpan();

  m_timeCursor->setMin(0);
  m_timeCursor->setMax(m_times.size() - 1);

  return true;
}

template<class T>
bool TimeGeometryComponentDataItem<T>::removeTime(SDKTemporal::DateTime* time)
{
  int  index = std::find(m_times.begin() ,m_times.end() ,time) - m_times.begin();

  if(index > -1)
  {
    m_times.erase(m_times.begin() + index);
    m_data.erase(m_data.begin() + index);
    resetTimeSpan();
    m_timeCursor->setMin(0);
    m_timeCursor->setMax(m_times.size() - 1);

    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
void TimeGeometryComponentDataItem<T>::resetTimeSpan()
{
  if(m_times.size())
  {
    double duration = m_times[m_times.size() -1]->julianDay() - m_times[0]->julianDay();
    m_timeSpan->setJulianDay(m_times[0]->julianDay());
    m_timeSpan->setDuration(duration);
  }
  else
  {
    m_timeSpan->setDuration(0.0);
  }
}

template<class T>
void TimeGeometryComponentDataItem<T>::addGeometry(const QSharedPointer<HCGeometry> &geometry)
{
  m_geometries.push_back(geometry);

  for(size_t i = 0; i < m_data.size(); i++)
  {
    m_data[i].push_back(m_defaultValue);
  }

  resetExtents();
}

template<class T>
void TimeGeometryComponentDataItem<T>::addGeometries(const QList<QSharedPointer<HCGeometry>> & geometries)
{
  for(QSharedPointer<HCGeometry> geometry : geometries)
  {
    assert(geometry->geometryType() == m_geometryType);
    m_geometries.push_back(geometry);
  }

  for(size_t i = 0; i < m_data.size() ; i++)
  {
    m_data[i].resize(m_geometries.size(), m_defaultValue);
  }

  resetExtents();
}

template<class T>
bool TimeGeometryComponentDataItem<T>::removeGeometry(const QSharedPointer<HCGeometry> &geometry)
{
  int index = std::find(m_geometries.begin(), m_geometries.end(), geometry) - m_geometries.begin();

  if(index >= 0)
  {
    m_geometries.erase(m_geometries.begin() + index);

    for(size_t i = 0; i < m_data.size() ; i++)
    {
      m_data[i].erase(m_data[i].begin() +  index);
    }

    resetExtents();

    return true;
  }

  return false;
}

template<class T>
void TimeGeometryComponentDataItem<T>::getValuesT(int timeIndex, int geomIndex,
                                                  int timeStride, int geomStride, void *data) const
{
  T* dataC = (T*) data;

  for(int i = 0; i < timeStride; i++)
  {
    for(int j = 0 ; j <  geomStride ; j++)
    {
      dataC[i + j*timeStride] = m_data[timeIndex+i][geomIndex + j];
    }
  }
}

template<class T>
void TimeGeometryComponentDataItem<T>::setValuesT(int timeIndex, int geomIndex,
                                                  int timeStride, int geomStride, const void *data)
{
  T* dataC = (T*) data;

  for(int i = 0; i < timeStride; i++)
  {
    for(int j = 0 ; j <  geomStride ; j++)
    {
      T value = dataC[i + j*timeStride];
      m_data[timeIndex+i][geomIndex + j] = value;
    }
  }
}

template<class T>
bool TimeGeometryComponentDataItem<T>::moveData(int fromTimeIndex, int toTimeIndex)
{
  if(fromTimeIndex < (int)m_data.size() && toTimeIndex < (int)m_data.size())
  {
    std::vector<T> &toData = m_data[toTimeIndex];
    std::vector<T> &fromData = m_data[fromTimeIndex];

    for(size_t j = 0 ; j < toData.size(); j++)
    {
      toData[j] = fromData[j];
    }

    return true;
  }

  return false;
}

template<class T>
void TimeGeometryComponentDataItem<T>::moveDataToPrevTime()
{
  if(m_times.size() > 1)
  {
    for(size_t i = 0; i < m_times.size() - 1; i++)
    {
      m_times[i]->setJulianDay(m_times[i+1]->julianDay());

      std::vector<T> & data = m_data[i];

      for(size_t j = 0 ; j < data.size() ; j++)
      {
        data[j] = m_data[i+1][j];
      }
    }
  }
}

template<class T>
T &TimeGeometryComponentDataItem<T>::operator()(int timeIndex, int geometryIndex)
{
  return m_data[timeIndex][geometryIndex];
}

template<class T>
const T &TimeGeometryComponentDataItem<T>::operator()(int timeIndex, int geometryIndex) const
{
  return m_data[timeIndex][geometryIndex];
}

template<class T>
IGeometry::GeometryType TimeGeometryComponentDataItem<T>::getGeometryType() const
{
  return m_geometryType;
}

template<class T>
HCGeometry *TimeGeometryComponentDataItem<T>::getGeometry(int index) const
{
  return m_geometries[index].data();
}

template<class T>
int TimeGeometryComponentDataItem<T>::getGeometryCount() const
{
  return m_geometries.size();
}

template<class T>
SDKTemporal::DateTime *TimeGeometryComponentDataItem<T>::getDateTime(int timeIndex) const
{
  return m_times[timeIndex];
}

template<class T>
int TimeGeometryComponentDataItem<T>::getDateTimeCount() const
{
  return m_times.size();
}

template<class T>
DataCursor *TimeGeometryComponentDataItem<T>::timeCursor() const
{
  return  m_timeCursor;
}

template<class T>
std::vector<SDKTemporal::DateTime *> TimeGeometryComponentDataItem<T>::timesInternal() const
{
  return m_times;
}

template<class T>
SDKTemporal::DateTime* TimeGeometryComponentDataItem<T>::timeInternal(int timeIndex) const
{
  return m_times[timeIndex];
}

template<class T>
SDKTemporal::TimeSpan * TimeGeometryComponentDataItem<T>::timeSpanInternal() const
{
  return m_timeSpan;
}

template<class T>
std::vector<QSharedPointer<HCGeometry> > TimeGeometryComponentDataItem<T>::geometriesInternal() const
{
  return m_geometries;
}

template<class T>
Envelope *TimeGeometryComponentDataItem<T>::envelopeInternal() const
{
  return m_envelope;
}

template<class T>
void TimeGeometryComponentDataItem<T>::clearGeometries()
{
  m_geometries.clear();
  m_data.clear();
}

template<class T>
void TimeGeometryComponentDataItem<T>::clearTimes()
{
  qDeleteAll(m_times);
  m_times.clear();
  m_data.clear();
}

template<class T>
void TimeGeometryComponentDataItem<T>::resetExtents()
{
  m_envelope->resetExtentsToInfinity();

  for(QSharedPointer<HCGeometry> &geometry : m_geometries)
  {
    m_envelope->merge(geometry->envelope());
  }
}

template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<QString>;
template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<QDateTime>;
