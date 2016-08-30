#include "stdafx.h"
#include "spatial/geometryargument.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "spatial/geometrycollection.h"
#include "spatial/geometryfactory.h"
#include "core/abstractmodelcomponent.h"

#include <QFileInfo>
#include <QDir>
#include <assert.h>
#include <QXmlStreamReader>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCGeometryArgumentDouble::HCGeometryArgumentDouble(const QString &id,
                                                   GeometryType geometryType,
                                                   Dimension *geometryDimension,
                                                   ValueDefinition *valueDefinition,
                                                   AbstractModelComponent *modelComponent)
  : AbstractArgument(id , QList<Dimension*>({geometryDimension}) , valueDefinition,modelComponent),
    ComponentDataItem1D<double>(0, valueDefinition->defaultValue().toDouble()),
    m_geometryType(geometryType),
    m_gdalDriverName("ESRI Shapefile"),
    m_geometryDimension(geometryDimension),
    m_geometryFileDataField("DataField"),
    m_overWriteFile(false)
{
  setCanReadFromFile(true);
  setCanReadFromString(true);
  addInputFileTypeFilter("");
}

HCGeometryArgumentDouble::~HCGeometryArgumentDouble()
{
}

GeometryType HCGeometryArgumentDouble::geometryType() const
{
  return m_geometryType;
}

int HCGeometryArgumentDouble::geometryCount() const
{
  return m_geometries.length();
}

IGeometry *HCGeometryArgumentDouble::geometry(int geometryIndex) const
{
  return m_geometries[geometryIndex];
}

IDimension *HCGeometryArgumentDouble::geometryDimension() const
{
  return dimensionsInternal()[0];
}

void HCGeometryArgumentDouble::addGeometry(HCGeometry *geometry )
{
  assert(geometry->geometryType() == m_geometryType);
  m_geometries.append(geometry);
  resetDataArray(m_geometries.length());
}

void HCGeometryArgumentDouble::addGeometries(QList<HCGeometry*> geometries)
{
  for(HCGeometry *geometry :geometries)
  {
    assert(geometry->geometryType() == geometryType());
    m_geometries.append(geometry);
  }

  resetDataArray(m_geometries.length());
}

bool HCGeometryArgumentDouble::removeGeometry(HCGeometry *geometry)
{
  bool removed = m_geometries.removeOne(geometry);

  if(removed)
  {
    resetDataArray(m_geometries.length());
  }

  return removed;
}

int HCGeometryArgumentDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  return length();
}

void HCGeometryArgumentDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void HCGeometryArgumentDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void HCGeometryArgumentDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void HCGeometryArgumentDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void HCGeometryArgumentDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void HCGeometryArgumentDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void HCGeometryArgumentDouble::getValue(int geometryDimensionIndex, QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(&geometryDimensionIndex,data);
}

void HCGeometryArgumentDouble::getValues(int geometryDimensionIndex, int stride, QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(&geometryDimensionIndex,&stride,data);
}

void HCGeometryArgumentDouble::getValues(int geometryDimensionIndex, int stride, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(&geometryDimensionIndex,&stride,data);
}

void HCGeometryArgumentDouble::setValue(int geometryDimensionIndex, const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(&geometryDimensionIndex,data);
}

void HCGeometryArgumentDouble::setValues(int geometryDimensionIndex, int stride, const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(&geometryDimensionIndex,&stride,data);
}

void HCGeometryArgumentDouble::setValues(int geometryDimensionIndex, int stride, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(&geometryDimensionIndex,&stride,data);
}

