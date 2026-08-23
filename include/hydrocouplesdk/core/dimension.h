// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   dimension.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete implementation of HydroCouple::IDimension.
 */

#ifndef HYDROCOUPLESDK_DIMENSION_H
#define HYDROCOUPLESDK_DIMENSION_H

#include "hydrocouplesdk/core/identity.h"
#include "hydrocouple.h"

namespace HydroCouple::SDK {

/*!
 * \brief Concrete implementation of HydroCouple::IDimension.
 *
 * A dimension describes one axis of a multi-dimensional data set.
 * It carries an identifier, caption, and a LengthType that indicates
 * whether the size along this axis is fixed (Static) or may change
 * at runtime (Dynamic).
 *
 * \threadsafety Not thread-safe on its own. Callers must synchronise
 * concurrent mutations.
 */
class HYDROCOUPLESDK_EXPORT Dimension
    : public Identity,
      public virtual HydroCouple::IDimension
{
public:
    /*!
     * \brief Construct with an auto-generated UUID and Dynamic length type.
     */
    Dimension() = default;

    /*!
     * \brief Construct with an explicit identifier.
     * \param id  Unique identifier string.
     */
    explicit Dimension(std::string_view id);

    /*!
     * \brief Construct with identifier and caption.
     * \param id      Unique identifier.
     * \param caption Human-readable caption.
     */
    Dimension(std::string_view id, std::string_view caption);

    ~Dimension() override = default;

    // ── IDimension ────────────────────────────────────────────────────────

    /*!
     * \brief Returns the length type (Static or Dynamic) for this dimension.
     * \return LengthType enum value.
     */
    [[nodiscard]] HydroCouple::IDimension::LengthType lengthType() const override;

    /*!
     * \brief Sets the length type and fires propertyChanged("LengthType").
     * \param lt  New length type.
     */
    void setLengthType(HydroCouple::IDimension::LengthType lt);

    // ── Copy helpers ──────────────────────────────────────────────────────

    /*!
     * \brief Deep-copies an IDimension into a new Dimension.
     * \param src  Source interface pointer; may be nullptr.
     * \return New Dimension or nullptr if \p src is nullptr.
     */
    static Dimension *copy(const HydroCouple::IDimension *src);

    /*!
     * \brief Deep-copies this Dimension.
     * \return New Dimension with the same id, caption, description, and length type.
     */
    [[nodiscard]] Dimension *copy() const;

private:
    HydroCouple::IDimension::LengthType m_lengthType =
        HydroCouple::IDimension::LengthType::Dynamic;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_DIMENSION_H
