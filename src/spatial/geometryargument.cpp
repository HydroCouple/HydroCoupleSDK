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
#include "spatial/geometryargument.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "spatial/geometrycollection.h"
#include "spatial/geometryfactory.h"
#include "core/abstractmodelcomponent.h"
#include "spatial/envelope.h"

#include <QFileInfo>
#include <QDir>
#include <assert.h>
#include <QXmlStreamReader>
#include <QVariant>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

GeometryArgumentDouble::GeometryArgumentDouble(const QString &id,
                                               IGeometry::GeometryType geometryType,
                                               Dimension *geometryDimension,
                                               ValueDefinition *valueDefinition,
                                               AbstractModelComponent *modelComponent)
  : AbstractArgument(id , QList<Dimension*>({geometryDimension}) , valueDefinition,modelComponent),
    GeometryComponentDataItem<double>(id, geometryType, valueDefinition->defaultValue().toDouble()),
    m_gdalDriverName("ESRI Shapefile"),
    m_geometryFileDataField(""),
    m_readFromFile(false),
    m_geometryDimension(geometryDimension)
{
  setIsReadOnly(true);
  setCanReadFromFile(true);
  setCanReadFromString(true);
  addFileFilter("");
}

GeometryArgumentDouble::~GeometryArgumentDouble()
{

}

IGeometry::GeometryType GeometryArgumentDouble::geometryType() const
{
  return getGeometryType();
}

int GeometryArgumentDouble::geometryCount() const
{
  return m_geometries.length();
}

IGeometry *GeometryArgumentDouble::geometry(int geometryIndex) const
{
  return m_geometries[geometryIndex].data();
}

IDimension *GeometryArgumentDouble::geometryDimension() const
{
  return dimensionsInternal()[0];
}

IEnvelope *GeometryArgumentDouble::envelope() const
{
  return envelopeInternal();
}

int GeometryArgumentDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert((int)dimensionIndexes.size() == 0);

  return length();
}

