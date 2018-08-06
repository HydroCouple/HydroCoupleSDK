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
#include "core/idbasedargument.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/idbasedinputs.h"
#include "core/abstractmodelcomponent.h"

#include <QString>
#include <QDebug>
#include <assert.h>

using namespace HydroCouple;

IdBasedArgumentInt::IdBasedArgumentInt(const QString& id,
                                       const QStringList& identifiers,
                                       Dimension* identifierDimension,
                                       ValueDefinition* valueDefinition,
                                       AbstractModelComponent* modelComponent)
  : AbstractArgument(id,QList<Dimension*>({identifierDimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<int>(id, identifiers,valueDefinition->defaultValue().toInt()),
    m_matchIdentifiersDuringRead(false),
    m_identifierDimension(identifierDimension)
{

}

IdBasedArgumentInt::~IdBasedArgumentInt()
{

}

QStringList IdBasedArgumentInt::identifiers() const
{
  return IdBasedComponentDataItem<int>::identifiersInternal();
}

IDimension* IdBasedArgumentInt::identifierDimension() const
{
  return m_identifierDimension;
}

int IdBasedArgumentInt::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);

  return length();
}

void IdBasedArgumentInt::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<int>::getValueT(dimensionIndexes,data);
}

void IdBasedArgumentInt::getValue(int idIndex, void *data) const
{
  ComponentDataItem1D<int>::getValuesT(idIndex,1,data);
}

void IdBasedArgumentInt::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<int>::getValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentInt::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<int>::setValueT(dimensionIndexes,data);
}

void IdBasedArgumentInt::setValue(int idIndex, const void *data)
{
  ComponentDataItem1D<int>::setValuesT(idIndex,1,data);
}

void IdBasedArgumentInt::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<int>::setValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentInt::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
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

              //              if(!m_identifierDimension->id().compare(id))
              //              {
              ////                QString length = attributes.value("Length").toString();

              ////                if(m_matchIdentifiersDuringRead &&
              ////                   length.toInt() != this->length())
              ////                {
              ////                  return;
              ////                }
              ////                if(!matchIdentifiersWhenReading())
              ////                {
              ////                  resizeDataArray(length.toInt());
              ////                }
              //              }
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
      else if(!xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        valueDefinitionInternal()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
        }
      }
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

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
              int value = atoi(qPrintable(xmlReader.readElementText()));
              values.push_back(value);
            }
          }
          xmlReader.readNext();
        }

        if(tempIdentifiers.length() && tempIdentifiers.length() == (int)values.size())
        {
          if(m_matchIdentifiersDuringRead)
          {
            QStringList currentIdentifiers = identifiers();
            int curId = 0;
            for(QString id : currentIdentifiers)
            {
              if(tempIdentifiers.contains(id))
              {
                int nId = tempIdentifiers.indexOf(id);
                setValues(curId,1,&values[nId]);
              }

              curId ++;
            }
          }
          else
          {
            clearIdentifiers();
            addIdentifiers(tempIdentifiers);
            setValues(0,tempIdentifiers.length(),values.data());
          }
        }
      }
      xmlReader.readNext();
    }
  }
}

void IdBasedArgumentInt::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("IdBasedArgument");
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
        xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
        xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      int *values = new int[length()];
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

