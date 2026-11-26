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
 * Stores the C++ type (as \c std::type_info), the value kind, the CF
 * standard name and concept URI that name the quantity, and the
 * double-valued missing/default sentinels of the v2 standard. The element
 * type a data item reports through dataKind() is the item's own; the
 * stored type is an SDK convenience that dataKindForType() can map.
 *
 * Subclasses (\c Quality, \c Quantity) add qualitative or quantitative
 * constraints. Persistence uses JSON (nlohmann/json) via toJson()/fromJson().
 *
 * Data items borrow their value definition: a component keeps the
 * definition alive (typically in a \c std::unique_ptr member) for as long as
 * the items that point to it. One definition may be shared by several items.
 *
 * What the SDK reads: the connection validator compares valueKind(),
 * conceptURI() and standardName() of the two ends of a link, and the run
 * recorder writes the value kind with each recorded quantity. Nothing in
 * the SDK substitutes missingValue() or defaultValue() into data.
 *
 * \threadsafety Not thread-safe (see Description).
 */
class HYDROCOUPLESDK_EXPORT ValueDefinition
    : public Description,
      public virtual HydroCouple::IValueDefinition
{
public:
    /*!
     * \brief Construct for a given C++ type.
     * \details Value kind Unknown, missing value -999999, default value 0,
     *          no names, empty caption.
     * \param[in] type  \c typeid of the value type (for example
     *                  \c typeid(double)); the reference must have static
     *                  storage duration, which every \c typeid result has.
     */
    explicit ValueDefinition(const std::type_info &type);

    /*!
     * \brief Construct with caption and type.
     * \details Other properties take the same defaults as ValueDefinition(const std::type_info &).
     * \param[in] caption  Human-readable caption; copied.
     * \param[in] type     \c typeid of the value type.
     */
    ValueDefinition(std::string_view caption, const std::type_info &type);

    //! Destroys the definition.
    ~ValueDefinition() override = default;

    // ── IValueDefinition ─────────────────────────────────────────────────

    /*!
     * \brief Returns the \c type_info for the value type.
     * \details SDK-level convenience (no longer part of the interface, whose
     * element type is the owning item's dataKind()).
     * \return The type given at construction.
     */
    [[nodiscard]] const std::type_info &type() const;

    /*!
     * \brief How values behave under regridding and aggregation.
     * \details Defaults to ValueKind::Unknown; a component that knows what its
     * quantity is (and every component does) sets it with setValueKind().
     * The connection validator reports two different declared kinds as an
     * error and an undeclared kind on either end as a warning
     * (Diagnostic::ValueKindMismatch).
     * \return The declared value kind.
     */
    [[nodiscard]] HydroCouple::ValueKind valueKind() const override;

    /*!
     * \brief Sets the value kind and fires propertyChanged("ValueKind").
     * \param[in] kind  Intensive, Extensive, Flux, Density, or Unknown to
     *                  withdraw the declaration.
     */
    void setValueKind(HydroCouple::ValueKind kind);

    /*!
     * \brief The CF standard name of the quantity; empty when not declared.
     * \return A copy of the name.
     */
    [[nodiscard]] std::string standardName() const override;

    /*!
     * \brief Sets the CF standard name and fires propertyChanged("StandardName").
     * \details Not checked against the CF Standard Name Table. The validator
     *          compares standard names only when one end has no concept URI.
     * \param[in] name  New name; copied. Empty means not declared.
     */
    void setStandardName(std::string_view name);

    /*!
     * \brief A URI naming the quantity in a published vocabulary; empty when
     *        not declared.
     * \return A copy of the URI.
     */
    [[nodiscard]] std::string conceptURI() const override;

    /*!
     * \brief Sets the concept URI and fires propertyChanged("ConceptURI").
     * \details Not parsed. When both ends of a link declare a URI, the
     *          validator compares the URIs as strings and ignores the
     *          standard names.
     * \param[in] uri  New URI; copied. Empty means not declared.
     */
    void setConceptURI(std::string_view uri);

    /*!
     * \brief Returns the sentinel value representing missing data
     *        (numeric DataKinds only).
     * \return The sentinel; -999999 unless changed.
     */
    [[nodiscard]] double missingValue() const override;

    /*!
     * \brief Sets the missing-value sentinel.
     * \details Fires propertyChanged("MissingValue"). Descriptive only: the
     *          SDK does not write it into data.
     * \param[in] value  New sentinel, in the quantity's unit.
     */
    void setMissingValue(double value);

    /*!
     * \brief Returns the default value for new data items
     *        (numeric DataKinds only).
     * \return The default; 0 unless changed.
     */
    [[nodiscard]] double defaultValue() const override;

    /*!
     * \brief Sets the default value.
     * \details Fires propertyChanged("DefaultValue"). The SDK's data items
     *          keep their own default element value (ComponentDataItem1D,
     *          for example) and do not read this property.
     * \param[in] value  New default, in the quantity's unit.
     */
    void setDefaultValue(double value);

    // ── JSON persistence ──────────────────────────────────────────────────

    /*!
     * \brief Serialize this value definition to a JSON object.
     * \details Writes "caption", "description", "missingValue",
     *          "defaultValue" and "valueKind" (the enumerator's integer
     *          value), and "standardName" and "conceptURI" only when they
     *          are declared. The C++ type is not written.
     * \return A new JSON object.
     */
    [[nodiscard]] virtual nlohmann::json toJson() const;

    /*!
     * \brief Populate this value definition from a JSON object.
     * \details Reads each key toJson() writes, when present, and leaves the
     *          others unchanged. Caption and description go through their
     *          setters and fire propertyChanged; the other properties are
     *          assigned without a signal.
     * \param[in] j  A JSON object as toJson() writes it.
     * \throws nlohmann::json::type_error when a present key has the wrong
     *         JSON type.
     */
    virtual void fromJson(const nlohmann::json &j);

    // ── Utility helpers ───────────────────────────────────────────────────

    /*!
     * \brief The DataKind corresponding to a C++ \c type_info, for the
     *        types the standard's data plane supports.
     * \details Maps double, float, the signed and unsigned 8-, 16-, 32- and
     *          64-bit integers, bool and std::string to Float64, Float32,
     *          Int8 to UInt64, Boolean and String.
     * \param[in] type  The C++ type to look up.
     * \return The matching DataKind, or DataKind::Opaque when unmapped.
     */
    [[nodiscard]] static DataKind dataKindForType(const std::type_info &type);

    /*!
     * \brief Create a deep copy of the given IValueDefinition.
     * \details Always creates a plain ValueDefinition, even when \p src is a
     *          quantity or a quality: the unit, bounds and categories are not
     *          copied (use Quantity::copy() or Quality::copy() for those).
     *          Copies caption, description, missing and default values,
     *          value kind, standard name and concept URI. The type is \p src's
     *          when \p src is an SDK ValueDefinition, otherwise \c double.
     * \param[in] src  Source definition; may be nullptr.
     * \return Heap-allocated copy; caller owns the returned pointer. nullptr
     *         when \p src is nullptr.
     */
    [[nodiscard]] static ValueDefinition *copy(const HydroCouple::IValueDefinition *src);

protected:
    //! The value type given at construction; non-owning, static storage duration.
    const std::type_info *m_type = &typeid(void);  // non-owning; static storage duration
    //! Declared value kind; Unknown until set.
    HydroCouple::ValueKind m_valueKind = HydroCouple::ValueKind::Unknown;

private:
    //! Missing-data sentinel.
    double m_missingValue = -999999.0;
    //! Default value.
    double m_defaultValue = 0.0;
    //! CF standard name; empty when not declared.
    std::string m_standardName;
    //! Vocabulary URI; empty when not declared.
    std::string m_conceptURI;
};


