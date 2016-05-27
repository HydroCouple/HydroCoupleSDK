#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/abstractmodelcomponent.h"
#include "temporal/timedata.h"
#include "temporal/timeseriescomponentdataitem.h"

using namespace Temporal;

template<class T>
TimeSeriesComponentDataItem<T>::TimeSeriesComponentDataItem(const QList<Time*> &times, Dimension *timeDimension, ValueDefinition *valueDefinition)
   :ComponentDataItem1D<T>(timeDimension , valueDefinition),
     m_times(times), m_timeDimension(timeDimension)

{
   double duration = times[0]->dateTime() - times[times.length() -1]->dateTime();
   m_timeSpan = new Temporal::TimeSpan(times[0]->qDateTime(), duration, nullptr);
}

template<class T>
TimeSeriesComponentDataItem<T>::~TimeSeriesComponentDataItem()
{
   qDeleteAll(m_times);
   m_times.clear();

   delete m_timeSpan;
}

template<class T>
QList<HydroCouple::Temporal::ITime*> TimeSeriesComponentDataItem<T>::times() const
{
   QList<HydroCouple::Temporal::ITime*> times;

   for(Time* time : m_times)
      times.append(time);

   return times;
}

template<class T>
bool TimeSeriesComponentDataItem<T>::addTime(Time *time, bool resetDataArray)
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
   {
      ComponentDataItem1D<T>::resetDataArray();
   }

   return true;
}

template<class T>
void TimeSeriesComponentDataItem<T>::addTimes(const QList<Time*> &times, bool resetDataArray)
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
   {
      ComponentDataItem1D<T>::resetDataArray();
   }
}

template<class T>
void TimeSeriesComponentDataItem<T>::setTimes(const QList<Time *> &times)
{
   qDeleteAll(m_times);
   m_times.clear();

   m_times = times;

   qSort(m_times.begin() , m_times.end() , &Time::compare);

   double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
   m_timeSpan->setDateTime(m_times[0]->qDateTime());
   m_timeSpan->setDuration(duration);

   m_timeDimension->setLength(m_times.length());
}

