/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */


#include "stdafx.h"
#include "core/valuedefinition.h"
#include "core/unit.h"

#include <QDataStream>

using namespace HydroCouple;

ValueDefinition::ValueDefinition(QVariant::Type type, QObject *parent)
  :Description(parent) , m_type(type)
{
  m_defaultValue = QVariant(type);
  m_missingValue = QVariant(type);
}

ValueDefinition::ValueDefinition(const QString &caption, QVariant::Type type, QObject *parent)
  :Description(caption , parent) , m_type(type)
{
  m_defaultValue = QVariant(type);
  m_missingValue = QVariant(type);
}

ValueDefinition::~ValueDefinition()
{

}

QVariant::Type ValueDefinition::type() const
{
  return m_type;
}

QVariant ValueDefinition::missingValue() const
{
  return m_missingValue;
}

void ValueDefinition::setMissingValue(const QVariant &value)
{
  m_missingValue = value;
  emit propertyChanged("MissingVlaue");
}

QVariant ValueDefinition::defaultValue() const
{
  return m_defaultValue;
}

void ValueDefinition::setDefaultValue(const QVariant &defaultValue)
{
  m_defaultValue =  defaultValue;
  emit propertyChanged("DefaultValue");
}

bool ValueDefinition::validateData(const QVariant &value)
{
  if(value.type() != m_type)
    return false;
  return true;
}

QString ValueDefinition::serializeData(const QVariant &value)
{
//  QByteArray data;
//  QDataStream dataStream(&data , QIODevice::WriteOnly);
//  dataStream << value;
  return value.toString();//QString(data.constData());
}

QVariant ValueDefinition::deserializeData(const QString &value, QVariant::Type type)
{

  QVariant outData(value);
  outData.convert(type);

//  if(!value.isNull() && !value.isEmpty())
//  {
//    QByteArray data(value.toStdString().c_str());
//    QDataStream dataStream(&data , QIODevice::ReadOnly);
//    dataStream >> outData;
//  }

  return outData;
}

ValueDefinition *ValueDefinition::copy(const IValueDefinition *valueDefinition, QObject *parent)
{
  if(dynamic_cast<const HydroCouple::IQuality*>(valueDefinition))
  {
    return Quality::copy(dynamic_cast<const HydroCouple::IQuality*>(valueDefinition), parent);
  }
  if(dynamic_cast<const HydroCouple::IQuantity*>(valueDefinition))
  {
    return Quantity::copy(dynamic_cast<const HydroCouple::IQuantity*>(valueDefinition), parent);
  }

  return nullptr;
}

//=================================================================================================================

Quality::Quality(QVariant::Type type, const QList<QVariant> &categories, QObject *parent)
  :ValueDefinition(type,parent) , m_categories(categories)
{
}

Quality::Quality(const QString& caption, QVariant::Type type, const QList<QVariant> &categories, QObject *parent)
  :ValueDefinition(caption,type,parent) , m_categories(categories)
{
}

Quality::~Quality()
{
}

QList<QVariant> Quality::categories() const
{
  return m_categories;
}

void Quality::setCategories(const QList<QVariant> &categories)
{
  m_categories = categories;
  emit propertyChanged("Categories");
}

void Quality::addCategory(const QVariant &category)
{
  if(m_categories.contains(category))
  {
    m_categories.append(category);
    emit propertyChanged("Categories");
  }
}

bool Quality::isOrdered() const
{
  return m_isOrdered;
}

void Quality::setIsOrdered(bool isOrdered)
{
  m_isOrdered = isOrdered;
  emit propertyChanged("Ordered");
}

void Quality::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive)
     && !xmlReader.hasError()
     &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
  {
    QXmlStreamAttributes attributes = xmlReader.attributes();

    if(attributes.hasAttribute("MissingValue"))
    {
      QString value = attributes.value("MissingValue").toString();
      QVariant dataValue = deserializeData(value, m_type);
      setMissingValue(dataValue);
    }

    if(attributes.hasAttribute("DefaultValue"))
    {
      QString value = attributes.value("DefaultValue").toString();
      QVariant dataValue = deserializeData(value, m_type);
      setDefaultValue(dataValue);
    }

    while (!(xmlReader.isEndElement()
             && !xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive))
           && !xmlReader.hasError())
    {

      if(!xmlReader.name().toString().compare("Categories", Qt::CaseInsensitive)
         && !xmlReader.hasError()
         &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        attributes = xmlReader.attributes();

        if(attributes.hasAttribute("IsOrdered"))
        {
          QString ordered = attributes.value("IsOrdered").toString();
          m_isOrdered = !ordered.compare("True" , Qt::CaseInsensitive);
        }

        m_categories.clear();

        while (!(xmlReader.isEndElement()
                 && !xmlReader.name().toString().compare("Categories", Qt::CaseInsensitive))
               && !xmlReader.hasError())
        {

          if(!xmlReader.name().toString().compare("Category", Qt::CaseInsensitive)
             && !xmlReader.hasError()
             &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
          {
            QString value = xmlReader.readElementText();
            m_categories.append(deserializeData(value, m_type));
          }

          xmlReader.readNext();
        }

      }
      xmlReader.readNext();
    }
  }
}

