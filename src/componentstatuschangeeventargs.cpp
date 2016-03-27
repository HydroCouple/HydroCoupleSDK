#include "stdafx.h"
#include "componentstatuschangeeventargs.h"

ComponentStatusChangeEventArgs::ComponentStatusChangeEventArgs(AbstractModelComponent *parent, HydroCouple::ComponentStatus newStatus, HydroCouple::ComponentStatus oldStatus, const QString& message, float percentProgress) : QObject(parent),m_component(parent), m_newStatus(newStatus), m_oldStatus(oldStatus), m_message(message), m_hasProgress(true), m_percentProgress(percentProgress)
{

}

ComponentStatusChangeEventArgs::ComponentStatusChangeEventArgs(AbstractModelComponent *parent, HydroCouple::ComponentStatus newStatus, HydroCouple::ComponentStatus oldStatus, const QString& message) 
   : QObject(parent), m_component(parent), m_newStatus(newStatus), m_oldStatus(oldStatus), m_message(message), m_hasProgress(false), m_percentProgress(0)
{

}

ComponentStatusChangeEventArgs::~ComponentStatusChangeEventArgs()
{

}

HydroCouple::IModelComponent* ComponentStatusChangeEventArgs::component() const
{
   return m_component;
}

HydroCouple::ComponentStatus ComponentStatusChangeEventArgs::oldStatus() const
{
   return m_oldStatus;
}

HydroCouple::ComponentStatus ComponentStatusChangeEventArgs::newStatus() const
{
   return m_newStatus;
}

QString ComponentStatusChangeEventArgs::message() const
{
   return m_message;
}

bool ComponentStatusChangeEventArgs::hasProgressMonitor() const
{
   return m_hasProgress;
}

float ComponentStatusChangeEventArgs::percentProgress() const 
{
   return m_percentProgress;
}
