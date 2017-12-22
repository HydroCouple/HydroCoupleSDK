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
