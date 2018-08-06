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
#include "core/abstractmodelcomponent.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "spatial/geometry.h"
#include "spatial/envelope.h"
#include "spatial/geometryfactory.h"
#include "temporal/timedata.h"
#include "spatiotemporal/timegeometryargument.h"

#include <assert.h>
#include <QTextStream>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::Temporal;

TimeGeometryArgumentDouble::TimeGeometryArgumentDouble(const QString &id,
                                                       IGeometry::GeometryType geometryType,
                                                       Dimension *timeDimension,
                                                       Dimension *geometryDimension,
                                                       ValueDefinition *valueDefinition,
                                                       AbstractModelComponent *modelComponent)
  : AbstractArgument(id,QList<Dimension*>({timeDimension, geometryDimension}),valueDefinition,modelComponent),
    TimeGeometryComponentDataItem<double>(id, geometryType, valueDefinition->defaultValue().toDouble()),
    m_gdalDriverName("ESRI Shapefile"),
    m_timeDimension(timeDimension),
    m_geometryDimension(geometryDimension),
    m_readGeometryFromFile(false),
    m_readTimeSeriesFromFile(false),
    m_overwriteGeometryFile(false),
    m_overwriteTimeSeriesFile(false)
{
  setIsReadOnly(true);
  setCanReadFromFile(false);
  setCanReadFromString(true);
  addFileFilter("");
}

TimeGeometryArgumentDouble::~TimeGeometryArgumentDouble()
{

}

QList<IDateTime*> TimeGeometryArgumentDouble::times() const
{
  QList<IDateTime*> otimes;
  std::vector<SDKTemporal::DateTime*> itimes = timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeGeometryArgumentDouble::time(int timeIndex) const
{
  return m_times[timeIndex];
}

int TimeGeometryArgumentDouble::timeCount() const
{
  return m_times.size();
}

ITimeSpan *TimeGeometryArgumentDouble::timeSpan() const
{
  return timeSpanInternal();
}

IDimension *TimeGeometryArgumentDouble::timeDimension() const
{
  return m_timeDimension;
}

IGeometry::GeometryType TimeGeometryArgumentDouble::geometryType() const
{
  return getGeometryType();
}

int TimeGeometryArgumentDouble::geometryCount() const
{
  return m_geometries.size();
}

HydroCouple::Spatial::IGeometry *TimeGeometryArgumentDouble::geometry(int geometryIndex) const
{
  return m_geometries[geometryIndex].data();
}

IDimension *TimeGeometryArgumentDouble::geometryDimension() const
{
  return m_geometryDimension;
}

IEnvelope *TimeGeometryArgumentDouble::envelope() const
{
  return envelopeInternal();
}

int TimeGeometryArgumentDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  if(dimensionIndexes.size() == 0)
    return m_times.size();

  return m_geometries.size();
}

void TimeGeometryArgumentDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  TimeGeometryComponentDataItem<double>::getValuesT(dimensionIndexes[0],dimensionIndexes[1],1,1,data);
}

void TimeGeometryArgumentDouble::getValue(int timeIndex, int geometryIndex, void *data) const
{
  TimeGeometryComponentDataItem<double>::getValuesT(timeIndex,geometryIndex,1,1,data);
}

void TimeGeometryArgumentDouble::getValues(int timeIndex, int geometryIndex,
                                           int timeStride, int geomStride, void *data) const
{
  TimeGeometryComponentDataItem<double>::getValuesT(timeIndex, geometryIndex,timeStride, geomStride,data);
}

void TimeGeometryArgumentDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  TimeGeometryComponentDataItem<double>::setValuesT(dimensionIndexes[0],dimensionIndexes[1],1,1,data);
}

void TimeGeometryArgumentDouble::setValue(int timeIndex, int geometryIndex, const void *data)
{
  TimeGeometryComponentDataItem<double>::setValuesT(timeIndex,geometryIndex,1,1,data);
}