void IdBasedArgumentInt::saveData()
{
  if(!isReadOnly() && currentArgumentIOType() == IArgument::File)
  {
    QFile file(m_inputFile.absoluteFilePath());

    if(file.open(QIODevice::WriteOnly))
    {
      QXmlStreamWriter xmlWriter(&file);
      xmlWriter.setAutoFormatting(true);

      xmlWriter.writeStartElement("IdBasedArgument");
      {
        xmlWriter.writeAttribute("Id" , id());
        xmlWriter.writeAttribute("Caption" , caption());
        xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

        //write value definition;
        valueDefinitionInternal()->writeData(xmlWriter);

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
          //          int ind[1] = {0};
          //          int str[1] = {length()};
          int *values = new int[length()];
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

      file.close();

      return;
    }
  }
}

QString IdBasedArgumentInt::toString() const
{
  if(currentArgumentIOType() == IArgument::File)
  {
    return modelComponentInternal()->getRelativeFilePath(m_inputFile.absoluteFilePath()).fileName();
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

bool IdBasedArgumentInt::readValues(const QString &value, QString &message, bool isFile)
{
  message ="";

  if(isFile)
  {
    m_inputFile = modelComponentInternal()->getRelativeFilePath(value);

    if(m_inputFile.exists())
    {
      QFile file(m_inputFile.absoluteFilePath());

      if(file.open(QIODevice::ReadOnly))
      {
        QXmlStreamReader xmlReader(&file);

        while(!xmlReader.isEndDocument() && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            readData(xmlReader);
          }
          xmlReader.readNext();
        }

        if(!xmlReader.hasError())
        {
          setArgumentIOType(ArgumentIOType::File);
          return true;
        }
      }
    }
  }
  else
  {
    QXmlStreamReader xmlReader(value);

    while(!xmlReader.isEndDocument() && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        readData(xmlReader);
      }

      xmlReader.readNext();
    }

    if(!xmlReader.hasError())
    {
      setArgumentIOType(ArgumentIOType::String);
      return true;
    }
  }

  return false;
}

bool IdBasedArgumentInt::readValues(const IComponentDataItem *componentDataItem, QString &message)
{
  message = "";

  const IIdBasedComponentDataItem *idBasedData = dynamic_cast<const IIdBasedComponentDataItem*>(componentDataItem);

  if(idBasedData)
  {
    if(valueDefinitionInternal()->type() == idBasedData->valueDefinition()->type() &&
       idBasedData->dimensions().length() == 1)
    {
      QStringList inputIdentifiers = idBasedData->identifiers();

      if(m_matchIdentifiersDuringRead)
      {
        QStringList currentIdentifiers = identifiers();
        int curId = 0;
        for(QString id : currentIdentifiers)
        {
          if(inputIdentifiers.contains(id))
          {
            int nId = inputIdentifiers.indexOf(id);
            int value = 0;
            idBasedData->getValues(nId,1,&value);
            setValues(curId,1,&value);
          }

          curId ++;
        }

        return true;
      }
      else
      {
        int dimLength = idBasedData->dimensionLength(std::vector<int>(0));
        clearIdentifiers();
        addIdentifiers(inputIdentifiers);
        int *values = new int[dimLength];
        idBasedData->getValues(0,dimLength,values);
        setValues(0,dimLength,values);

        delete[] values;

        return true;
      }
    }
  }

  return false;
}

bool IdBasedArgumentInt::matchIdentifiersWhenReading() const
{
  return m_matchIdentifiersDuringRead;
}

void IdBasedArgumentInt::setMatchIdentifiersWhenReading(bool match)
{
  m_matchIdentifiersDuringRead = match;
}

//==============================================================================================================================

IdBasedArgumentDouble::IdBasedArgumentDouble(const QString &id, const QStringList &identifiers,
                                             Dimension* identifierDimension,
                                             ValueDefinition* valueDefinition,
                                             AbstractModelComponent *modelComponent)
  : AbstractArgument(id,QList<Dimension*>({identifierDimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<double>(id, identifiers,valueDefinition->defaultValue().toDouble()),
    m_matchIdentifiersDuringRead(false),
    m_identifierDimension(identifierDimension)
{

}

IdBasedArgumentDouble::~IdBasedArgumentDouble()
{
}

QStringList IdBasedArgumentDouble::identifiers() const
{
  return IdBasedComponentDataItem<double>::identifiersInternal();
}

IDimension* IdBasedArgumentDouble::identifierDimension() const
{
  return m_identifierDimension;
}

int IdBasedArgumentDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);

  return length();
}

void IdBasedArgumentDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void IdBasedArgumentDouble::getValue(int idIndex, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(idIndex,1,data);
}

void IdBasedArgumentDouble::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void IdBasedArgumentDouble::setValue(int idIndex, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(idIndex,1,data);
}

void IdBasedArgumentDouble::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentDouble::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
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

                //                if(m_matchIdentifiersDuringRead &&
                //                   length.toInt() != this->length())
                //                {
                //                  return;
                //                }
                //                else
                //                {
                //                  resizeDataArray(length.toInt());
                //                }
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
        }
      }
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

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
              double value = atof(qPrintable(xmlReader.readElementText()));
              values.push_back(value);
            }
          }
          xmlReader.readNext();
        }

        if(tempIdentifiers.length() && tempIdentifiers.length() == (int)values.size())
        {
          if(m_matchIdentifiersDuringRead)
          {
            int curId = 0;

            for(QString id : tempIdentifiers)
            {
              (*this)[id] = values[curId];
              curId ++;
            }
          }
          else
          {
            clearIdentifiers();
            addIdentifiers(tempIdentifiers);
            setValues(0,tempIdentifiers.length(),values.data());
          }
        }
      }
      xmlReader.readNext();
    }
  }
}

