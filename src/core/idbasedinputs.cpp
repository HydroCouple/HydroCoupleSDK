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
#include "core/idbasedinputs.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/abstractmodelcomponent.h"

#include <assert.h>

using namespace HydroCouple;


IdBasedInputInt::IdBasedInputInt(const QString& id,
                                 const QStringList& identifiers,
                                 Dimension* dimension,
                                 ValueDefinition* valueDefinition,
                                 AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<int>(id, identifiers,valueDefinition->defaultValue().toInt()),
    m_identifierDimension(dimension)
{

}

IdBasedInputInt::~IdBasedInputInt()
{
}

QStringList IdBasedInputInt::identifiers() const
{
  return IdBasedComponentDataItem<int>::identifiersInternal();
}

IDimension* IdBasedInputInt::identifierDimension() const
{
  return m_identifierDimension;
}

int IdBasedInputInt::dimensionLength(const std::vector<int> &dimensionIndexes ) const
{
  assert(dimensionIndexes.size() == 0);
  return length();
}

void IdBasedInputInt::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<int>::getValueT(dimensionIndexes,data);
}

void IdBasedInputInt::getValue(int idIndex, void *data) const
{
  ComponentDataItem1D<int>::getValuesT(idIndex,1,data);
}

void IdBasedInputInt::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<int>::getValuesT(&idIndex,&stride,data);
}

void IdBasedInputInt::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<int>::setValueT(dimensionIndexes,data);
}

void IdBasedInputInt::setValue(int idIndex, const void *data)
{
  ComponentDataItem1D<int>::setValuesT(idIndex,1,data);
}

void IdBasedInputInt::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<int>::setValuesT(&idIndex,&stride,data);
}