// ── Quality ───────────────────────────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IQuality.
 *
 * Represents qualitative (categorical) data. Per the v2 standard,
 * categories are string labels and data values are indexes into the
 * category list: value k means categories()[k]. The value type is
 * \c int32_t. Two qualities connect only when their category lists are
 * equal element by element, in order (Diagnostic::ValueDefinitionMismatch).
 *
 * The value kind starts Unknown like every ValueDefinition; the interface
 * expects a quality to declare ValueKind::Intensive.
 *
 * \threadsafety Not thread-safe (see Description).
 */
class HYDROCOUPLESDK_EXPORT Quality
    : public ValueDefinition,
      public virtual HydroCouple::IQuality
{
public:
    /*!
     * \brief Construct with an initial category list.
     * \details Not ordered; empty caption.
     * \param[in] categories  Ordered category labels; copied. Duplicates
     *                        are not rejected.
     */
    explicit Quality(const std::vector<std::string> &categories = {});

    /*!
     * \brief Construct with caption and initial category list.
     * \param[in] caption     Human-readable caption; copied.
     * \param[in] categories  Category labels; copied.
     */
    Quality(std::string_view caption,
            const std::vector<std::string> &categories = {});

    //! Destroys the quality.
    ~Quality() override = default;

    // ── IQuality ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns the category labels.
     * \details Data values are indexes into this vector.
     * \return A copy of the labels, in stored order.
     */
    [[nodiscard]] std::vector<std::string> categories() const override;

    /*!
     * \brief Replace the category list.
     * \details Fires propertyChanged("Categories"). Values already stored as
     *          indexes are not remapped.
     * \param[in] categories  New labels; copied.
     */
    void setCategories(const std::vector<std::string> &categories);

    /*!
     * \brief Append a single category label.
     * \details Fires propertyChanged("Categories"). The new label's index is
     *          the previous size of the list.
     * \param[in] category  Label to append; copied.
     */
    void addCategory(const std::string &category);

    /*!
     * \brief Returns \c true if categories have a meaningful order.
     * \return \c false unless setIsOrdered(true) was called.
     */
    [[nodiscard]] bool isOrdered() const override;

    /*!
     * \brief Set whether categories are ordered.
     * \details Fires propertyChanged("IsOrdered"). The validator does not
     *          compare it.
     * \param[in] ordered  \c true for ordinal categories, \c false for nominal.
     */
    void setIsOrdered(bool ordered);

    /*!
     * \brief Serializes the quality.
     * \return ValueDefinition::toJson() plus "categories" (an array of
     *         strings) and "isOrdered".
     */
    [[nodiscard]] nlohmann::json toJson()    const override;

    /*!
     * \brief Reads a quality as toJson() writes it.
     * \details Reads the base keys, then "categories" and "isOrdered" when
     *          present, without firing a signal for those two.
     * \param[in] j  JSON object.
     * \throws nlohmann::json::type_error when a present key has the wrong type.
     */
    void fromJson(const nlohmann::json &j)         override;

    /*!
     * \brief Deep-copy a \c IQuality instance.
     * \details Copies categories, order flag, caption, description, missing
     *          and default values, value kind and both names. The copy's type
     *          is \c int32_t.
     * \param[in] src  Source quality; may be nullptr.
     * \return Heap-allocated copy; caller owns the returned pointer. nullptr
     *         when \p src is nullptr.
     */
    [[nodiscard]] static Quality *copy(const HydroCouple::IQuality *src);

private:
    //! Category labels; a data value is an index into this list.
    std::vector<std::string> m_categories;
    //! Whether the categories are ordinal.
    bool                     m_isOrdered = false;
};