void IdBasedArgumentDouble::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("IdBasedArgument");
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
        xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
        xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      double *values = new double[length()];
      getValues(0,length(),values);

      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("Id", identifiers()[i]);
          xmlWriter.writeCharacters(QString::number(values[i],'g',12));
        }
        xmlWriter.writeEndElement();
      }

      delete[] values;
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

void IdBasedArgumentDouble::saveData()
{
  if(currentArgumentIOType() == IArgument::File)
  {
    QFile file(m_inputFile.absoluteFilePath());

    if(file.open(QIODevice::WriteOnly))
    {
      QXmlStreamWriter xmlWriter(&file);
      xmlWriter.setAutoFormatting(true);

      xmlWriter.writeStartElement("IdBasedArgument");
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
            xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
            xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
            xmlWriter.writeAttribute("Length" , QString::number(length()));
          }
          xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("Values");
        {
          double *values = new double[length()];
          getValues(0,length(),values);

          for(int i = 0 ; i < length() ; i++)
          {
            xmlWriter.writeStartElement("Value");
            {
              xmlWriter.writeAttribute("Id", identifiers()[i]);
              xmlWriter.writeCharacters(QString::number(values[i],'g',12));
            }
            xmlWriter.writeEndElement();
          }

          delete[] values;
        }
        xmlWriter.writeEndElement();
      }
      xmlWriter.writeEndElement();

      file.close();

      return;
    }
  }
}

QString IdBasedArgumentDouble::toString() const
{
  if(currentArgumentIOType() == IArgument::File)
  {
    return  modelComponentInternal()->getRelativeFilePath(m_inputFile.absoluteFilePath()).fileName();
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

    //    xmlWriter.writeStartDocument();
    //    {
    //      xmlWriter.writeStartElement("IdBasedArgument");
    //      {
    //        xmlWriter.writeAttribute("Id" , id());
    //        xmlWriter.writeAttribute("Caption" , caption());
    //        xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

    //        for(const QString& comment : comments())
    //        {
    //          xmlWriter.writeComment(comment);
    //        }

    //        //write value definition;
    //        valueDefinitionInternal()->writeData(xmlWriter);

    //        xmlWriter.writeStartElement("Dimensions");
    //        {
    //          xmlWriter.writeStartElement("Dimension");
    //          {
    //            xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
    //            xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
    //            xmlWriter.writeAttribute("Length" , QString::number(length()));
    //          }
    //          xmlWriter.writeEndElement();
    //        }
    //        xmlWriter.writeEndElement();

    //        xmlWriter.writeStartElement("Values");
    //        {
    //          double values[length()];
    //          getValues(0,length(),values);

    //          for(int i = 0 ; i < length() ; i++)
    //          {
    //            xmlWriter.writeStartElement("Value");
    //            {
    //              xmlWriter.writeAttribute("Id", identifiers()[i]);
    //              xmlWriter.writeCharacters(QString::number(values[i]));
    //            }
    //            xmlWriter.writeEndElement();
    //          }
    //        }
    //        xmlWriter.writeEndElement();
    //      }
    //      xmlWriter.writeEndElement();
    //    }
    //    xmlWriter.writeEndDocument();

    return data;
  }
}