void TimeGeometryArgumentDouble::setValues(int timeIndex, int geometryIndex,
                                           int timeStride, int geomStride, const void *data)
{
  TimeGeometryComponentDataItem<double>::setValuesT(timeIndex,geometryIndex,timeStride,geomStride,data);
}

void TimeGeometryArgumentDouble::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("TimeGeometryArgument", Qt::CaseInsensitive)
     && !xmlReader.hasError()
     &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
  {
    QXmlStreamAttributes attributes = xmlReader.attributes();

    if(attributes.hasAttribute("Id"))
    {
      QString id = attributes.value("Id").toString();

      if(id.compare(this->id(), Qt::CaseSensitive))
      {
        return;
      }
    }

    while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("TimeGeometryArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        while (!(xmlReader.isEndElement()  && !xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Dimension", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Dimension", Qt::CaseInsensitive)) && !xmlReader.hasError())
            {
              xmlReader.readNext();
            }
          }
          xmlReader.readNext();
        }
      }
      else if(!xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        valueDefinitionInternal()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          xmlReader.readNext();
        }
      }
      else if(!xmlReader.name().toString().compare("Geometries", Qt::CaseInsensitive) &&
              !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

        QXmlStreamAttributes attributes = xmlReader.attributes();
        QString fromFile = attributes.value("IsFromFile").toString();

        if(attributes.hasAttribute("GDALDriver"))
        {
          m_gdalDriverName = attributes.value("GDALDriver").toString().trimmed();
        }

        if(!fromFile.compare("True",Qt::CaseInsensitive))
        {
          m_readGeometryFromFile = true;
          QString filepath = xmlReader.readElementText().trimmed();
          m_geometryFile = modelComponentInternal()->getAbsoluteFilePath(filepath);

          if(attributes.hasAttribute("OverwriteInputGeometryFile"))
          {
            m_overwriteGeometryFile = !attributes.value("OverwriteInputGeometryFile").toString().compare("True" , Qt::CaseInsensitive)
                                      ? true : false;
          }

          if(m_geometryFile.isFile() && m_geometryFile.exists())
          {
            QString message;

            clearGeometries();

            if(!GeometryFactory::readGeometryFromFile(m_geometryFile.absoluteFilePath(), this, *envelopeInternal(), message))
            {
              throw std::invalid_argument("Geometry file {" + m_geometryFile.absoluteFilePath().toStdString() + "} could not be read; Message: " + message.toStdString());
            }
          }
        }
        else
        {
          m_readGeometryFromFile = false;

          clearGeometries();

          while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Geometries", Qt::CaseInsensitive)) && !xmlReader.hasError())
          {
            if(!xmlReader.name().toString().compare("Geometry", Qt::CaseInsensitive) &&
               !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
            {
              HCGeometry* geometry = GeometryFactory::importFromWkt(xmlReader.readElementText().trimmed());

              assert(getGeometryType() == geometry->geometryType());

              m_geometries.push_back(QSharedPointer<HCGeometry>(geometry));

              while (!(xmlReader.isEndElement() &&
                       !xmlReader.name().toString().compare("Geometry", Qt::CaseInsensitive)) &&
                     !xmlReader.hasError())
              {
                xmlReader.readNext();
              }
            }
            xmlReader.readNext();
          }
        }

        emit propertyChanged("Geometries");

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Geometries", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          xmlReader.readNext();
        }
      }
      else if(!xmlReader.name().toString().compare("TimeSeriesValues", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        QXmlStreamAttributes attributes = xmlReader.attributes();
        QString fromFile = attributes.value("IsFromFile").toString();

        if(!fromFile.compare("True",Qt::CaseInsensitive))
        {
          m_readTimeSeriesFromFile = true;
          m_timeSeriesFile = modelComponentInternal()->getAbsoluteFilePath(xmlReader.readElementText().trimmed());

          if(m_timeSeriesFile.isFile() && m_timeSeriesFile.exists())
          {
            clearTimes();

            QFile tsFile(m_timeSeriesFile.absoluteFilePath());

            if(tsFile.open(QIODevice::ReadOnly))
            {
              QRegExp rx("(\\,|\\t|\\;)");

              QTextStream reader(&tsFile);

              while (!reader.atEnd())
              {
                QString line = reader.readLine();
                QStringList data = line.split(rx);

                if(data.length() > 1)
                {
                  QDateTime dt;

                  if(SDKTemporal::DateTime::tryParse(data[0],dt))
                  {
                    std::vector<double> values;//(m_geometries.length());
                    bool ok = false;

                    for(int i = 1 ; i < data.length() ; i++)
                    {
                      double value = data[i].toDouble(&ok);

                      if(ok)
                      {
                        values.push_back(value);
                      }
                    }

                    if(values.size())
                    {
                      addTime(new SDKTemporal::DateTime(dt,this));
                      setValues(m_times.size() - 1, 0,1,m_geometries.size(),values.data());
                    }
                  }
                }
              }
            }
          }
        }
        else
        {
          m_readTimeSeriesFromFile = false;

          while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("TimeSeriesValues", Qt::CaseInsensitive)) && !xmlReader.hasError())
          {
            if(!xmlReader.name().toString().compare("DateTimes", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
            {
              QList<SDKTemporal::DateTime*> dateTimes;

              while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("DateTimes", Qt::CaseInsensitive)) && !xmlReader.hasError())
              {
                if(!xmlReader.name().toString().compare("DateTime", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
                {
                  QDateTime tempDateTime;

                  if(SDKTemporal::DateTime::tryParse(xmlReader.readElementText(),tempDateTime))
                  {
                    SDKTemporal::DateTime *dateTime = new SDKTemporal::DateTime(tempDateTime,this);
                    dateTimes.append(dateTime);
                  }

                  while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("DateTime", Qt::CaseInsensitive)) && !xmlReader.hasError())
                  {
                    xmlReader.readNext();
                  }
                }
                xmlReader.readNext();
              }

              if(dateTimes.length())
              {
                addTimes(dateTimes);
              }
            }
            else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
            {
              while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
              {
                if(!xmlReader.name().toString().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
                {
                  QXmlStreamAttributes attributes = xmlReader.attributes();

                  int timeIndex = attributes.value("TimeIndex").toInt();
                  int geometryIndex = attributes.value("GeometryIndex").toInt();
                  double tempValue = xmlReader.readElementText().toDouble();
                  (*this)(timeIndex, geometryIndex) = tempValue;
                }
                xmlReader.readNext();
              }
            }
            xmlReader.readNext();
          }
        }
      }
      xmlReader.readNext();
    }
  }
}

