// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   workflowstatuschangeeventargs.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Concrete implementation of
 *         HydroCouple::IWorkflowComponentStatusChangeEventArgs.
 */

#ifndef HYDROCOUPLESDK_WORKFLOWSTATUSCHANGEEVENTARGS_H
#define HYDROCOUPLESDK_WORKFLOWSTATUSCHANGEEVENTARGS_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"

#include <string>

namespace HydroCouple::SDK {

/*!
 * \brief Concrete implementation of
 *        HydroCouple::IWorkflowComponentStatusChangeEventArgs.
 *
 * Carries the workflow component pointer, old and new status values, a
 * human-readable message, and an optional progress percentage (0-100).
 *
 * \threadsafety Immutable after construction; safe to share across threads.
 */
class HYDROCOUPLESDK_EXPORT WorkflowStatusChangeEventArgs
    : public HydroCouple::IWorkflowComponentStatusChangeEventArgs
{
public:
    /*!
     * \brief Construct with status change details.
     * \param component       The workflow component that fired the event.
     * \param newStatus       Status after the change.
     * \param oldStatus       Status before the change.
     * \param message         Human-readable description (empty by default).
     * \param percentProgress Progress value 0-100; if > 0, hasProgressMonitor
     *                        is set to true.
     */
    WorkflowStatusChangeEventArgs(
        HydroCouple::IWorkflowComponent *component,
        HydroCouple::IWorkflowComponent::WorkflowStatus newStatus,
        HydroCouple::IWorkflowComponent::WorkflowStatus oldStatus,
        const std::string &message = {},
        float percentProgress = 0.0f);

    ~WorkflowStatusChangeEventArgs() override = default;

    // ── IWorkflowComponentStatusChangeEventArgs ───────────────────────────

    [[nodiscard]] HydroCouple::IWorkflowComponent *workflowComponent() const override;

    [[nodiscard]] HydroCouple::IWorkflowComponent::WorkflowStatus previousStatus() const override;

    [[nodiscard]] HydroCouple::IWorkflowComponent::WorkflowStatus status() const override;

    [[nodiscard]] std::string message() const override;

    [[nodiscard]] bool  hasProgressMonitor() const override;

    [[nodiscard]] float percentProgress()    const override;

private:
    HydroCouple::IWorkflowComponent                  *m_component;
    HydroCouple::IWorkflowComponent::WorkflowStatus   m_newStatus;
    HydroCouple::IWorkflowComponent::WorkflowStatus   m_oldStatus;
    std::string                                        m_message;
    float                                              m_percentProgress;
    bool                                               m_hasProgressMonitor;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_WORKFLOWSTATUSCHANGEEVENTARGS_H