void Quality::writeData(QXmlStreamWriter &xmlWriter)
{

  xmlWriter.writeStartElement("ValueDefinition");
  {
    xmlWriter.writeAttribute("Type" , "Quality");
    xmlWriter.writeAttribute("ValueType" , QVariant::typeToName(type()));
    xmlWriter.writeAttribute("MissingValue" , ValueDefinition::serializeData(missingValue()));
    xmlWriter.writeAttribute("DefaultValue" , ValueDefinition::serializeData(defaultValue()));

    xmlWriter.writeStartElement("Categories");
    {
      xmlWriter.writeAttribute("IsOrdered" , m_isOrdered ? "True" : "False");

      for(const QVariant &category : m_categories)
      {
        xmlWriter.writeTextElement("Category" , ValueDefinition::serializeData(category));
      }
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

ValueDefinition *Quality::copy(QObject *parent)
{
  return copy(this,parent);
}

Quality *Quality::copy(const IQuality *quality, QObject *parent)
{
  return new Quality(quality->caption(), quality->type(), quality->categories(), parent);
}

//=================================================================================================================

Quantity::Quantity(QVariant::Type type, Unit *unit, QObject *parent)
  :ValueDefinition(type, parent)
{
  m_unit = unit;
}

Quantity::Quantity(const QString &caption, QVariant::Type type, Unit *unit, QObject *parent)
  :ValueDefinition(caption, type, parent)
{
  m_unit = unit;
}

Quantity::~Quantity()
{

}

IUnit* Quantity::unit() const
{
  return m_unit;
}

QVariant Quantity::minValue() const
{
  return m_minValue;
}

void Quantity::setMinValue(const QVariant &minValue)
{
  m_minValue = minValue;
  emit propertyChanged("MinValue");
}

QVariant Quantity::maxValue() const
{
  return m_maxValue;
}

void Quantity::setMaxValue(const QVariant &maxValue)
{
  m_maxValue = maxValue;
  emit propertyChanged("MaxValue");
}

void Quantity::readData(QXmlStreamReader &xmlReader)
{
  if(!xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive)
     && !xmlReader.hasError()
     &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
  {
    QXmlStreamAttributes attributes = xmlReader.attributes();

    if(attributes.hasAttribute("MissingValue"))
    {
      QString value = attributes.value("MissingValue").toString();
      QVariant dataValue = deserializeData(value, m_type);
      setMissingValue(dataValue);
    }

    if(attributes.hasAttribute("DefaultValue"))
    {
      QString value = attributes.value("DefaultValue").toString();
      QVariant dataValue = deserializeData(value, m_type);
      setDefaultValue(dataValue);
    }

    while (!(xmlReader.isEndElement()
             && !xmlReader.name().toString().compare("ValueDefinition", Qt::CaseInsensitive))
           && !xmlReader.hasError())
    {

      if(!xmlReader.name().toString().compare("Unit", Qt::CaseInsensitive)
         && !xmlReader.hasError()
         &&  xmlReader.tokenType() == QXmlStreamReader::StartElement )
      {
        attributes = xmlReader.attributes();

        if(attributes.hasAttribute("ConversionFactorToSI"))
        {
          QString ordered = attributes.value("ConversionFactorToSI").toString();
          m_unit->setConversionFactorToSI(ordered.toDouble());
        }

        if(attributes.hasAttribute("OffsetToSI"))
        {
          QString ordered = attributes.value("OffsetToSI").toString();
          m_unit->setOffsetToSI(ordered.toDouble());
        }

        while (!(xmlReader.isEndElement()
                 && !xmlReader.name().toString().compare("Unit", Qt::CaseInsensitive))
               && !xmlReader.hasError())
        {

          xmlReader.readNext();
        }

      }

      xmlReader.readNext();
    }
  }
}

void Quantity::writeData(QXmlStreamWriter &xmlWriter)
{
  xmlWriter.writeStartElement("ValueDefinition");
  {
    xmlWriter.writeAttribute("Type" , "Quantity");
    xmlWriter.writeAttribute("ValueType" , QVariant::typeToName(type()));
    xmlWriter.writeAttribute("MissingValue" , ValueDefinition::serializeData(missingValue()));
    xmlWriter.writeAttribute("DefaultValue" , ValueDefinition::serializeData(defaultValue()));

    xmlWriter.writeStartElement("Unit");
    {
      xmlWriter.writeAttribute("ConversionFactorToSI" , QString::number(m_unit->conversionFactorToSI()));
      xmlWriter.writeAttribute("OffsetToSI" , QString::number(m_unit->offsetToSI()));
    }
    xmlWriter.writeEndElement();
  }
  xmlWriter.writeEndElement();
}

ValueDefinition *Quantity::copy(QObject *parent)
{
  return copy(this,parent);
}

Quantity *Quantity::copy(const IQuantity *quantity, QObject *parent)
{
  Quantity *outQuantity = new Quantity(quantity->caption(), quantity->type(),
                                       Unit::copy(quantity->unit(),parent), parent);
  return outQuantity;
}

Quantity* Quantity::lengthInMeters(QObject *parent)
{

  Quantity* quantity = new Quantity("Length in meters", QVariant::Double, Unit::lengthInMeters(parent),parent);
  quantity->setDefaultValue(0.0);
  quantity->setMissingValue(-99999999);
  quantity->setDescription("Length in meters");

  return quantity;
}

Quantity* Quantity::lengthInFeet(QObject *parent)
{
  Quantity* quantity = new Quantity("Length in feet",QVariant::Double, Unit::lengthInFeet(parent),parent);
  quantity->setDefaultValue(0.0);
  quantity->setMissingValue(-99999999);
  quantity->setDescription("Length in feet");

  return quantity;
}

Quantity* Quantity::areaInSquareMeters(QObject *parent)
{
  Quantity* quantity = new Quantity("Area in square meters",QVariant::Double,Unit::areaInSquareMeters(parent),parent);
  quantity->setDefaultValue(0.0);
  quantity->setMissingValue(-99999999);
  quantity->setDescription("Area in square meters");

  return quantity;
}

Quantity* Quantity::areaInSquareFeet(QObject *parent)
{
  Quantity* quantity = new Quantity("Area in square feet", QVariant::Double,Unit::areaInSquareFeet(parent),parent);
  quantity->setDefaultValue(0.0);
  quantity->setMissingValue(-99999999);
  quantity->setDescription("Area in square feet");

  return quantity;
}

Quantity* Quantity::volumeInCubicMeters(QObject *parent)
{
  Quantity* quantity = new Quantity("Volume in cubic meters", QVariant::Double, Unit::volumeInCubicMeters(parent),parent);
  quantity->setDefaultValue(0.0);
  quantity->setMissingValue(-99999999);
  quantity->setDescription("Volume in cubic meters");

  return quantity;
}

Quantity* Quantity::volumeInCubicFeet(QObject *parent)
{

  Quantity* quantity = new Quantity("Volume in cubic feet", QVariant::Double, Unit::volumeInCubicFeet(parent),parent);
  quantity->setDefaultValue(0.0);
  quantity->setMissingValue(-99999999);
  quantity->setDescription("Volume in cubic feet");

  return quantity;
}

Quantity* Quantity::flowInCMS(QObject *parent)
{
  Quantity* quantity = new Quantity("Flow in cubic meters per second", QVariant::Double, Unit::flowInCMS(parent),parent);
  quantity->setDefaultValue(0.0);
  quantity->setMissingValue(-99999999);
  quantity->setDescription("Flow in cubic meters per second");

  return quantity;
}

Quantity* Quantity::flowInCFS(QObject *parent)
{
  Quantity* quantity = new Quantity("Flow in cubic feet per second", QVariant::Double, Unit::flowInCFS(parent),parent);
  quantity->setDefaultValue(0.0);
  quantity->setMissingValue(-99999999);
  quantity->setDescription("Flow in cubic feet per second");

  return quantity;
}

Quantity* Quantity::unitLessValues(const QString& caption, QVariant::Type type, QObject *parent)
{
  Quantity* quantity = new Quantity(caption, type, Unit::unitlessCoefficient(parent),parent);
  quantity->setDefaultValue(0.0);
  quantity->setMissingValue(-99999999);
  quantity->setDescription(caption);

  return quantity;
}
