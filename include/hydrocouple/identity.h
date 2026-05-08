// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   identity.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete implementation of HydroCouple::IIdentity.
 */

#ifndef HYDROCOUPLESDK_IDENTITY_H
#define HYDROCOUPLESDK_IDENTITY_H

#include "description.h"
#include "uuid.h"

#include <string>

namespace HydroCouple {

/*!
 * \brief Concrete implementation of HydroCouple::IIdentity.
 *
 * Adds a unique string identifier to \c Description. The id is immutable
 * after construction; a new UUID is generated automatically when no id
 * is supplied.
 *
 * All SDK classes that need a stable identity (components, data items,
 * arguments, exchange items) inherit from this class.
 */
class HYDROCOUPLESDK_EXPORT Identity
    : public Description,
      public virtual HydroCouple::IIdentity
{
public:
    /*!
     * \brief Construct with an auto-generated UUID v4 identifier.
     */
    Identity();

    /*!
     * \brief Construct with an explicit identifier.
     * \param id  Unique identifier string. Must not be empty.
     */
    explicit Identity(std::string_view id);

    /*!
     * \brief Construct with identifier and caption.
     * \param id      Unique identifier string.
     * \param caption Human-readable caption.
     */
    Identity(std::string_view id, std::string_view caption);

    /*!
     * \brief Construct with identifier, caption, and description.
     * \param id          Unique identifier string.
     * \param caption     Human-readable caption.
     * \param description Detailed description text.
     */
    Identity(std::string_view id,
             std::string_view caption,
             std::string_view description);

    ~Identity() override = default;

    // ── IIdentity ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns the unique identifier for this object.
     * \return Identifier string; never empty.
     */
    [[nodiscard]] std::string id() const override;

protected:
    /*!
     * \brief Replace the identifier (use sparingly; prefer setting at construction).
     * \param id  New identifier. Must not be empty.
     */
    void setId(std::string_view id);

private:
    std::string m_id;
};

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_IDENTITY_H
