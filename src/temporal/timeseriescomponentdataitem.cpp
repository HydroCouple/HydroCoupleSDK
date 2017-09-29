#include "stdafx.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/abstractmodelcomponent.h"
#include "temporal/timedata.h"
#include "temporal/timeseriescomponentdataitem.h"

#include <assert.h>
#include <QtAlgorithms>

using namespace HydroCouple;
using namespace HydroCouple::Temporal;

template<class T>
TimeSeriesComponentDataItem<T>::TimeSeriesComponentDataItem(const QString &id, const T& defaultValue, int initialSize):
  m_defaultValue(defaultValue),
  m_id(id)
{
  m_timeCursor = new DataCursor(0,initialSize - 1);

  if(initialSize > 0)
  {
    m_times = std::vector<SDKTemporal::DateTime*>(initialSize);
    m_times[0] = new SDKTemporal::DateTime(0,nullptr);
    m_times[initialSize - 1] = m_times[0] ;
    m_data = std::vector<T>(initialSize);
  }


  if(m_times.size())
  {
    double duration = m_times[0]->modifiedJulianDay() - m_times[m_times.size() -1]->modifiedJulianDay();
    m_timeSpan = new SDKTemporal::TimeSpan(m_times[0]->dateTime(), duration, nullptr);
  }
  else
    m_timeSpan = new SDKTemporal::TimeSpan(nullptr);

}

template<class T>
TimeSeriesComponentDataItem<T>::TimeSeriesComponentDataItem(const QString &id, const std::vector<SDKTemporal::DateTime*>& times, const T& defaultValue)
  :m_times(times),
    m_defaultValue(defaultValue),
    m_id(id)
{
  m_timeCursor = new DataCursor(0,times.size() - 1);

  qSort(m_times.begin() , m_times.end() , &SDKTemporal::DateTime::compare);
  double duration = times[0]->modifiedJulianDay() - times[times.size() -1]->modifiedJulianDay();
  m_timeSpan = new SDKTemporal::TimeSpan(times[0]->dateTime(), duration, nullptr);
  m_data = std::vector<T>(m_times.size());
  qFill(m_data, m_defaultValue);
}

template<class T>
TimeSeriesComponentDataItem<T>::TimeSeriesComponentDataItem(const QString &id, const std::list<SDKTemporal::DateTime *> &times, const T& defaultValue)
  : m_defaultValue(defaultValue),
    m_id(id)
{
  m_times = std::vector<SDKTemporal::DateTime*>(std::begin(times), std::end(times));
  m_timeCursor = new DataCursor(0,times.size() - 1);

  qSort(m_times.begin() , m_times.end() , &SDKTemporal::DateTime::compare);
  double duration = times.front()->modifiedJulianDay() - times.back()->modifiedJulianDay();
  m_timeSpan = new SDKTemporal::TimeSpan(times.front()->dateTime(), duration, nullptr);
  m_data = std::vector<T>(m_times.size());
  qFill(m_data, m_defaultValue);
}

template<class T>
TimeSeriesComponentDataItem<T>::~TimeSeriesComponentDataItem()
{
  delete m_timeCursor;

  qDeleteAll(m_times);
  m_times.clear();
  delete m_timeSpan;
}

template<class T>
QString TimeSeriesComponentDataItem<T>::getId() const
{
  return m_id;
}

template<class T>
bool TimeSeriesComponentDataItem<T>::addTime(SDKTemporal::DateTime* time)
{
  if(m_times.size() && m_times[m_times.size() - 1]->dateTime() >= time->dateTime())
  {
    return false;
  }

  m_times.push_back(time);
  m_data.push_back(m_defaultValue);

  double duration = m_times[0]->modifiedJulianDay() - m_times[m_times.size() -1]->modifiedJulianDay();
  m_timeSpan->setDateTime(m_times[0]->dateTime());
  m_timeSpan->setDuration(duration);

  m_timeCursor->setMin(0);
  m_timeCursor->setMax(m_times.size() - 1);

  return true;
}

