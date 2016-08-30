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
    ComponentDataItem1D<int>(length,valueDefinition->defaultValue().toInt())
{
}

Input1DInt::~Input1DInt()
{
}

int Input1DInt::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void Input1DInt::getValue(int dimensionIndexes[], QVariant & data) const
{
  getValueT(dimensionIndexes,data);
}

void Input1DInt::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  getValuesT(dimensionIndexes,stride,data);
}

void Input1DInt::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  getValuesT(dimensionIndexes,stride,data);
}

void Input1DInt::setValue(int dimensionIndexes[], const QVariant &data)
{
  setValueT(dimensionIndexes,data);
}

void Input1DInt::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  setValuesT(dimensionIndexes,stride,data);
}

void Input1DInt::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  setValuesT(dimensionIndexes,stride,data);
}

void Input1DInt::readData(QXmlStreamReader &xmlReader)
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

void Input1DInt::writeData(QXmlStreamWriter &xmlWriter)
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


Input1DDouble::Input1DDouble(const QString& id,
                             Dimension* dimension,
                             int dimensionLength,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent *modelComponent)
  : AbstractInput(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<double>(dimensionLength,valueDefinition->defaultValue().toDouble())
{
}

Input1DDouble::~Input1DDouble()
{
}

int Input1DDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  return length();
}

void Input1DDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void Input1DDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void Input1DDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void Input1DDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void Input1DDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void Input1DDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void Input1DDouble::readData(QXmlStreamReader &xmlReader)
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

void Input1DDouble::writeData(QXmlStreamWriter &xmlWriter)
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

Input1DString::Input1DString(const QString& id,
                             Dimension* dimension,
                             int length,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<QString>(length,valueDefinition->defaultValue().toString())
{
}

Input1DString::~Input1DString()
{
}

int Input1DString::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void Input1DString::getValue(int dimensionIndexes[], QVariant & data) const
{
  getValueT(dimensionIndexes,data);
}

void Input1DString::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  getValuesT(dimensionIndexes,stride,data);
}

void Input1DString::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  getValuesT(dimensionIndexes,stride,data);
}

void Input1DString::setValue(int dimensionIndexes[], const QVariant &data)
{
  setValueT(dimensionIndexes,data);
}

void Input1DString::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  setValuesT(dimensionIndexes,stride,data);
}

void Input1DString::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  setValuesT(dimensionIndexes,stride,data);
}

void Input1DString::readData(QXmlStreamReader &xmlReader)
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

void Input1DString::writeData(QXmlStreamWriter &xmlWriter)
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
