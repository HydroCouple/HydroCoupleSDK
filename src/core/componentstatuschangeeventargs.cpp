#include "stdafx.h"
#include "core/componentstatuschangeeventargs.h"
#include "core/abstractmodelcomponent.h"
#include <QDebug>
#include <algorithm>

using namespace HydroCouple;

ComponentStatusChangeEventArgs::ComponentStatusChangeEventArgs(IModelComponent::ComponentStatus newStatus,
                                                               IModelComponent::ComponentStatus oldStatus,
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

ComponentStatusChangeEventArgs::ComponentStatusChangeEventArgs(IModelComponent::ComponentStatus newStatus,
                                                               IModelComponent::ComponentStatus oldStatus,
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

IModelComponent::ComponentStatus ComponentStatusChangeEventArgs::previousStatus() const
{
    return m_oldStatus;
}

void ComponentStatusChangeEventArgs::setPreviousStatus(IModelComponent::ComponentStatus previousStatus)
{
    m_oldStatus = previousStatus;
}

IModelComponent::ComponentStatus ComponentStatusChangeEventArgs::status() const
{
    return m_newStatus;
}

void ComponentStatusChangeEventArgs::setStatus(IModelComponent::ComponentStatus status)
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
  m_percentProgress = std::min(std::max(0.0f,value), 100.0f) ;
}
