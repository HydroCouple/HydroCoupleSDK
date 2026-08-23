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
 * expose description/caption metadata inherit from this class.
 *
 * \threadsafety Not thread-safe on its own; subclasses that require
 * thread safety must add their own locking.
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
     * \param caption  Human-readable caption for this object.
     */
    explicit Description(std::string_view caption);

    /*!
     * \brief Construct with caption and description.
     * \param caption     Human-readable caption.
     * \param description Detailed description text.
     */
    Description(std::string_view caption, std::string_view description);

    ~Description() override = default;

    // Non-copyable (signal ownership; subclasses manage identity)
    Description(const Description &) = delete;
    Description &operator=(const Description &) = delete;

    // ── IDescription ─────────────────────────────────────────────────────

    /*!
     * \brief Returns the human-readable caption.
     * \return Caption string.
     */
    [[nodiscard]] const std::string &caption() const override;

    /*!
     * \brief Sets the caption and fires \c propertyChanged("Caption").
     * \param caption  New caption value.
     */
    void setCaption(const std::string &caption) override;

    /*!
     * \brief Returns the detailed description text.
     * \return Description string.
     */
    [[nodiscard]] const std::string &description() const override;

    /*!
     * \brief Sets the description and fires \c propertyChanged("Description").
     * \param description  New description text.
     */
    void setDescription(const std::string &description) override;

    // ── IPropertyChanged (ISignal<std::string>) ───────────────────────────

    /*!
     * \brief Connect a slot to receive property-change notifications.
     *
     * The slot receives the name of the property that changed as a
     * \c std::string (e.g., \c "Caption", \c "Description").
     *
     * \param slot  Shared pointer to the slot to connect.
     */
    void connect(const std::shared_ptr<ISlot<std::string>> &slot) override;

    /*!
     * \brief Disconnect a previously connected slot.
     * \param slot  Shared pointer to the slot to disconnect.
     */
    void disconnect(const std::shared_ptr<ISlot<std::string>> &slot) override;

    /*!
     * \brief Suppress or restore property-change notifications.
     * \param block  \c true to suppress; \c false to restore.
     */
    void blockSignals(bool block) override;

protected:
    /*!
     * \brief Fire the \c propertyChanged signal with the given property name.
     * \param propertyName  Name of the changed property.
     */
    void emitPropertyChanged(std::string_view propertyName);

    /*!
     * \brief Implements the protected ISignal<std::string>::emit().
     *
     * Delegates to the internal Signal<std::string> member.
     * Called by the signal infrastructure; prefer emitPropertyChanged() from
     * subclasses.
     */
    void emit(std::string propertyName) override;

    std::string m_caption;
    std::string m_description;

private:
    Signal<std::string> m_propertyChanged;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_DESCRIPTION_H