bool IdBasedArgumentDouble::readValues(const QString &value, QString &message, bool isFile)
{
  message = "";

  if(isFile)
  {
    m_inputFile = modelComponentInternal()->getRelativeFilePath(value);

    if(m_inputFile.exists())
    {
      QFile file(m_inputFile.absoluteFilePath());

      if(file.open(QIODevice::ReadOnly))
      {
        QXmlStreamReader xmlReader(&file);

        while(!xmlReader.isEndDocument() && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            readData(xmlReader);
          }
          xmlReader.readNext();
        }

        if(!xmlReader.hasError())
        {
          setArgumentIOType(ArgumentIOType::File);
          return true;
        }
      }
    }
  }
  else
  {
    QXmlStreamReader xmlReader(value);

    while(!xmlReader.isEndDocument() && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        readData(xmlReader);
      }

      xmlReader.readNext();
    }

    if(!xmlReader.hasError())
    {
      setArgumentIOType(ArgumentIOType::String);
      return true;
    }
  }

  return false;
}

bool IdBasedArgumentDouble::readValues(const IComponentDataItem *componentDataItem, QString &message)
{
  message = "";

  const IIdBasedComponentDataItem *idBasedData = dynamic_cast<const IIdBasedComponentDataItem*>(componentDataItem);

  if(idBasedData)
  {
    if(valueDefinitionInternal()->type() == idBasedData->valueDefinition()->type())
    {
      QStringList inputIdentifiers = idBasedData->identifiers();

      if(m_matchIdentifiersDuringRead)
      {
        QStringList currentIdentifiers = identifiers();
        int curId = 0;
        for(QString id : currentIdentifiers)
        {
          if(inputIdentifiers.contains(id))
          {
            int nId = inputIdentifiers.indexOf(id);
            double value = 0;
            idBasedData->getValues(nId,1,&value);
            setValues(curId,1,&value);
          }

          curId ++;
        }

        return true;
      }
      else
      {
        int dimLength  = idBasedData->dimensionLength(std::vector<int>());
        clearIdentifiers();
        addIdentifiers(inputIdentifiers);
        int *values = new int[dimLength];
        idBasedData->getValues(0,dimLength,values);
        setValues(0,dimLength,values);

        delete[] values;

        return true;
      }
    }
  }

  return false;
}

bool IdBasedArgumentDouble::matchIdentifiersWhenReading() const
{
  return m_matchIdentifiersDuringRead;
}

void IdBasedArgumentDouble::setMatchIdentifiersWhenReading(bool match)
{
  m_matchIdentifiersDuringRead = match;
}

//==============================================================================================================================


IdBasedArgumentString::IdBasedArgumentString(const QString &id,
                                             const QStringList &identifiers,
                                             Dimension* identifierDimension,
                                             ValueDefinition* valueDefinition,
                                             AbstractModelComponent *modelComponent)
  : AbstractArgument(id,QList<Dimension*>({identifierDimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<QString>(id, identifiers,valueDefinition->defaultValue().toString()),
    m_matchIdentifiersDuringRead(false),
    m_identifierDimension(identifierDimension)
{

}

IdBasedArgumentString::~IdBasedArgumentString()
{

}

QStringList IdBasedArgumentString::identifiers() const
{
  return IdBasedComponentDataItem<QString>::identifiersInternal();
}

IDimension* IdBasedArgumentString::identifierDimension() const
{
  return m_identifierDimension;
}

int IdBasedArgumentString::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);

  return length();
}

void IdBasedArgumentString::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<QString>::getValueT(dimensionIndexes,data);
}

void IdBasedArgumentString::getValue(int idIndex, void *data) const
{
  ComponentDataItem1D<QString>::getValuesT(idIndex,1,data);
}

void IdBasedArgumentString::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<QString>::getValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentString::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<QString>::setValueT(dimensionIndexes,data);
}