void IdBasedInputInt::readData(QXmlStreamReader &xmlReader)
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

              if(!m_identifierDimension->id().compare(id))
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
        resetDataArray();
      }
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        clearIdentifiers();
        QList<QString> tempIdentifiers;
        std::vector<int> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();

            if(attributes.hasAttribute("Id"))
            {
              QString index = attributes.value("Id").toString();
              tempIdentifiers.append(index);
              QString value = xmlReader.readElementText();
              values.push_back(value.toInt());
            }
          }
          xmlReader.readNext();
        }

        if(tempIdentifiers.length() && tempIdentifiers.length() == (int)values.size())
        {
          addIdentifiers(tempIdentifiers);
          setValues(0,tempIdentifiers.length(),values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void IdBasedInputInt::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("ComponentDataItem");
  {
    xmlWriter.writeAttribute("Id" , id());
    xmlWriter.writeAttribute("Caption" , caption());

    for(const QString& comment : comments())
    {
      xmlWriter.writeComment(comment);
    }

    xmlWriter.writeStartElement("Dimensions");
    {
      xmlWriter.writeStartElement("Dimension");
      {
        xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
        xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
//      int ind[1] = {0};
//      int str[1] = {length()};
      int* values = new int[length()];
      getValues(0,length(),values);

      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("Id", identifiers()[i]);
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

IdBasedInputDouble::IdBasedInputDouble(const QString& id,
                                       const QStringList& identifiers,
                                       Dimension* identifierDimension,
                                       ValueDefinition* valueDefinition,
                                       AbstractModelComponent *modelComponent)
  : AbstractInput(id,QList<Dimension*>({identifierDimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<double>(id, identifiers,valueDefinition->defaultValue().toDouble()),
    m_identifierDimension(identifierDimension)
{
}

IdBasedInputDouble::~IdBasedInputDouble()
{

}

QStringList IdBasedInputDouble::identifiers() const
{
  return IdBasedComponentDataItem<double>::identifiersInternal();
}

IDimension* IdBasedInputDouble::identifierDimension() const
{
  return m_identifierDimension;
}

int IdBasedInputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  return length();
}

void IdBasedInputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void IdBasedInputDouble::getValue(int idIndex, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(idIndex,1,data);
}

void IdBasedInputDouble::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(&idIndex,&stride,data);
}

void IdBasedInputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void IdBasedInputDouble::setValue(int idIndex, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(idIndex,1,data);
}

void IdBasedInputDouble::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(&idIndex,&stride,data);
}

void IdBasedInputDouble::readData(QXmlStreamReader &xmlReader)
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

              if(!m_identifierDimension->id().compare(id))
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
        resetDataArray();
      }
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        clearIdentifiers();
        QList<QString> tempIdentifiers;
        std::vector<double> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();

            if(attributes.hasAttribute("Id"))
            {
              QString index = attributes.value("Id").toString();
              tempIdentifiers.append(index);
              QString value = xmlReader.readElementText();
              values.push_back(value.toDouble());
            }
          }
          xmlReader.readNext();
        }

        if(tempIdentifiers.length() && tempIdentifiers.length() == (int)values.size())
        {
          addIdentifiers(tempIdentifiers);
          setValues(0,tempIdentifiers.length(),values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void IdBasedInputDouble::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("ComponentDataItem");
  {
    xmlWriter.writeAttribute("Id" , id());
    xmlWriter.writeAttribute("Caption" , caption());

    for(const QString& comment : comments())
    {
      xmlWriter.writeComment(comment);
    }

    xmlWriter.writeStartElement("Dimensions");
    {
      xmlWriter.writeStartElement("Dimension");
      {
        xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
        xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
//      int ind[1] = {0};
//      int str[1] = {length()};
      double *values = new double [length()];
      getValues(0,length(),values);

      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("Id", identifiers()[i]);
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

IdBasedInputString::IdBasedInputString(const QString &id, const QStringList &identifiers,
                                                               Dimension *identifierDimension,
                                                               ValueDefinition* valueDefinition,
                                                               AbstractModelComponent *modelComponent)
  : AbstractInput(id,QList<Dimension*>({identifierDimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<QString>(id, identifiers,valueDefinition->defaultValue().toString()),
    m_identifierDimension(identifierDimension)
{
}

IdBasedInputString::~IdBasedInputString()
{

}

QStringList IdBasedInputString::identifiers() const
{
  return IdBasedComponentDataItem<QString>::identifiersInternal();
}

IDimension* IdBasedInputString::identifierDimension() const
{
  return m_identifierDimension;
}

int IdBasedInputString::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  return length();
}

void IdBasedInputString::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<QString>::getValueT(dimensionIndexes,data);
}

void IdBasedInputString::getValue(int idIndex, void *data) const
{
  ComponentDataItem1D<QString>::getValuesT(idIndex,1,data);
}

void IdBasedInputString::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<QString>::getValuesT(&idIndex,&stride,data);
}

void IdBasedInputString::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<QString>::setValueT(dimensionIndexes,data);
}

void IdBasedInputString::setValue(int idIndex, const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(idIndex,1,data);
}

void IdBasedInputString::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(&idIndex,&stride,data);
}

void IdBasedInputString::readData(QXmlStreamReader &xmlReader)
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

              if(!m_identifierDimension->id().compare(id))
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
        resetDataArray();
      }
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        clearIdentifiers();
        QList<QString> tempIdentifiers;
        std::vector<QString> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();

            if(attributes.hasAttribute("Id"))
            {
              QString index = attributes.value("Id").toString();
              tempIdentifiers.append(index);
              QString value = xmlReader.readElementText();
              values.push_back(value);
            }
          }
          xmlReader.readNext();
        }

        if(tempIdentifiers.length() && tempIdentifiers.length() == (int)values.size())
        {
          addIdentifiers(tempIdentifiers);
          setValues(0,tempIdentifiers.length(),values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void IdBasedInputString::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("ComponentDataItem");
  {
    xmlWriter.writeAttribute("Id" , id());
    xmlWriter.writeAttribute("Caption" , caption());

    for(const QString& comment : comments())
    {
      xmlWriter.writeComment(comment);
    }

    xmlWriter.writeStartElement("Dimensions");
    {
      xmlWriter.writeStartElement("Dimension");
      {
        xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
        xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
//      int ind[1] = {0};
//      int str[1] = {length()};
      QString* values = new QString[length()];
      getValues(0,length(),values);

      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("Id", identifiers()[i]);
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
