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
  if(m_times.removeAll(time))
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

//==============================================================================================================================

TimeSeriesComponentDataItemDouble::TimeSeriesComponentDataItemDouble(const QString& id,
                                                                     const QList<SDKTemporal::Time*>& times,
                                                                     Dimension* timeDimension,
                                                                     ValueDefinition* valueDefinition,
                                                                     AbstractModelComponent* modelComponent)
  : AbstractComponentDataItem(id,QList<Dimension*>({timeDimension}), valueDefinition, modelComponent),
    TimeSeriesComponentDataItem<double>(times,valueDefinition->defaultValue().toDouble()),
    m_timeDimension(timeDimension)
{

}

TimeSeriesComponentDataItemDouble::~TimeSeriesComponentDataItemDouble()
{
}

QList<ITime*> TimeSeriesComponentDataItemDouble::times() const
{
  QList<ITime*> otimes;
  QList<SDKTemporal::Time*> itimes = TimeSeriesComponentDataItem<double>::timesInternal();

  for(SDKTemporal::Time* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

ITimeSpan* TimeSeriesComponentDataItemDouble::timeSpan() const
{
  return TimeSeriesComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesComponentDataItemDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesComponentDataItemDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void TimeSeriesComponentDataItemDouble::getValue(int dimensionIndexes[], QVariant &data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesComponentDataItemDouble::getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesComponentDataItemDouble::getValues(int dimensionIndexes[], int stride[],  void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesComponentDataItemDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesComponentDataItemDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesComponentDataItemDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesComponentDataItemDouble::getValue(int timeIndex, QVariant &data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(timeIndex,data);
}

void TimeSeriesComponentDataItemDouble::getValues(int timeIndex, int stride, QVariant data[]) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesComponentDataItemDouble::getValues(int timeIndex, int stride, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesComponentDataItemDouble::setValue(int timeIndex, const QVariant &data)
{
  TimeSeriesComponentDataItem<double>::setValueT(timeIndex,data);
}

void TimeSeriesComponentDataItemDouble::setValues(int timeIndex, int stride, const QVariant data[])
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex,stride,data);
}

void TimeSeriesComponentDataItemDouble::setValues(int timeIndex, int stride, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex,stride,data);
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

              if(!m_timeDimension->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                ComponentDataItem1D<double>::setLength(length.toInt());
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
        QList<SDKTemporal::Time*> times;
        std::vector<double> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();

            if(attributes.hasAttribute("DateTime"))
            {
              QDateTime timeIndex = QDateTime::fromString(attributes.value("DateTime").toString(), Qt::ISODate);
              times.append(new SDKTemporal::Time(timeIndex,this));

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
        xmlWriter.writeAttribute("Id" , m_timeDimension->id());
        xmlWriter.writeAttribute("Caption" , m_timeDimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      double values[length()];
      getValues(0,length(),values);

      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("DateTime", timesInternal()[i]->qDateTime().toString(Qt::ISODate));
          xmlWriter.writeCharacters(QString::number(values[i]));
        }
        xmlWriter.writeEndElement();
      }
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<QString>;