// ── Quantity ──────────────────────────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IQuantity.
 *
 * Represents quantitative data with a physical unit and double-valued
 * minimum / maximum bounds per the v2 standard. The bounds are
 * descriptive: no SDK code checks values against them.
 *
 * Ownership: a Quantity stores the Unit pointer it is given and never
 * deletes it; the destructor does nothing. The caller owns the unit and
 * keeps it alive at least as long as the quantity. The factory helpers
 * (flowInCMS() and the others) and copy() allocate a new Unit that nothing
 * deletes, so each quantity made that way leaks its Unit, and the Unit's
 * UnitDimensions, when the quantity is destroyed. To avoid the leak, own
 * the Unit yourself:
 *
 * \code
 *     std::unique_ptr<Unit>     unit(Unit::flowInCMS());
 *     std::unique_ptr<Quantity> flow(
 *         new Quantity("Discharge", typeid(double), unit.get()));
 * \endcode
 *
 * \threadsafety Not thread-safe (see Description).
 */
class HYDROCOUPLESDK_EXPORT Quantity
    : public ValueDefinition,
      public virtual HydroCouple::IQuantity
{
public:
    /*!
     * \brief Construct with a type and a borrowed unit.
     * \details Bounds start at the lowest and highest finite doubles.
     * \param[in] type  \c typeid of the numeric value type.
     * \param[in] unit  Physical unit; borrowed, never deleted by the
     *                  quantity, and must outlive it. nullptr is accepted
     *                  and makes the validator warn that the quantity
     *                  declares no unit.
     */
    Quantity(const std::type_info &type, Unit *unit);

    /*!
     * \brief Construct with caption, type, and a borrowed unit.
     * \param[in] caption  Human-readable caption; copied.
     * \param[in] type     \c typeid of the numeric value type.
     * \param[in] unit     Physical unit; borrowed, as for Quantity(const std::type_info &, Unit *).
     */
    Quantity(std::string_view caption, const std::type_info &type, Unit *unit);

    //! Destroys the quantity. The unit is not deleted.
    ~Quantity() override = default;

    // ── IQuantity ────────────────────────────────────────────────────────

    /*!
     * \brief Returns the physical unit associated with this quantity.
     * \return The borrowed unit, or nullptr when constructed with none.
     */
    [[nodiscard]] HydroCouple::IUnit *unit() const override;

    /*!
     * \brief Returns the minimum allowed value.
     * \return The lower bound, in the quantity's unit;
     *         \c std::numeric_limits<double>::lowest() unless changed.
     */
    [[nodiscard]] double minValue() const override;

    /*!
     * \brief Sets the minimum allowed value.
     * \details Fires propertyChanged("MinValue"). Not checked against
     *          maxValue().
     * \param[in] min  New lower bound, in the quantity's unit.
     */
    void setMinValue(double min);

    /*!
     * \brief Returns the maximum allowed value.
     * \return The upper bound, in the quantity's unit;
     *         \c std::numeric_limits<double>::max() unless changed.
     */
    [[nodiscard]] double maxValue() const override;

    /*!
     * \brief Sets the maximum allowed value.
     * \details Fires propertyChanged("MaxValue"). Not checked against
     *          minValue().
     * \param[in] max  New upper bound, in the quantity's unit.
     */
    void setMaxValue(double max);

    /*!
     * \brief Serializes the quantity.
     * \return ValueDefinition::toJson() plus "minValue", "maxValue" and,
     *         when a unit is set, a "unit" object with "caption",
     *         "conversionFactorToSI", "offsetToSI" and, when declared,
     *         "symbol" and "ucumCode". The unit's dimensions are not written.
     */
    [[nodiscard]] nlohmann::json toJson()    const override;

    /*!
     * \brief Reads a quantity as toJson() writes it.
     * \details Reads the base keys, then "minValue" and "maxValue" when
     *          present, without firing a signal for those two. The "unit"
     *          object is not read: the quantity keeps the unit it was
     *          constructed with.
     * \param[in] j  JSON object.
     * \throws nlohmann::json::type_error when a present key has the wrong type.
     */
    void fromJson(const nlohmann::json &j)         override;

    // ── Factory helpers ───────────────────────────────────────────────────
    //
    // Each declares its QUDT quantity kind as conceptURI() (VolumeFlowRate,
    // Length, Area, Volume, Time) and no CF standard name; unitLess() declares
    // neither. Set narrower names on the returned quantity when you know them.
    // Each returns a new double-valued quantity owned by the caller over a
    // new Unit that nothing deletes (see the class description).

    /*!
     * \brief Returns a \c double Quantity for flow in m³/s.
     * \return New quantity, caption "Flow (m³/s)", unit Unit::flowInCMS(),
     *         concept URI http://qudt.org/vocab/quantitykind/VolumeFlowRate.
     */
    [[nodiscard]] static Quantity *flowInCMS();
    /*!
     * \brief Returns a \c double Quantity for flow in ft³/s.
     * \return New quantity, caption "Flow (ft³/s)", unit Unit::flowInCFS(),
     *         concept URI http://qudt.org/vocab/quantitykind/VolumeFlowRate.
     */
    [[nodiscard]] static Quantity *flowInCFS();
    /*!
     * \brief Returns a \c double Quantity for length in meters.
     * \return New quantity, caption "Length (m)", unit
     *         Unit::lengthInMeters(), concept URI
     *         http://qudt.org/vocab/quantitykind/Length.
     */
    [[nodiscard]] static Quantity *lengthInMeters();
    /*!
     * \brief Returns a \c double Quantity for length in feet.
     * \return New quantity, caption "Length (ft)", unit
     *         Unit::lengthInFeet(), concept URI
     *         http://qudt.org/vocab/quantitykind/Length.
     */
    [[nodiscard]] static Quantity *lengthInFeet();
    /*!
     * \brief Returns a \c double Quantity for area in m².
     * \return New quantity, caption "Area (m²)", unit
     *         Unit::areaInSquareMeters(), concept URI
     *         http://qudt.org/vocab/quantitykind/Area.
     */
    [[nodiscard]] static Quantity *areaInSquareMeters();
    /*!
     * \brief Returns a \c double Quantity for volume in m³.
     * \return New quantity, caption "Volume (m³)", unit
     *         Unit::volumeInCubicMeters(), concept URI
     *         http://qudt.org/vocab/quantitykind/Volume.
     */
    [[nodiscard]] static Quantity *volumeInCubicMeters();
    /*!
     * \brief Returns a \c double Quantity for time in days.
     * \return New quantity, caption "Time (days)", unit Unit::timeInDays(),
     *         concept URI http://qudt.org/vocab/quantitykind/Time.
     */
    [[nodiscard]] static Quantity *timeInDays();
    /*!
     * \brief Returns a \c double Quantity for time in seconds.
     * \return New quantity, caption "Time (s)", unit Unit::timeInSeconds(),
     *         concept URI http://qudt.org/vocab/quantitykind/Time.
     */
    [[nodiscard]] static Quantity *timeInSeconds();
    /*!
     * \brief Returns a dimensionless \c double Quantity.
     * \details No concept URI or standard name. Against a dimensioned unit
     *          the validator reports only a warning, because a
     *          dimensionless unit is also what an undeclared placeholder
     *          looks like.
     * \param[in] caption  Caption of the new quantity; copied.
     * \return New quantity over Unit::unitlessCoefficient().
     */
    [[nodiscard]] static Quantity *unitLess(std::string_view caption = "Unitless");

    /*!
     * \brief Deep-copy an \c IQuantity instance.
     * \details Copies the unit with Unit::copy() (a new Unit that nothing
     *          deletes), the bounds, caption, description, missing and
     *          default values, value kind and both names. The type is
     *          \p src's when \p src is an SDK ValueDefinition, otherwise
     *          \c double. A source without a unit gives a copy without one.
     * \param[in] src  Source quantity; may be nullptr.
     * \return Heap-allocated copy; caller owns the returned pointer. nullptr
     *         when \p src is nullptr.
     */
    [[nodiscard]] static Quantity *copy(const HydroCouple::IQuantity *src);

private:
    //! The borrowed unit; never deleted by the quantity.
    Unit   *m_unit     = nullptr;
    //! Lower bound, descriptive only.
    double  m_minValue = std::numeric_limits<double>::lowest();
    //! Upper bound, descriptive only.
    double  m_maxValue = std::numeric_limits<double>::max();
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_VALUEDEFINITION_H
