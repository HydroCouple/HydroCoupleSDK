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
#include "core/exchangeitems1d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"

#include <QDebug>
#include <assert.h>

using namespace HydroCouple;


Input1DInt::Input1DInt(const QString &id,
                       Dimension* dimension,
                       int length,
                       ValueDefinition* valueDefinition,
                       AbstractModelComponent *modelComponent)
  : AbstractInput(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<int>(id, length,valueDefinition->defaultValue().toInt())
{
}

Input1DInt::~Input1DInt()
{
}

int Input1DInt::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  return length();
}

void Input1DInt::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  getValueT(dimensionIndexes,data);
}

void Input1DInt::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  setValueT(dimensionIndexes,data);
}

void Input1DInt::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("ComponentDataItem", Qt::CaseInsensitive)
     && !xmlReader.hasError()
     &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
  {
    QXmlStreamAttributes attributes = xmlReader.attributes();
    
    if(attributes.hasAttribute("Id"))
    {
      QString id = attributes.value("Id").toString();
    }
    
    while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("ComponentDataItem", Qt::CaseInsensitive)) && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        while (!(xmlReader.isEndElement()  && !xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Dimension", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();
            
            if(attributes.hasAttribute("Id") && attributes.hasAttribute("Length"))
            {
              QString id = attributes.value("Id").toString();
              
              if(!dimensions()[0]->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                resizeDataArray(length.toInt());
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
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        std::vector<int> indexes(1);

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();
            
            if(attributes.hasAttribute("Index"))
            {
              int index = attributes.value("Index").toString().toInt();
              indexes[0] = index;

              QString value = xmlReader.readElementText();
              int ivalue = value.toInt();
              setValue(indexes,&ivalue);
            }
          }
          xmlReader.readNext();
        }
      }
      xmlReader.readNext();
    }
  }
}

void Input1DInt::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("ComponentDataItem");
  {
    xmlWriter.writeAttribute("Id" , id());
    xmlWriter.writeAttribute("Caption" , caption());
    
    for(const QString& comment : comments())
    {
      xmlWriter.writeComment(comment);
    }
    
    IDimension* dimension = dimensions()[0];

    xmlWriter.writeStartElement("Dimensions");
    {
      xmlWriter.writeStartElement("Dimension");
      {
        xmlWriter.writeAttribute("Id" , dimension->id());
        xmlWriter.writeAttribute("Caption" , dimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
    
    xmlWriter.writeStartElement("Values");
    {
      int ind[1] = {0};
      int str[1] = {length()};
      int *values = new int[length()];
      ComponentDataItem1D<int>::getValuesT(ind,str,values);
      
      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("Index" , QString::number(i));
          xmlWriter.writeCharacters(QString::number(values[i]));
        }
        xmlWriter.writeEndElement();
      }

      delete[] values;
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}


//==============================================================================================================================


Input1DDouble::Input1DDouble(const QString& id,
                             Dimension* dimension,
                             int dimensionLength,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent *modelComponent)
  : AbstractInput(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<double>(id, dimensionLength,valueDefinition->defaultValue().toDouble())
{
}

Input1DDouble::~Input1DDouble()
{
}

int Input1DDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  return length();
}

void Input1DDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void Input1DDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void Input1DDouble::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("ComponentDataItem", Qt::CaseInsensitive)
     && !xmlReader.hasError()
     &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
  {
    QXmlStreamAttributes attributes = xmlReader.attributes();
    
    if(attributes.hasAttribute("Id"))
    {
      QString id = attributes.value("Id").toString();
    }
    
    while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("ComponentDataItem", Qt::CaseInsensitive)) && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        while (!(xmlReader.isEndElement()  && !xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Dimension", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();
            
            if(attributes.hasAttribute("Id") && attributes.hasAttribute("Length"))
            {
              QString id = attributes.value("Id").toString();
              
              if(!dimensions()[0]->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                resizeDataArray(length.toInt());
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
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        std::vector<int> indexes(1);

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();
            
            if(attributes.hasAttribute("Index"))
            {
              indexes[0] = attributes.value("Index").toString().toInt();
              double value = atof(qPrintable(xmlReader.readElementText()));
              setValue(indexes,&value);
            }
          }
          xmlReader.readNext();
        }
      }
      xmlReader.readNext();
    }
  }
}

