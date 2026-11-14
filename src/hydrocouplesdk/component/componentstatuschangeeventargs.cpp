// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/component/componentstatuschangeeventargs.h"

namespace HydroCouple::SDK {

ComponentStatusChangeEventArgs::ComponentStatusChangeEventArgs(
    HydroCouple::IModelComponent *component,
    HydroCouple::IModelComponent::ComponentStatus newStatus,
    HydroCouple::IModelComponent::ComponentStatus oldStatus,
    const std::string &message,
    float percentProgress)
    : m_component(component)
    , m_newStatus(newStatus)
    , m_oldStatus(oldStatus)
    , m_message(message)
    , m_percentProgress(percentProgress)
    , m_hasProgressMonitor(percentProgress > 0.0f)
{
}

HydroCouple::IModelComponent *ComponentStatusChangeEventArgs::component() const
{
    return m_component;
}

HydroCouple::IModelComponent::ComponentStatus
ComponentStatusChangeEventArgs::previousStatus() const
{
    return m_oldStatus;
}

HydroCouple::IModelComponent::ComponentStatus
ComponentStatusChangeEventArgs::status() const
{
    return m_newStatus;
}

std::string ComponentStatusChangeEventArgs::message() const
{
    return m_message;
}

bool ComponentStatusChangeEventArgs::hasProgressMonitor() const
{
    return m_hasProgressMonitor;
}

float ComponentStatusChangeEventArgs::percentProgress() const
{
    return m_percentProgress;
}

void ComponentStatusChangeEventArgs::setStatus(
    HydroCouple::IModelComponent::ComponentStatus status)
{
    m_newStatus = status;
}

void ComponentStatusChangeEventArgs::setPreviousStatus(
    HydroCouple::IModelComponent::ComponentStatus previousStatus)
{
    m_oldStatus = previousStatus;
}

void ComponentStatusChangeEventArgs::setMessage(const std::string &message)
{
    m_message = message;
}

void ComponentStatusChangeEventArgs::setHasProgressMonitor(bool has)
{
    m_hasProgressMonitor = has;
}

void ComponentStatusChangeEventArgs::setPercentProgress(float value)
{
    m_percentProgress = value;
    m_hasProgressMonitor = true;
}

} // namespace HydroCouple::SDK
