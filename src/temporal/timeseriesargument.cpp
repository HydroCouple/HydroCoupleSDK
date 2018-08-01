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
#include "temporal/timeseriesargument.h"
#include "temporal/timedata.h"

#include <assert.h>
#include <QTextStream>

using namespace HydroCouple;
using namespace HydroCouple::Temporal;

TimeSeriesArgumentDouble::TimeSeriesArgumentDouble(const QString &id,
                                                   Dimension *timeDimension,
                                                   ValueDefinition *valueDefinition,
                                                   AbstractModelComponent *modelComponent)
  : AbstractArgument(id, QList<Dimension*>({timeDimension}), valueDefinition, modelComponent),
    TimeSeriesComponentDataItem<double>(id, valueDefinition->defaultValue().toDouble(),1),
    m_timeDimension(timeDimension)
{
  setIsReadOnly(true);
  setCanReadFromFile(true);
  setCanReadFromString(true);
  addFileFilter("Comma Separated (*.csv)");
  addFileFilter("Text File (*.txt)");
}

TimeSeriesArgumentDouble::~TimeSeriesArgumentDouble()
{

}


QList<IDateTime*> TimeSeriesArgumentDouble::times() const
{
  QList<IDateTime*> otimes;
  std::vector<SDKTemporal::DateTime*> itimes = TimeSeriesComponentDataItem<double>::timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeSeriesArgumentDouble::time(int timeIndex) const
{
  return timeInternal(timeIndex);
}

int TimeSeriesArgumentDouble::timeCount() const
{
  return timeCountInternal();
}

ITimeSpan* TimeSeriesArgumentDouble::timeSpan() const
{
  return TimeSeriesComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesArgumentDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesArgumentDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void TimeSeriesArgumentDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesArgumentDouble::getValue(int timeIndex, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(timeIndex,data);
}

void TimeSeriesArgumentDouble::getValues(int timeIndex, int stride, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesArgumentDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesArgumentDouble::setValue(int timeIndex, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValueT(timeIndex,data);
}

void TimeSeriesArgumentDouble::setValues(int timeIndex, int stride, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex, stride , data);
}

void TimeSeriesArgumentDouble::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("TimeSeriesArgument", Qt::CaseInsensitive)
     && !xmlReader.hasError()
     &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
  {
    QXmlStreamAttributes attributes = xmlReader.attributes();

    if(attributes.hasAttribute("Id"))
    {
      QString id = attributes.value("Id").toString();

      if(id.compare(this->id() , Qt::CaseSensitive))
      {
        return;
      }
    }

    while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("TimeSeriesArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        while (!(xmlReader.isEndElement()  && !xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Dimension", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            //            QXmlStreamAttributes attributes = xmlReader.attributes();

            //            if(attributes.hasAttribute("Id"))
            //            {
            //              QString id = attributes.value("Id").toString();

            //              if(m_geometryDimension->id().compare(id))
            //              {
            //                throw std::invalid_argument("The id for the specified dimension did not match id expected");
            //              }
            //            }
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
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        QList<SDKTemporal::DateTime*> times;
        std::vector<double> tData;

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();
            QDateTime dt;
            bool ok = false;
            double value = xmlReader.readElementText().toDouble(&ok);

            if(attributes.hasAttribute("DateTime") &&
               SDKTemporal::DateTime::tryParse(attributes.value("DateTime").toString(), dt) && value)
            {
              times.append(new SDKTemporal::DateTime(dt,this));
              tData.push_back(value);
            }
          }

          xmlReader.readNext();
        }

        if(times.length())
        {
          clearTimes();
          addTimes(times);
          setValues(0,times.length(),tData.data());
        }
      }

      xmlReader.readNext();
    }
  }
}

void TimeSeriesArgumentDouble::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("TimeSeriesArgument");
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
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      for(int i = 0; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("DateTime", timesInternal()[i]->dateTime().toString(Qt::ISODate));

          xmlWriter.writeCharacters(QString::number((*this)[i],'g',10));
        }
        xmlWriter.writeEndElement();
      }
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

void TimeSeriesArgumentDouble::saveData()
{

}

QString TimeSeriesArgumentDouble::toString() const
{
  if(currentArgumentIOType() == IArgument::File)
  {
    return modelComponentInternal()->getRelativeFilePath(m_inputFile.absoluteFilePath()).filePath();
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
    xmlWriter.writeEndElement();

    return data;
  }
}

bool TimeSeriesArgumentDouble::readValues(const QString &value, QString &message, bool isFile)
{
  message = "";

  if(isFile)
  {
    m_inputFile = modelComponentInternal()->getAbsoluteFilePath(value);

    if(m_inputFile.exists())
    {
      if(readTimeSeriesFile(m_inputFile, message))
      {
        setArgumentIOType(IArgument::File);
        return true;
      }
    }
    else
    {
      message = "Specified file does not exist";
      return false;
    }
  }
  else
  {
    QXmlStreamReader xmlReader(value);

    while(!xmlReader.isEndDocument() && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("TimeSeriesArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
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
  }

  return false;
}

bool TimeSeriesArgumentDouble::readValues(const IComponentDataItem *componentDataItem, QString &message)
{
  message = "";

  return false;
}

bool TimeSeriesArgumentDouble::readTimeSeriesFile(QFileInfo &fileInfo, QString &message)
{
  QFile file(fileInfo.absoluteFilePath());

  if(file.open(QIODevice::ReadOnly))
  {

    QRegExp rx("(\\,|\\t|\\;)");

    QList<SDKTemporal::DateTime*> times;
    std::vector<double> tData;

    QTextStream stream(&file);

    while (!stream.atEnd())
    {
      QString line = stream.readLine();
      QStringList data = line.split(rx);

      assert(data.length()  > 1);

      QDateTime dt;
      bool ok = false;
      double value = data[1].toDouble(&ok);

      if(SDKTemporal::DateTime::tryParse(data[0],dt) && ok)
      {
        times.append(new SDKTemporal::DateTime(dt,this));
        tData.push_back(value);
      }
    }

    if(times.length())
    {
      clearTimes();
      addTimes(times);
      setValues(0,times.length(),tData.data());
      return true;
    }
    else
    {
      message = "Could not read input file. Make sure file format is correct";
    }

    file.close();
  }

  return false;
}