template<class T>
bool TimeSeriesComponentDataItem<T>::addTimes(const QList<SDKTemporal::DateTime*>& times)
{
  if(m_times.size())
  {
    QList<SDKTemporal::DateTime*> timesAdded ;
    timesAdded.append(m_times[m_times.size() - 1]);


    for(SDKTemporal::DateTime* ntime : times)
    {
      if(timesAdded[timesAdded.length() - 1]->dateTime() >= ntime->dateTime())
      {
        return false;
      }
      else
      {
        timesAdded.append(ntime);
      }
    }
  }

  for(SDKTemporal::DateTime *time : times)
    m_times.push_back(time);

  for(int i = 0 ; i < times.length() ; i++)
  {
    m_data.push_back(m_defaultValue);
  }

  double duration = m_times[0]->modifiedJulianDay() - m_times[m_times.size() -1]->modifiedJulianDay();
  m_timeSpan->setDateTime(m_times[0]->dateTime());
  m_timeSpan->setDuration(duration);

  m_timeCursor->setMin(0);
  m_timeCursor->setMax(m_times.size() - 1);

  return true;
}

template<class T>
bool TimeSeriesComponentDataItem<T>::removeTime(SDKTemporal::DateTime* time)
{
  std::vector<SDKTemporal::DateTime*>::iterator index = std::find(m_times.begin(), m_times.end(), time);

  if(index != m_times.end())
  {
    int loc = std::distance(m_times.begin(), index);
    m_times.erase(index);

    double duration = m_times[0]->modifiedJulianDay() - m_times[m_times.size() -1]->modifiedJulianDay();
    m_timeSpan->setDateTime(m_times[0]->dateTime());
    m_timeSpan->setDuration(duration);


    m_data.erase(m_data.begin() + loc);

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
void TimeSeriesComponentDataItem<T>::getValueT(const std::vector<int> &dimensionIndexes, void *data) const
{
  T *dataC =  (T*)data;
  *dataC = m_data[dimensionIndexes[0]];
}

template<class T>
void TimeSeriesComponentDataItem<T>::getValueT(int timeIndex, void *data) const
{
  T *dataC =  (T*)data;
  *dataC = m_data[timeIndex];
}

template<class T>
void TimeSeriesComponentDataItem<T>::getValuesT(int timeIndex, int stride, void *data) const
{
  T *dataC = (T*)(data);

  for(int i = 0 ; i < stride; i++)
  {
    dataC[i] = m_data[timeIndex+ i];
  }
}

template<class T>
void TimeSeriesComponentDataItem<T>::setValueT(const std::vector<int> &dimensionIndexes, const void *data)
{
  m_data[dimensionIndexes[0]] = *((T*)data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::setValueT(int timeIndex, const void *data)
{
  m_data[timeIndex] = *((T*)data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::setValuesT(int timeIndex, int stride, const void *data)
{
  T *dataC = (T*)(data);

  for(int i = 0 ; i < stride; i++)
  {
    m_data[timeIndex + i] = dataC[i];
  }
}

template<class T>
int TimeSeriesComponentDataItem<T>::length() const
{
  return m_times.size();
}

template<class T>
void TimeSeriesComponentDataItem<T>::moveDataToPrevTime()
{
  if(m_times.size() > 1)
  {
    for(size_t i = 0; i < m_times.size() - 1; i++)
    {
      m_data[i] = m_data[i+1];
      m_times[i]->setModifiedJulianDay(m_times[i+1]->modifiedJulianDay());
    }
  }
}

template<class T>
T &TimeSeriesComponentDataItem<T>::operator [](int index)
{
  return m_data[index];
}

template<class T>
const T &TimeSeriesComponentDataItem<T>::operator [](int index) const
{
  return m_data[index];
}

template<class T>
DataCursor *TimeSeriesComponentDataItem<T>::timeCursor() const
{
  return m_timeCursor;
}

template<class T>
std::vector<SDKTemporal::DateTime *> TimeSeriesComponentDataItem<T>::timesInternal() const
{
  return m_times;
}

template<class T>
SDKTemporal::DateTime* TimeSeriesComponentDataItem<T>::timeInternal(int timeIndex) const
{
  return m_times[timeIndex];
}

template<class T>
int TimeSeriesComponentDataItem<T>::timeCountInternal() const
{
  return m_times.size();
}

template<class T>
void TimeSeriesComponentDataItem<T>::clearTimes()
{
  qDeleteAll(m_times);

  m_times.clear();
  m_data.clear();
}

template<class T>
SDKTemporal::TimeSpan* TimeSeriesComponentDataItem<T>::timeSpanInternal() const
{
  return m_timeSpan;
}


template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<QString>;
