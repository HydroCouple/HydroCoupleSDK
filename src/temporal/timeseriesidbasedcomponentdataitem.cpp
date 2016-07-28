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
TimeSeriesIdBasedComponentDataItem<T>::TimeSeriesIdBasedComponentDataItem(const QStringList& identifiers,
                                                                          const QList<SDKTemporal::Time*>& times,
                                                                          const T& defaultValue)
  : ComponentDataItem2D<T>(identifiers.length(),times.length(),defaultValue),
    m_identifiers(identifiers),
    m_times(times)
{
  double duration = times[0]->dateTime() - times[times.length() -1]->dateTime();
  m_timeSpan = new TimeSpan(times[0]->qDateTime(), duration, nullptr);
}

template<class T>
TimeSeriesIdBasedComponentDataItem<T>::~TimeSeriesIdBasedComponentDataItem()
{
  qDeleteAll(m_times);
  m_times.clear();

  delete m_timeSpan;
}

template<class T>
bool TimeSeriesIdBasedComponentDataItem<T>::addIdentifier(const QString& identifier, bool resetDataArray)
{
  if(!m_identifiers.contains(identifier))
  {
    m_identifiers.append(identifier);
    ComponentDataItem2D<T>::setILength(m_identifiers.length());

    if(resetDataArray)
    {
      ComponentDataItem2D<T>::resetDataArray();
    }

    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::addIdentifiers(const QList<QString>& identifiers, bool resetDataArray)
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
    ComponentDataItem2D<T>::setILength(m_identifiers.length());

    if(resetDataArray)
    {
      ComponentDataItem2D<T>::resetDataArray();
    }
  }
}

