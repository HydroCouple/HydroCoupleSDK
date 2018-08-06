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
