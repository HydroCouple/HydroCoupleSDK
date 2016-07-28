#include "stdafx.h"
#include "core/idbasedargument.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/idbasedcomponentdataitem.h"
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
    IdBasedComponentDataItem<int>(identifiers,valueDefinition->defaultValue().toInt()),
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

int IdBasedArgumentInt::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void IdBasedArgumentInt::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<int>::getValueT(dimensionIndexes,data);
}

void IdBasedArgumentInt::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<int>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentInt::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<int>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentInt::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<int>::setValueT(dimensionIndexes,data);
}

void IdBasedArgumentInt::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<int>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentInt::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<int>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentInt::getValue(int idIndex, QVariant& data) const
{
  ComponentDataItem1D<int>::getValueT(&idIndex,data);
}

void IdBasedArgumentInt::getValues(int idIndex, int stride, QVariant data[]) const
{
  ComponentDataItem1D<int>::getValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentInt::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<int>::getValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentInt::setValue(int idIndex, const QVariant &data)
{
  ComponentDataItem1D<int>::setValueT(&idIndex,data);
}

void IdBasedArgumentInt::setValues(int idIndex, int stride, const QVariant data[])
{
  ComponentDataItem1D<int>::setValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentInt::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<int>::setValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentInt::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
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

              if(!m_identifierDimension->id().compare(id))
              {
                QString length = attributes.value("Length").toString();

                if(m_matchIdentifiersDuringRead &&
                   length.toInt() != this->length())
                {
                  return;
                }
                else
                {
                  setLength(length.toInt());
                }
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
      else if(!xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        valueDefinitionInternal()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
        }
      }
      else if(!xmlReader.name().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

        QList<QString> tempIdentifiers;
        std::vector<int> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
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

void IdBasedArgumentInt::writeData(QXmlStreamWriter &xmlWriter)
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
      int values[length()];
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
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

QString IdBasedArgumentInt::toString() const
{
  if(currentArgumentIOType() == HydroCouple::File)
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
          int ind[1] = {0};
          int str[1] = {length()};
          int values[length()];
          getValues(ind,str,values);

          for(int i = 0 ; i < length() ; i++)
          {
            xmlWriter.writeStartElement("Value");
            {
              xmlWriter.writeAttribute("Id", identifiers()[i]);
              xmlWriter.writeCharacters(QString::number(values[i]));
            }
            xmlWriter.writeEndElement();
          }
        }
        xmlWriter.writeEndElement();
      }
      xmlWriter.writeEndElement();

      file.close();

      return m_inputFile.absoluteFilePath();
    }
  }

  QString data;
  QXmlStreamWriter xmlWriter(&data);
  xmlWriter.setAutoFormatting(true);

  xmlWriter.writeStartDocument();
  {
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
        int values[length()];
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
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndDocument();

  return data;
}

