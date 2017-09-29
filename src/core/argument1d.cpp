#include "stdafx.h"
#include "core/argument1d.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/abstractmodelcomponent.h"

#include <QFile>
#include <assert.h>

Argument1DInt::Argument1DInt(const QString& id,
                             Dimension* dimension,
                             int length,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent *modelComponent)
  : AbstractArgument(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<int>(id, length , valueDefinition->defaultValue().toInt()),
    m_dimension(dimension)
{
}

Argument1DInt::~Argument1DInt()
{

}

int Argument1DInt::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  return length();
}

void Argument1DInt::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<int>::getValueT(dimensionIndexes,data);
}

void Argument1DInt::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<int>::setValueT(dimensionIndexes,data);
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

              if(!m_dimension->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                resizeDataArray(length.toInt());
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

        std::vector<int> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            int value = atoi(qPrintable(xmlReader.readElementText()));
            values.push_back(value);
          }

          xmlReader.readNext();
        }

        if((int)values.size() == length())
        {
          ComponentDataItem1D<int>::setValuesT(0,length(),values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void Argument1DInt::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("Argument1D");
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
        xmlWriter.writeAttribute("Id" , m_dimension->id());
        xmlWriter.writeAttribute("Caption" , m_dimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      int values[length()];
      ComponentDataItem1D<int>::getValuesT(0,length(),values);

      for(int i = 0 ; i < length() ; i++)
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

void Argument1DInt::saveData()
{
  if(currentArgumentIOType() == IArgument::File)
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
            xmlWriter.writeAttribute("Id" , m_dimension->id());
            xmlWriter.writeAttribute("Caption" , m_dimension->caption());
            xmlWriter.writeAttribute("Length" , QString::number(length()));
          }
          xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("Values");
        {
          int values[length()];
          ComponentDataItem1D<int>::getValuesT(0,length(),values);

          for(int i = 0 ; i < length() ; i++)
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

      return;
    }
  }
}

QString Argument1DInt::toString() const
{
  if(currentArgumentIOType() == IArgument::File)
  {
    return  modelComponentInternal()->getRelativeFilePath(m_inputFile.absoluteFilePath()).filePath();
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

bool Argument1DInt::readValues(const QString &value, QString &message, bool isFile)
{
  message = "";

  if(isFile)
  {
    m_inputFile = modelComponentInternal()->getAbsoluteFilePath(value);

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
          setArgumentIOType(IArgument::File);
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
      setArgumentIOType(IArgument::String);
      return true;
    }
  }

  return false;
}

bool Argument1DInt::readValues(const IComponentDataItem *componentDataItem, QString &message)
{
  message = "";

  if(componentDataItem->valueDefinition()->type() == valueDefinitionInternal()->type() &&
     componentDataItem->dimensions().length() == 1)
  {
    int size = componentDataItem->dimensionLength(std::vector<int>());
    resizeDataArray(size);

    std::vector<int> indexes(1);
    for(int i = 0; i < size ; i++)
    {
      indexes[0] = i;
      int data = defaultValue();
      componentDataItem->getValue(indexes, &data);
      setValue(indexes,&data);
    }

    return true;
  }

  return false;
}

//===================================================================================================================

Argument1DDouble::Argument1DDouble(const QString& id,
                                   Dimension* dimension,
                                   int length,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent *modelComponent)
  : AbstractArgument(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<double>(id, length , valueDefinition->defaultValue().toDouble()),
    m_dimension(dimension)
{
}

Argument1DDouble::~Argument1DDouble()
{

}

int Argument1DDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  return length();
}

void Argument1DDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void Argument1DDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
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

              if(!m_dimension->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                resizeDataArray(length.toInt());
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

        std::vector<double> values;

        while (!(xmlReader.isEndElement() && !xmlReader.name().compare("Values", Qt::CaseInsensitive)) && !xmlReader.hasError())
        {
          if(!xmlReader.name().compare("Value", Qt::CaseInsensitive) && !xmlReader.hasError() &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            double value = atof(qPrintable(xmlReader.readElementText()));
            values.push_back(value);
          }

          xmlReader.readNext();
        }

        if((int)values.size() == length())
        {
          ComponentDataItem1D<double>::setValuesT(0,length(),values.data());
        }
      }
      xmlReader.readNext();
    }
  }
}

void Argument1DDouble::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("Argument1D");
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
        xmlWriter.writeAttribute("Id" , m_dimension->id());
        xmlWriter.writeAttribute("Caption" , m_dimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      double values[length()];
      ComponentDataItem1D<double>::getValuesT(0,length(),values);


      for(int i = 0 ; i < length() ; i++)
      {
        xmlWriter.writeStartElement("Value");
        {
          xmlWriter.writeCharacters(QString::number(values[i],'g',10));
        }
        xmlWriter.writeEndElement();
      }
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

void Argument1DDouble::saveData()
{
  if(currentArgumentIOType() == IArgument::File)
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
            xmlWriter.writeAttribute("Id" , m_dimension->id());
            xmlWriter.writeAttribute("Caption" , m_dimension->caption());
            xmlWriter.writeAttribute("Length" , QString::number(length()));
          }
          xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("Values");
        {
          double values[length()];
          ComponentDataItem1D<double>::getValuesT(0,length(),values);

          for(int i = 0 ; i < length() ; i++)
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

      return;
    }
  }
}

