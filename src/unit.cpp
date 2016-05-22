#include "unit.h"
#include "unitdimensions.h"

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



