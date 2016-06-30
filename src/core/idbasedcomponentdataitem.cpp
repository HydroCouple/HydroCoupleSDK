#include "stdafx.h"
#include "core/idbasedcomponentdataitem.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/abstractmodelcomponent.h"

using namespace HydroCouple;

template<class T>
IdBasedComponentDataItem<T>::IdBasedComponentDataItem(const QStringList& identifiers,
                                                      Dimension* dimension,
                                                      const T& defaultValue)
  :ComponentDataItem1D<T>(dimension, defaultValue),
    m_identifiers(identifiers)
{
}

template<class T>
IdBasedComponentDataItem<T>::~IdBasedComponentDataItem()
{

}

template<class T>
bool IdBasedComponentDataItem<T>::addIdentifier(const QString &identifier, bool resetDataArray)
{
  if(!m_identifiers.contains(identifier))
  {
    m_identifiers.append(identifier);
    identifierDimensionInternal()->setLength(m_identifiers.length());

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
void IdBasedComponentDataItem<T>::addIdentifiers(const QList<QString> &identifiers, bool resetDataArray)
{
  bool added = false;

  for(const QString &id : identifiers)
  {
    if(!m_identifiers.contains(id))
    {
      m_identifiers.append(id);
      added = true;
    }
  }

  if(added)
  {
    identifierDimensionInternal()->setLength(m_identifiers.length());

    if(resetDataArray)
    {
      ComponentDataItem1D<T>::resetDataArray();
    }
  }
}

template<class T>
bool IdBasedComponentDataItem<T>::removeIdentifier(const QString &identifier, bool resetDataArray)
{
  if(m_identifiers.removeOne(identifier))
  {
    identifierDimensionInternal()->setLength(m_identifiers.length());

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
QStringList IdBasedComponentDataItem<T>::identifiersInternal() const
{
  return m_identifiers;
}

template<class T>
Dimension* IdBasedComponentDataItem<T>::identifierDimensionInternal() const
{
  return ComponentDataItem1D<T>::dimensionInternal();
}

template<class T>
void IdBasedComponentDataItem<T>::clearIdentifiers()
{
  m_identifiers.clear();
}

//==============================================================================================================================

IdBasedComponentDataItemInt::IdBasedComponentDataItemInt(const QString& id, const QStringList& identifiers,
                                                         Dimension* dimension, ValueDefinition* valueDefinition,
                                                         AbstractModelComponent* modelComponent)
  : AbstractComponentDataItem(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<int>(identifiers,dimension,valueDefinition->defaultValue().toInt())
{

}

IdBasedComponentDataItemInt::~IdBasedComponentDataItemInt()
{

}

QStringList IdBasedComponentDataItemInt::identifiers() const
{
  return IdBasedComponentDataItem<int>::identifiersInternal();
}

IDimension* IdBasedComponentDataItemInt::identifierDimension() const
{
  return IdBasedComponentDataItem<int>::identifierDimensionInternal();
}

void IdBasedComponentDataItemInt::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<int>::getValueT(dimensionIndexes,data);
}

void IdBasedComponentDataItemInt::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<int>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemInt::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<int>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemInt::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<int>::setValueT(dimensionIndexes,data);
}

void IdBasedComponentDataItemInt::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<int>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemInt::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<int>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemInt::getValue(int idIndex, QVariant& data) const
{
   ComponentDataItem1D<int>::getValueT(&idIndex,data);
}

void IdBasedComponentDataItemInt::getValues(int idIndex, int stride, QVariant data[]) const
{
    ComponentDataItem1D<int>::getValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemInt::getValues(int idIndex, int stride, void *data) const
{
   ComponentDataItem1D<int>::getValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemInt::setValue(int idIndex, const QVariant &data)
{
    ComponentDataItem1D<int>::setValueT(&idIndex,data);
}

void IdBasedComponentDataItemInt::setValues(int idIndex, int stride, const QVariant data[])
{
   ComponentDataItem1D<int>::setValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemInt::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<int>::setValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemInt::readData(QXmlStreamReader &xmlReader)
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

              if(!identifierDimensionInternal()->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                identifierDimensionInternal()->setLength(length.toInt());
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

void IdBasedComponentDataItemInt::writeData(QXmlStreamWriter &xmlWriter)
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
        xmlWriter.writeAttribute("Id" , identifierDimensionInternal()->id());
        xmlWriter.writeAttribute("Caption" , identifierDimensionInternal()->caption());
        xmlWriter.writeAttribute("Length" , QString::number(identifierDimensionInternal()->length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      int ind[1] = {0};
      int str[1] = {identifierDimensionInternal()->length()};
      int values[identifierDimensionInternal()->length()];
      getValues(ind,str,values);

      for(int i = 0 ; i < identifierDimensionInternal()->length() ; i++)
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

IdBasedComponentDataItemDouble::IdBasedComponentDataItemDouble(const QString& id,
                                                               const QStringList& identifiers,
                                                               Dimension* dimension,
                                                               ValueDefinition* valueDefinition,
                                                               AbstractModelComponent *modelComponent)
  : AbstractComponentDataItem(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<double>(identifiers,dimension,valueDefinition->defaultValue().toDouble())
{
}

IdBasedComponentDataItemDouble::~IdBasedComponentDataItemDouble()
{

}

QStringList IdBasedComponentDataItemDouble::identifiers() const
{
  return IdBasedComponentDataItem<double>::identifiersInternal();
}

IDimension* IdBasedComponentDataItemDouble::identifierDimension() const
{
  return IdBasedComponentDataItem<double>::identifierDimensionInternal();
}

void IdBasedComponentDataItemDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void IdBasedComponentDataItemDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void IdBasedComponentDataItemDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemDouble::getValue(int idIndex, QVariant& data) const
{
   ComponentDataItem1D<double>::getValueT(&idIndex,data);
}

void IdBasedComponentDataItemDouble::getValues(int idIndex, int stride, QVariant data[]) const
{
    ComponentDataItem1D<double>::getValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemDouble::getValues(int idIndex, int stride, void *data) const
{
   ComponentDataItem1D<double>::getValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemDouble::setValue(int idIndex, const QVariant &data)
{
    ComponentDataItem1D<double>::setValueT(&idIndex,data);
}

void IdBasedComponentDataItemDouble::setValues(int idIndex, int stride, const QVariant data[])
{
   ComponentDataItem1D<double>::setValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemDouble::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemDouble::readData(QXmlStreamReader &xmlReader)
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

              if(!identifierDimensionInternal()->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                identifierDimensionInternal()->setLength(length.toInt());
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

void IdBasedComponentDataItemDouble::writeData(QXmlStreamWriter &xmlWriter)
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
        xmlWriter.writeAttribute("Id" , identifierDimensionInternal()->id());
        xmlWriter.writeAttribute("Caption" , identifierDimensionInternal()->caption());
        xmlWriter.writeAttribute("Length" , QString::number(identifierDimensionInternal()->length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      int ind[1] = {0};
      int str[1] = {identifierDimensionInternal()->length()};
      double values[identifierDimensionInternal()->length()];
      getValues(ind,str,values);

      for(int i = 0 ; i < identifierDimensionInternal()->length() ; i++)
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

IdBasedComponentDataItemString::IdBasedComponentDataItemString(const QString &id, const QStringList &identifiers,
                                                               Dimension *dimension, ValueDefinition* valueDefinition,
                                                               AbstractModelComponent *modelComponent)
  : AbstractComponentDataItem(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    IdBasedComponentDataItem<QString>(identifiers,dimension,valueDefinition->defaultValue().toString())
{
}

IdBasedComponentDataItemString::~IdBasedComponentDataItemString()
{

}

QStringList IdBasedComponentDataItemString::identifiers() const
{
  return IdBasedComponentDataItem<QString>::identifiersInternal();
}

IDimension* IdBasedComponentDataItemString::identifierDimension() const
{
  return IdBasedComponentDataItem<QString>::identifierDimensionInternal();
}

void IdBasedComponentDataItemString::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<QString>::getValueT(dimensionIndexes,data);
}

void IdBasedComponentDataItemString::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemString::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemString::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<QString>::setValueT(dimensionIndexes,data);
}

void IdBasedComponentDataItemString::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemString::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void IdBasedComponentDataItemString::getValue(int idIndex, QVariant& data) const
{
   ComponentDataItem1D<QString>::getValueT(&idIndex,data);
}

void IdBasedComponentDataItemString::getValues(int idIndex, int stride, QVariant data[]) const
{
    ComponentDataItem1D<QString>::getValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemString::getValues(int idIndex, int stride, void *data) const
{
   ComponentDataItem1D<QString>::getValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemString::setValue(int idIndex, const QVariant &data)
{
    ComponentDataItem1D<QString>::setValueT(&idIndex,data);
}

void IdBasedComponentDataItemString::setValues(int idIndex, int stride, const QVariant data[])
{
   ComponentDataItem1D<QString>::setValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemString::setValues(int idIndex, int stride, const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(&idIndex,&stride,data);
}

void IdBasedComponentDataItemString::readData(QXmlStreamReader &xmlReader)
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

              if(!identifierDimensionInternal()->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                identifierDimensionInternal()->setLength(length.toInt());
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

void IdBasedComponentDataItemString::writeData(QXmlStreamWriter &xmlWriter)
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
        xmlWriter.writeAttribute("Id" , identifierDimensionInternal()->id());
        xmlWriter.writeAttribute("Caption" , identifierDimensionInternal()->caption());
        xmlWriter.writeAttribute("Length" , QString::number(identifierDimensionInternal()->length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      int ind[1] = {0};
      int str[1] = {identifierDimensionInternal()->length()};
      QString* values = new QString[identifierDimensionInternal()->length()];
      getValues(ind,str,values);

      for(int i = 0 ; i < identifierDimensionInternal()->length() ; i++)
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

template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<QString>;

template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem<QString>;