void HCGeometryArgumentDouble::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().compare("GeometryArgument", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().compare("GeometryArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
    {
      if(!xmlReader.name().compare("Dimensions", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        while (!(xmlReader.isEndElement()  && !xmlReader.name().compare("Dimensions", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Dimension", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
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

            while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Dimension", Qt::CaseInsensitive)) && !xmlReader.hasError())
            {
              xmlReader.readNext();
            }
          }
          xmlReader.readNext();
        }
      }
      else if(!xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        valueDefinitionInternal()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          xmlReader.readNext();
        }
      }
      else if(!xmlReader.name().compare("Geometries", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        m_geometries.clear();

        QXmlStreamAttributes attributes = xmlReader.attributes();
        QString fromFile = attributes.value("IsFromFile").toString();

        if(!fromFile.compare("True",Qt::CaseInsensitive))
        {
          m_writeToFile = true;
          m_geometryFile = modelComponentInternal()->getRelativeFilePath(xmlReader.readElementText().trimmed());

          if(attributes.hasAttribute("GeometryFileDataField"))
          {
            m_geometryFileDataField = attributes.value("GeometryFileDataField").toString();
          }

          if(attributes.hasAttribute("OverwriteInputGeometryFile"))
          {
            m_geometryFileDataField = !attributes.value("OverwriteInputGeometryFile").toString().compare("True" , Qt::CaseInsensitive) ? true : false;
          }

          if(!m_geometryFile.exists())
          {
            throw std::invalid_argument("Geometry file {" + m_geometryFile.filePath().toStdString() + "} could not be found");
          }

          QString message;

          if(!GeometryFactory::readGeometryDataItemFromFile(m_geometryFile.absoluteFilePath(), m_geometryFileDataField, this, message))
          {
            throw std::invalid_argument("Geometry file {" + m_geometryFile.absoluteFilePath().toStdString() + "} could not be read; Message: " + message.toStdString());
          }
        }
        else
        {
          m_writeToFile = false;

          while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Geometries", Qt::CaseInsensitive)) && !xmlReader.hasError())
          {
            if(!xmlReader.name().compare("Geometry", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
            {
              HCGeometry* geometry = GeometryFactory::importFromWkt(xmlReader.readElementText().trimmed());

              assert(m_geometryType == geometry->geometryType());
              m_geometries.append(geometry);

              while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Geometry", Qt::CaseInsensitive)) && !xmlReader.hasError())
              {
                xmlReader.readNext();
              }
            }

            xmlReader.readNext();
          }

          resetDataArray(m_geometries.length());
        }

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Geometries", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          xmlReader.readNext();
        }
      }
      else if(!xmlReader.name().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        std::vector<double> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QString value = xmlReader.readElementText();
            values.push_back(value.toDouble());
          }

          xmlReader.readNext();
        }

        if((int)values.size() == length())
        {
          int ind[1] ={0};
          int str[1] = {length()};
          setValues(ind,str,values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void HCGeometryArgumentDouble::writeData(QXmlStreamWriter &xmlWriter)
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
      xmlWriter.writeAttribute("GeometryType" , HCGeometry::geometryTypeToString(m_geometryType));

      if(m_writeToFile)
      {
        xmlWriter.writeAttribute("IsFromFile", "True");
        xmlWriter.writeAttribute("GeometryFileDataField", m_geometryFileDataField);
        xmlWriter.writeAttribute("OverwriteInputGeometryFile", m_overWriteFile ? "True" :  "False");
        xmlWriter.writeCharacters(QFileInfo(modelComponent()->componentInfo()->libraryFilePath()).absoluteDir().relativeFilePath(m_geometryFile.absoluteFilePath()));
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

    if(!m_writeToFile)
    {
      xmlWriter.writeStartElement("Values");
      {
        int ind[1] = {0};
        int str[1] = {length()};
        double* values = new double[length()];
        getValues(ind,str,values);


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

bool HCGeometryArgumentDouble::writeToFile() const
{
  if((currentArgumentIOType() == HydroCouple::File ||
     m_writeToFile) && m_overWriteFile)
  {

    QString errMsg;

    if(m_geometryFile.absoluteDir().exists())
    {
      if(!GeometryFactory::writeGeometryDataItemToFile(this,m_geometryFileDataField, m_gdalDriverName,m_geometryFile.absoluteFilePath(),errMsg))
      {
        throw std::invalid_argument("Geometry file {" + m_geometryFile.filePath().toStdString() + "} could not be written; Message: " + errMsg.toStdString());
      }
      else
      {
        return true;
      }
    }
    else
    {
       throw std::invalid_argument("Geometry file {" + m_geometryFile.filePath().toStdString() + "} could not be written because directory does not exist" );
    }
  }

  return false;
}

QString HCGeometryArgumentDouble::toString() const
{
  if(currentArgumentIOType() == HydroCouple::File)
  {
    return  QFileInfo(modelComponent()->componentInfo()->libraryFilePath()).absoluteDir().relativeFilePath(m_geometryFile.absoluteFilePath());
  }
  else
  {
    QString data;
    QXmlStreamWriter xmlWriter(&data);
    xmlWriter.setAutoFormatting(true);

    xmlWriter.writeStartDocument();
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
          xmlWriter.writeAttribute("GeometryType" , HCGeometry::geometryTypeToString(m_geometryType));

          if(m_writeToFile)
          {
            xmlWriter.writeAttribute("IsFromFile", "True");
            xmlWriter.writeAttribute("GeometryFileDataField", m_geometryFileDataField);
            xmlWriter.writeCharacters(QFileInfo(modelComponent()->componentInfo()->libraryFilePath()).absoluteDir().relativeFilePath(m_geometryFile.absoluteFilePath()));
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

        if(!m_writeToFile)
        {
          xmlWriter.writeStartElement("Values");
          {
            int ind[1] = {0};
            int str[1] = {length()};
            double *values = new double[length()];
            getValues(ind,str,values);

            for(int i = 0 ; i < length() ; i++)
            {
              xmlWriter.writeStartElement("Value");
              {
                xmlWriter.writeCharacters(QString::fromStdString(std::to_string(values[i])));
              }
              xmlWriter.writeEndElement();
            }

            delete [] values;
          }
          xmlWriter.writeEndElement();
        }
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndDocument();

    return data;
  }
}

bool HCGeometryArgumentDouble::readValues(const QString &value, bool isFile)
{
  if(isFile)
  {
    m_geometryFile = modelComponentInternal()->getRelativeFilePath(value);

    if(m_geometryFile.exists())
    {
      QString errMsg;

      m_geometries.clear();
      m_geometryFileDataField = "";

      if(!GeometryFactory::readGeometryDataItemFromFile(m_geometryFile.absoluteFilePath(), m_geometryFileDataField , this,errMsg))
      {
         throw std::invalid_argument("Geometry file {" + m_geometryFile.absoluteFilePath().toStdString() + "} could not be read: Message: " + errMsg.toStdString());
      }
      else
      {
        return true;
      }
    }
  }
  else
  {

    QXmlStreamReader xmlReader(value);

    while(!xmlReader.isEndDocument() && !xmlReader.hasError())
    {
      if(!xmlReader.name().compare("GeometryArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        readData(xmlReader);
      }

      xmlReader.readNext();
    }

    if(!xmlReader.hasError())
    {
      setArgumentIOType(HydroCouple::String);
      return true;
    }
  }

  return false;
}

bool HCGeometryArgumentDouble::readValues(const IComponentDataItem *componentDataItem)
{

  return false;
}

QList<HCGeometry*> HCGeometryArgumentDouble::geometries() const
{
  return m_geometries;
}