void IdBasedArgumentString::setValue(int idIndex, const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(idIndex,1,data);
}

void IdBasedArgumentString::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentString::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
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

                //                if(m_matchIdentifiersDuringRead && length.toInt() != this->length())
                //                {
                //                  return;
                //                }
                //                else
                //                {
                //                  resizeDataArray(length.toInt());
                //                }
              }
            }

            while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Dimension", Qt::CaseInsensitive)) && !xmlReader.hasError())
            {
              xmlReader.readNext();
            }
          }
          xmlReader.readNext();
        }
        //        resetDataArray();
      }
      else if(!xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        valueDefinitionInternal()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
        }
      }
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

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
          if(m_matchIdentifiersDuringRead)
          {
            QStringList currentIdentifiers = identifiers();
            int curId = 0;
            for(QString id : currentIdentifiers)
            {
              if(tempIdentifiers.contains(id))
              {
                int nId = tempIdentifiers.indexOf(id);
                setValues(curId,1,&values[nId]);
              }

              curId ++;
            }
          }
          else
          {
            clearIdentifiers();
            addIdentifiers(tempIdentifiers);
            setValues(0,tempIdentifiers.length(),values.data());
          }
        }
      }
      xmlReader.readNext();
    }
  }
}

void IdBasedArgumentString::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("IdBasedArgument");
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
        xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
        xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      std::vector<QString>values(length());
      getValues(0,length(),values.data());

      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("Id", identifiers()[i]);
          xmlWriter.writeCharacters(values[i]);
        }
        xmlWriter.writeEndElement();
      }
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

void IdBasedArgumentString::saveData()
{
  if(currentArgumentIOType() == IArgument::File)
  {
    QFile file(m_inputFile.absoluteFilePath());

    if(file.open(QIODevice::WriteOnly))
    {
      QXmlStreamWriter xmlWriter(&file);
      xmlWriter.setAutoFormatting(true);

      xmlWriter.writeStartElement("IdBasedArgument");
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
            xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
            xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
            xmlWriter.writeAttribute("Length" , QString::number(length()));
          }
          xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("Values");
        {
          double *values = new double[length()];
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

      file.close();

      return;
    }
  }
}

QString IdBasedArgumentString::toString() const
{
  if(currentArgumentIOType() == IArgument::File)
  {
    return  modelComponentInternal()->getRelativeFilePath(m_inputFile.absoluteFilePath()).fileName();
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

    //    xmlWriter.writeStartDocument();
    //    {
    //      xmlWriter.writeStartElement("IdBasedArgument");
    //      {
    //        xmlWriter.writeAttribute("Id" , id());
    //        xmlWriter.writeAttribute("Caption" , caption());
    //        xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

    //        for(const QString& comment : comments())
    //        {
    //          xmlWriter.writeComment(comment);
    //        }

    //        //write value definition;
    //        valueDefinitionInternal()->writeData(xmlWriter);

    //        xmlWriter.writeStartElement("Dimensions");
    //        {
    //          xmlWriter.writeStartElement("Dimension");
    //          {
    //            xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
    //            xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
    //            xmlWriter.writeAttribute("Length" , QString::number(length()));
    //          }
    //          xmlWriter.writeEndElement();
    //        }
    //        xmlWriter.writeEndElement();

    //        xmlWriter.writeStartElement("Values");
    //        {
    //          qDebug() << length();

    //          QString *values = new QString[length()];
    //          getValues(0,length(),values);

    //          for(int i = 0 ; i < length() ; i++)
    //          {
    //            xmlWriter.writeStartElement("Value");
    //            {
    //              xmlWriter.writeAttribute("Id", identifiers()[i]);
    //              xmlWriter.writeCharacters(values[i]);
    //            }
    //            xmlWriter.writeEndElement();
    //          }

    //          delete[] values;
    //        }
    //        xmlWriter.writeEndElement();
    //      }
    //      xmlWriter.writeEndElement();
    //    }
    //    xmlWriter.writeEndDocument();

    return data;
  }
}

