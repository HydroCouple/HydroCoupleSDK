#include "stdafx.h"
#include "core/componentdataitem1d.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"

#include <QDebug>
#include <assert.h>

using namespace HydroCouple;

template<class T>
ComponentDataItem1D<T>::ComponentDataItem1D(int length, const T& defaultValue)
  : m_length(length),
    m_defaultValue(defaultValue),
    m_data(nullptr)
{
  createData();
}

template<class T>
ComponentDataItem1D<T>::~ComponentDataItem1D()
{
  deleteData();
}

template<class T>
void ComponentDataItem1D<T>::getValueT(int dimensionIndexes[], QVariant & data) const
{
  data = QVariant::fromValue(m_data[dimensionIndexes[0]]);
}

template<class T>
void ComponentDataItem1D<T>::getValuesT(int dimensionIndexes[], int stride[], QVariant* data) const
{
  for(int i = 0 ; i < stride[0] ; i++)
  {
    data[i] = QVariant::fromValue(m_data[dimensionIndexes[0] + i]);
  }
}

template<class T>
void ComponentDataItem1D<T>::getValuesT(int dimensionIndexes[], int stride[], void *data) const
{
  T* dataCast = (T*) data;
  
  for(int i = 0 ; i < stride[0] ; i++)
  {
    dataCast[i] = m_data[dimensionIndexes[0] + i];
  }
}

template<class T>
void ComponentDataItem1D<T>::setValueT(int dimensionIndexes[], const QVariant &data)
{
  m_data[dimensionIndexes[0]] = qvariant_cast<T>(data);
}

template<class T>
void ComponentDataItem1D<T>::setValuesT(int dimensionIndexes[], int stride[], const QVariant data[])
{
  for(int i = 0 ; i < stride[0] ; i++)
  {
    m_data[dimensionIndexes[0] + i] = qvariant_cast<T>(data[i]);
  }
}

template<class T>
void ComponentDataItem1D<T>::setValuesT(int dimensionIndexes[], int stride[], const void *data)
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
void ComponentDataItem1D<T>::resetDataArray(int length)
{
  m_length = length;
  createData();
}

template<class T>
T ComponentDataItem1D<T>::defaultValue() const
{
  return m_defaultValue;
}

template<class T>
void ComponentDataItem1D<T>::setDefaultValue(const T &defaultValue)
{
  m_defaultValue = defaultValue;
}

template<class T>
int ComponentDataItem1D<T>::length() const
{
  return m_length;
}

template<class T>
void ComponentDataItem1D<T>::setLength(int length)
{
  m_length = length;
}

template<class T>
void ComponentDataItem1D<T>::createData()
{
  deleteData();
  
  if(m_length > 0)
  {
    m_data = new T[m_length];

    for(int i = 0 ; i < m_length ; i++)
    {
      m_data[i] = m_defaultValue;
    }
  }
}

template<class T>
void ComponentDataItem1D<T>::deleteData()
{
  if(m_data)
  {
    delete[] m_data;
    m_data = nullptr;
  }
}

//==============================================================================================================================

ComponentDataItem1DInt::ComponentDataItem1DInt(const QString &id,
                                               Dimension* dimension,
                                               int length,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent *modelComponent)
  : AbstractComponentDataItem(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<int>(length,valueDefinition->defaultValue().toInt())
{
}

ComponentDataItem1DInt::~ComponentDataItem1DInt()
{
}

int ComponentDataItem1DInt::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void ComponentDataItem1DInt::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<int>::getValueT(dimensionIndexes,data);
}

void ComponentDataItem1DInt::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<int>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem1DInt::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<int>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem1DInt::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<int>::setValueT(dimensionIndexes,data);
}

void ComponentDataItem1DInt::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<int>::setValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem1DInt::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<int>::setValuesT(dimensionIndexes,stride,data);
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
              
              if(!dimensions()[0]->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                resetDataArray(length.toInt());

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
      int values[length()];
      getValues(ind,str,values);
      
      for(int i = 0 ; i < length() ; i++)
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


//==============================================================================================================================


ComponentDataItem1DDouble::ComponentDataItem1DDouble(const QString& id,
                                                     Dimension* dimension,
                                                     int dimensionLength,
                                                     ValueDefinition* valueDefinition,
                                                     AbstractModelComponent *modelComponent)
  : AbstractComponentDataItem(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<double>(dimensionLength,valueDefinition->defaultValue().toDouble())
{
}

ComponentDataItem1DDouble::~ComponentDataItem1DDouble()
{
}

int ComponentDataItem1DDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  return length();
}

void ComponentDataItem1DDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void ComponentDataItem1DDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem1DDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem1DDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void ComponentDataItem1DDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem1DDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
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
              
              if(!dimensions()[0]->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                resetDataArray(length.toInt());
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
      int values[length()];
      getValues(ind,str,values);
      
      for(int i = 0 ; i < length() ; i++)
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

//==============================================================================================================================

ComponentDataItem1DString::ComponentDataItem1DString(const QString& id,
                                                     Dimension* dimension,
                                                     int length,
                                                     ValueDefinition* valueDefinition,
                                                     AbstractModelComponent* modelComponent)
  : AbstractComponentDataItem(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<QString>(length,valueDefinition->defaultValue().toString())
{
}

ComponentDataItem1DString::~ComponentDataItem1DString()
{
}

int ComponentDataItem1DString::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void ComponentDataItem1DString::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<QString>::getValueT(dimensionIndexes,data);
}

void ComponentDataItem1DString::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem1DString::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<QString>::getValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem1DString::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<QString>::setValueT(dimensionIndexes,data);
}

void ComponentDataItem1DString::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<QString>::setValuesT(dimensionIndexes,stride,data);
}

void ComponentDataItem1DString::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<QString>::setValuesT(dimensionIndexes,stride,data);
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
              
              if(!dimensions()[0]->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                resetDataArray(length.toInt());
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
      getValues(ind,str,values);
      
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