QString Argument1DDouble::toString() const
{
  if(currentArgumentIOType() == IArgument::File)
  {
    return   modelComponentInternal()->getRelativeFilePath(m_inputFile.absoluteFilePath()).filePath();
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

bool Argument1DDouble::readValues(const QString &value, QString &message, bool isFile)
{
  message = "";

  if(isFile)
  {
    m_inputFile = modelComponentInternal()->getAbsoluteFilePath(value);

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
          setArgumentIOType(IArgument::File);
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
      setArgumentIOType(IArgument::String);
      return true;
    }
  }

  return false;
}

bool Argument1DDouble::readValues(const IComponentDataItem *componentDataItem, QString &message)
{

  message = "";

  if(componentDataItem->valueDefinition()->type() == valueDefinitionInternal()->type() &&
     componentDataItem->dimensions().length() == 1)
  {
    int size = componentDataItem->dimensionLength(std::vector<int>());
    resizeDataArray(size);

    std::vector<int> indexes(1);
    for(int i = 0; i < size ; i++)
    {
      indexes[0] = i;
      double data = defaultValue();
      componentDataItem->getValue(indexes, &data);
      setValue(indexes, &data);
    }

    return true;
  }

  return false;
}

//===================================================================================================================

Argument1DString::Argument1DString(const QString& id,
                                   Dimension* dimension,
                                   int length,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent *modelComponent)
  : AbstractArgument(id,QList<Dimension*>({dimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<QString>(id, length, valueDefinition->defaultValue().toString()),
    m_dimension(dimension)
{
}

Argument1DString::~Argument1DString()
{

}

int Argument1DString::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  return length();
}

void Argument1DString::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  ComponentDataItem1D<QString>::getValueT(dimensionIndexes,data);
}

void Argument1DString::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<QString>::setValueT(dimensionIndexes,data);
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

              if(!m_dimension->id().compare(id))
              {
                QString length = attributes.value("Length").toString();
                resizeDataArray(length.toInt());
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

        if((int)values.size() == length())
        {
          ComponentDataItem1D<QString>::setValuesT(0, length(),values.data());
        }
      }

      xmlReader.readNext();
    }
  }
}

void Argument1DString::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeStartElement("Argument1D");
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
        xmlWriter.writeAttribute("Id" , m_dimension->id());
        xmlWriter.writeAttribute("Caption" , m_dimension->caption());
        xmlWriter.writeAttribute("Length" , QString::number(length()));
      }
      xmlWriter.writeEndElement();
    }
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Values");
    {
      QString* values = new QString[length()];
      ComponentDataItem1D<QString>::getValuesT(0,length(),values);

      for(int i = 0 ; i < length() ; i++)
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

void Argument1DString::saveData()
{
  if(currentArgumentIOType() == IArgument::File)
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
            xmlWriter.writeAttribute("Id" , m_dimension->id());
            xmlWriter.writeAttribute("Caption" , m_dimension->caption());
            xmlWriter.writeAttribute("Length" , QString::number(length()));
          }
          xmlWriter.writeEndElement();
        }
        xmlWriter.writeEndElement();

        xmlWriter.writeStartElement("Values");
        {
          QString *values = new QString[length()];
          ComponentDataItem1D<QString>::getValuesT(0,length(),values);

          for(int i = 0 ; i < length() ; i++)
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

      return;
    }
  }
}

QString Argument1DString::toString() const
{
  if(currentArgumentIOType() == IArgument::File)
  {
    return  modelComponentInternal()->getRelativeFilePath(m_inputFile.absoluteFilePath()).filePath();
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

bool Argument1DString::readValues(const QString &value, QString &message, bool isFile)
{
  message = "";

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
          setArgumentIOType(IArgument::File);
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
      setArgumentIOType(IArgument::String);
      return true;
    }
  }

  return false;
}

bool Argument1DString::readValues(const IComponentDataItem *componentDataItem, QString &message)
{
  message = "";
  if(componentDataItem->valueDefinition()->type() == valueDefinitionInternal()->type() &&
     componentDataItem->dimensions().length() == 1)
  {
    int size = componentDataItem->dimensionLength(std::vector<int>());
    resizeDataArray(size);

    std::vector<int> indexes(1);
    for(int i = 0; i < size ; i++)
    {
      indexes[0] = i;
      QString data = defaultValue();
      componentDataItem->getValue(indexes, &data);
      setValue(indexes,&data);
    }

    return true;
  }

  return false;
}



