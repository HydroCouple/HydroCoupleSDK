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
 * at runtime (Dynamic), and a DimensionRole that says what the axis
 * means. A Dimension holds no length: the extent of the axis is the
 * owning data item's shape().
 *
 * Data items borrow their dimensions: a component typically keeps its
 * Dimension objects as members and passes pointers to the items it
 * creates, so the Dimension must outlive every item that refers to it.
 * One Dimension may be shared by several items.
 *
 * The role is inferred from well-known ids at construction (see role());
 * the connection validator compares roles axis by axis
 * (Diagnostic::DimensionRoleMismatch).
 *
 * \threadsafety Not thread-safe on its own. Callers must synchronize
 * concurrent mutations.
 */
class HYDROCOUPLESDK_EXPORT Dimension
    : public Identity,
      public virtual HydroCouple::IDimension
{
public:
    /*!
     * \brief Construct with an auto-generated UUID and Dynamic length type.
     * \details The role is DimensionRole::Unknown, because a generated id
     *          matches none of the well-known names.
     */
    Dimension() = default;

    /*!
     * \brief Construct with an explicit identifier.
     * \details The role is inferred from \p id (see role()); the length
     *          type is Dynamic.
     * \param[in] id  Unique identifier string; copied. Empty is replaced by
     *                a generated UUID v4.
     */
    explicit Dimension(std::string_view id);

    /*!
     * \brief Construct with identifier and caption.
     * \details The role is inferred from \p id (see role()); the length
     *          type is Dynamic.
     * \param[in] id      Unique identifier; copied. Empty is replaced by a
     *                    generated UUID v4.
     * \param[in] caption Human-readable caption; copied.
     */
    Dimension(std::string_view id, std::string_view caption);

    //! Destroys the dimension. Items that still point to it are left dangling.
    ~Dimension() override = default;

    // ── IDimension ────────────────────────────────────────────────────────

    /*!
     * \brief Returns the length type (Static or Dynamic) for this dimension.
     * \return LengthType enum value; Dynamic unless setLengthType() changed it.
     */
    [[nodiscard]] HydroCouple::IDimension::LengthType lengthType() const override;

    /*!
     * \brief What this axis means (time, entity, layer, ...).
     * \details Inferred from a well-known id at construction, matched exactly
     * and case-sensitively: "time" and "times" give Time; "layers" and
     * "layer" give Layer; "band" and "bands" give Band; "y_cells", "rows",
     * "row" and "y" give Row; "x_cells", "columns", "column" and "x" give
     * Column; "z_cells" and "z" give Depth; "coordinate", "component" and
     * "components" give Component; "identifiers", "identifier",
     * "geometries", "nodes", "node", "edges", "edge", "faces", "face",
     * "volumes", "patches", "vertices", "entities", "element" and
     * "elements" give Entity. Any other id is DimensionRole::Unknown until
     * setRole() is called.
     * \return The current role.
     */
    [[nodiscard]] HydroCouple::IDimension::DimensionRole role() const override;

    /*!
     * \brief Sets the role and fires propertyChanged("Role").
     * \param[in] role  New role. DimensionRole::Unknown and
     *                  DimensionRole::Other make the connection validator
     *                  skip this axis.
     */
    void setRole(HydroCouple::IDimension::DimensionRole role);

    /*!
     * \brief Sets the length type and fires propertyChanged("LengthType").
     * \details The length type is descriptive: no SDK code resizes or
     *          refuses a resize because of it.
     * \param[in] lt  New length type.
     */
    void setLengthType(HydroCouple::IDimension::LengthType lt);

    // ── Copy helpers ──────────────────────────────────────────────────────

    /*!
     * \brief Deep-copies an IDimension into a new Dimension.
     * \details Copies id, caption, description, length type and role. The
     *          copy's signal has no connections.
     * \param[in] src  Source interface pointer; may be nullptr.
     * \return New Dimension owned by the caller, or nullptr if \p src is
     *         nullptr.
     */
    static Dimension *copy(const HydroCouple::IDimension *src);

    /*!
     * \brief Deep-copies this Dimension.
     * \return New Dimension, owned by the caller, with the same id, caption,
     *         description, length type and role.
     */
    [[nodiscard]] Dimension *copy() const;

private:
    //! Length type returned by lengthType(); Dynamic by default.
    HydroCouple::IDimension::LengthType m_lengthType =
        HydroCouple::IDimension::LengthType::Dynamic;
    //! Role returned by role(); inferred from the id or set by setRole().
    HydroCouple::IDimension::DimensionRole m_role =
        HydroCouple::IDimension::DimensionRole::Unknown;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_DIMENSION_H
