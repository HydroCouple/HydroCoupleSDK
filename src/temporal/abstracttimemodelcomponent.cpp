/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
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
#include "temporal/abstracttimemodelcomponent.h"
#include "temporal/timedata.h"

using namespace SDKTemporal;
using namespace HydroCouple::Temporal;

AbstractTimeModelComponent::AbstractTimeModelComponent(const QString &id, AbstractModelComponentInfo *modelComponentInfo):
  AbstractModelComponent(id,modelComponentInfo)
{
  m_dateTime = new SDKTemporal::DateTime(this);
  m_timeHorizon = new SDKTemporal::TimeSpan(this);
}

AbstractTimeModelComponent::AbstractTimeModelComponent(const QString &id, const QString &caption, AbstractModelComponentInfo *modelComponentInfo):
  AbstractModelComponent(id, caption, modelComponentInfo)
{
  m_dateTime = new SDKTemporal::DateTime(this);
  m_timeHorizon = new SDKTemporal::TimeSpan(this);
}

AbstractTimeModelComponent::~AbstractTimeModelComponent()
{

}

IDateTime *AbstractTimeModelComponent::currentDateTime() const
{
  return m_dateTime;
}

ITimeSpan *AbstractTimeModelComponent::timeHorizon() const
{
  return m_timeHorizon;
}

DateTime *AbstractTimeModelComponent::currentDateTimeInternal() const
{
  return m_dateTime;
}

TimeSpan *AbstractTimeModelComponent::timeHorizonInternal() const
{
  return m_timeHorizon;
}
