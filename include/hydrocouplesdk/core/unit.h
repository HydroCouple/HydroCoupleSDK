// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   unit.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete implementations of HydroCouple::IUnit and IUnitDimensions.
 */

#ifndef HYDROCOUPLESDK_UNIT_H
#define HYDROCOUPLESDK_UNIT_H

#include "hydrocouplesdk/core/description.h"
#include "hydrocouple.h"

#include <array>
#include <cstddef>
#include <string>

namespace HydroCouple::SDK {

// ── UnitDimensions ────────────────────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IUnitDimensions.
 *
 * Stores the power exponents for each of the seven fundamental SI dimensions
 * plus Currency, Unitless and PlaneAngle: one slot per
 * FundamentalUnitDimension value, all zero at construction. Meters per
 * second, for example, is Length 1 and Time -1.
 *
 * The connection validator compares two units' powers on every axis except
 * Unitless, with an absolute tolerance of 1e-12
 * (Diagnostic::UnitMismatch). Unitless is a slot like the others; nothing
 * in the SDK reads it.
 *
 * \threadsafety Not thread-safe (see Description).
 */
class HYDROCOUPLESDK_EXPORT UnitDimensions
    : public Description,
      public virtual HydroCouple::IUnitDimensions
{
public:
    //! Creates dimensions with every power zero and an empty caption.
    UnitDimensions() = default;

    /*!
     * \brief Construct with a caption.
     * \details Every power starts at zero.
     * \param[in] caption  Human-readable caption (for example "Length²·Time⁻¹"); copied.
     */
    explicit UnitDimensions(std::string_view caption);

    //! Destroys the dimensions.
    ~UnitDimensions() override = default;

    // ── IUnitDimensions ───────────────────────────────────────────────────

    /*!
     * \brief Returns the power exponent for the given fundamental dimension.
     * \param[in] dimension  The fundamental unit dimension.
     * \return           The exponent (for example 1 for Length in meters,
     *                   2 for area); 0 for a dimension value outside the
     *                   enumeration.
     */
    [[nodiscard]] double power(
        HydroCouple::IUnitDimensions::FundamentalUnitDimension dimension) const override;

    /*!
     * \brief Sets the power exponent for the given fundamental dimension.
     * \details Fires propertyChanged("Power"). A dimension value outside the
     *          enumeration is ignored and nothing fires.
     * \param[in] dimension  The fundamental unit dimension.
     * \param[in] power      New exponent value; fractional exponents are
     *                       allowed.
     */
    void setPower(HydroCouple::IUnitDimensions::FundamentalUnitDimension dimension,
                  double power);

    /*!
     * \brief Deep-copy a IUnitDimensions instance.
     * \details Copies caption, description and every power.
     * \param[in] src  Source dimensions; may be nullptr.
     * \return Heap-allocated copy; caller owns the returned pointer. nullptr
     *         when \p src is nullptr.
     */
    [[nodiscard]] static UnitDimensions *copy(const HydroCouple::IUnitDimensions *src);

    //! One slot per FundamentalUnitDimension value. Sized from the enum's
    //! last value so that adding a dimension to the standard cannot leave
    //! it reading and writing past the end, as PlaneAngle once did.
    static constexpr size_t kDimensionCount =
        static_cast<size_t>(HydroCouple::IUnitDimensions::FundamentalUnitDimension::PlaneAngle) + 1;

private:
    //! Exponents indexed by FundamentalUnitDimension value.
    std::array<double, kDimensionCount> m_powers{};
};


// ── Unit ──────────────────────────────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IUnit.
 *
 * Represents a physical unit with a conversion factor \f$A\f$
 * (conversionFactorToSI()) and offset \f$B\f$ (offsetToSI()) relative to
 * the corresponding SI unit:
 * \f[ x_{SI} = A\,x + B \f]
 * Feet, for example, is \f$A = 0.3048\f$ and \f$B = 0\f$; degrees
 * Celsius to kelvin is \f$A = 1\f$ and \f$B = 273.15\f$. A unit also carries its
 * fundamental dimensions and, optionally, a UDUNITS-2 symbol and a UCUM
 * code, which the writers record as unit metadata.
 *
 * The SDK converts nothing on its own: a connection between two units of
 * the same dimension but a different factor or offset is reported as an
 * error (Diagnostic::UnitMismatch) and needs a unit-conversion adapted
 * output between the two ends.
 *
 * Ownership: the constructor that takes a caption allocates a
 * UnitDimensions object that dimensions() returns, and the destructor does
 * not free it, so every such Unit leaks its UnitDimensions when destroyed.
 * Quantity borrows its Unit (it never deletes it), so whoever creates a
 * Quantity keeps the Unit alive at least as long and deletes it afterwards.
 *
 * \threadsafety Not thread-safe (see Description).
 */
class HYDROCOUPLESDK_EXPORT Unit
    : public Description,
      public virtual HydroCouple::IUnit
{
public:
    /*!
     * \brief Creates a unit with factor 1, offset 0 and no dimensions object.
     * \details dimensions() returns nullptr for a unit made this way, which
     *          the connection validator reports as a warning ("a unit
     *          declares no dimensions"). Prefer the constructor that takes a
     *          caption.
     */
    Unit() = default;

    /*!
     * \brief Construct with caption, SI conversion factor, and SI offset.
     * \details Allocates a UnitDimensions with every power zero; set the
     *          powers through dimensionsInternal().
     * \param[in] caption              Human-readable unit name (for example
     *                                 "meters per second"); copied.
     * \param[in] conversionFactorToSI Multiply a value in this unit by this
     *                                 factor to get SI. No range is checked.
     * \param[in] offsetToSI           Add this offset after multiplying to get SI.
     */
    Unit(std::string_view caption,
         double conversionFactorToSI,
         double offsetToSI);

    //! Destroys the unit. The UnitDimensions it allocated is not freed.
    ~Unit() override = default;

    // ── IUnit ─────────────────────────────────────────────────────────────

    /*!
     * \brief Returns the fundamental dimensions for this unit.
     * \return Pointer to the \c UnitDimensions owned by this unit; nullptr
     *         for a default-constructed unit.
     */
    [[nodiscard]] HydroCouple::IUnitDimensions *dimensions() const override;

    /*!
     * \brief Returns the internal \c UnitDimensions object (typed).
     * \return The same object as dimensions(), typed so its powers can be
     *         set; nullptr for a default-constructed unit.
     */
    [[nodiscard]] UnitDimensions *dimensionsInternal() const;

    /*!
     * \brief Returns the factor to multiply to convert from this unit to the SI unit.
     * \return Conversion factor ('A' in SI = A × value + B).
     */
    [[nodiscard]] double conversionFactorToSI() const override;

    /*!
     * \brief Sets the SI conversion factor.
     * \details Fires propertyChanged("ConversionFactorToSI"). No range is
     *          checked; the connection validator compares factors with a
     *          relative tolerance of 1e-12.
     * \param[in] factor  New conversion factor.
     */
    void setConversionFactorToSI(double factor);

    /*!
     * \brief Returns the additive offset applied after multiplying by conversionFactorToSI().
     * \return Offset value ('B' in SI = A × value + B).
     */
    [[nodiscard]] double offsetToSI() const override;

    /*!
     * \brief Sets the SI offset.
     * \details Fires propertyChanged("OffsetToSI").
     * \param[in] offset  New offset value, in SI units.
     */
    void setOffsetToSI(double offset);

    /*!
     * \brief The unit as a UDUNITS-2 string (for example "m3 s-1"); empty when not declared.
     * \return A copy of the symbol.
     */
    [[nodiscard]] std::string symbol() const override;

    /*!
     * \brief Sets the UDUNITS-2 symbol and fires propertyChanged("Symbol").
     * \details Not parsed or checked against dimensions() or the factor; the
     *          interface requires them to agree.
     * \param[in] symbol  New symbol; copied. Empty means not declared.
     */
    void setSymbol(std::string_view symbol);

    /*!
     * \brief The unit as a case-sensitive UCUM code (for example "m3/s"); empty when
     *        not declared.
     * \return A copy of the code.
     */
    [[nodiscard]] std::string ucumCode() const override;

    /*!
     * \brief Sets the UCUM code and fires propertyChanged("UcumCode").
     * \details Not parsed or checked.
     * \param[in] code  New UCUM code; copied. Empty means not declared.
     */
    void setUcumCode(std::string_view code);

    // ── Factory helpers ───────────────────────────────────────────────────
    //
    // Each returns a new Unit owned by the caller, with its dimensions,
    // UDUNITS-2 symbol and UCUM code set.

    /*!
     * \brief Unit for meters (m).
     * \return New unit owned by the caller: caption "metres", factor 1,
     *         Length 1, symbol "m", UCUM "m".
     */
    [[nodiscard]] static Unit *lengthInMeters();
    /*!
     * \brief Unit for feet (ft).
     * \return New unit owned by the caller: caption "feet", factor 0.3048,
     *         Length 1, symbol "ft", UCUM "[ft_i]".
     */
    [[nodiscard]] static Unit *lengthInFeet();
    /*!
     * \brief Unit for square meters (m²).
     * \return New unit owned by the caller: caption "square metres",
     *         factor 1, Length 2, symbol "m2", UCUM "m2".
     */
    [[nodiscard]] static Unit *areaInSquareMeters();
    /*!
     * \brief Unit for cubic meters (m³).
     * \return New unit owned by the caller: caption "cubic metres",
     *         factor 1, Length 3, symbol "m3", UCUM "m3".
     */
    [[nodiscard]] static Unit *volumeInCubicMeters();
    /*!
     * \brief Unit for cubic meters per second (m³/s).
     * \return New unit owned by the caller: caption "m³/s", factor 1,
     *         Length 3 and Time -1, symbol "m3 s-1", UCUM "m3/s".
     */
    [[nodiscard]] static Unit *flowInCMS();
    /*!
     * \brief Unit for cubic feet per second (ft³/s).
     * \return New unit owned by the caller: caption "ft³/s", factor
     *         0.0283168, Length 3 and Time -1, symbol "ft3 s-1", UCUM
     *         "[ft_i]3/s".
     */
    [[nodiscard]] static Unit *flowInCFS();
    /*!
     * \brief Unit for days.
     * \return New unit owned by the caller: caption "days", factor 86400,
     *         Time 1, symbol "d", UCUM "d".
     */
    [[nodiscard]] static Unit *timeInDays();
    /*!
     * \brief Unit for seconds.
     * \return New unit owned by the caller: caption "seconds", factor 1,
     *         Time 1, symbol "s", UCUM "s".
     */
    [[nodiscard]] static Unit *timeInSeconds();
    /*!
     * \brief Dimensionless unit (coefficient).
     * \return New unit owned by the caller: caption "unitless", factor 1,
     *         every power zero, symbol "1", UCUM "1".
     */
    [[nodiscard]] static Unit *unitlessCoefficient();

    /*!
     * \brief Deep-copy an IUnit instance.
     * \details Copies caption, factor, offset, symbol, UCUM code and, when
     *          \p src has them, its dimensions (otherwise the copy keeps an
     *          all-zero UnitDimensions). The description is not copied.
     * \param[in] src  Source unit; may be nullptr.
     * \return Heap-allocated copy; caller owns the returned pointer. nullptr
     *         when \p src is nullptr.
     */
    [[nodiscard]] static Unit *copy(const HydroCouple::IUnit *src);

private:
    //! Dimensions allocated by the caption constructor; nullptr after
    //! default construction.
    UnitDimensions *m_dimensions            = nullptr;
    //! Factor 'A' in SI = A × value + B.
    double          m_conversionFactorToSI  = 1.0;
    //! Offset 'B' in SI = A × value + B.
    double          m_offsetToSI            = 0.0;
    //! UDUNITS-2 symbol; empty when not declared.
    std::string     m_symbol;
    //! UCUM code; empty when not declared.
    std::string     m_ucumCode;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_UNIT_H
