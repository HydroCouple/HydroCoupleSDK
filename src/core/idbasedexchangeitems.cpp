#include "stdafx.h"
#include "core/idbasedexchangeitems.h"
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
    IdBasedComponentDataItem<int>(identifiers,valueDefinition->defaultValue().toInt()),
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

int IdBasedInputInt::dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  return length();
}

void IdBasedInputInt::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<int>::getValueT(dimensionIndexes,data);
}

void IdBasedInputInt::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<int>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputInt::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<int>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputInt::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<int>::setValueT(dimensionIndexes,data);
}

void IdBasedInputInt::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<int>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputInt::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<int>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputInt::getValue(int idIndex, QVariant& data) const
{
  ComponentDataItem1D<int>::getValueT(&idIndex,data);
}

void IdBasedInputInt::getValues(int idIndex, int stride, QVariant data[]) const
{
  ComponentDataItem1D<int>::getValuesT(&idIndex,&stride,data);
}

void IdBasedInputInt::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<int>::getValuesT(&idIndex,&stride,data);
}

void IdBasedInputInt::setValue(int idIndex, const QVariant &data)
{
  ComponentDataItem1D<int>::setValueT(&idIndex,data);
}

void IdBasedInputInt::setValues(int idIndex, int stride, const QVariant data[])
{
  ComponentDataItem1D<int>::setValuesT(&idIndex,&stride,data);
}

void IdBasedInputInt::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<int>::setValuesT(&idIndex,&stride,data);
}

void IdBasedInputInt::readData(QXmlStreamReader &xmlReader)
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

              if(!m_identifierDimension->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                setLength(length.toInt());
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
        clearIdentifiers();
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
          addIdentifiers(tempIdentifiers);
          setValues(0,tempIdentifiers.length(),values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void IdBasedInputInt::writeData(QXmlStreamWriter &xmlWriter)
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
}

//==============================================================================================================================

IdBasedInputDouble::IdBasedInputDouble(const QString& id,
                                       const QStringList& identifiers,
                                       Dimension* identifierDimension,
                                       ValueDefinition* valueDefinition,
                                       AbstractModelComponent *modelComponent)
  : AbstractInput(id,QList<Dimension*>({identifierDimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<double>(identifiers,valueDefinition->defaultValue().toDouble()),
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

int IdBasedInputDouble::dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  return length();
}

void IdBasedInputDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void IdBasedInputDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void IdBasedInputDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputDouble::getValue(int idIndex, QVariant& data) const
{
  ComponentDataItem1D<double>::getValueT(&idIndex,data);
}

void IdBasedInputDouble::getValues(int idIndex, int stride, QVariant data[]) const
{
  ComponentDataItem1D<double>::getValuesT(&idIndex,&stride,data);
}

void IdBasedInputDouble::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(&idIndex,&stride,data);
}

void IdBasedInputDouble::setValue(int idIndex, const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(&idIndex,data);
}

void IdBasedInputDouble::setValues(int idIndex, int stride, const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(&idIndex,&stride,data);
}

void IdBasedInputDouble::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(&idIndex,&stride,data);
}

void IdBasedInputDouble::readData(QXmlStreamReader &xmlReader)
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

              if(!m_identifierDimension->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                setLength(length.toInt());
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
        clearIdentifiers();
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
          addIdentifiers(tempIdentifiers);
          setValues(0,tempIdentifiers.length(),values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void IdBasedInputDouble::writeData(QXmlStreamWriter &xmlWriter)
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
      int ind[1] = {0};
      int str[1] = {length()};
      double values[length()];
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
}

//==============================================================================================================================

IdBasedInputString::IdBasedInputString(const QString &id, const QStringList &identifiers,
                                                               Dimension *identifierDimension,
                                                               ValueDefinition* valueDefinition,
                                                               AbstractModelComponent *modelComponent)
  : AbstractInput(id,QList<Dimension*>({identifierDimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<QString>(identifiers,valueDefinition->defaultValue().toString()),
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

int IdBasedInputString::dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  return length();
}

void IdBasedInputString::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<QString>::getValueT(dimensionIndexes,data);
}

void IdBasedInputString::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputString::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputString::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<QString>::setValueT(dimensionIndexes,data);
}

void IdBasedInputString::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputString::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedInputString::getValue(int idIndex, QVariant& data) const
{
  ComponentDataItem1D<QString>::getValueT(&idIndex,data);
}

void IdBasedInputString::getValues(int idIndex, int stride, QVariant data[]) const
{
  ComponentDataItem1D<QString>::getValuesT(&idIndex,&stride,data);
}

void IdBasedInputString::getValues(int idIndex, int stride, void *data) const
{
  ComponentDataItem1D<QString>::getValuesT(&idIndex,&stride,data);
}

void IdBasedInputString::setValue(int idIndex, const QVariant &data)
{
  ComponentDataItem1D<QString>::setValueT(&idIndex,data);
}

void IdBasedInputString::setValues(int idIndex, int stride, const QVariant data[])
{
  ComponentDataItem1D<QString>::setValuesT(&idIndex,&stride,data);
}

void IdBasedInputString::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(&idIndex,&stride,data);
}

void IdBasedInputString::readData(QXmlStreamReader &xmlReader)
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

              if(!m_identifierDimension->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                setLength(length.toInt());
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
        clearIdentifiers();
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
          addIdentifiers(tempIdentifiers);
          setValues(0,tempIdentifiers.length(),values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void IdBasedInputString::writeData(QXmlStreamWriter &xmlWriter)
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
      int ind[1] = {0};
      int str[1] = {length()};
      QString* values = new QString[length()];
      getValues(ind,str,values);

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