void TimeGeometryArgumentDouble::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("TimeGeometryArgument");
  {
    xmlWriter.writeAttribute("Id" , id());
    xmlWriter.writeAttribute("Caption" , caption());
    xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

    for(const QString& comment : comments())
    {
      xmlWriter.writeComment(comment);
    }
    //write value definition;
    valueDefinitionInternal()->writeData(xmlWriter);

    xmlWriter.writeStartElement("Dimensions");
    {
      xmlWriter.writeStartElement("Dimension");
      {
        xmlWriter.writeAttribute("Id" , m_timeDimension->id());
        xmlWriter.writeAttribute("Caption" , m_timeDimension->caption());
      }
      xmlWriter.writeEndElement();

      xmlWriter.writeStartElement("Dimension");
      {
        xmlWriter.writeAttribute("Id" , m_geometryDimension->id());
        xmlWriter.writeAttribute("Caption" , m_geometryDimension->caption());
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    //Geometries
    xmlWriter.writeStartElement("Geometries");
    {
      xmlWriter.writeAttribute("GeometryType" , HCGeometry::geometryTypeToString(getGeometryType()));

      if(m_readGeometryFromFile)
      {
        xmlWriter.writeAttribute("IsFromFile", "True");
        xmlWriter.writeAttribute("GDALDriver", m_gdalDriverName);
        xmlWriter.writeAttribute("OverwriteInputGeometryFile", m_overwriteGeometryFile ? "True" :  "False");
        xmlWriter.writeCharacters(modelComponentInternal()->getRelativeFilePath(m_geometryFile.absoluteFilePath()).filePath());
      }
      else
      {
        xmlWriter.writeAttribute("IsFromFile", "False");

        for(size_t i =0 ; i < m_geometries.size() ; i++)
        {
          xmlWriter.writeTextElement("Geometry", m_geometries[i]->getWKT());
        }
      }
    }
    xmlWriter.writeEndElement();

    //Timeseries values
    xmlWriter.writeStartElement("TimeSeriesValues");
    {
      if(m_readTimeSeriesFromFile)
      {
        xmlWriter.writeAttribute("IsFromFile", "True");
        xmlWriter.writeCharacters(modelComponentInternal()->getRelativeFilePath(m_timeSeriesFile.absoluteFilePath()).filePath());
      }
      else
      {
        xmlWriter.writeAttribute("IsFromFile", "False");

        xmlWriter.writeStartElement("DateTimes");
        {
          for(int i = 0; i < getDateTimeCount(); i++)
          {
            SDKTemporal::DateTime *dateTime  = m_times[i];
            xmlWriter.writeTextElement("DateTime",dateTime->dateTime().toString(Qt::ISODate));
          }
        }
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("Values");
        {
          for(int i = 0; i < getDateTimeCount(); i++)
          {
            for(int j = 0; j < getGeometryCount(); j++)
            {
              xmlWriter.writeStartElement("Value");
              {
                xmlWriter.writeAttribute("TimeIndex",QString::number(i));
                xmlWriter.writeAttribute("GeometryIndex",QString::number(j));
                double tval = (*this)(i,j);
                xmlWriter.writeCharacters(QString::number(tval,'g',10));
              }
              xmlWriter.writeEndElement();
            }
          }
        }
        xmlWriter.writeEndElement();
      }
    }
    xmlWriter.writeEndElement();

  }
  xmlWriter.writeEndElement();
}

void TimeGeometryArgumentDouble::saveData()
{
  if(!isReadOnly())
  {

  }
}

QString TimeGeometryArgumentDouble::toString() const
{
  if(currentArgumentIOType() == IArgument::File)
  {
    return "";
  }
  else
  {
    QString data;
    QXmlStreamWriter xmlWriter(&data);
    xmlWriter.setAutoFormatting(true);

    xmlWriter.writeStartDocument();
    {
      writeData(xmlWriter);
    }
    xmlWriter.writeEndDocument();

    return data;
  }
}

bool TimeGeometryArgumentDouble::readValues(const QString &value, QString &message, bool isFile)
{
  if(isFile)
  {

  }
  else
  {
    QXmlStreamReader xmlReader(value);

    while(!xmlReader.isEndDocument() && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("TimeGeometryArgument", Qt::CaseInsensitive) &&
         !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        readData(xmlReader);
      }

      xmlReader.readNext();
    }

    if(!xmlReader.hasError())
    {
      setArgumentIOType(IArgument::String);
      return true;
    }
    else
    {
      message = QString(xmlReader.errorString());
    }
  }

  return false;
}

bool TimeGeometryArgumentDouble::readValues(const IComponentDataItem *componentDataItem, QString &message)
{
  message = "";
  const ITimeGeometryComponentDataItem *timeGeometryComponentDataItem = dynamic_cast<const ITimeGeometryComponentDataItem*>(componentDataItem);

  if(timeGeometryComponentDataItem)
  {

  }

  return false;
}

template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<QString>;
template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<QDateTime>;