bool IdBasedArgumentString::readValues(const QString &value, QString &message, bool isFile)
{
  message = "";

  if(isFile)
  {
    m_inputFile = modelComponentInternal()->getRelativeFilePath(value);

    if(m_inputFile.exists())
    {
      QFile file(m_inputFile.absoluteFilePath());

      if(file.open(QIODevice::ReadOnly))
      {
        QXmlStreamReader xmlReader(&file);

        while(!xmlReader.isEndDocument() && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            readData(xmlReader);
          }
          xmlReader.readNext();
        }

        if(!xmlReader.hasError())
        {
          setArgumentIOType(ArgumentIOType::File);
          return true;
        }
      }
    }
  }
  else
  {
    QXmlStreamReader xmlReader(value);

    while(!xmlReader.isEndDocument() && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        readData(xmlReader);
      }

      xmlReader.readNext();
    }

    if(!xmlReader.hasError())
    {
      setArgumentIOType(ArgumentIOType::String);
      return true;
    }
  }

  return false;
}

bool IdBasedArgumentString::readValues(const IComponentDataItem *componentDataItem, QString &message)
{
  message = "";

  const IIdBasedComponentDataItem *idBasedData = dynamic_cast<const IIdBasedComponentDataItem*>(componentDataItem);

  if(idBasedData)
  {
    if(valueDefinitionInternal()->type() == idBasedData->valueDefinition()->type() && idBasedData->dimensions().length() == 1)
    {
      QStringList inputIdentifiers = idBasedData->identifiers();

      if(m_matchIdentifiersDuringRead)
      {
        QStringList currentIdentifiers = identifiers();
        int curId = 0;
        for(QString id : currentIdentifiers)
        {
          if(inputIdentifiers.contains(id))
          {
            int nId = inputIdentifiers.indexOf(id);
            QString value = "";
            idBasedData->getValues(nId,1,&value);
            setValues(curId,1,&value);
          }

          curId ++;
        }

        return true;
      }
      else
      {
        int dimLength = idBasedData->dimensionLength(std::vector<int>());
        clearIdentifiers();
        addIdentifiers(inputIdentifiers);
        QString *values = new QString[dimLength];
        idBasedData->getValues(0,dimLength,values);
        setValues(0,dimLength,values);

        delete[] values;

        return true;
      }
    }
  }

  return false;
}

bool IdBasedArgumentString::matchIdentifiersWhenReading() const
{
  return m_matchIdentifiersDuringRead;
}

void IdBasedArgumentString::setMatchIdentifiersWhenReading(bool match)
{
  m_matchIdentifiersDuringRead = match;
}

//==============================================================================================================================


IdBasedArgumentDateTime::IdBasedArgumentDateTime(const QString &id,
                                                 const QStringList &identifiers,
                                                 Dimension* identifierDimension,
                                                 ValueDefinition* valueDefinition,
                                                 AbstractModelComponent *modelComponent)
  : AbstractArgument(id,QList<Dimension*>({identifierDimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<QDateTime>(id, identifiers,valueDefinition->defaultValue().toDateTime()),
    m_matchIdentifiersDuringRead(false),
    m_identifierDimension(identifierDimension)
{

}

IdBasedArgumentDateTime::~IdBasedArgumentDateTime()
{
}

QStringList IdBasedArgumentDateTime::identifiers() const
{
  return IdBasedComponentDataItem<QDateTime>::identifiersInternal();
}

IDimension* IdBasedArgumentDateTime::identifierDimension() const
{
  return m_identifierDimension;
}

int IdBasedArgumentDateTime::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);

  return length();
}

void IdBasedArgumentDateTime::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<QDateTime>::getValueT(dimensionIndexes,data);
}

void IdBasedArgumentDateTime::getValue(int idIndex, void *data) const
{
  ComponentDataItem1D<QDateTime>::getValuesT(idIndex,1,data);
}

void IdBasedArgumentDateTime::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<QDateTime>::getValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentDateTime::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<QDateTime>::setValueT(dimensionIndexes,data);
}

