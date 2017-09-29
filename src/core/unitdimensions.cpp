#include "stdafx.h"
#include "core/unitdimensions.h"

using namespace HydroCouple;

UnitDimensions::UnitDimensions(QObject* parent)
   :Description(parent)
{
   m_powers[FundamentalUnitDimension::Length] = 0;
   m_powers[FundamentalUnitDimension::Mass] = 0;
   m_powers[FundamentalUnitDimension::Time] = 0;
   m_powers[FundamentalUnitDimension::ElectricCurrent] = 0;
   m_powers[FundamentalUnitDimension::Temperature] = 0;
   m_powers[FundamentalUnitDimension::AmountOfSubstance] = 0;
   m_powers[FundamentalUnitDimension::LuminousIntensity] = 0;
   m_powers[FundamentalUnitDimension::Currency] = 0;
}

UnitDimensions::UnitDimensions(const QString &caption, QObject* parent)
   :Description(caption,parent)
{
   m_powers[FundamentalUnitDimension::Length] = 0;
   m_powers[FundamentalUnitDimension::Mass] = 0;
   m_powers[FundamentalUnitDimension::Time] = 0;
   m_powers[FundamentalUnitDimension::ElectricCurrent] = 0;
   m_powers[FundamentalUnitDimension::Temperature] = 0;
   m_powers[FundamentalUnitDimension::AmountOfSubstance] = 0;
   m_powers[FundamentalUnitDimension::LuminousIntensity] = 0;
   m_powers[FundamentalUnitDimension::Currency] = 0;
}


UnitDimensions::~UnitDimensions()
{

}

double UnitDimensions::getPower(FundamentalUnitDimension dimension)
{
   return m_powers[dimension];
}

void UnitDimensions::setPower(FundamentalUnitDimension dimension, double power)
{
   m_powers[dimension] = power;
}

