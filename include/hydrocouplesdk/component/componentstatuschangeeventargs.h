// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   componentstatuschangeeventargs.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete implementation of HydroCouple::IComponentStatusChangeEventArgs.
 */

#ifndef HYDROCOUPLESDK_COMPONENTSTATUSCHANGEEVENTARGS_H
#define HYDROCOUPLESDK_COMPONENTSTATUSCHANGEEVENTARGS_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"

#include <string>

namespace HydroCouple::SDK {

/*!
 * \brief Concrete implementation of HydroCouple::IComponentStatusChangeEventArgs.
 *
 * Carries the model component pointer, old and new status values, a human-readable
 * message, and an optional progress percentage (0-100).
 * AbstractModelComponent::setStatus() creates one per call, wrapped in a
 * \c std::shared_ptr, and passes it to every slot connected to the
 * component's status signal; a slot may keep the pointer.
 *
 * The component pointer is not owned and is valid only while the
 * component lives; a slot that keeps the event past the component's
 * destruction must not dereference it.
 *
 * \threadsafety The SDK never modifies an event after constructing it, so
 * sharing one across threads is safe as long as nobody calls the setters,
 * which have no lock.
 */
class HYDROCOUPLESDK_EXPORT ComponentStatusChangeEventArgs
    : public HydroCouple::IComponentStatusChangeEventArgs
{
public:
    /*!
     * \brief Construct with status change details.
     * \param[in] component       The component that fired the event; not
     *                            owned, may be nullptr.
     * \param[in] newStatus       Status after the change.
     * \param[in] oldStatus       Status before the change (equal to
     *                            \p newStatus for a message or progress
     *                            refresh).
     * \param[in] message         Human-readable description (empty by default); copied.
     * \param[in] percentProgress Progress value 0-100, not range-checked;
     *                            hasProgressMonitor() is true when it is
     *                            greater than 0.
     */
    ComponentStatusChangeEventArgs(
        HydroCouple::IModelComponent *component,
        HydroCouple::IModelComponent::ComponentStatus newStatus,
        HydroCouple::IModelComponent::ComponentStatus oldStatus,
        const std::string &message = {},
        float percentProgress = 0.0f);

    //! Destroys the event; the component is not affected.
    ~ComponentStatusChangeEventArgs() override = default;

    // ── IComponentStatusChangeEventArgs ──────────────────────────────────

    /*!
     * \brief The component whose status changed.
     * \return Non-owning pointer given at construction.
     */
    [[nodiscard]] HydroCouple::IModelComponent *component() const override;

    /*!
     * \brief The status before the change.
     * \return The previous status.
     */
    [[nodiscard]] HydroCouple::IModelComponent::ComponentStatus previousStatus() const override;

    /*!
     * \brief The status after the change.
     * \return The new status, which the component already holds when the
     *         event is delivered.
     */
    [[nodiscard]] HydroCouple::IModelComponent::ComponentStatus status() const override;

    /*!
     * \brief The message passed with the status.
     * \return A copy of the message; empty when none was given.
     */
    [[nodiscard]] std::string message() const override;

    /*!
     * \brief Whether the event reports a progress percentage.
     * \return \c true when constructed with progress greater than 0, or after
     *         setPercentProgress() or setHasProgressMonitor(true).
     */
    [[nodiscard]] bool  hasProgressMonitor() const override;

    /*!
     * \brief Progress made by the component.
     * \return Percentage, nominally 0 to 100; 0 when not reported.
     */
    [[nodiscard]] float percentProgress()    const override;

    // ── Setters (for in-place reuse) ──────────────────────────────────────
    //
    // No SDK code calls these; they exist for a host that reuses one event
    // object. They take no lock.

    /*!
     * \brief Replaces the new status.
     * \param[in] status  Status after the change.
     */
    void setStatus(HydroCouple::IModelComponent::ComponentStatus status);
    /*!
     * \brief Replaces the previous status.
     * \param[in] previousStatus  Status before the change.
     */
    void setPreviousStatus(HydroCouple::IModelComponent::ComponentStatus previousStatus);
    /*!
     * \brief Replaces the message.
     * \param[in] message  New message; copied.
     */
    void setMessage(const std::string &message);
    /*!
     * \brief Sets whether progress is reported.
     * \param[in] has  New flag; percentProgress() is unchanged.
     */
    void setHasProgressMonitor(bool has);
    /*!
     * \brief Sets the progress and marks progress as reported.
     * \details hasProgressMonitor() becomes \c true, whatever \p value is.
     * \param[in] value  Percentage, nominally 0 to 100; not range-checked.
     */
    void setPercentProgress(float value);

private:
    HydroCouple::IModelComponent                   *m_component;          //!< Not owned.
    HydroCouple::IModelComponent::ComponentStatus   m_newStatus;          //!< Status after.
    HydroCouple::IModelComponent::ComponentStatus   m_oldStatus;          //!< Status before.
    std::string                                      m_message;           //!< Message text.
    float                                            m_percentProgress;   //!< 0 to 100.
    bool                                             m_hasProgressMonitor; //!< Progress reported.
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_COMPONENTSTATUSCHANGEEVENTARGS_H