void IdBasedArgumentDateTime::setValue(int idIndex, const void *data)
{
  ComponentDataItem1D<QDateTime>::setValuesT(idIndex,1,data);
}

void IdBasedArgumentDateTime::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<QDateTime>::setValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentDateTime::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
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

              //              if(!m_identifierDimension->id().compare(id))
              //              {
              //                QString length = attributes.value("Length").toString();

              //                if(m_matchIdentifiersDuringRead &&
              //                   length.toInt() != this->length())
              //                {
              //                  return;
              //                }
              //                else
              //                {
              //                  resizeDataArray(length.toInt());
              //                }
              //              }
            }

            while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Dimension", Qt::CaseInsensitive)) && !xmlReader.hasError())
            {
              xmlReader.readNext();
            }
          }
          xmlReader.readNext();
        }
        //        resetDataArray();
      }
      else if(!xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        valueDefinitionInternal()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
        }
      }
      else if(!xmlReader.name().toString().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

        QList<QString> tempIdentifiers;
        std::vector<QDateTime> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().toString().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QXmlStreamAttributes attributes = xmlReader.attributes();

            if(attributes.hasAttribute("Id"))
            {
              QString index = attributes.value("Id").toString();
              tempIdentifiers.append(index);
              QDateTime value = QDateTime::fromString(xmlReader.readElementText(),Qt::ISODate);
              values.push_back(value);
            }
          }
          xmlReader.readNext();
        }

        if(tempIdentifiers.length() && tempIdentifiers.length() == (int)values.size())
        {
          if(m_matchIdentifiersDuringRead)
          {
            QStringList currentIdentifiers = identifiers();
            int curId = 0;
            for(QString id : currentIdentifiers)
            {
              if(tempIdentifiers.contains(id))
              {
                int nId = tempIdentifiers.indexOf(id);
                setValues(curId,1,&values[nId]);
              }

              curId ++;
            }
          }
          else
          {
            clearIdentifiers();
            addIdentifiers(tempIdentifiers);
            setValues(0,tempIdentifiers.length(),values.data());
          }
        }
      }
      xmlReader.readNext();
    }
  }
}

void IdBasedArgumentDateTime::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("IdBasedArgument");
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
        xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
        xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      std::vector<QDateTime> values(length());
      getValues(0,length(),values.data());

      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeAttribute("Id", identifiers()[i]);
          xmlWriter.writeCharacters(values[i].toString(Qt::ISODate));
        }
        xmlWriter.writeEndElement();
      }
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

void IdBasedArgumentDateTime::saveData()
{
  if(currentArgumentIOType() == IArgument::File)
  {
    QFile file(m_inputFile.absoluteFilePath());

    if(file.open(QIODevice::WriteOnly))
    {
      QXmlStreamWriter xmlWriter(&file);
      xmlWriter.setAutoFormatting(true);

      xmlWriter.writeStartElement("IdBasedArgument");
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
            xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
            xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
            xmlWriter.writeAttribute("Length" , QString::number(length()));
          }
          xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("Values");
        {
          double *values = new double[length()];
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

      file.close();

      return;
    }
  }
}

