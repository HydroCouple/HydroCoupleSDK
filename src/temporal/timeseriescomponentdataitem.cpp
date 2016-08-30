#include "stdafx.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/abstractmodelcomponent.h"
#include "temporal/timedata.h"
#include "temporal/timeseriescomponentdataitem.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Temporal;


template<class T>
TimeSeriesComponentDataItem<T>::TimeSeriesComponentDataItem(const QList<SDKTemporal::Time*>& times, const T& defaultValue)
  : ComponentDataItem1D<T>(times.length() , defaultValue),
    m_times(times)
{
  double duration = times[0]->dateTime() - times[times.length() -1]->dateTime();
  m_timeSpan = new SDKTemporal::TimeSpan(times[0]->qDateTime(), duration, nullptr);
}

template<class T>
TimeSeriesComponentDataItem<T>::~TimeSeriesComponentDataItem()
{
  qDeleteAll(m_times);
  m_times.clear();

  delete m_timeSpan;
}

template<class T>
bool TimeSeriesComponentDataItem<T>::addTime(SDKTemporal::Time* time, bool resetDataArray)
{
  for(SDKTemporal::Time *time : m_times)
  {
    if(time->dateTime() == time->dateTime())
    {
      return false;
    }
  }

  m_times.append(time);

  qSort(m_times.begin() , m_times.end() , &SDKTemporal::Time::compare);

  double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
  m_timeSpan->setDateTime(m_times[0]->qDateTime());
  m_timeSpan->setDuration(duration);

  ComponentDataItem1D<T>::setLength(m_times.length());

  if(resetDataArray)
  {
    ComponentDataItem1D<T>::resetDataArray();
  }

  return true;
}

template<class T>
void TimeSeriesComponentDataItem<T>::addTimes(const QList<SDKTemporal::Time*>& times, bool resetDataArray)
{
  for(SDKTemporal::Time* ntime : times)
  {
    bool found = false;

    for(SDKTemporal::Time *time : m_times)
    {
      if(time->dateTime() == ntime->dateTime())
      {
        found = true;
      }
    }

    if(!found)
    {
      m_times.append(ntime);
    }
  }

  qSort(m_times.begin() , m_times.end() , &SDKTemporal::Time::compare);

  double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
  m_timeSpan->setDateTime(m_times[0]->qDateTime());
  m_timeSpan->setDuration(duration);

  ComponentDataItem1D<T>::setLength(m_times.length());

  if(resetDataArray)
  {
    ComponentDataItem1D<T>::resetDataArray();
  }
}

template<class T>
bool TimeSeriesComponentDataItem<T>::removeTime(SDKTemporal::Time* time, bool resetDataArray)
{
  if(m_times.removeOne(time))
  {
    double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
    m_timeSpan->setDateTime(m_times[0]->qDateTime());
    m_timeSpan->setDuration(duration);

    ComponentDataItem1D<T>::setLength(m_times.length());

    if(resetDataArray)
    {
      ComponentDataItem1D<T>::resetDataArray();
    }

    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
void TimeSeriesComponentDataItem<T>::setTimes(const QList<SDKTemporal::Time*>& times)
{
  qDeleteAll(m_times);
  m_times.clear();

  m_times = times;

  qSort(m_times.begin() , m_times.end() , &SDKTemporal::Time::compare);

  double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
  m_timeSpan->setDateTime(m_times[0]->qDateTime());
  m_timeSpan->setDuration(duration);

  ComponentDataItem1D<T>::setLength(m_times.length());
}

template<class T>
void TimeSeriesComponentDataItem<T>::getValueT(int timeIndex, QVariant &data) const
{
  ComponentDataItem1D<T>::getValueT(&timeIndex,data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::getValuesT(int timeIndex, int stride, QVariant data[]) const
{
  ComponentDataItem1D<T>::getValuesT(&timeIndex,&stride,data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::getValuesT(int timeIndex, int stride, void *data) const
{
  ComponentDataItem1D<T>::getValuesT(&timeIndex,&stride,data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::setValueT(int timeIndex, const QVariant &data)
{
  ComponentDataItem1D<T>::setValueT(&timeIndex,data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::setValuesT(int timeIndex, int stride, const QVariant data[])
{
  ComponentDataItem1D<T>::setValuesT(&timeIndex,&stride,data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::setValuesT(int timeIndex, int stride, const void *data)
{
  ComponentDataItem1D<T>::setValuesT(&timeIndex,&stride,data);
}

template<class T>
QList<SDKTemporal::Time*> TimeSeriesComponentDataItem<T>::timesInternal() const
{
  return m_times;
}

template<class T>
void TimeSeriesComponentDataItem<T>::clearTimes()
{
  qDeleteAll(m_times);
  m_times.clear();
}

template<class T>
SDKTemporal::TimeSpan* TimeSeriesComponentDataItem<T>::timeSpanInternal() const
{
  return m_timeSpan;
}


template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<QString>;
