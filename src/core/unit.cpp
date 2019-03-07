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
#include "core/unit.h"
#include "core/unitdimensions.h"

using namespace HydroCouple;

Unit::Unit(QObject *parent)
   :Description(parent)
   , m_conversionFactorToSI(0)
   , m_offsetToSI(0)
{
   m_dimensions = new UnitDimensions(this);
}


Unit::Unit(const QString &caption, double conversionFactorToSI, double offsetToSI, QObject *parent)
   :Description(caption,parent)
   , m_conversionFactorToSI(conversionFactorToSI)
   , m_offsetToSI(offsetToSI)
{
   m_dimensions = new UnitDimensions(this);
}

Unit::~Unit()
{
  delete m_dimensions;
}

IUnitDimensions* Unit::dimensions() const
{
   return m_dimensions;
}

UnitDimensions* Unit::dimensionsInternal() const
{
   return m_dimensions;
}

double Unit::conversionFactorToSI() const
{
   return m_conversionFactorToSI;
}

void Unit::setConversionFactorToSI(double conversionFactor)
{
   m_conversionFactorToSI = conversionFactor;
   emit propertyChanged("ConversionFactorToSI");
}

double Unit::offsetToSI() const
{
   return m_offsetToSI;
}

void Unit::setOffsetToSI(double offsetToSI)
{
   m_offsetToSI = offsetToSI;
   emit propertyChanged("OffsetToSI");
}

Unit *Unit::copy(const IUnit *unit, QObject *parent)
{
  Unit *oUnit = new Unit(unit->caption(), unit->conversionFactorToSI(),
                        unit->offsetToSI(), parent);

  oUnit->dimensionsInternal()->setPower(HydroCouple::Length, unit->dimensions()->getPower(HydroCouple::Length));
  oUnit->dimensionsInternal()->setPower(HydroCouple::Mass, unit->dimensions()->getPower(HydroCouple::Mass));
  oUnit->dimensionsInternal()->setPower(HydroCouple::Time, unit->dimensions()->getPower(HydroCouple::Time));
  oUnit->dimensionsInternal()->setPower(HydroCouple::ElectricCurrent, unit->dimensions()->getPower(HydroCouple::ElectricCurrent));
  oUnit->dimensionsInternal()->setPower(HydroCouple::Temperature, unit->dimensions()->getPower(HydroCouple::Temperature));
  oUnit->dimensionsInternal()->setPower(HydroCouple::AmountOfSubstance, unit->dimensions()->getPower(HydroCouple::AmountOfSubstance));
  oUnit->dimensionsInternal()->setPower(HydroCouple::LuminousIntensity, unit->dimensions()->getPower(HydroCouple::LuminousIntensity));
  oUnit->dimensionsInternal()->setPower(HydroCouple::Currency, unit->dimensions()->getPower(HydroCouple::Currency));

  return oUnit;
}

Unit* Unit::lengthInMeters(QObject *parent)
{
   Unit* unit = new Unit("Length (m)" , 1.0, 0.0 , parent);
   unit->m_dimensions->setPower(HydroCouple::Length , 1.0);
   return unit;
}

Unit* Unit::lengthInFeet(QObject *parent)
{
   Unit* unit = new Unit("Length (ft)" , 0.3048, 0.0 , parent);
   unit->m_dimensions->setPower(HydroCouple::Length , 1.0);
   return unit;
}

Unit* Unit::areaInSquareMeters(QObject *parent)
{
   Unit* unit = new Unit("Area (m²)" , 1.0, 0.0 , parent);
   unit->m_dimensions->setPower(HydroCouple::Length , 2.0);
   return unit;
}

Unit* Unit::areaInSquareFeet(QObject *parent)
{
   Unit* unit = new Unit("Area (ft²)" , 0.092903, 0.0 , parent);
   unit->m_dimensions->setPower(HydroCouple::Length , 2.0);
   return unit;
}

Unit* Unit::volumeInCubicMeters(QObject *parent)
{
   Unit* unit = new Unit("Volume (m³)" , 1, 0.0 , parent);
   unit->m_dimensions->setPower(HydroCouple::Length , 3.0);
   return unit;
}

Unit* Unit::volumeInCubicFeet(QObject *parent)
{
   Unit* unit = new Unit("Volume (ft³)" , 0.0283168 , 0.0 , parent);
   unit->m_dimensions->setPower(HydroCouple::Length , 3.0);
   return unit;
}

Unit* Unit::flowInCMS(QObject *parent)
{
   Unit* unit = new Unit("Discharge (m³/s)" , 1.0, 0.0 , parent);
   unit->m_dimensions->setPower(HydroCouple::Length , 3.0);
   unit->m_dimensions->setPower(HydroCouple::Time , -1.0);
   return unit;
}

Unit* Unit::flowInCFS(QObject *parent)
{
   Unit* unit = new Unit("Discharge (ft³/s)" , 0.0283168 , 0.0 , parent);
   unit->m_dimensions->setPower(HydroCouple::Length , 3.0);
   unit->m_dimensions->setPower(HydroCouple::Time , -1.0);
   return unit;
}

Unit* Unit::unitlessCoefficient(QObject *parent)
{
   Unit* unit = new Unit("Coefficient" , 1.0 , 0.0 , parent);
   return unit;
}

Unit* Unit::timeInDays(QObject *parent)
{
  Unit* unit = new Unit("Time (days)", 1.0 , 0.0 , parent);
  unit->m_dimensions->setPower(HydroCouple::Time , 1.0);
  return unit;
}

Unit* Unit::timeInSeconds(QObject *parent)
{
  Unit* unit = new Unit("Time (s)", 1.0 / 86400.0 , 0.0 , parent);
  unit->m_dimensions->setPower(HydroCouple::Time , 1.0);
  return unit;
}

Unit *Unit::parseUnit(const QString &line, QObject *parent)
{
  QString delimiters = ",";
  QStringList columns = line.split(delimiters, QString::SkipEmptyParts);

  if(columns.size() == 11)
  {
    QString name = columns[0];

    bool ok;

  }

  return nullptr;
}




