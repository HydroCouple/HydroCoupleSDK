// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   valuedefinition.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Concrete implementations of HydroCouple::IValueDefinition,
 *         IQuality, and IQuantity for the v2 variant-free standard.
 *
 * Per the v2.0.0 interface standard, metadata scalars are native types:
 * missing/default/min/max values are double (meaningful for numeric
 * DataKinds only) and quality categories are string labels whose data
 * values are indexes into the category list.
 */

#ifndef HYDROCOUPLESDK_VALUEDEFINITION_H
#define HYDROCOUPLESDK_VALUEDEFINITION_H

#include "hydrocouplesdk/core/description.h"
#include "hydrocouple.h"

#include <nlohmann/json.hpp>
#include <string>
#include <limits>
#include <typeinfo>
#include <vector>

namespace HydroCouple::SDK {

// Forward declarations within this file
class Unit;

// ── ValueDefinition ───────────────────────────────────────────────────────

/*!
 * \brief Concrete base implementation of HydroCouple::IValueDefinition.
 *
 * Stores the C++ type (as \c std::type_info), the corresponding DataKind,
 * and the double-valued missing/default sentinels of the v2 standard.
 *
 * Subclasses (\c Quality, \c Quantity) add qualitative or quantitative
 * constraints. Persistence uses JSON (nlohmann/json) via toJson()/fromJson().
 */
class HYDROCOUPLESDK_EXPORT ValueDefinition
    : public Description,
      public virtual HydroCouple::IValueDefinition
{
public:
    /*!
     * \brief Construct for a given C++ type.
     * \param type  \c typeid of the value type (e.g. \c typeid(double)).
     */
    explicit ValueDefinition(const std::type_info &type);

    /*!
     * \brief Construct with caption and type.
     * \param caption  Human-readable caption.
     * \param type     \c typeid of the value type.
     */
    ValueDefinition(std::string_view caption, const std::type_info &type);

    ~ValueDefinition() override = default;

    // ── IValueDefinition ─────────────────────────────────────────────────

    /*!
     * \brief Returns the \c type_info for the value type.
     */
    [[nodiscard]] const std::type_info &type() const override;

    /*!
     * \brief Returns the sentinel value representing missing data
     *        (numeric DataKinds only).
     */
    [[nodiscard]] double missingValue() const override;

    /*!
     * \brief Sets the missing-value sentinel.
     */
    void setMissingValue(double value);

    /*!
     * \brief Returns the default value for new data items
     *        (numeric DataKinds only).
     */
    [[nodiscard]] double defaultValue() const override;

    /*!
     * \brief Sets the default value.
     */
    void setDefaultValue(double value);

    // ── JSON persistence ──────────────────────────────────────────────────

    /*!
     * \brief Serialise this value definition to a JSON object.
     */
    [[nodiscard]] virtual nlohmann::json toJson() const;

    /*!
     * \brief Populate this value definition from a JSON object.
     */
    virtual void fromJson(const nlohmann::json &j);

    // ── Utility helpers ───────────────────────────────────────────────────

    /*!
     * \brief The DataKind corresponding to a C++ \c type_info, for the
     *        types the standard's data plane supports.
     * \return The matching DataKind, or DataKind::Opaque when unmapped.
     */
    [[nodiscard]] static DataKind dataKindForType(const std::type_info &type);

    /*!
     * \brief Create a deep copy of the given IValueDefinition.
     * \return Heap-allocated copy; caller owns the returned pointer.
     */
    [[nodiscard]] static ValueDefinition *copy(const HydroCouple::IValueDefinition *src);

protected:
    const std::type_info *m_type = &typeid(void);  // non-owning; static storage duration

private:
    double m_missingValue = -999999.0;
    double m_defaultValue = 0.0;
};


// ── Quality ───────────────────────────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IQuality.
 *
 * Represents qualitative (categorical) data. Per the v2 standard,
 * categories are string labels and data values are indexes into the
 * category list.
 */
class HYDROCOUPLESDK_EXPORT Quality
    : public ValueDefinition,
      public virtual HydroCouple::IQuality
{
public:
    /*!
     * \brief Construct with an initial category list.
     * \param categories  Ordered category labels.
     */
    explicit Quality(const std::vector<std::string> &categories = {});

    /*!
     * \brief Construct with caption and initial category list.
     */
    Quality(std::string_view caption,
            const std::vector<std::string> &categories = {});

    ~Quality() override = default;

    // ── IQuality ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns the category labels. Data values are indexes into
     *        this vector.
     */
    [[nodiscard]] std::vector<std::string> categories() const override;

    /*!
     * \brief Replace the category list.
     */
    void setCategories(const std::vector<std::string> &categories);

    /*!
     * \brief Append a single category label.
     */
    void addCategory(const std::string &category);

    /*!
     * \brief Returns \c true if categories have a meaningful order.
     */
    [[nodiscard]] bool isOrdered() const override;

    /*!
     * \brief Set whether categories are ordered.
     */
    void setIsOrdered(bool ordered);

    [[nodiscard]] nlohmann::json toJson()    const override;
    void fromJson(const nlohmann::json &j)         override;

    /*!
     * \brief Deep-copy a \c IQuality instance.
     * \return Heap-allocated copy; caller owns the returned pointer.
     */
    [[nodiscard]] static Quality *copy(const HydroCouple::IQuality *src);

private:
    std::vector<std::string> m_categories;
    bool                     m_isOrdered = false;
};


// ── Quantity ──────────────────────────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IQuantity.
 *
 * Represents quantitative data with a physical unit and double-valued
 * minimum / maximum bounds per the v2 standard.
 */
class HYDROCOUPLESDK_EXPORT Quantity
    : public ValueDefinition,
      public virtual HydroCouple::IQuantity
{
public:
    /*!
     * \brief Construct with type and owning unit (non-owning pointer).
     * \param type  \c typeid of the numeric value type.
     * \param unit  Physical unit; must outlive this object.
     */
    Quantity(const std::type_info &type, Unit *unit);

    /*!
     * \brief Construct with caption, type, and unit.
     */
    Quantity(std::string_view caption, const std::type_info &type, Unit *unit);

    ~Quantity() override = default;

    // ── IQuantity ────────────────────────────────────────────────────────

    /*!
     * \brief Returns the physical unit associated with this quantity.
     */
    [[nodiscard]] HydroCouple::IUnit *unit() const override;

    /*!
     * \brief Returns the minimum allowed value.
     */
    [[nodiscard]] double minValue() const override;

    /*!
     * \brief Sets the minimum allowed value.
     */
    void setMinValue(double min);

    /*!
     * \brief Returns the maximum allowed value.
     */
    [[nodiscard]] double maxValue() const override;

    /*!
     * \brief Sets the maximum allowed value.
     */
    void setMaxValue(double max);

    [[nodiscard]] nlohmann::json toJson()    const override;
    void fromJson(const nlohmann::json &j)         override;

    // ── Factory helpers ───────────────────────────────────────────────────

    /*! \brief Returns a \c double Quantity for flow in m³/s. */
    [[nodiscard]] static Quantity *flowInCMS();
    /*! \brief Returns a \c double Quantity for flow in ft³/s. */
    [[nodiscard]] static Quantity *flowInCFS();
    /*! \brief Returns a \c double Quantity for length in metres. */
    [[nodiscard]] static Quantity *lengthInMeters();
    /*! \brief Returns a \c double Quantity for length in feet. */
    [[nodiscard]] static Quantity *lengthInFeet();
    /*! \brief Returns a \c double Quantity for area in m². */
    [[nodiscard]] static Quantity *areaInSquareMeters();
    /*! \brief Returns a \c double Quantity for volume in m³. */
    [[nodiscard]] static Quantity *volumeInCubicMeters();
    /*! \brief Returns a \c double Quantity for time in days. */
    [[nodiscard]] static Quantity *timeInDays();
    /*! \brief Returns a \c double Quantity for time in seconds. */
    [[nodiscard]] static Quantity *timeInSeconds();
    /*! \brief Returns a dimensionless \c double Quantity. */
    [[nodiscard]] static Quantity *unitLess(std::string_view caption = "Unitless");

    /*!
     * \brief Deep-copy an \c IQuantity instance.
     * \return Heap-allocated copy; caller owns the returned pointer.
     */
    [[nodiscard]] static Quantity *copy(const HydroCouple::IQuantity *src);

private:
    Unit   *m_unit     = nullptr;
    double  m_minValue = std::numeric_limits<double>::lowest();
    double  m_maxValue = std::numeric_limits<double>::max();
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_VALUEDEFINITION_H
