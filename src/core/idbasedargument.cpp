#include "stdafx.h"
#include "core/idbasedargument.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/idbasedcomponentdataitem.h"
#include <QString>
#include <QDebug>

using namespace HydroCouple;

IdBasedArgumentInt::IdBasedArgumentInt(const QString &id, const QStringList &identifiers,
                                       Dimension *dimension, ValueDefinition* valueDefinition,
                                       AbstractModelComponent *parentModelComponent)
   : IdBasedComponentDataItem<int>(identifiers,dimension,valueDefinition),
     AbstractArgument(id,parentModelComponent) , m_matchIdentifiersDuringRead(false)

{

}

IdBasedArgumentInt::~IdBasedArgumentInt()
{

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

                     if(!hDimension()->id().compare(id))
                     {
                        QString length = attributes.value("Length").toString();

                        if(m_matchIdentifiersDuringRead &&
                              length.toInt() != hDimension()->length())
                        {
                           return;
                        }
                        else
                        {
                           hDimension()->setLength(length.toInt());
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
            hValueDefinition()->readData(xmlReader);

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
         int values[hDimension()->length()];
         IdBasedComponentDataItem<int>::getValues(0,hDimension()->length(),values);

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
            int values[hDimension()->length()];
            IdBasedComponentDataItem<int>::getValues(0,hDimension()->length(),values);

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
      if(hValueDefinition()->type() == idBasedData->valueDefinition()->type() && idBasedData->dimensions().length() == 1)
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
                  IdBasedComponentDataItem<int>::setValues(curId,1,&value);
               }

               curId ++;
            }

            return true;
         }
         else
         {
            int dimLength = idBasedData->dimensions()[0]->length();
            clearIdentifiers();
            addIdentifiers(inputIdentifiers);
            int values[dimLength];
            idBasedData->getValues(0,dimLength,values);
            IdBasedComponentDataItem<int>::setValues(0,dimLength,values);

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


IdBasedArgumentDouble::IdBasedArgumentDouble(const QString &id, const QStringList &identifiers,
                                       Dimension *dimension, ValueDefinition* valueDefinition,
                                       AbstractModelComponent *parentModelComponent)
   : IdBasedComponentDataItem<double>(identifiers,dimension,valueDefinition),
     AbstractArgument(id,parentModelComponent) , m_matchIdentifiersDuringRead(false)

{

}

IdBasedArgumentDouble::~IdBasedArgumentDouble()
{

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

                     if(!hDimension()->id().compare(id))
                     {
                        QString length = attributes.value("Length").toString();

                        if(m_matchIdentifiersDuringRead &&
                              length.toInt() != hDimension()->length())
                        {
                           return;
                        }
                        else
                        {
                           hDimension()->setLength(length.toInt());
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
            hValueDefinition()->readData(xmlReader);

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
                        IdBasedComponentDataItem<double>::setValues(curId,1,&values[nId]);
                     }

                     curId ++;
                  }
               }
               else
               {
                  clearIdentifiers();
                  addIdentifiers(tempIdentifiers);
                  IdBasedComponentDataItem<double>::setValues(0,tempIdentifiers.length(),values.data());
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
         double values[hDimension()->length()];
         IdBasedComponentDataItem<double>::getValues(0,hDimension()->length(),values);

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
               double values[hDimension()->length()];
               IdBasedComponentDataItem<double>::getValues(0,hDimension()->length(),values);

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
            double values[hDimension()->length()];
            IdBasedComponentDataItem<double>::getValues(0,hDimension()->length(),values);

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
      if(hValueDefinition()->type() == idBasedData->valueDefinition()->type())
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
                  IdBasedComponentDataItem<double>::setValues(curId,1,&value);
               }

               curId ++;
            }

            return true;
         }
         else
         {
            int dimLength  = idBasedData->dimensions()[0]->length();
            clearIdentifiers();
            addIdentifiers(inputIdentifiers);
            int values[dimLength];
            idBasedData->getValues(0,dimLength,values);
            IdBasedComponentDataItem<double>::setValues(0,dimLength,values);

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



IdBasedArgumentQString::IdBasedArgumentQString(const QString &id, const QStringList &identifiers,
                                       Dimension *dimension, ValueDefinition* valueDefinition,
                                       AbstractModelComponent *parentModelComponent)
   : IdBasedComponentDataItem<QString>(identifiers,dimension,valueDefinition),
     AbstractArgument(id,parentModelComponent) , m_matchIdentifiersDuringRead(false)

{

}

IdBasedArgumentQString::~IdBasedArgumentQString()
{

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

                     if(!hDimension()->id().compare(id))
                     {
                        QString length = attributes.value("Length").toString();

                        if(m_matchIdentifiersDuringRead &&
                              length.toInt() != hDimension()->length())
                        {
                           return;
                        }
                        else
                        {
                           hDimension()->setLength(length.toInt());
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
            hValueDefinition()->readData(xmlReader);

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
                        IdBasedComponentDataItem<QString>::setValues(curId,1,&values[nId]);
                     }

                     curId ++;
                  }
               }
               else
               {
                  clearIdentifiers();
                  addIdentifiers(tempIdentifiers);
                  IdBasedComponentDataItem<QString>::setValues(0,tempIdentifiers.length(),values.data());
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
         double values[hDimension()->length()];
         IdBasedComponentDataItem<QString>::getValues(0,hDimension()->length(),values);

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
               double values[hDimension()->length()];
               IdBasedComponentDataItem<QString>::getValues(0,hDimension()->length(),values);

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
            qDebug() << hDimension()->length();

            QString *values = new QString[hDimension()->length()];
            IdBasedComponentDataItem<QString>::getValues(0,hDimension()->length(),values);

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
    const IIdBasedComponentDataItem *idBasedData = dynamic_cast<const IdBasedComponentDataItem*>(componentDataItem);

   if(idBasedData)
   {
      if(hValueDefinition()->type() == idBasedData->valueDefinition()->type() && idBasedData->dimensions().length() == 1)
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
                  IdBasedComponentDataItem<QString>::setValues(curId,1,&value);
               }

               curId ++;
            }

            return true;
         }
         else
         {
            int dimLength = idBasedData->dimensions()[0]->length();
            clearIdentifiers();
            addIdentifiers(inputIdentifiers);
            QString *values = new QString[dimLength];
            idBasedData->getValues(0,dimLength,values);
            IdBasedComponentDataItem<QString>::setValues(0,dimLength,values);

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
