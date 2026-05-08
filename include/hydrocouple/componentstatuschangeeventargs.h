// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   componentstatuschangeeventargs.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete implementation of HydroCouple::IComponentStatusChangeEventArgs.
 */

#ifndef HYDROCOUPLESDK_COMPONENTSTATUSCHANGEEVENTARGS_H
#define HYDROCOUPLESDK_COMPONENTSTATUSCHANGEEVENTARGS_H

#include "hydrocouplesdk.h"
#include "hydrocouple.h"

#include <string>

namespace HydroCouple {

/*!
 * \brief Concrete implementation of HydroCouple::IComponentStatusChangeEventArgs.
 *
 * Carries the model component pointer, old and new status values, a human-readable
 * message, and an optional progress percentage (0-100).
 *
 * \threadsafety Immutable after construction; safe to share across threads.
 */
class HYDROCOUPLESDK_EXPORT ComponentStatusChangeEventArgs
    : public HydroCouple::IComponentStatusChangeEventArgs
{
public:
    /*!
     * \brief Construct with status change details.
     * \param component       The component that fired the event.
     * \param newStatus       Status after the change.
     * \param oldStatus       Status before the change.
     * \param message         Human-readable description (empty by default).
     * \param percentProgress Progress value 0-100; if > 0, hasProgressMonitor is set to true.
     */
    ComponentStatusChangeEventArgs(
        HydroCouple::IModelComponent *component,
        HydroCouple::IModelComponent::ComponentStatus newStatus,
        HydroCouple::IModelComponent::ComponentStatus oldStatus,
        const std::string &message = {},
        float percentProgress = 0.0f);

    ~ComponentStatusChangeEventArgs() override = default;

    // ── IComponentStatusChangeEventArgs ──────────────────────────────────

    [[nodiscard]] HydroCouple::IModelComponent *component() const override;

    [[nodiscard]] HydroCouple::IModelComponent::ComponentStatus previousStatus() const override;

    [[nodiscard]] HydroCouple::IModelComponent::ComponentStatus status() const override;

    [[nodiscard]] std::string message() const override;

    [[nodiscard]] bool  hasProgressMonitor() const override;

    [[nodiscard]] float percentProgress()    const override;

    // ── Setters (for in-place reuse) ──────────────────────────────────────

    void setStatus(HydroCouple::IModelComponent::ComponentStatus status);
    void setPreviousStatus(HydroCouple::IModelComponent::ComponentStatus previousStatus);
    void setMessage(const std::string &message);
    void setHasProgressMonitor(bool has);
    void setPercentProgress(float value);

private:
    HydroCouple::IModelComponent                   *m_component;
    HydroCouple::IModelComponent::ComponentStatus   m_newStatus;
    HydroCouple::IModelComponent::ComponentStatus   m_oldStatus;
    std::string                                      m_message;
    float                                            m_percentProgress;
    bool                                             m_hasProgressMonitor;
};

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_COMPONENTSTATUSCHANGEEVENTARGS_H
