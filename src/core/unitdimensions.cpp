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

