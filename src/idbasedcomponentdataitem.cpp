#include "stdafx.h"
#include "idbasedcomponentdataitem.h"
#include "dimension.h"
#include  "valuedefinition.h"
#include "abstractmodelcomponent.h"

using namespace HydroCouple;

template<class T>
IdBasedComponentDataItem<T>::IdBasedComponentDataItem(const QStringList &identifiers, Dimension *dimension, ValueDefinition *valueDefinition)
   :ComponentDataItem1D<T>(dimension , valueDefinition), m_identifiers(identifiers), m_identifierDimension(dimension)
{
}

template<class T>
IdBasedComponentDataItem<T>::~IdBasedComponentDataItem()
{

}

template<class T>
QStringList IdBasedComponentDataItem<T>::identifiers() const
{
   return m_identifiers;
}

template<class T>
bool IdBasedComponentDataItem<T>::addIdentifier(const QString &identifier, bool resetDataArray)
{
   if(!m_identifiers.contains(identifier))
   {
      m_identifiers.append(identifier);
      m_identifierDimension->setLength(m_identifiers.length());

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
      m_identifierDimension->setLength(m_identifiers.length());

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
      m_identifierDimension->setLength(m_identifiers.length());

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
IDimension* IdBasedComponentDataItem<T>::identifierDimension() const
{
   return m_identifierDimension;
}

template<class T>
void IdBasedComponentDataItem<T>::getValue(int idIndex, QVariant &data) const
{
   ComponentDataItem1D<T>::getValue(&idIndex , data);
}

template<class T>
void IdBasedComponentDataItem<T>::getValues(int idIndex, int stride, QVariant *data) const
{
   ComponentDataItem1D<T>::getValues(&idIndex, &stride, data);
}

template<class T>
void IdBasedComponentDataItem<T>::getValues(int idIndex, int stride, void *data) const
{
   ComponentDataItem1D<T>::getValues(&idIndex, &stride, data);
}

template<class T>
void IdBasedComponentDataItem<T>::setValue(int idIndex, const QVariant &data)
{
   ComponentDataItem1D<T>::setValue(&idIndex, data);
}

template<class T>
void IdBasedComponentDataItem<T>::setValues(int idIndex, int stride, const QVariant data[])
{
   ComponentDataItem1D<T>::setValues(&idIndex, &stride, data);
}

template<class T>
void IdBasedComponentDataItem<T>::setValues(int idIndex, int stride, const void *data)
{
   ComponentDataItem1D<T>::setValues(&idIndex, &stride, data);
}

template<class T>
void IdBasedComponentDataItem<T>::clearIdentifiers()
{
   m_identifiers.clear();
}

template<class T>
Dimension* IdBasedComponentDataItem<T>::hIdentifierDimension() const
{
   return m_identifierDimension;
}


IdBasedComponentDataItemInt::IdBasedComponentDataItemInt(const QString &id, const QStringList &identifiers,
                                                         Dimension *dimension, ValueDefinition* valueDefinition,
                                                         AbstractModelComponent *parentModelComponent)
   : IdBasedComponentDataItem<int>(identifiers,dimension,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)

{

}

IdBasedComponentDataItemInt::~IdBasedComponentDataItemInt()
{

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

                     if(!hDimension()->id().compare(id))
                     {
                        QString length = attributes.value("Length").toString();
                        hDimension()->setLength(length.toInt());
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
               IdBasedComponentDataItem<int>::setValues(0,tempIdentifiers.length(),values.data());
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

      xmlWriter.writeStartElement("Dimensions");
      {
         xmlWriter.writeStartElement("Dimension");
         {
            xmlWriter.writeAttribute("Id" , hDimension()->id());
            xmlWriter.writeAttribute("Caption" , hDimension()->caption());
            xmlWriter.writeAttribute("Length" , QString::number(hDimension()->length()));
         }
         xmlWriter.writeEndElement();
      }
      xmlWriter.writeEndElement();

      xmlWriter.writeStartElement("Values");
      {
         int ind[1] = {0};
         int str[1] = {hDimension()->length()};
         int values[hDimension()->length()];
         getValues(ind,str,values);

         for(int i = 0 ; i < hDimension()->length() ; i++)
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

IdBasedComponentDataItemDouble::IdBasedComponentDataItemDouble(const QString &id, const QStringList &identifiers,
                                                               Dimension *dimension, ValueDefinition* valueDefinition,
                                                               AbstractModelComponent *parentModelComponent)
   : IdBasedComponentDataItem<double>(identifiers,dimension,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{
}

IdBasedComponentDataItemDouble::~IdBasedComponentDataItemDouble()
{

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

                     if(!hDimension()->id().compare(id))
                     {
                        QString length = attributes.value("Length").toString();
                        hDimension()->setLength(length.toInt());
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
               IdBasedComponentDataItem<double>::setValues(0,tempIdentifiers.length(),values.data());
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

      xmlWriter.writeStartElement("Dimensions");
      {
         xmlWriter.writeStartElement("Dimension");
         {
            xmlWriter.writeAttribute("Id" , hDimension()->id());
            xmlWriter.writeAttribute("Caption" , hDimension()->caption());
            xmlWriter.writeAttribute("Length" , QString::number(hDimension()->length()));
         }
         xmlWriter.writeEndElement();
      }
      xmlWriter.writeEndElement();

      xmlWriter.writeStartElement("Values");
      {
         int ind[1] = {0};
         int str[1] = {hDimension()->length()};
         double values[hDimension()->length()];
         getValues(ind,str,values);

         for(int i = 0 ; i < hDimension()->length() ; i++)
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


IdBasedComponentDataItemString::IdBasedComponentDataItemString(const QString &id, const QStringList &identifiers,
                                                               Dimension *dimension, ValueDefinition* valueDefinition,
                                                               AbstractModelComponent *parentModelComponent)
   : IdBasedComponentDataItem<QString>(identifiers,dimension,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{
}

IdBasedComponentDataItemString::~IdBasedComponentDataItemString()
{

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

                     if(!hDimension()->id().compare(id))
                     {
                        QString length = attributes.value("Length").toString();
                        hDimension()->setLength(length.toInt());
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
               IdBasedComponentDataItem<QString>::setValues(0,tempIdentifiers.length(),values.data());
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

      xmlWriter.writeStartElement("Dimensions");
      {
         xmlWriter.writeStartElement("Dimension");
         {
            xmlWriter.writeAttribute("Id" , hDimension()->id());
            xmlWriter.writeAttribute("Caption" , hDimension()->caption());
            xmlWriter.writeAttribute("Length" , QString::number(hDimension()->length()));
         }
         xmlWriter.writeEndElement();
      }
      xmlWriter.writeEndElement();

      xmlWriter.writeStartElement("Values");
      {
         int ind[1] = {0};
         int str[1] = {hDimension()->length()};
         QString* values = new QString[hDimension()->length()];
         getValues(ind,str,values);

         for(int i = 0 ; i < hDimension()->length() ; i++)
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
