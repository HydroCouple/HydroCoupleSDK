// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   description.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete implementation of HydroCouple::IDescription.
 */

#ifndef HYDROCOUPLESDK_DESCRIPTION_H
#define HYDROCOUPLESDK_DESCRIPTION_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"
#include "hydrocouplesdk/core/signal.h"

#include <string>

namespace HydroCouple::SDK {

/*!
 * \brief Concrete implementation of HydroCouple::IDescription.
 *
 * Provides a caption, a description string, and the \c propertyChanged
 * signal inherited from \c IPropertyChanged (via \c IDescription).
 *
 * This is the root of the SDK class hierarchy. All SDK classes that
 * expose description/caption metadata inherit from this class: Identity
 * (and through it components, workflows, data items and dimensions),
 * Unit, UnitDimensions and ValueDefinition.
 *
 * The property-change signal is a Signal of `std::string` owned by this
 * object. Every SDK setter that changes a property emits it with the
 * property's name ("Caption", "Description", "Status", "Inputs", ...).
 * Slots are held weakly: the caller keeps the `shared_ptr` it connected
 * alive for as long as it wants notifications. The \c sender argument a
 * slot receives is the internal Signal object, not this Description, so a
 * slot that needs the object must capture it.
 *
 * \threadsafety Not thread-safe on its own: caption and description are
 * plain strings with no lock. The signal itself is thread-safe (see
 * Signal). Subclasses that require thread safety must add their own
 * locking.
 */
class HYDROCOUPLESDK_EXPORT Description
    : public virtual HydroCouple::IDescription
{
public:
    /*!
     * \brief Construct with an empty caption and description.
     */
    Description() = default;

    /*!
     * \brief Construct with the given caption.
     * \param[in] caption  Human-readable caption for this object; copied.
     *                     The description starts empty.
     */
    explicit Description(std::string_view caption);

    /*!
     * \brief Construct with caption and description.
     * \param[in] caption     Human-readable caption; copied.
     * \param[in] description Detailed description text; copied.
     */
    Description(std::string_view caption, std::string_view description);

    /*!
     * \brief Destroys the object and its signal; connected slots are not notified.
     */
    ~Description() override = default;

    //! Not copyable: the object owns its signal and its connections, and
    //! subclasses manage identity.
    Description(const Description &) = delete;
    //! Not copy-assignable, for the same reason as the copy constructor.
    Description &operator=(const Description &) = delete;

    // ── IDescription ─────────────────────────────────────────────────────

    /*!
     * \brief Returns the human-readable caption.
     * \return Reference to the caption string, valid until the caption is
     *         changed or this object is destroyed. Empty when never set.
     */
    [[nodiscard]] const std::string &caption() const override;

    /*!
     * \brief Sets the caption and fires \c propertyChanged("Caption").
     * \details The signal fires even when the new value equals the old one.
     * \param[in] caption  New caption value; copied.
     */
    void setCaption(const std::string &caption) override;

    /*!
     * \brief Returns the detailed description text.
     * \return Reference to the description string, valid until the
     *         description is changed or this object is destroyed. Empty
     *         when never set.
     */
    [[nodiscard]] const std::string &description() const override;

    /*!
     * \brief Sets the description and fires \c propertyChanged("Description").
     * \details The signal fires even when the new value equals the old one.
     * \param[in] description  New description text; copied.
     */
    void setDescription(const std::string &description) override;

    // ── IPropertyChanged (ISignal<std::string>) ───────────────────────────

    /*!
     * \brief Connect a slot to receive property-change notifications.
     *
     * The slot receives the name of the property that changed as a
     * \c std::string (for example \c "Caption" or \c "Description").
     * A null slot, or a slot already connected, is ignored. The signal
     * keeps only a weak reference, so the slot stops receiving once the
     * caller releases its last \c shared_ptr.
     *
     * \param[in] slot  Shared pointer to the slot to connect; may be null.
     */
    void connect(const std::shared_ptr<ISlot<std::string>> &slot) override;

    /*!
     * \brief Disconnect a previously connected slot.
     * \details A null slot, or one that is not connected, is ignored.
     *          Expired slots are removed at the same time.
     * \param[in] slot  Shared pointer to the slot to disconnect; may be null.
     */
    void disconnect(const std::shared_ptr<ISlot<std::string>> &slot) override;

    /*!
     * \brief Suppress or restore property-change notifications.
     * \details While blocked, changes are applied but no slot is called and
     *          the notifications are not queued. Connections are kept.
     * \param[in] block  \c true to suppress; \c false to restore.
     */
    void blockSignals(bool block) override;

protected:
    /*!
     * \brief Fire the \c propertyChanged signal with the given property name.
     * \details Calls every connected slot on the calling thread, unless
     *          signals are blocked.
     * \param[in] propertyName  Name of the changed property; copied into a
     *                          \c std::string for the slots.
     */
    void emitPropertyChanged(std::string_view propertyName);

    /*!
     * \brief Implements the protected ISignal of `std::string` emit().
     *
     * Delegates to the internal Signal of `std::string` member.
     * Called by the signal infrastructure; prefer emitPropertyChanged() from
     * subclasses.
     *
     * \param[in] propertyName  Name of the changed property, passed to each slot.
     */
    void emit(std::string propertyName) override;

    //! Caption returned by caption(). Subclasses that assign it directly
    //! bypass the "Caption" notification.
    std::string m_caption;
    //! Description returned by description(). Subclasses that assign it
    //! directly bypass the "Description" notification.
    std::string m_description;

private:
    //! The property-change signal behind connect(), disconnect(),
    //! blockSignals() and emitPropertyChanged().
    Signal<std::string> m_propertyChanged;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_DESCRIPTION_H
