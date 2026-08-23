// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/component/workflowstatuschangeeventargs.h"

namespace HydroCouple::SDK {

WorkflowStatusChangeEventArgs::WorkflowStatusChangeEventArgs(
    HydroCouple::IWorkflowComponent *component,
    HydroCouple::IWorkflowComponent::WorkflowStatus newStatus,
    HydroCouple::IWorkflowComponent::WorkflowStatus oldStatus,
    const std::string &message, float percentProgress)
    : m_component(component),
      m_newStatus(newStatus),
      m_oldStatus(oldStatus),
      m_message(message),
      m_percentProgress(percentProgress),
      m_hasProgressMonitor(percentProgress > 0.0f)
{
}

HydroCouple::IWorkflowComponent *
WorkflowStatusChangeEventArgs::workflowComponent() const
{
    return m_component;
}

HydroCouple::IWorkflowComponent::WorkflowStatus
WorkflowStatusChangeEventArgs::previousStatus() const
{
    return m_oldStatus;
}

HydroCouple::IWorkflowComponent::WorkflowStatus
WorkflowStatusChangeEventArgs::status() const
{
    return m_newStatus;
}

std::string WorkflowStatusChangeEventArgs::message() const
{
    return m_message;
}

bool WorkflowStatusChangeEventArgs::hasProgressMonitor() const
{
    return m_hasProgressMonitor;
}

float WorkflowStatusChangeEventArgs::percentProgress() const
{
    return m_percentProgress;
}

} // namespace HydroCouple::SDK