template<class T>
bool TimeSeriesComponentDataItem<T>::removeTime(Time *time, bool resetDataArray)
{
   if(m_times.removeAll(time))
   {
      double duration = m_times[0]->dateTime() - m_times[m_times.length() -1]->dateTime();
      m_timeSpan->setDateTime(m_times[0]->qDateTime());
      m_timeSpan->setDuration(duration);

      m_timeDimension->setLength(m_times.length());

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
HydroCouple::Temporal::ITimeSpan* TimeSeriesComponentDataItem<T>::timeSpan() const
{
   return m_timeSpan;
}

template<class T>
HydroCouple::IDimension* Temporal::TimeSeriesComponentDataItem<T>::timeDimension() const
{
   return m_timeDimension;
}

template<class T>
void TimeSeriesComponentDataItem<T>::getValue(int idIndex, QVariant &data) const
{
   ComponentDataItem1D<T>::getValue(&idIndex , data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::getValues(int idIndex, int stride, QVariant data[]) const
{
   ComponentDataItem1D<T>::getValues(&idIndex, &stride, data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::getValues(int idIndex, int stride, void *data) const
{
   ComponentDataItem1D<T>::getValues(&idIndex, &stride, data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::setValue(int idIndex, const QVariant &data)
{
   ComponentDataItem1D<T>::setValue(&idIndex, data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::setValues(int idIndex, int stride, const QVariant data[])
{
   ComponentDataItem1D<T>::setValues(&idIndex, &stride, data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::setValues(int idIndex, int stride, const void *data)
{
   ComponentDataItem1D<T>::setValues(&idIndex, &stride, data);
}

template<class T>
void TimeSeriesComponentDataItem<T>::clearTimes()
{
   qDeleteAll(m_times);
   m_times.clear();
}

template<class T>
QList<Time*> TimeSeriesComponentDataItem<T>::hTimes() const
{
   return m_times;
}

template<class T>
TimeSpan* TimeSeriesComponentDataItem<T>::hTimeSpan() const
{
   return m_timeSpan;
}

template<class T>
Dimension* TimeSeriesComponentDataItem<T>::hTimeDimension() const
{
   return m_timeDimension;
}


TimeSeriesComponentDataItemDouble::TimeSeriesComponentDataItemDouble(const QString &id, const QList<Time*> &times,
                                                         Dimension *timeDimension, ValueDefinition *valueDefinition,
                                                         AbstractModelComponent *parentModelComponent)
   : TimeSeriesComponentDataItem<double>(times,timeDimension,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{

}

TimeSeriesComponentDataItemDouble::~TimeSeriesComponentDataItemDouble()
{

}

void TimeSeriesComponentDataItemDouble::readData(QXmlStreamReader &xmlReader)
{
   if(!xmlReader.name().compare("ComponentDataItem", Qt::CaseInsensitive)
         && !xmlReader.hasError()
         &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
   {
      QXmlStreamAttributes attributes = xmlReader.attributes();

      if(attributes.hasAttribute("Id"))
      {
         QString id = attributes.value("Id").toString();
      }

      while (!(xmlReader.isEndElement() && !xmlReader.name().compare("ComponentDataItem", Qt::CaseInsensitive)) && !xmlReader.hasError())
      {
         if(!xmlReader.name().compare("Dimensions", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
         {
            while (!(xmlReader.isEndElement()  && !xmlReader.name().compare("Dimensions", Qt::CaseInsensitive)) && !xmlReader.hasError())
            {
               if(!xmlReader.name().compare("Dimension", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
               {
                  QXmlStreamAttributes attributes = xmlReader.attributes();

                  if(attributes.hasAttribute("Id") && attributes.hasAttribute("Length"))
                  {
                     QString id = attributes.value("Id").toString();

                     if(!hTimeDimension()->id().compare(id))
                     {
                        QString length = attributes.value("Length").toString();
                        hTimeDimension()->setLength(length.toInt());
                     }
                  }

                  while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Dimension", Qt::CaseInsensitive)) && !xmlReader.hasError())
                  {
                     xmlReader.readNext();
                  }
               }
               xmlReader.readNext();
            }
            resetDataArray();
         }
         else if(!xmlReader.name().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
         {
            QList<Time*> times;
            std::vector<double> values;

            while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
            {
               if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
               {
                  QXmlStreamAttributes attributes = xmlReader.attributes();

                  if(attributes.hasAttribute("DateTime"))
                  {
                     QDateTime timeIndex = QDateTime::fromString(attributes.value("DateTime").toString(), Qt::ISODate);
                     times.append(new Time(timeIndex,this));

                     QString value = xmlReader.readElementText();
                     values.push_back(value.toDouble());

                  }
               }
               xmlReader.readNext();
            }

            if(times.length())
            {
               clearTimes();
               addTimes(times);
               setValues(0,times.length(),values.data());
            }
         }
         xmlReader.readNext();
      }
   }
}

void TimeSeriesComponentDataItemDouble::writeData(QXmlStreamWriter &xmlWriter)
{
   xmlWriter.writeStartElement("ComponentDataItem");
   {
      xmlWriter.writeAttribute("Id" , id());
      xmlWriter.writeAttribute("Caption" , caption());

      xmlWriter.writeStartElement("Dimensions");
      {
         xmlWriter.writeStartElement("Dimension");
         {
            xmlWriter.writeAttribute("Id" , hTimeDimension()->id());
            xmlWriter.writeAttribute("Caption" , hTimeDimension()->caption());
            xmlWriter.writeAttribute("Length" , QString::number(hTimeDimension()->length()));
         }
         xmlWriter.writeEndElement();
      }
      xmlWriter.writeEndElement();

      xmlWriter.writeStartElement("Values");
      {
         double values[hTimeDimension()->length()];
         getValues(0,hTimeDimension()->length(),values);

         for(int i = 0 ; i < hTimeDimension()->length() ; i++)
         {
            xmlWriter.writeStartElement("Value");
            {
               xmlWriter.writeAttribute("DateTime", hTimes()[i]->qDateTime().toString(Qt::ISODate));
               xmlWriter.writeCharacters(QString::number(values[i]));
            }
            xmlWriter.writeEndElement();
         }
      }
      xmlWriter.writeEndElement();
   }
   xmlWriter.writeEndElement();
}

template class Temporal::TimeSeriesComponentDataItem<int>;
template class Temporal::TimeSeriesComponentDataItem<double>;
template class Temporal::TimeSeriesComponentDataItem<QString>;
