#include "stdafx.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/abstractmodelcomponent.h"
#include "temporal/timeidbasedcomponentdataitem.h"
#include "temporal/timedata.h"

using namespace Temporal;

template<class T>
TimeIdBasedComponentDataItem<T>::TimeIdBasedComponentDataItem(const QStringList &identifiers, const QList<Time*> &times,
                                                              Dimension* identifierDimension, Dimension *timeDimension,
                                                              ValueDefinition *valueDefinition)
   :ComponentDataItem2D<T>(QList<Dimension*>({identifierDimension,timeDimension}) , valueDefinition),
     m_identifiers(identifiers), m_identifierDimension(identifierDimension),
     m_times(times), m_timeDimension(timeDimension)

{
   double duration = times[0]->dateTime() - times[times.length() -1]->dateTime();
   m_timeSpan = new TimeSpan(times[0]->qDateTime(), duration, nullptr);
}

template<class T>
TimeIdBasedComponentDataItem<T>::~TimeIdBasedComponentDataItem()
{
   qDeleteAll(m_times);
   m_times.clear();

   delete m_timeSpan;
}

template<class T>
QStringList TimeIdBasedComponentDataItem<T>::identifiers() const
{
   return m_identifiers;
}

template<class T>
bool TimeIdBasedComponentDataItem<T>::addIdentifier(const QString &identifier, bool resetDataArray)
{
   if(!m_identifiers.contains(identifier))
   {
      m_identifiers.append(identifier);
      m_identifierDimension->setLength(m_identifiers.length());

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
void TimeIdBasedComponentDataItem<T>::addIdentifiers(const QList<QString> &identifiers, bool resetDataArray)
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
      m_identifierDimension->setLength(m_identifiers.length());

      if(resetDataArray)
      {
         ComponentDataItem2D<T>::resetDataArray();
      }
   }
}

template<class T>
bool TimeIdBasedComponentDataItem<T>::removeIdentifier(const QString &identifier, bool resetDataArray)
{
   if(m_identifiers.removeOne(identifier))
   {
      m_identifierDimension->setLength(m_identifiers.length());

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
HydroCouple::IDimension* TimeIdBasedComponentDataItem<T>::identifierDimension() const
{
   return m_identifierDimension;
}

template<class T>
QList<HydroCouple::Temporal::ITime*> TimeIdBasedComponentDataItem<T>::times() const
{
   QList<HydroCouple::Temporal::ITime*> times;

   for(Time* time : m_times)
      times.append(time);

   return times;
}

template<class T>
bool TimeIdBasedComponentDataItem<T>::addTime(Time *time, bool resetDataArray)
{
   for(Time *time : m_times)
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

   m_timeDimension->setLength(m_times.length());

   if(resetDataArray)
      ComponentDataItem2D<T>::resetDataArray();

   return true;
}

template<class T>
void TimeIdBasedComponentDataItem<T>::addTimes(const QList<Time*> &times, bool resetDataArray)
{
   for(Time* ntime : times)
   {
      bool found = false;

      for(Time *time : m_times)
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

   m_timeDimension->setLength(m_times.length());

   if(resetDataArray)
      ComponentDataItem2D<T>::resetDataArray();

}

template<class T>
bool TimeIdBasedComponentDataItem<T>::removeTime(Time *time, bool resetDataArray)
{
   if(m_times.removeAll(time))
   {
      double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
      m_timeSpan->setDateTime(m_times[0]->qDateTime());
      m_timeSpan->setDuration(duration);

      m_timeDimension->setLength(m_times.length());


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
HydroCouple::Temporal::ITimeSpan* TimeIdBasedComponentDataItem<T>::timeSpan() const
{
   return m_timeSpan;
}

template<class T>
HydroCouple::IDimension* TimeIdBasedComponentDataItem<T>::timeDimension() const
{
   return m_timeDimension;
}

template<class T>
void TimeIdBasedComponentDataItem<T>::getValue(int idIndex, int timeIndex, QVariant &data) const
{
   int indexes[] = {idIndex , timeIndex};
   ComponentDataItem2D<T>::getValue(indexes , data);
}

template<class T>
void TimeIdBasedComponentDataItem<T>::getValues(int idIndex, int timeIndex, int idStride, int timeStride, QVariant data[]) const
{
   int indexes[] = {idIndex , timeIndex};
   int stride[] = {idStride , timeStride };
   ComponentDataItem2D<T>::getValues(indexes , stride, data);
}

template<class T>
void TimeIdBasedComponentDataItem<T>::getValues(int idIndex, int timeIndex, int idStride, int timeStride, void* data) const
{
   int indexes[] = {idIndex , timeIndex};
   int stride[] = {idStride , timeStride };
   ComponentDataItem2D<T>::getValues(indexes , stride, data);
}

template<class T>
void TimeIdBasedComponentDataItem<T>::setValue(int idIndex, int timeIndex, const QVariant &data)
{
   int indexes[] = {idIndex , timeIndex};
   ComponentDataItem2D<T>::setValue(indexes , data);
}

template<class T>
void TimeIdBasedComponentDataItem<T>::setValues(int idIndex, int timeIndex, int idStride, int timeStride, const QVariant data[])
{
   int indexes[] = {idIndex , timeIndex};
   int stride[] = {idStride , timeStride };
   ComponentDataItem2D<T>::setValues(indexes , stride, data);
}

template<class T>
void TimeIdBasedComponentDataItem<T>::setValues(int idIndex, int timeIndex, int idStride, int timeStride, const void *data)
{
   int indexes[] = {idIndex , timeIndex};
   int stride[] = {idStride , timeStride };
   ComponentDataItem2D<T>::setValues(indexes , stride, data);
}

template<class T>
void TimeIdBasedComponentDataItem<T>::clearIdentifiers()
{
   m_identifiers.clear();
}

template<class T>
Dimension* TimeIdBasedComponentDataItem<T>::hIdentifierDimension() const
{
   return m_identifierDimension;
}

template<class T>
void TimeIdBasedComponentDataItem<T>::clearTimes()
{
   qDeleteAll(m_times);
   m_times.clear();
}

template<class T>
QList<Time*> TimeIdBasedComponentDataItem<T>::hTimes() const
{
   return m_times;
}

template<class T>
TimeSpan* TimeIdBasedComponentDataItem<T>::hTimeSpan() const
{
   return m_timeSpan;
}

template<class T>
Dimension* TimeIdBasedComponentDataItem<T>::hTimeDimension() const
{
   return m_timeDimension;
}


TimeIdBasedComponentDataItemDouble::TimeIdBasedComponentDataItemDouble(const QString &id, const QStringList &identifiers, const QList<Time *> &times,
                                                                       Dimension *identifierDimension, Dimension *timeDimension, 
                                                                       ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent)
   :TimeIdBasedComponentDataItem<double>(identifiers,times,identifierDimension,timeDimension,valueDefinition),
     AbstractComponentDataItem(id,parentModelComponent)
{
   
}

TimeIdBasedComponentDataItemDouble::~TimeIdBasedComponentDataItemDouble()
{
}

template class HYDROCOUPLESDK_EXPORT Temporal::TimeIdBasedComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT Temporal::TimeIdBasedComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT Temporal::TimeIdBasedComponentDataItem<QString>;