QString IdBasedArgumentDateTime::toString() const
{
  if(currentArgumentIOType() == IArgument::File)
  {
    return  modelComponentInternal()->getRelativeFilePath(m_inputFile.absoluteFilePath()).fileName();
  }
  else
  {
    QString data;
    QXmlStreamWriter xmlWriter(&data);
    xmlWriter.setAutoFormatting(true);

    xmlWriter.writeStartDocument();
    {
      writeData(xmlWriter);
      //      xmlWriter.writeStartElement("IdBasedArgument");
      //      {
      //        xmlWriter.writeAttribute("Id" , id());
      //        xmlWriter.writeAttribute("Caption" , caption());
      //        xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

      //        for(const QString& comment : comments())
      //        {
      //          xmlWriter.writeComment(comment);
      //        }

      //        //write value definition;
      //        valueDefinitionInternal()->writeData(xmlWriter);

      //        xmlWriter.writeStartElement("Dimensions");
      //        {
      //          xmlWriter.writeStartElement("Dimension");
      //          {
      //            xmlWriter.writeAttribute("Id" , m_identifierDimension->id());
      //            xmlWriter.writeAttribute("Caption" , m_identifierDimension->caption());
      //            xmlWriter.writeAttribute("Length" , QString::number(length()));
      //          }
      //          xmlWriter.writeEndElement();
      //        }
      //        xmlWriter.writeEndElement();

      //        xmlWriter.writeStartElement("Values");
      //        {
      //          qDebug() << length();

      //          QDateTime *values = new QDateTime[length()];
      //          getValues(0,length(),values);

      //          for(int i = 0 ; i < length() ; i++)
      //          {
      //            xmlWriter.writeStartElement("Value");
      //            {
      //              xmlWriter.writeAttribute("Id", identifiers()[i]);
      //              xmlWriter.writeCharacters(values[i].toString(Qt::ISODate));
      //            }
      //            xmlWriter.writeEndElement();
      //          }

      //          delete[] values;
      //        }
      //        xmlWriter.writeEndElement();
      //      }
      //      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndDocument();

    return data;
  }
}

bool IdBasedArgumentDateTime::readValues(const QString &value, QString &message, bool isFile)
{
  message = "";

  if(isFile)
  {
    m_inputFile = modelComponentInternal()->getRelativeFilePath(value);

    if(m_inputFile.exists())
    {
      QFile file(m_inputFile.absoluteFilePath());

      if(file.open(QIODevice::ReadOnly))
      {
        QXmlStreamReader xmlReader(&file);

        while(!xmlReader.isEndDocument() && !xmlReader.hasError())
        {
          if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            readData(xmlReader);
          }
          xmlReader.readNext();
        }

        if(!xmlReader.hasError())
        {
          setArgumentIOType(ArgumentIOType::File);
          return true;
        }
      }
    }
  }
  else
  {
    QXmlStreamReader xmlReader(value);

    while(!xmlReader.isEndDocument() && !xmlReader.hasError())
    {
      if(!xmlReader.name().toString().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        readData(xmlReader);
      }

      xmlReader.readNext();
    }

    if(!xmlReader.hasError())
    {
      setArgumentIOType(ArgumentIOType::String);
      return true;
    }
  }

  return false;
}

bool IdBasedArgumentDateTime::readValues(const IComponentDataItem *componentDataItem, QString &message)
{
  message = "";

  const IIdBasedComponentDataItem *idBasedData = dynamic_cast<const IIdBasedComponentDataItem*>(componentDataItem);

  if(idBasedData)
  {
    if(valueDefinitionInternal()->type() == idBasedData->valueDefinition()->type() && idBasedData->dimensions().length() == 1)
    {
      QStringList inputIdentifiers = idBasedData->identifiers();

      if(m_matchIdentifiersDuringRead)
      {
        QStringList currentIdentifiers = identifiers();
        int curId = 0;
        for(QString id : currentIdentifiers)
        {
          if(inputIdentifiers.contains(id))
          {
            int nId = inputIdentifiers.indexOf(id);
            QDateTime value;
            idBasedData->getValues(nId,1,&value);
            setValues(curId,1,&value);
          }

          curId ++;
        }

        return true;
      }
      else
      {
        int dimLength = idBasedData->dimensionLength(std::vector<int>());
        clearIdentifiers();
        addIdentifiers(inputIdentifiers);
        QDateTime *values = new QDateTime[dimLength];
        idBasedData->getValues(0,dimLength,values);
        setValues(0,dimLength,values);

        delete[] values;

        return true;
      }
    }
  }

  return false;
}

bool IdBasedArgumentDateTime::matchIdentifiersWhenReading() const
{
  return m_matchIdentifiersDuringRead;
}

void IdBasedArgumentDateTime::setMatchIdentifiersWhenReading(bool match)
{
  m_matchIdentifiersDuringRead = match;
}

