#include "stdafx.h"
#include "idbasedtimecomponentdataitem.h"
#include "dimension.h"
#include "timedata.h"
#include  "valuedefinition.h"
#include "abstractmodelcomponent.h"

using namespace HydroCouple;
using namespace HydroCouple::Temporal;

template<class T>
IdBasedTimeComponentDataItem<T>::IdBasedTimeComponentDataItem(const QStringList &identifiers, const QList<HTime*> &times,
                                                              Dimension* identifierDimension, Dimension *timeDimension,
                                                              ValueDefinition *valueDefinition)
   :ComponentDataItem2D<T>(QList<Dimension*>({identifierDimension,timeDimension}) , valueDefinition),
     m_identifiers(identifiers), m_identifierDimension(identifierDimension),
     m_times(times), m_timeDimension(timeDimension)

{
   double duration = times[0]->dateTime() - times[times.length() -1]->dateTime();
   m_timeSpan = new HTimeSpan(times[0]->qDateTime(), duration, nullptr);
}

template<class T>
IdBasedTimeComponentDataItem<T>::~IdBasedTimeComponentDataItem()
{
   qDeleteAll(m_times);
   m_times.clear();

   delete m_timeSpan;
}

template<class T>
QStringList IdBasedTimeComponentDataItem<T>::identifiers() const
{
   return m_identifiers;
}

template<class T>
bool IdBasedTimeComponentDataItem<T>::addIdentifier(const QString &identifier, bool resetDataArray)
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
void IdBasedTimeComponentDataItem<T>::addIdentifiers(const QList<QString> &identifiers, bool resetDataArray)
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
bool IdBasedTimeComponentDataItem<T>::removeIdentifier(const QString &identifier, bool resetDataArray)
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
IDimension* IdBasedTimeComponentDataItem<T>::identifierDimension() const
{
   return m_identifierDimension;
}

template<class T>
QList<ITime*> IdBasedTimeComponentDataItem<T>::times() const
{
   QList<ITime*> times;

   for(HTime* time : m_times)
      times.append(time);

   return times;
}

template<class T>
bool IdBasedTimeComponentDataItem<T>::addTime(HTime *time, bool resetDataArray)
{
   for(HTime *time : m_times)
   {
      if(time->dateTime() == time->dateTime())
      {
         return false;
      }
   }

   m_times.append(time);

   qSort(m_times.begin() , m_times.end() , &HTime::compare);

   double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
   m_timeSpan->setDateTime(m_times[0]->qDateTime());
   m_timeSpan->setDuration(duration);

   m_timeDimension->setLength(m_times.length());

   if(resetDataArray)
      ComponentDataItem2D<T>::resetDataArray();

   return true;
}

template<class T>
void IdBasedTimeComponentDataItem<T>::addTimes(const QList<HTime*> &times, bool resetDataArray)
{
   for(HTime* ntime : times)
   {
      bool found = false;

      for(HTime *time : m_times)
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

   qSort(m_times.begin() , m_times.end() , &HTime::compare);

   double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
   m_timeSpan->setDateTime(m_times[0]->qDateTime());
   m_timeSpan->setDuration(duration);

   m_timeDimension->setLength(m_times.length());

   if(resetDataArray)
      ComponentDataItem2D<T>::resetDataArray();

}

template<class T>
bool IdBasedTimeComponentDataItem<T>::removeTime(HTime *time, bool resetDataArray)
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
ITimeSpan* IdBasedTimeComponentDataItem<T>::timeSpan() const
{
   return m_timeSpan;
}

template<class T>
IDimension* IdBasedTimeComponentDataItem<T>::timeDimension() const
{
   return m_timeDimension;
}

template<class T>
void IdBasedTimeComponentDataItem<T>::getValue(int idIndex, int timeIndex, QVariant &data) const
{
   int indexes[] = {idIndex , timeIndex};
   ComponentDataItem2D<T>::getValue(indexes , data);
}

template<class T>
void IdBasedTimeComponentDataItem<T>::getValues(int idIndex, int timeIndex, int idStride, int timeStride, QVariant data[]) const
{
   int indexes[] = {idIndex , timeIndex};
   int stride[] = {idStride , timeStride };
   ComponentDataItem2D<T>::getValues(indexes , stride, data);
}

template<class T>
void IdBasedTimeComponentDataItem<T>::getValues(int idIndex, int timeIndex, int idStride, int timeStride, void* data) const
{
   int indexes[] = {idIndex , timeIndex};
   int stride[] = {idStride , timeStride };
   ComponentDataItem2D<T>::getValues(indexes , stride, data);
}

template<class T>
void IdBasedTimeComponentDataItem<T>::setValue(int idIndex, int timeIndex, const QVariant &data)
{
   int indexes[] = {idIndex , timeIndex};
   ComponentDataItem2D<T>::setValue(indexes , data);
}

template<class T>
void IdBasedTimeComponentDataItem<T>::setValues(int idIndex, int timeIndex, int idStride, int timeStride, const QVariant data[])
{
   int indexes[] = {idIndex , timeIndex};
   int stride[] = {idStride , timeStride };
   ComponentDataItem2D<T>::setValues(indexes , stride, data);
}

template<class T>
void IdBasedTimeComponentDataItem<T>::setValues(int idIndex, int timeIndex, int idStride, int timeStride, const void *data)
{
   int indexes[] = {idIndex , timeIndex};
   int stride[] = {idStride , timeStride };
   ComponentDataItem2D<T>::setValues(indexes , stride, data);
}

template<class T>
void IdBasedTimeComponentDataItem<T>::clearIdentifiers()
{
   m_identifiers.clear();
}

template<class T>
Dimension* IdBasedTimeComponentDataItem<T>::hIdentifierDimension() const
{
   return m_identifierDimension;
}

template<class T>
void IdBasedTimeComponentDataItem<T>::clearTimes()
{
   qDeleteAll(m_times);
   m_times.clear();
}

template<class T>
QList<HTime*> IdBasedTimeComponentDataItem<T>::hTimes() const
{
   return m_times;
}

template<class T>
HTimeSpan* IdBasedTimeComponentDataItem<T>::hTimeSpan() const
{
   return m_timeSpan;
}

template<class T>
Dimension* IdBasedTimeComponentDataItem<T>::hTimeDimension() const
{
   return m_timeDimension;
}


IdBasedTimeComponentDataItemDouble::IdBasedTimeComponentDataItemDouble(const QString &id, const QStringList &identifiers, const QList<HTime *> &times,
                                                                       Dimension *identifierDimension, Dimension *timeDimension, 
                                                                       ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent)
   :IdBasedTimeComponentDataItem<double>(identifiers,times,identifierDimension,timeDimension,valueDefinition),
     AbstractComponentDataItem(id,parentModelComponent)
{
   
}

IdBasedTimeComponentDataItemDouble::~IdBasedTimeComponentDataItemDouble()
{
}

template class HYDROCOUPLESDK_EXPORT IdBasedTimeComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT IdBasedTimeComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT IdBasedTimeComponentDataItem<QString>;
