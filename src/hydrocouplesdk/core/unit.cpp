// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/core/unit.h"

namespace HydroCouple::SDK {

using Dim = HydroCouple::IUnitDimensions::FundamentalUnitDimension;

// ── UnitDimensions ────────────────────────────────────────────────────────

UnitDimensions::UnitDimensions(std::string_view caption)
    : Description(caption)
{
    m_powers.fill(0.0);
}

double UnitDimensions::power(Dim dimension) const
{
    const auto slot = static_cast<size_t>(dimension);
    return slot < m_powers.size() ? m_powers[slot] : 0.0;
}

void UnitDimensions::setPower(Dim dimension, double power)
{
    const auto slot = static_cast<size_t>(dimension);
    if (slot >= m_powers.size())
        return;
    m_powers[slot] = power;
    emitPropertyChanged("Power");
}

UnitDimensions *UnitDimensions::copy(const HydroCouple::IUnitDimensions *src)
{
    if (!src) return nullptr;
    auto *d = new UnitDimensions(src->caption());
    d->setDescription(src->description());
    for (size_t i = 0; i < kDimensionCount; ++i)
        d->m_powers[i] = src->power(static_cast<Dim>(i));
    return d;
}


// ── Unit ──────────────────────────────────────────────────────────────────

Unit::Unit(std::string_view caption, double conversionFactorToSI, double offsetToSI)
    : Description(caption),
      m_dimensions(new UnitDimensions()),
      m_conversionFactorToSI(conversionFactorToSI),
      m_offsetToSI(offsetToSI)
{
}

HydroCouple::IUnitDimensions *Unit::dimensions() const
{
    return m_dimensions;
}

UnitDimensions *Unit::dimensionsInternal() const
{
    return m_dimensions;
}

double Unit::conversionFactorToSI() const
{
    return m_conversionFactorToSI;
}

void Unit::setConversionFactorToSI(double factor)
{
    m_conversionFactorToSI = factor;
    emitPropertyChanged("ConversionFactorToSI");
}

double Unit::offsetToSI() const
{
    return m_offsetToSI;
}

void Unit::setOffsetToSI(double offset)
{
    m_offsetToSI = offset;
    emitPropertyChanged("OffsetToSI");
}

Unit *Unit::copy(const HydroCouple::IUnit *src)
{
    if (!src) return nullptr;
    auto *u = new Unit(src->caption(), src->conversionFactorToSI(), src->offsetToSI());
    if (src->dimensions()) {
        delete u->m_dimensions;
        u->m_dimensions = UnitDimensions::copy(src->dimensions());
    }
    return u;
}

// ── Factory helpers ────────────────────────────────────────────────────────

Unit *Unit::lengthInMeters()
{
    auto *u = new Unit("metres", 1.0, 0.0);
    u->dimensionsInternal()->setPower(Dim::Length, 1.0);
    return u;
}

Unit *Unit::lengthInFeet()
{
    auto *u = new Unit("feet", 0.3048, 0.0);
    u->dimensionsInternal()->setPower(Dim::Length, 1.0);
    return u;
}

Unit *Unit::areaInSquareMeters()
{
    auto *u = new Unit("square metres", 1.0, 0.0);
    u->dimensionsInternal()->setPower(Dim::Length, 2.0);
    return u;
}

Unit *Unit::volumeInCubicMeters()
{
    auto *u = new Unit("cubic metres", 1.0, 0.0);
    u->dimensionsInternal()->setPower(Dim::Length, 3.0);
    return u;
}

Unit *Unit::flowInCMS()
{
    auto *u = new Unit("m³/s", 1.0, 0.0);
    u->dimensionsInternal()->setPower(Dim::Length, 3.0);
    u->dimensionsInternal()->setPower(Dim::Time, -1.0);
    return u;
}

Unit *Unit::flowInCFS()
{
    auto *u = new Unit("ft³/s", 0.0283168, 0.0);
    u->dimensionsInternal()->setPower(Dim::Length, 3.0);
    u->dimensionsInternal()->setPower(Dim::Time, -1.0);
    return u;
}

Unit *Unit::timeInDays()
{
    auto *u = new Unit("days", 86400.0, 0.0);
    u->dimensionsInternal()->setPower(Dim::Time, 1.0);
    return u;
}

Unit *Unit::timeInSeconds()
{
    auto *u = new Unit("seconds", 1.0, 0.0);
    u->dimensionsInternal()->setPower(Dim::Time, 1.0);
    return u;
}

Unit *Unit::unitlessCoefficient()
{
    return new Unit("unitless", 1.0, 0.0);
}

} // namespace HydroCouple::SDK