void GeometryArgumentDouble::getValue(const std::vector<int> &dimensionIndexes,  void *data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void GeometryArgumentDouble::getValue(int geometryDimensionIndex, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(geometryDimensionIndex,1,data);
}

void GeometryArgumentDouble::getValues(int geometryDimensionIndex, int stride,  void *data) const
{
  ComponentDataItem1D<double>::getValuesT(geometryDimensionIndex,stride,data);
}

void GeometryArgumentDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void GeometryArgumentDouble::setValue(int geometryDimensionIndex, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(geometryDimensionIndex,1,data);
}

void GeometryArgumentDouble::setValues(int geometryDimensionIndex , int stride, const void* data)
{
  ComponentDataItem1D<double>::setValuesT(geometryDimensionIndex,stride,data);
}

void GeometryArgumentDouble::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("GeometryArgument", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("GeometryArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        while (!(xmlReader.isEndElement()  && !xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Dimension", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();

            if(attributes.hasAttribute("Id"))
            {
              QString id = attributes.value("Id").toString();

              if(m_geometryDimension->id().compare(id))
              {
                throw std::invalid_argument("The id for the specified dimension did not match id expected");
              }
            }

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
      else if(!xmlReader.name().toString().compare("Geometries", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        m_geometries.clear();

        QXmlStreamAttributes attributes = xmlReader.attributes();
        QString fromFile = attributes.value("IsFromFile").toString();

        if(!fromFile.compare("True",Qt::CaseInsensitive))
        {
          m_readFromFile = true;
          m_geometryFile = modelComponentInternal()->getAbsoluteFilePath(xmlReader.readElementText().trimmed());

          if(attributes.hasAttribute("GeometryFileDataField"))
          {
            m_geometryFileDataField = attributes.value("GeometryFileDataField").toString();
          }

          if(attributes.hasAttribute("OverwriteInputGeometryFile"))
          {
            m_saveFile = !attributes.value("OverwriteInputGeometryFile").toString().compare("True" , Qt::CaseInsensitive) ? true : false;
          }

          if(m_geometryFile.isFile() && m_geometryFile.exists())
          {
            QString message;
            if(!GeometryFactory::readGeometryFromFile(m_geometryFile.absoluteFilePath(), m_geometryFileDataField, this, *envelopeInternal() , message))
            {
              //throw std::invalid_argument("Geometry file {" + m_geometryFile.absoluteFilePath().toStdString() + "} could not be read; Message: " + message.toStdString());
            }
          }
        }
        else
        {
          m_readFromFile = false;

          while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Geometries", Qt::CaseInsensitive)) && !xmlReader.hasError())
          {
            if(!xmlReader.name().toString().compare("Geometry", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
            {
              QString wktText = xmlReader.readElementText().trimmed();

              if(!wktText.isNull() && !wktText.isEmpty())
              {
                
                HCGeometry* geometry = GeometryFactory::importFromWkt(wktText);

                assert(getGeometryType() == geometry->geometryType());
                m_geometries.append( QSharedPointer<HCGeometry>(geometry));

                while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Geometry", Qt::CaseInsensitive)) && !xmlReader.hasError())
                {
                  xmlReader.readNext();
                }
              }
            }

            xmlReader.readNext();
          }

          resizeDataArray(m_geometries.length());
        }

        emit propertyChanged("Geometries");

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Geometries", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          xmlReader.readNext();
        }
      }
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        std::vector<double> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QString value = xmlReader.readElementText();
            values.push_back(value.toDouble());
          }

          xmlReader.readNext();
        }

        if((int)values.size() == length())
        {
          ComponentDataItem1D<double>::setValuesT(0,length(),values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void GeometryArgumentDouble::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("GeometryArgument");
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
        xmlWriter.writeAttribute("Id" , m_geometryDimension->id());
        xmlWriter.writeAttribute("Caption" , m_geometryDimension->caption());
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Geometries");
    {
      xmlWriter.writeAttribute("GeometryType" , HCGeometry::geometryTypeToString(getGeometryType()));

      if(m_readFromFile)
      {
        xmlWriter.writeAttribute("IsFromFile", "True");
        xmlWriter.writeAttribute("GeometryFileDataField", m_geometryFileDataField);
        xmlWriter.writeAttribute("GDALDriver", m_gdalDriverName);
        xmlWriter.writeAttribute("OverwriteInputGeometryFile", m_saveFile ? "True" :  "False");
        xmlWriter.writeCharacters(modelComponentInternal()->getRelativeFilePath(m_geometryFile.absoluteFilePath()).filePath());
      }
      else
      {
        xmlWriter.writeAttribute("IsFromFile", "False");

        for(int i =0 ; i < m_geometries.length() ; i++)
        {
          xmlWriter.writeTextElement("Geometry", m_geometries[i]->getWKT());
        }
      }
    }
    xmlWriter.writeEndElement();

    if(m_geometryFileDataField.isEmpty() || m_geometryFileDataField.isNull())
    {
      xmlWriter.writeStartElement("Values");
      {
        double* values = new double[length()];
        getValues(0,length(),values);

        for(int i = 0 ; i < length() ; i++)
        {
          xmlWriter.writeStartElement("Value");
          {
            xmlWriter.writeCharacters(QString::fromStdString(std::to_string(values[i])));
          }
          xmlWriter.writeEndElement();
        }

        delete[] values;
      }
      xmlWriter.writeEndElement();
    }
  }
  xmlWriter.writeEndElement();
}

void GeometryArgumentDouble::saveData()
{
  if(!isReadOnly())
  {
    QString errMsg;

    if(m_geometryFile.absoluteDir().exists())
    {
      if(!GeometryFactory::writeGeometryToFile(this,m_geometryFileDataField, m_gdalDriverName,m_geometryFile.absoluteFilePath(),errMsg))
      {
        throw std::invalid_argument("Geometry file {" + m_geometryFile.filePath().toStdString() + "} could not be written; Message: " + errMsg.toStdString());
      }
      else
      {
        return;
      }
    }
    else
    {
      throw std::invalid_argument("Geometry file {" + m_geometryFile.filePath().toStdString() + "} could not be written because directory does not exist" );
    }
  }
}

QString GeometryArgumentDouble::toString() const
{
  if(currentArgumentIOType() == IArgument::File)
  {
    return  modelComponentInternal()->getRelativeFilePath(m_geometryFile.absoluteFilePath()).filePath();
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

bool GeometryArgumentDouble::readValues(const QString &value, QString &message, bool isFile)
{
  message = "";

  if(isFile)
  {
    m_geometryFile = modelComponentInternal()->getAbsoluteFilePath(value);

    if(m_geometryFile.exists() && m_geometryFile.isFile())
    {
      QString errMsg;

      m_geometries.clear();
      m_geometryFileDataField = "";

      if(!GeometryFactory::readGeometryFromFile(m_geometryFile.absoluteFilePath(), m_geometryFileDataField, this, *envelopeInternal(), errMsg))
      {
        emit propertyChanged("Geometries");
        return false;
      }
      else
      {
        setArgumentIOType(IArgument::File);
        emit propertyChanged("Geometries");
        return true;
      }
    }
  }
  else
  {

    QXmlStreamReader xmlReader(value);

    while(!xmlReader.isEndDocument() && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("GeometryArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
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

bool GeometryArgumentDouble::readValues(const IComponentDataItem *componentDataItem, QString &message)
{
  message = "";

  return false;
}

QList<QSharedPointer<HCGeometry>> GeometryArgumentDouble::geometries() const
{
  return m_geometries;
}
