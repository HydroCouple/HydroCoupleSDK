#include "stdafx.h"
#include "core/argument1d.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include <QFile>

Argument1DInt::Argument1DInt(const QString &id, Dimension *dimension,
                             ValueDefinition *valueDefinition, AbstractModelComponent *modelComponent):
  ComponentDataItem1D<int>(dimension , valueDefinition),
  AbstractArgument(id,modelComponent)
{

}

void Argument1DInt::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().compare("Argument1D", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Argument1D", Qt::CaseInsensitive)) && !xmlReader.hasError())
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
      else if(!xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        hValueDefinition()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
        }
      }
      else if(!xmlReader.name().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

        std::vector<int> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QString value = xmlReader.readElementText();
            values.push_back(value.toInt());
          }

          xmlReader.readNext();
        }

        if(values.size() == hDimension()->length())
        {
          int ind[1] ={0};
          int str[1] = {hDimension()->length()};
          setValues(ind,str,values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void Argument1DInt::writeData(QXmlStreamWriter &xmlWriter)
{
  xmlWriter.writeStartElement("Argument1D");
  {
    xmlWriter.writeAttribute("Id" , id());
    xmlWriter.writeAttribute("Caption" , caption());
    xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

    //write value definition;
    hValueDefinition()->writeData(xmlWriter);

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
          xmlWriter.writeCharacters(QString::number(values[i]));
        }
        xmlWriter.writeEndElement();
      }
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

QString Argument1DInt::toString() const
{
  if(currentArgumentIOType() == HydroCouple::File)
  {
    QFile file(m_inputFile.absoluteFilePath());

    if(file.open(QIODevice::WriteOnly))
    {
      QXmlStreamWriter xmlWriter(&file);
      xmlWriter.setAutoFormatting(true);

      xmlWriter.writeStartElement("Argument1D");
      {
        xmlWriter.writeAttribute("Id" , id());
        xmlWriter.writeAttribute("Caption" , caption());
        xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

        //write value definition;
        hValueDefinition()->writeData(xmlWriter);

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
    xmlWriter.writeStartElement("Argument1D");
    {
      xmlWriter.writeAttribute("Id" , id());
      xmlWriter.writeAttribute("Caption" , caption());
      xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

      //write value definition;
      hValueDefinition()->writeData(xmlWriter);

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

bool Argument1DInt::readValues(const QString &value, bool isFile)
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
          if(!xmlReader.name().compare("Argument1D", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            readData(xmlReader);
          }
          xmlReader.readNext();
        }

        if(!xmlReader.hasError())
        {
          setArgumentIOType(HydroCouple::File);
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
      if(!xmlReader.name().compare("Argument1D", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        readData(xmlReader);
      }

      xmlReader.readNext();
    }

    if(!xmlReader.hasError())
    {
      setArgumentIOType(HydroCouple::String);
      return true;
    }
  }

  return false;
}

bool Argument1DInt::readValues(const IComponentDataItem *componentDataItem)
{

  if(componentDataItem->valueDefinition()->type() == hValueDefinition()->type() &&
     componentDataItem->dimensions().length() == 1)
  {
    hDimension()->setLength(componentDataItem->dimensions()[0]->length());
    resetDataArray();
    int ind[1] = {0};
    int str[1] = {hDimension()->length()};
    int data[hDimension()->length()];
    componentDataItem->getValues(ind,str,data);
    setValues(ind,str,data);
    return true;
  }

  return false;
}



Argument1DDouble::Argument1DDouble(const QString &id, Dimension *dimension,
                                   ValueDefinition *valueDefinition, AbstractModelComponent *modelComponent):
  ComponentDataItem1D<double>(dimension , valueDefinition),
  AbstractArgument(id,modelComponent)
{

}

void Argument1DDouble::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().compare("Argument1D", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Argument1D", Qt::CaseInsensitive)) && !xmlReader.hasError())
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
      else if(!xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        hValueDefinition()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
        }
      }
      else if(!xmlReader.name().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

        std::vector<double> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QString value = xmlReader.readElementText();
            values.push_back(value.toDouble());
          }

          xmlReader.readNext();
        }

        if(values.size() == hDimension()->length())
        {
          int ind[1] = {0};
          int str[1] = {hDimension()->length()};

          setValues(ind,str,values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void Argument1DDouble::writeData(QXmlStreamWriter &xmlWriter)
{
  xmlWriter.writeStartElement("Argument1D");
  {
    xmlWriter.writeAttribute("Id" , id());
    xmlWriter.writeAttribute("Caption" , caption());
    xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

    //write value definition;
    hValueDefinition()->writeData(xmlWriter);

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
          xmlWriter.writeCharacters(QString::number(values[i]));
        }
        xmlWriter.writeEndElement();
      }
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

QString Argument1DDouble::toString() const
{
  if(currentArgumentIOType() == HydroCouple::File)
  {
    QFile file(m_inputFile.absoluteFilePath());

    if(file.open(QIODevice::WriteOnly))
    {
      QXmlStreamWriter xmlWriter(&file);
      xmlWriter.setAutoFormatting(true);

      xmlWriter.writeStartElement("Argument1D");
      {
        xmlWriter.writeAttribute("Id" , id());
        xmlWriter.writeAttribute("Caption" , caption());
        xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

        //write value definition;
        hValueDefinition()->writeData(xmlWriter);

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
    xmlWriter.writeStartElement("Argument1D");
    {
      xmlWriter.writeAttribute("Id" , id());
      xmlWriter.writeAttribute("Caption" , caption());
      xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

      //write value definition;
      hValueDefinition()->writeData(xmlWriter);

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

bool Argument1DDouble::readValues(const QString &value, bool isFile)
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
          if(!xmlReader.name().compare("Argument1D", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            readData(xmlReader);
          }
          xmlReader.readNext();
        }

        if(!xmlReader.hasError())
        {
          setArgumentIOType(HydroCouple::File);
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
      if(!xmlReader.name().compare("Argument1D", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        readData(xmlReader);
      }

      xmlReader.readNext();
    }

    if(!xmlReader.hasError())
    {
      setArgumentIOType(HydroCouple::String);
      return true;
    }
  }

  return false;
}

bool Argument1DDouble::readValues(const IComponentDataItem *componentDataItem)
{

  if(componentDataItem->valueDefinition()->type() == hValueDefinition()->type() &&
     componentDataItem->dimensions().length() == 1)
  {
    hDimension()->setLength(componentDataItem->dimensions()[0]->length());
    resetDataArray();
    int ind[1] = {0};
    int str[1] = {hDimension()->length()};
    double data[hDimension()->length()];
    componentDataItem->getValues(ind,str,data);
    setValues(ind,str,data);
    return true;
  }

  return false;
}



Argument1DString::Argument1DString(const QString &id, Dimension *dimension,
                                   ValueDefinition *valueDefinition, AbstractModelComponent *modelComponent):
  ComponentDataItem1D<QString>(dimension , valueDefinition),
  AbstractArgument(id,modelComponent)
{

}

void Argument1DString::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().compare("Argument1D", Qt::CaseInsensitive)
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

    while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Argument1D", Qt::CaseInsensitive)) && !xmlReader.hasError())
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
      else if(!xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        hValueDefinition()->readData(xmlReader);

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("ValueDefinition", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
        }
      }
      else if(!xmlReader.name().compare("Values", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {

        std::vector<QString> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QString value = xmlReader.readElementText();
            values.push_back(value);
          }

          xmlReader.readNext();
        }

        if(values.size() == hDimension()->length())
        {
          int ind[1] = {0};
          int str[1] = {hDimension()->length()};
          setValues(ind, str,values.data());
        }
      }

      xmlReader.readNext();
    }
  }
}

void Argument1DString::writeData(QXmlStreamWriter &xmlWriter)
{
  xmlWriter.writeStartElement("Argument1D");
  {
    xmlWriter.writeAttribute("Id" , id());
    xmlWriter.writeAttribute("Caption" , caption());
    xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

    //write value definition;
    hValueDefinition()->writeData(xmlWriter);

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

QString Argument1DString::toString() const
{
  if(currentArgumentIOType() == HydroCouple::File)
  {
    QFile file(m_inputFile.absoluteFilePath());

    if(file.open(QIODevice::WriteOnly))
    {
      QXmlStreamWriter xmlWriter(&file);
      xmlWriter.setAutoFormatting(true);

      xmlWriter.writeStartElement("Argument1D");
      {
        xmlWriter.writeAttribute("Id" , id());
        xmlWriter.writeAttribute("Caption" , caption());
        xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

        //write value definition;
        hValueDefinition()->writeData(xmlWriter);

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
              xmlWriter.writeCharacters(values[i]);
            }
            xmlWriter.writeEndElement();
          }

          delete[] values;
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
    xmlWriter.writeStartElement("Argument1D");
    {
      xmlWriter.writeAttribute("Id" , id());
      xmlWriter.writeAttribute("Caption" , caption());
      xmlWriter.writeAttribute("IsOptional" , isOptional() ? "True" : "False");

      //write value definition;
      hValueDefinition()->writeData(xmlWriter);

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
            xmlWriter.writeCharacters(values[i]);
          }
          xmlWriter.writeEndElement();
        }

        delete [] values;
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndDocument();

  return data;
}

bool Argument1DString::readValues(const QString &value, bool isFile)
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
          if(!xmlReader.name().compare("Argument1D", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            readData(xmlReader);
          }
          xmlReader.readNext();
        }

        if(!xmlReader.hasError())
        {
          setArgumentIOType(HydroCouple::File);
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
      if(!xmlReader.name().compare("Argument1D", Qt::CaseInsensitive) && !xmlReader.hasError() && xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        readData(xmlReader);
      }

      xmlReader.readNext();
    }

    if(!xmlReader.hasError())
    {
      setArgumentIOType(HydroCouple::String);
      return true;
    }
  }

  return false;
}

bool Argument1DString::readValues(const IComponentDataItem *componentDataItem)
{

  if(componentDataItem->valueDefinition()->type() == hValueDefinition()->type() &&
     componentDataItem->dimensions().length() == 1)
  {
    hDimension()->setLength(componentDataItem->dimensions()[0]->length());
    resetDataArray();
    int ind[1] = {0};
    int str[1] = {hDimension()->length()};
    QString *data = new QString[hDimension()->length()];
    componentDataItem->getValues(ind,str,data);
    setValues(ind,str,data);

    delete[] data;

    return true;
  }

  return false;
}