void Input1DDouble::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("ComponentDataItem");
  {
    xmlWriter.writeAttribute("Id" , id());
    xmlWriter.writeAttribute("Caption" , caption());
    
    for(const QString& comment : comments())
    {
      xmlWriter.writeComment(comment);
    }
    
    IDimension* dimension = dimensions()[0];

    xmlWriter.writeStartElement("Dimensions");
    {
      xmlWriter.writeStartElement("Dimension");
      {
        xmlWriter.writeAttribute("Id" , dimension->id());
        xmlWriter.writeAttribute("Caption" , dimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
    
    xmlWriter.writeStartElement("Values");
    {
      int ind[1] = {0};
      int str[1] = {length()};
      int *values = new int[length()];
      ComponentDataItem1D<double>::getValuesT(ind,str,values);
      
      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("Index" , QString::number(i));
          xmlWriter.writeCharacters(QString::number(values[i]));
        }
        xmlWriter.writeEndElement();
      }

      delete[] values;
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

//==============================================================================================================================

Input1DString::Input1DString(const QString& id,
                             Dimension* dimension,
                             int length,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<QString>(id, length,valueDefinition->defaultValue().toString())
{
}

Input1DString::~Input1DString()
{
}

int Input1DString::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void Input1DString::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  getValueT(dimensionIndexes,data);
}

void Input1DString::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  setValueT(dimensionIndexes,data);
}

void Input1DString::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("ComponentDataItem", Qt::CaseInsensitive)
     && !xmlReader.hasError()
     &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
  {
    QXmlStreamAttributes attributes = xmlReader.attributes();
    
    if(attributes.hasAttribute("Id"))
    {
      QString id = attributes.value("Id").toString();
    }
    

    while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("ComponentDataItem", Qt::CaseInsensitive)) && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        while (!(xmlReader.isEndElement()  && !xmlReader.name().toString().compare("Dimensions", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Dimension", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();
            
            if(attributes.hasAttribute("Id") && attributes.hasAttribute("Length"))
            {
              QString id = attributes.value("Id").toString();
              
              if(!dimensions()[0]->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                resizeDataArray(length.toInt());
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
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        std::vector<int> indexes(1);

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();
            
            if(attributes.hasAttribute("Index"))
            {
              indexes[0] = attributes.value("Index").toString().toInt();
              QString value = xmlReader.readElementText();
              setValue(indexes,&value);
            }
          }
          xmlReader.readNext();
        }
      }
      xmlReader.readNext();
    }
  }
}

void Input1DString::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("ComponentDataItem");
  {
    xmlWriter.writeAttribute("Id" , id());
    xmlWriter.writeAttribute("Caption" , caption());
    
    for(const QString& comment : comments())
    {
      xmlWriter.writeComment(comment);
    }
    
    IDimension* dimension = dimensions()[0];

    xmlWriter.writeStartElement("Dimensions");
    {
      xmlWriter.writeStartElement("Dimension");
      {
        xmlWriter.writeAttribute("Id" , dimension->id());
        xmlWriter.writeAttribute("Caption" , dimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
    
    xmlWriter.writeStartElement("Values");
    {
      int ind[1] = {0};
      int str[1] = {length()};
      QString *values = new QString[length()];
      getValuesT(ind,str,values);
      
      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("Index" , QString::number(i));
          xmlWriter.writeCharacters(values[i]);
        }
        xmlWriter.writeEndElement();
      }
      
      delete[] values;
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<QString>;