template<class T>
bool TimeSeriesIdBasedComponentDataItem<T>::removeIdentifier(const QString& identifier, bool resetDataArray)
{
  if(m_identifiers.removeOne(identifier))
  {
    ComponentDataItem2D<T>::setILength(m_identifiers.length());

    if(resetDataArray)
    {
      ComponentDataItem2D<T>::resetDataArray();
    }
    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
bool TimeSeriesIdBasedComponentDataItem<T>::addTime(SDKTemporal::Time* time, bool resetDataArray)
{
  for(SDKTemporal::Time *time : m_times)
  {
    if(time->dateTime() == time->dateTime())
    {
      return false;
    }
  }

  m_times.append(time);

  qSort(m_times.begin() , m_times.end() , &Time::compare);

  double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
  m_timeSpan->setDateTime(m_times[0]->qDateTime());
  m_timeSpan->setDuration(duration);
  ComponentDataItem2D<T>::setJLength(m_times.length());

  if(resetDataArray)
  {
    ComponentDataItem2D<T>::resetDataArray();
  }

  return true;
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::addTimes(const QList<SDKTemporal::Time*>& times, bool resetDataArray)
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

  qSort(m_times.begin() , m_times.end() , &Time::compare);

  double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
  m_timeSpan->setDateTime(m_times[0]->qDateTime());
  m_timeSpan->setDuration(duration);
  ComponentDataItem2D<T>::setJLength(m_times.length());

  if(resetDataArray)
  {
    ComponentDataItem2D<T>::resetDataArray();
  }
}

template<class T>
bool TimeSeriesIdBasedComponentDataItem<T>::removeTime(SDKTemporal::Time* time, bool resetDataArray)
{
  if(m_times.removeAll(time))
  {
    double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
    m_timeSpan->setDateTime(m_times[0]->qDateTime());
    m_timeSpan->setDuration(duration);
    ComponentDataItem2D<T>::setJLength(m_times.length());

    if(resetDataArray)
    {
      ComponentDataItem2D<T>::resetDataArray();
    }

    return true;
  }
  else
  {
    return false;
  }
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::setTimes(const QList<SDKTemporal::Time*>& times)
{
  qDeleteAll(m_times);
  m_times.clear();

  m_times = times;

  qSort(m_times.begin() , m_times.end() , &SDKTemporal::Time::compare);

  double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
  m_timeSpan->setDateTime(m_times[0]->qDateTime());
  m_timeSpan->setDuration(duration);
  ComponentDataItem2D<T>::setJLength(m_times.length());
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::getValueT(int idIndex, int timeIndex, QVariant& data) const
{
  int indexes[] = {idIndex , timeIndex};
  ComponentDataItem2D<T>::getValueT(indexes, data);
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::getValuesT(int idIndex, int timeIndex, int idStride, int timeStride, QVariant data[]) const
{
  int indexes[] = {idIndex , timeIndex};
  int stride[] = {idStride , timeStride };
  ComponentDataItem2D<T>::getValuesT(indexes , stride, data);
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::getValuesT(int idIndex, int timeIndex, int idStride, int timeStride, void* data) const
{
  int indexes[] = {idIndex , timeIndex};
  int stride[] = {idStride , timeStride };
  ComponentDataItem2D<T>::getValuesT(indexes,stride, data);
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::setValueT(int idIndex, int timeIndex, const QVariant& data)
{
  int indexes[] = {idIndex , timeIndex};
  ComponentDataItem2D<T>::setValueT(indexes, data);
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::setValuesT(int idIndex, int timeIndex, int idStride, int timeStride, const QVariant data[])
{
  int indexes[] = {idIndex , timeIndex};
  int stride[] = {idStride , timeStride };
  ComponentDataItem2D<T>::setValuesT(indexes,stride, data);
}

template<class T>
void TimeSeriesIdBasedComponentDataItem<T>::setValuesT(int idIndex, int timeIndex, int idStride, int timeStride, const void *data)
{
  int indexes[] = {idIndex , timeIndex};
  int stride[] = {idStride , timeStride };
  ComponentDataItem2D<T>::setValuesT(indexes, stride, data);
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
QList<SDKTemporal::Time*> TimeSeriesIdBasedComponentDataItem<T>::timesInternal() const
{
  return m_times;
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
}

//==============================================================================================================================

TimeSeriesIdBasedComponentDataItemDouble::TimeSeriesIdBasedComponentDataItemDouble(const QString& id,
                                                                                   const QStringList& identifiers,
                                                                                   Dimension* identifierDimension,
                                                                                   const QList<SDKTemporal::Time *>& times,
                                                                                   Dimension* timeDimension,
                                                                                   ValueDefinition* valueDefinition,
                                                                                   AbstractModelComponent* modelComponent)
  : AbstractComponentDataItem(id,QList<Dimension*>({identifierDimension,timeDimension}), valueDefinition, modelComponent),
    TimeSeriesIdBasedComponentDataItem<double>(identifiers,times,valueDefinition->defaultValue().toDouble()),
    m_identifierDimension(identifierDimension),
    m_timeDimension(timeDimension)
{
}

TimeSeriesIdBasedComponentDataItemDouble::~TimeSeriesIdBasedComponentDataItemDouble()
{
}

QStringList TimeSeriesIdBasedComponentDataItemDouble::identifiers() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::identifiersInternal();
}

IDimension* TimeSeriesIdBasedComponentDataItemDouble::identifierDimension() const
{
  return m_identifierDimension;
}

QList<ITime*> TimeSeriesIdBasedComponentDataItemDouble::times() const
{
  QList<ITime*> otimes;
  QList<SDKTemporal::Time*> itimes = TimeSeriesIdBasedComponentDataItem<double>::timesInternal();

  for(SDKTemporal::Time* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

ITimeSpan* TimeSeriesIdBasedComponentDataItemDouble::timeSpan() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesIdBasedComponentDataItemDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesIdBasedComponentDataItemDouble::dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength < dimensions().length());

  if(dimensionIndexesLength == 0)
  {
    return iLength();
  }
  else
  {
    return jLength();
  }
}

void TimeSeriesIdBasedComponentDataItemDouble::getValue(int dimensionIndexes[], QVariant &data) const
{
  ComponentDataItem2D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::getValues(int dimensionIndexes[], int stride[],  void *data) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem2D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::getValue(int timeIndex, int idIndex, QVariant &data) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride, QVariant data[]) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride, void *data) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::setValue(int timeIndex, int idIndex, const QVariant &data)
{
  TimeSeriesIdBasedComponentDataItem<double>::setValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const QVariant data[])
{
  TimeSeriesIdBasedComponentDataItem<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedComponentDataItemDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data)
{
  TimeSeriesIdBasedComponentDataItem<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<QString>;
