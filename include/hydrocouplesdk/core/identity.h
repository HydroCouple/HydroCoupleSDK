// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   identity.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete implementation of HydroCouple::IIdentity.
 */

#ifndef HYDROCOUPLESDK_IDENTITY_H
#define HYDROCOUPLESDK_IDENTITY_H

#include "hydrocouplesdk/core/description.h"
#include "hydrocouplesdk/core/uuid.h"

#include <string>

namespace HydroCouple::SDK {

/*!
 * \brief Concrete implementation of HydroCouple::IIdentity.
 *
 * Adds a unique string identifier to \c Description. The id is fixed at
 * construction for users of the class; only a subclass can replace it,
 * through setId(). A new UUID v4 (generateUUID()) is generated when no id,
 * or an empty id, is supplied, so id() is never empty.
 *
 * All SDK classes that need a stable identity (components, workflows,
 * component infos, data items, arguments, exchange items, dimensions)
 * inherit from this class. The interface requires an id to be unique only
 * within its context (the inputs of one component, for example); Identity
 * does not check uniqueness.
 *
 * \threadsafety Not thread-safe on its own (see Description).
 */
class HYDROCOUPLESDK_EXPORT Identity
    : public Description,
      public virtual HydroCouple::IIdentity
{
public:
    /*!
     * \brief Construct with an auto-generated UUID v4 identifier.
     * \details Caption and description start empty.
     */
    Identity();

    /*!
     * \brief Construct with an explicit identifier.
     * \param[in] id  Unique identifier string; copied. An empty value is
     *                replaced by a generated UUID v4.
     */
    explicit Identity(std::string_view id);

    /*!
     * \brief Construct with identifier and caption.
     * \param[in] id      Unique identifier string; copied. An empty value is
     *                    replaced by a generated UUID v4.
     * \param[in] caption Human-readable caption; copied.
     */
    Identity(std::string_view id, std::string_view caption);

    /*!
     * \brief Construct with identifier, caption, and description.
     * \param[in] id          Unique identifier string; copied. An empty value
     *                        is replaced by a generated UUID v4.
     * \param[in] caption     Human-readable caption; copied.
     * \param[in] description Detailed description text; copied.
     */
    Identity(std::string_view id,
             std::string_view caption,
             std::string_view description);

    //! Destroys the object (see Description::~Description()).
    ~Identity() override = default;

    // ── IIdentity ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns the unique identifier for this object.
     * \return Reference to the identifier, valid until setId() or
     *         destruction; never empty.
     */
    [[nodiscard]] const std::string &id() const override;

protected:
    /*!
     * \brief Replace the identifier (use sparingly; prefer setting at construction).
     * \details Fires \c propertyChanged("Id") for every nonempty value, even
     *          an unchanged one. AbstractModelComponent::setInstanceId() is
     *          the public route for a component.
     * \param[in] id  New identifier; copied. An empty value is ignored: the
     *                id is left unchanged and no signal fires.
     */
    void setId(std::string_view id);

private:
    //! The identifier returned by id(); never empty.
    std::string m_id;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_IDENTITY_H
