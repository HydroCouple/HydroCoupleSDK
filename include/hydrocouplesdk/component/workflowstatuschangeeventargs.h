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
 * AbstractWorkflowComponent::setStatus() creates one per call and passes
 * it, in a \c std::shared_ptr, to every connected slot.
 *
 * The workflow pointer is not owned and is valid only while the workflow
 * lives.
 *
 * \threadsafety Immutable after construction; safe to share across threads.
 */
class HYDROCOUPLESDK_EXPORT WorkflowStatusChangeEventArgs
    : public HydroCouple::IWorkflowComponentStatusChangeEventArgs
{
public:
    /*!
     * \brief Construct with status change details.
     * \param[in] component       The workflow component that fired the
     *                            event; not owned, may be nullptr.
     * \param[in] newStatus       Status after the change.
     * \param[in] oldStatus       Status before the change.
     * \param[in] message         Human-readable description (empty by
     *                            default); copied.
     * \param[in] percentProgress Progress value 0-100, not range-checked;
     *                            if > 0, hasProgressMonitor() is true.
     */
    WorkflowStatusChangeEventArgs(
        HydroCouple::IWorkflowComponent *component,
        HydroCouple::IWorkflowComponent::WorkflowStatus newStatus,
        HydroCouple::IWorkflowComponent::WorkflowStatus oldStatus,
        const std::string &message = {},
        float percentProgress = 0.0f);

    //! Destroys the event; the workflow is not affected.
    ~WorkflowStatusChangeEventArgs() override = default;

    // ── IWorkflowComponentStatusChangeEventArgs ───────────────────────────

    /*!
     * \brief The workflow whose status changed.
     * \return Non-owning pointer given at construction.
     */
    [[nodiscard]] HydroCouple::IWorkflowComponent *workflowComponent() const override;

    /*!
     * \brief The status before the change.
     * \return The previous status.
     */
    [[nodiscard]] HydroCouple::IWorkflowComponent::WorkflowStatus previousStatus() const override;

    /*!
     * \brief The status after the change.
     * \return The new status, which the workflow already holds when the
     *         event is delivered.
     */
    [[nodiscard]] HydroCouple::IWorkflowComponent::WorkflowStatus status() const override;

    /*!
     * \brief The message passed with the status.
     * \return A copy of the message; empty when none was given.
     */
    [[nodiscard]] std::string message() const override;

    /*!
     * \brief Whether the event reports a progress percentage.
     * \return \c true when constructed with progress greater than 0. The SDK
     *         workflows always pass 0, so they never report progress.
     */
    [[nodiscard]] bool  hasProgressMonitor() const override;

    /*!
     * \brief Progress made by the workflow.
     * \return Percentage, nominally 0 to 100; 0 when not reported.
     */
    [[nodiscard]] float percentProgress()    const override;

private:
    HydroCouple::IWorkflowComponent                  *m_component;          //!< Not owned.
    HydroCouple::IWorkflowComponent::WorkflowStatus   m_newStatus;          //!< Status after.
    HydroCouple::IWorkflowComponent::WorkflowStatus   m_oldStatus;          //!< Status before.
    std::string                                        m_message;           //!< Message text.
    float                                              m_percentProgress;   //!< 0 to 100.
    bool                                               m_hasProgressMonitor; //!< Progress reported.
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_WORKFLOWSTATUSCHANGEEVENTARGS_H
