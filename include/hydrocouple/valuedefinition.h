// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   valuedefinition.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete implementations of HydroCouple::IValueDefinition,
 *         IQuality, and IQuantity.
 */

#ifndef HYDROCOUPLESDK_VALUEDEFINITION_H
#define HYDROCOUPLESDK_VALUEDEFINITION_H

#include "description.h"
#include "hydrocouple.h"

#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <typeinfo>

namespace HydroCouple {

// Forward declarations within this file
class Unit;

// ── ValueDefinition ───────────────────────────────────────────────────────

/*!
 * \brief Concrete base implementation of HydroCouple::IValueDefinition.
 *
 * Stores the C++ type (as \c std::type_index), a missing-value sentinel,
 * and a default value, both as \c hydrocouple_variant.
 *
 * Subclasses (\c Quality, \c Quantity) add qualitative or quantitative
 * constraints.
 *
 * \note XML serialisation is replaced by JSON (nlohmann/json). Subclasses
 * override \c toJson() / \c fromJson() for persistence.
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
     * \return Reference to the \c std::type_info object.
     */
    [[nodiscard]] const std::type_info &type() const override;  // returns ref; type_info is not copyable

    /*!
     * \brief Returns the sentinel value used to represent missing data.
     * \return Missing-value variant.
     */
    [[nodiscard]] hydrocouple_variant missingValue() const override;

    /*!
     * \brief Sets the missing-value sentinel.
     * \param value  New missing-value. Must be compatible with \c type().
     */
    void setMissingValue(const hydrocouple_variant &value);

    /*!
     * \brief Returns the default value for new data items.
     * \return Default-value variant.
     */
    [[nodiscard]] hydrocouple_variant defaultValue() const override;

    /*!
     * \brief Sets the default value.
     * \param value  New default value. Must be compatible with \c type().
     */
    void setDefaultValue(const hydrocouple_variant &value);

    // ── JSON persistence ──────────────────────────────────────────────────

    /*!
     * \brief Serialise this value definition to a JSON object.
     * \return nlohmann::json object.
     */
    [[nodiscard]] virtual nlohmann::json toJson() const;

    /*!
     * \brief Populate this value definition from a JSON object.
     * \param j  Source JSON object.
     */
    virtual void fromJson(const nlohmann::json &j);

    // ── Utility helpers ───────────────────────────────────────────────────

    /*!
     * \brief Serialise a variant value to a JSON-compatible string.
     * \param value  Variant to serialise.
     * \return String representation suitable for JSON storage.
     */
    [[nodiscard]] static std::string serializeVariant(const hydrocouple_variant &value);

    /*!
     * \brief Deserialise a variant value from a string given a target type.
     * \param text  String previously returned by \c serializeVariant().
     * \param type  Target \c type_index describing the expected variant type.
     * \return Reconstructed variant.
     */
    [[nodiscard]] static hydrocouple_variant deserializeVariant(
        std::string_view text, const std::type_info &type);

    /*!
     * \brief Create a deep copy of the given IValueDefinition.
     * \param src  Source value definition to copy.
     * \return Heap-allocated copy; caller owns the returned pointer.
     */
    [[nodiscard]] static ValueDefinition *copy(const HydroCouple::IValueDefinition *src);

protected:
    /*!
     * \brief Validate that \p value is compatible with \c m_type.
     * \param value  Variant to check.
     * \return \c true if the variant's active type matches \c m_type.
     */
    [[nodiscard]] bool validateValue(const hydrocouple_variant &value) const;

    const std::type_info *m_type = &typeid(void);  // non-owning; type_info has static storage duration

private:
    hydrocouple_variant  m_missingValue;
    hydrocouple_variant  m_defaultValue;
};


// ── Quality ───────────────────────────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IQuality.
 *
 * Represents qualitative (categorical) data with an ordered or unordered
 * set of allowed category values.
 */
class HYDROCOUPLESDK_EXPORT Quality
    : public ValueDefinition,
      public virtual HydroCouple::IQuality
{
public:
    /*!
     * \brief Construct with a type and initial category set.
     * \param type        \c typeid of the category value type.
     * \param categories  Initial set of allowed values.
     */
    Quality(const std::type_info &type,
            const std::set<hydrocouple_variant> &categories = {});

    /*!
     * \brief Construct with caption, type, and initial category set.
     */
    Quality(std::string_view caption,
            const std::type_info &type,
            const std::set<hydrocouple_variant> &categories = {});

    ~Quality() override = default;

    // ── IQuality ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns the set of allowed category values.
     * \return Ordered set of category variants.
     */
    [[nodiscard]] std::set<hydrocouple_variant> categories() const override;

    /*!
     * \brief Replace the category set.
     * \param categories  New set of allowed values.
     */
    void setCategories(const std::set<hydrocouple_variant> &categories);

    /*!
     * \brief Add a single category value.
     * \param category  Value to add.
     */
    void addCategory(const hydrocouple_variant &category);

    /*!
     * \brief Returns \c true if categories have a meaningful order.
     * \return Ordered flag.
     */
    [[nodiscard]] bool isOrdered() const override;

    /*!
     * \brief Set whether categories are ordered.
     * \param ordered  \c true = ordered; \c false = unordered.
     */
    void setIsOrdered(bool ordered);

    [[nodiscard]] nlohmann::json toJson()    const override;
    void fromJson(const nlohmann::json &j)         override;

    /*!
     * \brief Deep-copy a \c IQuality instance.
     * \param src  Source quality definition.
     * \return Heap-allocated copy; caller owns the returned pointer.
     */
    [[nodiscard]] static Quality *copy(const HydroCouple::IQuality *src);

private:
    std::set<hydrocouple_variant> m_categories;
    bool                          m_isOrdered = false;
};


// ── Quantity ──────────────────────────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IQuantity.
 *
 * Represents quantitative data with a physical unit and optional
 * minimum / maximum bounds.
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
     * \return Pointer to the Unit; may be \c nullptr for dimensionless quantities.
     */
    [[nodiscard]] HydroCouple::IUnit *unit() const override;

    /*!
     * \brief Returns the minimum allowed value (or a default-constructed
     *        variant if no minimum is set).
     */
    [[nodiscard]] hydrocouple_variant minValue() const override;

    /*!
     * \brief Sets the minimum allowed value.
     * \param min  Minimum bound. Pass a default-constructed variant to remove.
     */
    void setMinValue(const hydrocouple_variant &min);

    /*!
     * \brief Returns the maximum allowed value (or a default-constructed
     *        variant if no maximum is set).
     */
    [[nodiscard]] hydrocouple_variant maxValue() const override;

    /*!
     * \brief Sets the maximum allowed value.
     * \param max  Maximum bound. Pass a default-constructed variant to remove.
     */
    void setMaxValue(const hydrocouple_variant &max);

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
     * \param src  Source quantity definition.
     * \return Heap-allocated copy; caller owns the returned pointer.
     */
    [[nodiscard]] static Quantity *copy(const HydroCouple::IQuantity *src);

private:
    Unit               *m_unit      = nullptr;
    hydrocouple_variant m_minValue;
    hydrocouple_variant m_maxValue;
};

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_VALUEDEFINITION_H
