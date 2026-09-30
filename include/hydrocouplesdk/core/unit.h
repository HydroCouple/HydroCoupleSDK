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
#include <string>

namespace HydroCouple::SDK {

// ── UnitDimensions ────────────────────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IUnitDimensions.
 *
 * Stores the power exponents for each of the seven fundamental SI dimensions
 * plus Currency and Unitless.
 */
class HYDROCOUPLESDK_EXPORT UnitDimensions
    : public Description,
      public virtual HydroCouple::IUnitDimensions
{
public:
    UnitDimensions() = default;

    /*!
     * \brief Construct with a caption.
     * \param caption  Human-readable caption (e.g. "Length²·Time⁻¹").
     */
    explicit UnitDimensions(std::string_view caption);

    ~UnitDimensions() override = default;

    // ── IUnitDimensions ───────────────────────────────────────────────────

    /*!
     * \brief Returns the power exponent for the given fundamental dimension.
     * \param dimension  The fundamental unit dimension.
     * \return           The exponent (e.g. 1 for Length in metres, 2 for area).
     */
    [[nodiscard]] double power(
        HydroCouple::IUnitDimensions::FundamentalUnitDimension dimension) const override;

    /*!
     * \brief Sets the power exponent for the given fundamental dimension.
     * \param dimension  The fundamental unit dimension.
     * \param power      New exponent value.
     */
    void setPower(HydroCouple::IUnitDimensions::FundamentalUnitDimension dimension,
                  double power);

    /*!
     * \brief Deep-copy a IUnitDimensions instance.
     * \param src  Source dimensions.
     * \return Heap-allocated copy; caller owns the returned pointer.
     */
    [[nodiscard]] static UnitDimensions *copy(const HydroCouple::IUnitDimensions *src);

private:
    // One slot per FundamentalUnitDimension enum value (max 9 dimensions)
    std::array<double, 9> m_powers{};
};


// ── Unit ──────────────────────────────────────────────────────────────────

/*!
 * \brief Concrete implementation of HydroCouple::IUnit.
 *
 * Represents a physical unit with a conversion factor and offset relative
 * to the corresponding SI unit.
 */
class HYDROCOUPLESDK_EXPORT Unit
    : public Description,
      public virtual HydroCouple::IUnit
{
public:
    Unit() = default;

    /*!
     * \brief Construct with caption, SI conversion factor, and SI offset.
     * \param caption              Human-readable unit name (e.g. "metres per second").
     * \param conversionFactorToSI Multiply value in this unit by this factor to get SI.
     * \param offsetToSI           Add this offset after multiplying to get SI.
     */
    Unit(std::string_view caption,
         double conversionFactorToSI,
         double offsetToSI);

    ~Unit() override = default;

    // ── IUnit ─────────────────────────────────────────────────────────────

    /*!
     * \brief Returns the fundamental dimensions for this unit.
     * \return Pointer to the \c UnitDimensions; never null after construction.
     */
    [[nodiscard]] HydroCouple::IUnitDimensions *dimensions() const override;

    /*!
     * \brief Returns the internal \c UnitDimensions object (typed).
     */
    [[nodiscard]] UnitDimensions *dimensionsInternal() const;

    /*!
     * \brief Returns the factor to multiply to convert from this unit to the SI unit.
     * \return Conversion factor.
     */
    [[nodiscard]] double conversionFactorToSI() const override;

    /*!
     * \brief Sets the SI conversion factor.
     * \param factor  New conversion factor.
     */
    void setConversionFactorToSI(double factor);

    /*!
     * \brief Returns the additive offset applied after multiplying by conversionFactorToSI().
     * \return Offset value.
     */
    [[nodiscard]] double offsetToSI() const override;

    /*!
     * \brief Sets the SI offset.
     * \param offset  New offset value.
     */
    void setOffsetToSI(double offset);

    // ── Factory helpers ───────────────────────────────────────────────────

    /*! \brief Unit for metres (m). */
    [[nodiscard]] static Unit *lengthInMeters();
    /*! \brief Unit for feet (ft). */
    [[nodiscard]] static Unit *lengthInFeet();
    /*! \brief Unit for square metres (m²). */
    [[nodiscard]] static Unit *areaInSquareMeters();
    /*! \brief Unit for cubic metres (m³). */
    [[nodiscard]] static Unit *volumeInCubicMeters();
    /*! \brief Unit for cubic metres per second (m³/s). */
    [[nodiscard]] static Unit *flowInCMS();
    /*! \brief Unit for cubic feet per second (ft³/s). */
    [[nodiscard]] static Unit *flowInCFS();
    /*! \brief Unit for days. */
    [[nodiscard]] static Unit *timeInDays();
    /*! \brief Unit for seconds. */
    [[nodiscard]] static Unit *timeInSeconds();
    /*! \brief Dimensionless unit (coefficient). */
    [[nodiscard]] static Unit *unitlessCoefficient();

    /*!
     * \brief Deep-copy an IUnit instance.
     * \param src  Source unit.
     * \return Heap-allocated copy; caller owns the returned pointer.
     */
    [[nodiscard]] static Unit *copy(const HydroCouple::IUnit *src);

private:
    UnitDimensions *m_dimensions            = nullptr;
    double          m_conversionFactorToSI  = 1.0;
    double          m_offsetToSI            = 0.0;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_UNIT_H
