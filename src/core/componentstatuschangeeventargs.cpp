#include "stdafx.h"
#include "core/componentstatuschangeeventargs.h"
#include "core/abstractmodelcomponent.h"
#include <QDebug>

ComponentStatusChangeEventArgs::ComponentStatusChangeEventArgs(HydroCouple::ComponentStatus newStatus,
                                                               HydroCouple::ComponentStatus oldStatus,
                                                               const QString& message, float percentProgress,
                                                               AbstractModelComponent* parent):
    //because thread crashes
    QObject(nullptr)
  , m_hasProgress(true)
  , m_message(message)
  , m_percentProgress(percentProgress)
  , m_component(parent)
  , m_oldStatus(oldStatus)
  , m_newStatus(newStatus)
{

}

ComponentStatusChangeEventArgs::ComponentStatusChangeEventArgs(HydroCouple::ComponentStatus newStatus,
                                                               HydroCouple::ComponentStatus oldStatus,
                                                               const QString& message,
                                                               AbstractModelComponent *parent)
    : QObject(nullptr)
    , m_hasProgress(false)
    , m_message(message)
    , m_percentProgress(0)
    , m_component(parent)
    , m_oldStatus(oldStatus)
    , m_newStatus(newStatus)
{

}

ComponentStatusChangeEventArgs::~ComponentStatusChangeEventArgs()
{
}

HydroCouple::IModelComponent* ComponentStatusChangeEventArgs::component() const
{
    return m_component;
}

HydroCouple::ComponentStatus ComponentStatusChangeEventArgs::previousStatus() const
{
    return m_oldStatus;
}

void ComponentStatusChangeEventArgs::setPreviousStatus(HydroCouple::ComponentStatus previousStatus)
{
    m_oldStatus = previousStatus;
}

HydroCouple::ComponentStatus ComponentStatusChangeEventArgs::status() const
{
    return m_newStatus;
}

void ComponentStatusChangeEventArgs::setStatus(HydroCouple::ComponentStatus status)
{
    m_newStatus = status;
}

QString ComponentStatusChangeEventArgs::message() const
{
    return m_message;
}

void ComponentStatusChangeEventArgs::setMessage(const QString &message)
{
    m_message = message;
}

bool ComponentStatusChangeEventArgs::hasProgressMonitor() const
{
    return m_hasProgress;
}

void ComponentStatusChangeEventArgs::setHasProgressMonitor(bool hasProgress)
{
    m_hasProgress = hasProgress;
}

float ComponentStatusChangeEventArgs::percentProgress() const 
{
    return m_percentProgress;
}

void ComponentStatusChangeEventArgs::setPercentProgress(float value)
{
    if(value >= 0 && value <= 100)
    {
        m_percentProgress = value;
    }
}
