#include "stdafx.h"
#include "core/componentdataitem1d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include <QDebug>

using namespace HydroCouple;

template<class T>
ComponentDataItem1D<T>::ComponentDataItem1D(Dimension* dimension, ValueDefinition* valueDefinition)
   : m_dimension(dimension) , m_data(nullptr)
{
   m_valueDefinition = valueDefinition;
   createData();
}

template<class T>
ComponentDataItem1D<T>::~ComponentDataItem1D()
{
   deleteData();
}

template<class T>
QList<IDimension*> ComponentDataItem1D<T>::dimensions() const
{
   QList<IDimension*> dimensions;
   dimensions.append(m_dimension);
   return dimensions;
}

template<class T>
IValueDefinition* ComponentDataItem1D<T>::valueDefinition() const
{
   return m_valueDefinition;
}

template<class T>
void ComponentDataItem1D<T>::getValue(int dimensionIndexes[], QVariant & data) const
{
   data = QVariant::fromValue(m_data[dimensionIndexes[0]]);
}

template<class T>
void ComponentDataItem1D<T>::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
   for(int i = 0 ; i < stride[0] ; i++)
   {
      data[i] = QVariant::fromValue(m_data[dimensionIndexes[0] + i]);
   }
}

template<class T>
void ComponentDataItem1D<T>::getValues(int dimensionIndexes[], int stride[], void *data) const
{
   T* dataCast = (T*) data;

   for(int i = 0 ; i < stride[0] ; i++)
   {
      dataCast[i] = m_data[dimensionIndexes[0] + i];
   }
}

template<class T>
void ComponentDataItem1D<T>::setValue(int dimensionIndexes[], const QVariant &data)
{
   m_data[dimensionIndexes[0]] = qvariant_cast<T>(data);
}

template<class T>
void ComponentDataItem1D<T>::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
   for(int i = 0 ; i < stride[0] ; i++)
   {
      m_data[dimensionIndexes[0] + i] = qvariant_cast<T>(data[i]);
   }
}

template<class T>
void ComponentDataItem1D<T>::setValues(int dimensionIndexes[], int stride[], const void *data)
{
   T* dataCast = (T*) data;

   for(int i = 0 ; i < stride[0] ; i++)
   {
      m_data[dimensionIndexes[0] + i] = dataCast[i];
   }
}

template<class T>
void ComponentDataItem1D<T>::resetDataArray()
{
   createData();
}

template<class T>
Dimension* ComponentDataItem1D<T>::hDimension() const
{
   return m_dimension;
}

template<class T>
ValueDefinition* ComponentDataItem1D<T>::hValueDefinition() const
{
   return m_valueDefinition;
}


template<class T>
void ComponentDataItem1D<T>::createData()
{
   if(m_data)
      deleteData();

   m_data = new T[m_dimension->length()];

   for(int i = 0 ; i < m_dimension->length() ; i++)
   {
      m_data[i] = qvariant_cast<T>(m_valueDefinition->defaultValue());
   }
}

template<class T>
void ComponentDataItem1D<T>::deleteData()
{
   if(m_data)
      delete[] m_data;
}


ComponentDataItem1DInt::ComponentDataItem1DInt(const QString &id, Dimension *dimension,
                                               ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent)
   :  ComponentDataItem1D<int>(dimension,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{
}

ComponentDataItem1DInt::~ComponentDataItem1DInt()
{
}

void ComponentDataItem1DInt::readData(QXmlStreamReader &xmlReader)
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
            while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
            {
               if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
               {
                  QXmlStreamAttributes attributes = xmlReader.attributes();

                  if(attributes.hasAttribute("Index"))
                  {
                     int index = attributes.value("Index").toString().toInt();

                     QString value = xmlReader.readElementText();

                     setValue(&index,value.toInt());
                  }
               }
               xmlReader.readNext();
            }
         }
         xmlReader.readNext();
      }
   }
}

void ComponentDataItem1DInt::writeData(QXmlStreamWriter &xmlWriter)
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
               xmlWriter.writeAttribute("Index" , QString::number(i));
               xmlWriter.writeCharacters(QString::number(values[i]));
            }
            xmlWriter.writeEndElement();
         }
      }
      xmlWriter.writeEndElement();
   }
   xmlWriter.writeEndElement();
}

ComponentDataItem1DDouble::ComponentDataItem1DDouble(const QString &id, Dimension* dimension,
                                                     ValueDefinition* valueDefinition, AbstractModelComponent *parentModelComponent)
   : ComponentDataItem1D<double>(dimension,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{
}

ComponentDataItem1DDouble::~ComponentDataItem1DDouble()
{
}

void ComponentDataItem1DDouble::readData(QXmlStreamReader &xmlReader)
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
            while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
            {
               if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
               {
                  QXmlStreamAttributes attributes = xmlReader.attributes();

                  if(attributes.hasAttribute("Index"))
                  {
                     int index = attributes.value("Index").toString().toInt();

                     QString value = xmlReader.readElementText();

                     setValue(&index,value.toDouble());
                  }
               }
               xmlReader.readNext();
            }
         }
         xmlReader.readNext();
      }
   }
}

void ComponentDataItem1DDouble::writeData(QXmlStreamWriter &xmlWriter)
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
               xmlWriter.writeAttribute("Index" , QString::number(i));
               xmlWriter.writeCharacters(QString::number(values[i]));
            }
            xmlWriter.writeEndElement();
         }
      }
      xmlWriter.writeEndElement();
   }
   xmlWriter.writeEndElement();
}


ComponentDataItem1DString::ComponentDataItem1DString(const QString &id, Dimension *dimension,
                                                     ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent)
   : ComponentDataItem1D<QString>(dimension,valueDefinition), AbstractComponentDataItem(id,parentModelComponent)
{

}

ComponentDataItem1DString::~ComponentDataItem1DString()
{

}

void ComponentDataItem1DString::readData(QXmlStreamReader &xmlReader)
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
            while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
            {
               if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
               {
                  QXmlStreamAttributes attributes = xmlReader.attributes();

                  if(attributes.hasAttribute("Index"))
                  {
                     int index = attributes.value("Index").toString().toInt();

                     QString value = xmlReader.readElementText();

                     setValue(&index,value);
                  }
               }
               xmlReader.readNext();
            }
         }
         xmlReader.readNext();
      }
   }
}

void ComponentDataItem1DString::writeData(QXmlStreamWriter &xmlWriter)
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
         QString *values = new QString[hDimension()->length()];
         getValues(ind,str,values);

         for(int i = 0 ; i < hDimension()->length() ; i++)
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