bool IdBasedArgumentInt::readValues(const QString &value, bool isFile)
{
  if(isFile)
  {
    m_inputFile = QFileInfo(value);

    if(m_inputFile.exists())
    {
      QFile file(m_inputFile.absoluteFilePath());

      if(file.open(QIODevice::ReadOnly))
      {
        QXmlStreamReader xmlReader(&file);

        while(!xmlReader.isEndDocument() && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
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
      if(!xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
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

bool IdBasedArgumentInt::readValues(const IComponentDataItem *componentDataItem)
{
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
        int dimLength = idBasedData->dimensionLength(nullptr,0);
        clearIdentifiers();
        addIdentifiers(inputIdentifiers);
        int values[dimLength];
        idBasedData->getValues(0,dimLength,values);
        setValues(0,dimLength,values);

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
    IdBasedComponentDataItem<double>(identifiers,valueDefinition->defaultValue().toDouble()),
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

int IdBasedArgumentDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void IdBasedArgumentDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void IdBasedArgumentDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void IdBasedArgumentDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentDouble::getValue(int idIndex, QVariant& data) const
{
  ComponentDataItem1D<double>::getValueT(&idIndex,data);
}

void IdBasedArgumentDouble::getValues(int idIndex, int stride, QVariant data[]) const
{
  ComponentDataItem1D<double>::getValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentDouble::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentDouble::setValue(int idIndex, const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(&idIndex,data);
}

void IdBasedArgumentDouble::setValues(int idIndex, int stride, const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentDouble::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentDouble::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
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

              if(!m_identifierDimension->id().compare(id))
              {
                QString length = attributes.value("Length").toString();

                if(m_matchIdentifiersDuringRead &&
                   length.toInt() != this->length())
                {
                  return;
                }
                else
                {
                  setLength(length.toInt());
                }
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
      else if(!xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        valueDefinitionInternal()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
        }
      }
      else if(!xmlReader.name().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

        QList<QString> tempIdentifiers;
        std::vector<double> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
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

void IdBasedArgumentDouble::writeData(QXmlStreamWriter &xmlWriter)
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
      double values[length()];
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
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

QString IdBasedArgumentDouble::toString() const
{
  if(currentArgumentIOType() == HydroCouple::File)
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
          double values[length()];
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
        }
        xmlWriter.writeEndElement();
      }
      xmlWriter.writeEndElement();

      file.close();

      return m_inputFile.absoluteFilePath();
    }
  }

  QString data;
  QXmlStreamWriter xmlWriter(&data);
  xmlWriter.setAutoFormatting(true);

  xmlWriter.writeStartDocument();
  {
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
        double values[length()];
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
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndDocument();

  return data;
}

bool IdBasedArgumentDouble::readValues(const QString &value, bool isFile)
{
  if(isFile)
  {
    m_inputFile = QFileInfo(value);

    if(m_inputFile.exists())
    {
      QFile file(m_inputFile.absoluteFilePath());

      if(file.open(QIODevice::ReadOnly))
      {
        QXmlStreamReader xmlReader(&file);

        while(!xmlReader.isEndDocument() && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
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
      if(!xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
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

bool IdBasedArgumentDouble::readValues(const IComponentDataItem *componentDataItem)
{
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
        int dimLength  = idBasedData->dimensionLength(nullptr,0);
        clearIdentifiers();
        addIdentifiers(inputIdentifiers);
        int values[dimLength];
        idBasedData->getValues(0,dimLength,values);
        setValues(0,dimLength,values);

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


IdBasedArgumentQString::IdBasedArgumentQString(const QString &id,
                                               const QStringList &identifiers,
                                               Dimension* identifierDimension,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent *modelComponent)
  : AbstractArgument(id,QList<Dimension*>({identifierDimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<QString>(identifiers,valueDefinition->defaultValue().toString()),
    m_matchIdentifiersDuringRead(false),
    m_identifierDimension(identifierDimension)
{

}

IdBasedArgumentQString::~IdBasedArgumentQString()
{

}


QStringList IdBasedArgumentQString::identifiers() const
{
  return IdBasedComponentDataItem<QString>::identifiersInternal();
}

IDimension* IdBasedArgumentQString::identifierDimension() const
{
  return m_identifierDimension;
}

int IdBasedArgumentQString::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void IdBasedArgumentQString::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<QString>::getValueT(dimensionIndexes,data);
}

void IdBasedArgumentQString::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentQString::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentQString::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<QString>::setValueT(dimensionIndexes,data);
}

void IdBasedArgumentQString::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentQString::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedArgumentQString::getValue(int idIndex, QVariant& data) const
{
  ComponentDataItem1D<QString>::getValueT(&idIndex,data);
}

void IdBasedArgumentQString::getValues(int idIndex, int stride, QVariant data[]) const
{
  ComponentDataItem1D<QString>::getValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentQString::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<QString>::getValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentQString::setValue(int idIndex, const QVariant &data)
{
  ComponentDataItem1D<QString>::setValueT(&idIndex,data);
}

void IdBasedArgumentQString::setValues(int idIndex, int stride, const QVariant data[])
{
  ComponentDataItem1D<QString>::setValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentQString::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(&idIndex,&stride,data);
}

void IdBasedArgumentQString::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive)) && !xmlReader.hasError())
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

              if(!m_identifierDimension->id().compare(id))
              {
                QString length = attributes.value("Length").toString();

                if(m_matchIdentifiersDuringRead &&
                   length.toInt() != this->length())
                {
                  return;
                }
                else
                {
                  setLength(length.toInt());
                }
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
      else if(!xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        valueDefinitionInternal()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
        }
      }
      else if(!xmlReader.name().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

        QList<QString> tempIdentifiers;
        std::vector<QString> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
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

void IdBasedArgumentQString::writeData(QXmlStreamWriter &xmlWriter)
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
      double values[length()];
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
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

QString IdBasedArgumentQString::toString() const
{
  if(currentArgumentIOType() == HydroCouple::File)
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
          double values[length()];
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
        }
        xmlWriter.writeEndElement();
      }
      xmlWriter.writeEndElement();

      file.close();

      return m_inputFile.absoluteFilePath();
    }
  }

  QString data;
  QXmlStreamWriter xmlWriter(&data);
  xmlWriter.setAutoFormatting(true);

  xmlWriter.writeStartDocument();
  {
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
        qDebug() << length();

        QString *values = new QString[length()];
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
  xmlWriter.writeEndDocument();

  return data;
}

bool IdBasedArgumentQString::readValues(const QString &value, bool isFile)
{
  if(isFile)
  {
    m_inputFile = QFileInfo(value);

    if(m_inputFile.exists())
    {
      QFile file(m_inputFile.absoluteFilePath());

      if(file.open(QIODevice::ReadOnly))
      {
        QXmlStreamReader xmlReader(&file);

        while(!xmlReader.isEndDocument() && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
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
      if(!xmlReader.name().compare("IdBasedArgument", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
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

bool IdBasedArgumentQString::readValues(const IComponentDataItem *componentDataItem)
{
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
        int dimLength = idBasedData->dimensionLength(nullptr,0);
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

bool IdBasedArgumentQString::matchIdentifiersWhenReading() const
{
  return m_matchIdentifiersDuringRead;
}

void IdBasedArgumentQString::setMatchIdentifiersWhenReading(bool match)
{
  m_matchIdentifiersDuringRead = match;
}

template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<QString>;

