// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/component/connectionvalidator.h"
#include "hydrocouplesdk/core/diagnosticcodes.h"

#include "hydrocoupletemporal.h"
#include "hydrocouplespatial.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace HydroCouple::SDK {

namespace {

using Severity = HydroCouple::ErrorEntry::Severity;
namespace Spatial  = HydroCouple::Spatial;
namespace Temporal = HydroCouple::Temporal;

class Findings
{
public:
    Findings(const HydroCouple::IOutput &provider, const HydroCouple::IInput &consumer)
        : m_prefix("link '" + provider.id() + "' -> '" + consumer.id() + "': ")
    {
    }

    void error(int32_t code, const std::string &what)
    {
        m_items.push_back({Severity::Error, code, m_prefix + what});
    }

    void warning(int32_t code, const std::string &what)
    {
        m_items.push_back({Severity::Warning, code, m_prefix + what});
    }

    std::vector<ConnectionFinding> take() { return std::move(m_items); }

private:
    std::string                    m_prefix;
    std::vector<ConnectionFinding> m_items;
};

// ── Names for messages ────────────────────────────────────────────────────

const char *name(HydroCouple::ValueKind kind)
{
    switch (kind)
    {
        case HydroCouple::ValueKind::Intensive: return "Intensive";
        case HydroCouple::ValueKind::Extensive: return "Extensive";
        case HydroCouple::ValueKind::Flux:      return "Flux";
        case HydroCouple::ValueKind::Density:   return "Density";
        default:                                return "Unknown";
    }
}

const char *name(Temporal::TimeKind kind)
{
    switch (kind)
    {
        case Temporal::TimeKind::Instantaneous:   return "Instantaneous";
        case Temporal::TimeKind::IntervalMean:    return "IntervalMean";
        case Temporal::TimeKind::IntervalMinimum: return "IntervalMinimum";
        case Temporal::TimeKind::IntervalMaximum: return "IntervalMaximum";
        case Temporal::TimeKind::Accumulated:     return "Accumulated";
        default:                                  return "Unknown";
    }
}

const char *name(Temporal::TimeInterpolation policy)
{
    switch (policy)
    {
        case Temporal::TimeInterpolation::None:     return "None";
        case Temporal::TimeInterpolation::Previous: return "Previous";
        case Temporal::TimeInterpolation::Nearest:  return "Nearest";
        case Temporal::TimeInterpolation::Linear:   return "Linear";
        default:                                    return "Unknown";
    }
}

const char *name(Temporal::TimeExtrapolation policy)
{
    switch (policy)
    {
        case Temporal::TimeExtrapolation::Refuse:   return "Refuse";
        case Temporal::TimeExtrapolation::HoldLast: return "HoldLast";
        case Temporal::TimeExtrapolation::Linear:   return "Linear";
        default:                                    return "Unknown";
    }
}

const char *name(Spatial::MeshLocation location)
{
    switch (location)
    {
        case Spatial::MeshLocation::Node:   return "Node";
        case Spatial::MeshLocation::Edge:   return "Edge";
        case Spatial::MeshLocation::Face:   return "Face";
        case Spatial::MeshLocation::Volume: return "Volume";
    }
    return "?";
}

const char *name(Spatial::SpatialDataType type)
{
    switch (type)
    {
        case Spatial::SpatialDataType::Scalar:      return "Scalar";
        case Spatial::SpatialDataType::MultiScalar: return "MultiScalar";
        case Spatial::SpatialDataType::Vector:      return "Vector";
        case Spatial::SpatialDataType::Tensor:      return "Tensor";
    }
    return "?";
}

const char *name(Spatial::VectorBasis basis)
{
    switch (basis)
    {
        case Spatial::VectorBasis::Cartesian:        return "Cartesian";
        case Spatial::VectorBasis::EastNorthUp:      return "EastNorthUp";
        case Spatial::VectorBasis::NormalTangential: return "NormalTangential";
        case Spatial::VectorBasis::AlongEntity:      return "AlongEntity";
        default:                                     return "Unknown";
    }
}

const char *name(HydroCouple::IDimension::DimensionRole role)
{
    using Role = HydroCouple::IDimension::DimensionRole;
    switch (role)
    {
        case Role::Time:        return "Time";
        case Role::Entity:      return "Entity";
        case Role::Layer:       return "Layer";
        case Role::Band:        return "Band";
        case Role::Row:         return "Row";
        case Role::Column:      return "Column";
        case Role::Depth:       return "Depth";
        case Role::Component:   return "Component";
        case Role::Realization: return "Realization";
        case Role::Other:       return "Other";
        default:                return "Unknown";
    }
}

const char *name(Spatial::VerticalCoordinateKind kind)
{
    switch (kind)
    {
        case Spatial::VerticalCoordinateKind::Sigma:             return "Sigma";
        case Spatial::VerticalCoordinateKind::ZLevel:            return "ZLevel";
        case Spatial::VerticalCoordinateKind::ZStar:             return "ZStar";
        case Spatial::VerticalCoordinateKind::Hybrid:            return "Hybrid";
        case Spatial::VerticalCoordinateKind::Isopycnal:         return "Isopycnal";
        case Spatial::VerticalCoordinateKind::DepthBelowSurface: return "DepthBelowSurface";
        default:                                                 return "Unknown";
    }
}

// ── Data kind ─────────────────────────────────────────────────────────────

void checkDataKind(const HydroCouple::IOutput &provider,
                   const HydroCouple::IInput &consumer, Findings &findings)
{
    if (provider.dataKind() != consumer.dataKind())
        findings.error(Diagnostic::DataKindMismatch,
                       "element types differ (the data plane converts nothing)");
}

// ── Value definitions: quantity/quality, ValueKind, units ─────────────────

std::string powersOf(HydroCouple::IUnitDimensions &dims)
{
    using D = HydroCouple::IUnitDimensions::FundamentalUnitDimension;
    static const std::pair<D, const char *> axes[] = {
        {D::Length, "L"}, {D::Mass, "M"}, {D::Time, "T"},
        {D::ElectricCurrent, "I"}, {D::Temperature, "Θ"},
        {D::AmountOfSubstance, "N"}, {D::LuminousIntensity, "J"},
        {D::Currency, "$"}, {D::PlaneAngle, "rad"}};
    std::ostringstream text;
    for (const auto &[axis, symbol] : axes)
    {
        const double power = dims.power(axis);
        if (power != 0.0)
            text << symbol << "^" << power << " ";
    }
    std::string result = text.str();
    return result.empty() ? "dimensionless" : result.substr(0, result.size() - 1);
}

bool samePowers(HydroCouple::IUnitDimensions &a, HydroCouple::IUnitDimensions &b)
{
    using D = HydroCouple::IUnitDimensions::FundamentalUnitDimension;
    for (D axis : {D::Length, D::Mass, D::Time, D::ElectricCurrent, D::Temperature,
                   D::AmountOfSubstance, D::LuminousIntensity, D::Currency,
                   D::PlaneAngle})
    {
        if (std::abs(a.power(axis) - b.power(axis)) > 1e-12)
            return false;
    }
    return true;
}

bool isDimensionless(HydroCouple::IUnitDimensions &dims)
{
    return powersOf(dims) == "dimensionless";
}

bool closeRelative(double a, double b)
{
    return std::abs(a - b) <= 1e-12 * std::max({1.0, std::abs(a), std::abs(b)});
}

void checkValueDefinitions(const HydroCouple::IOutput &provider,
                           const HydroCouple::IInput &consumer, Findings &findings)
{
    HydroCouple::IValueDefinition *produced = provider.valueDefinition();
    HydroCouple::IValueDefinition *expected = consumer.valueDefinition();

    // An adapted output that declares no value definition owns its value
    // semantics (a unit conversion, a regrid): nothing to compare.
    if (!produced &&
        dynamic_cast<const HydroCouple::IAdaptedOutput *>(&provider) != nullptr)
        return;

    if (!produced || !expected)
    {
        findings.warning(Diagnostic::ValueDefinitionMismatch,
                         std::string(!produced ? "provider" : "consumer") +
                             " declares no value definition; units and value kind "
                             "cannot be checked");
        return;
    }

    // ValueKind: how the numbers regrid. Same mesh or not, an Intensive
    // value consumed as Extensive is summed where it should be averaged.
    const HydroCouple::ValueKind producedKind = produced->valueKind();
    const HydroCouple::ValueKind expectedKind = expected->valueKind();
    if (producedKind == HydroCouple::ValueKind::Unknown ||
        expectedKind == HydroCouple::ValueKind::Unknown)
    {
        if (producedKind != expectedKind)
            findings.warning(Diagnostic::ValueKindMismatch,
                             std::string(producedKind == HydroCouple::ValueKind::Unknown
                                             ? "provider"
                                             : "consumer") +
                                 " declares no ValueKind (the other says " +
                                 name(producedKind == HydroCouple::ValueKind::Unknown
                                          ? expectedKind
                                          : producedKind) +
                                 "); an adapter could not choose between averaging "
                                 "and summing");
        else
            findings.warning(Diagnostic::ValueKindMismatch,
                             "neither end declares a ValueKind");
    }
    else if (producedKind != expectedKind)
    {
        findings.error(Diagnostic::ValueKindMismatch,
                       std::string("provider values are ") + name(producedKind) +
                           ", consumer expects " + name(expectedKind));
    }

    // What the quantity is (ABI 5's vocabulary binding). Matched by concept
    // URI when both declare one, otherwise by CF standard name; a difference
    // is only a warning, and an undeclared name says nothing.
    const std::string producedURI = produced->conceptURI();
    const std::string expectedURI = expected->conceptURI();
    if (!producedURI.empty() && !expectedURI.empty())
    {
        if (producedURI != expectedURI)
            findings.warning(Diagnostic::QuantityNameMismatch,
                             "provider measures <" + producedURI +
                                 ">, consumer expects <" + expectedURI + ">");
    }
    else
    {
        const std::string producedName = produced->standardName();
        const std::string expectedName = expected->standardName();
        if (!producedName.empty() && !expectedName.empty() &&
            producedName != expectedName)
            findings.warning(Diagnostic::QuantityNameMismatch,
                             "provider measures '" + producedName +
                                 "', consumer expects '" + expectedName + "'");
    }

    const auto *producedQuantity = dynamic_cast<HydroCouple::IQuantity *>(produced);
    const auto *expectedQuantity = dynamic_cast<HydroCouple::IQuantity *>(expected);
    const auto *producedQuality  = dynamic_cast<HydroCouple::IQuality *>(produced);
    const auto *expectedQuality  = dynamic_cast<HydroCouple::IQuality *>(expected);

    if ((producedQuantity && expectedQuality) || (producedQuality && expectedQuantity))
    {
        findings.error(Diagnostic::ValueDefinitionMismatch,
                       std::string("provider is a ") +
                           (producedQuantity ? "quantity" : "quality") +
                           ", consumer a " + (expectedQuantity ? "quantity" : "quality"));
        return;
    }

    if (producedQuality && expectedQuality)
    {
        if (producedQuality->categories() != expectedQuality->categories())
            findings.error(Diagnostic::ValueDefinitionMismatch,
                           "the qualities' category lists differ, so category index k "
                           "names different things on the two ends");
        return;
    }

    if (!producedQuantity || !expectedQuantity)
        return;

    HydroCouple::IUnit *producedUnit = producedQuantity->unit();
    HydroCouple::IUnit *expectedUnit = expectedQuantity->unit();
    if (!producedUnit || !expectedUnit)
    {
        findings.warning(Diagnostic::UnitMismatch,
                         std::string(!producedUnit ? "provider" : "consumer") +
                             " quantity declares no unit");
        return;
    }

    HydroCouple::IUnitDimensions *producedDims = producedUnit->dimensions();
    HydroCouple::IUnitDimensions *expectedDims = expectedUnit->dimensions();
    if (!producedDims || !expectedDims)
    {
        findings.warning(Diagnostic::UnitMismatch, "a unit declares no dimensions");
        return;
    }

    if (!samePowers(*producedDims, *expectedDims))
    {
        // A dimensionless unit is also what a placeholder looks like, so a
        // dimensionless/dimensioned pair is reported as a missing
        // declaration; two dimensioned units that differ are wrong.
        const std::string what = "units differ in dimension (" + powersOf(*producedDims) +
                                 " vs " + powersOf(*expectedDims) + ")";
        if (isDimensionless(*producedDims) || isDimensionless(*expectedDims))
            findings.warning(Diagnostic::UnitMismatch,
                             what + "; one side may simply not have declared its unit");
        else
            findings.error(Diagnostic::UnitMismatch, what);
        return;
    }

    if (!closeRelative(producedUnit->conversionFactorToSI(),
                       expectedUnit->conversionFactorToSI()) ||
        !closeRelative(producedUnit->offsetToSI(), expectedUnit->offsetToSI()))
    {
        findings.error(Diagnostic::UnitMismatch,
                       "units share a dimension but not a scale ('" +
                           producedUnit->caption() + "' vs '" + expectedUnit->caption() +
                           "'); wire a unit-conversion adapter between them");
    }
}

// ── Temporal semantics ────────────────────────────────────────────────────

void checkTemporal(const HydroCouple::IOutput &provider,
                   const HydroCouple::IInput &consumer, Findings &findings)
{
    const auto *produced = dynamic_cast<const Temporal::ITimeSeriesComponentDataItem *>(&provider);
    const auto *expected = dynamic_cast<const Temporal::ITimeSeriesComponentDataItem *>(&consumer);
    if (!produced || !expected)
        return; // A non-temporal end takes current values; nothing to compare.

    // TimeKind: what a timestamp means.
    const Temporal::TimeKind producedKind = produced->timeKind();
    const Temporal::TimeKind expectedKind = expected->timeKind();
    if (producedKind == Temporal::TimeKind::Unknown ||
        expectedKind == Temporal::TimeKind::Unknown)
    {
        findings.warning(Diagnostic::TimeKindMismatch,
                         producedKind == expectedKind
                             ? std::string("neither end declares what its timestamps mean "
                                           "(TimeKind)")
                             : std::string(producedKind == Temporal::TimeKind::Unknown
                                               ? "provider"
                                               : "consumer") +
                                   " declares no TimeKind; the consumer must not assume "
                                   "Instantaneous");
    }
    else if (producedKind != expectedKind)
    {
        findings.error(Diagnostic::TimeKindMismatch,
                       std::string("provider timestamps are ") + name(producedKind) +
                           ", consumer expects " + name(expectedKind));
    }
    else if (producedKind != Temporal::TimeKind::Instantaneous)
    {
        const double a = produced->intervalLength();
        const double b = expected->intervalLength();
        if (a > 0.0 && b > 0.0 && !closeRelative(a, b))
            findings.error(Diagnostic::TimeKindMismatch,
                           "both are " + std::string(name(producedKind)) +
                               " but over different intervals (" + std::to_string(a) +
                               " vs " + std::to_string(b) + " days)");
    }

    // Interpolation: the provider's behaviour must be one the consumer
    // accepts. A provider that never fabricates (None) is acceptable to any
    // consumer; otherwise the methods must match. Both undeclared is an
    // error by the interface's own rule (TimeInterpolation::Unknown).
    const Temporal::TimeInterpolation does    = produced->timeInterpolation();
    const Temporal::TimeInterpolation accepts = expected->timeInterpolation();
    if (does == Temporal::TimeInterpolation::Unknown &&
        accepts == Temporal::TimeInterpolation::Unknown)
        findings.error(Diagnostic::TemporalPolicyMismatch,
                       "neither end declares a time interpolation policy");
    else if (does == Temporal::TimeInterpolation::Unknown ||
             accepts == Temporal::TimeInterpolation::Unknown)
        findings.warning(Diagnostic::TemporalPolicyMismatch,
                         std::string(does == Temporal::TimeInterpolation::Unknown
                                         ? "provider"
                                         : "consumer") +
                             " declares no time interpolation policy");
    else if (does != Temporal::TimeInterpolation::None && does != accepts)
        findings.error(Diagnostic::TemporalPolicyMismatch,
                       std::string("provider interpolates ") + name(does) +
                           " between its instants; consumer accepts " + name(accepts));

    const Temporal::TimeExtrapolation beyond   = produced->timeExtrapolation();
    const Temporal::TimeExtrapolation tolerate = expected->timeExtrapolation();
    if (beyond == Temporal::TimeExtrapolation::Unknown &&
        tolerate == Temporal::TimeExtrapolation::Unknown)
        findings.error(Diagnostic::TemporalPolicyMismatch,
                       "neither end declares a time extrapolation policy");
    else if (beyond == Temporal::TimeExtrapolation::Unknown ||
             tolerate == Temporal::TimeExtrapolation::Unknown)
        findings.warning(Diagnostic::TemporalPolicyMismatch,
                         std::string(beyond == Temporal::TimeExtrapolation::Unknown
                                         ? "provider"
                                         : "consumer") +
                             " declares no time extrapolation policy");
    else if (beyond != Temporal::TimeExtrapolation::Refuse && beyond != tolerate)
        findings.error(Diagnostic::TemporalPolicyMismatch,
                       std::string("provider extrapolates ") + name(beyond) +
                           " beyond its range; consumer accepts " + name(tolerate));
}

// ── Reference systems ─────────────────────────────────────────────────────

const Spatial::ISpatialReferenceSystem *referenceOf(const HydroCouple::IComponentDataItem &item)
{
    if (const auto *geometries = dynamic_cast<const Spatial::IGeometryComponentDataItem *>(&item))
    {
        if (geometries->geometryCount() > 0)
            if (Spatial::IGeometry *first = geometries->geometry(0))
                return first->spatialReferenceSystem();
        return nullptr;
    }
    if (const auto *surface = dynamic_cast<const Spatial::IPolyhedralSurfaceComponentDataItem *>(&item))
        return surface->polyhedralSurface() ? surface->polyhedralSurface()->spatialReferenceSystem()
                                            : nullptr;
    if (const auto *network = dynamic_cast<const Spatial::INetworkComponentDataItem *>(&item))
    {
        Spatial::INetwork *graph = network->network();
        if (graph && graph->vertexCount() > 0)
            if (Spatial::IVertex *vertex = graph->vertex(0))
                return vertex->spatialReferenceSystem();
        return nullptr;
    }
    if (const auto *raster = dynamic_cast<const Spatial::IRasterComponentDataItem *>(&item))
        return raster->raster() ? raster->raster()->spatialReferenceSystem() : nullptr;
    if (const auto *grid = dynamic_cast<const Spatial::IRegularGrid2DComponentDataItem *>(&item))
        return grid->grid() ? grid->grid()->spatialReferenceSystem() : nullptr;
    if (const auto *grid = dynamic_cast<const Spatial::IRegularGrid3DComponentDataItem *>(&item))
        return grid->grid() ? grid->grid()->spatialReferenceSystem() : nullptr;
    return nullptr;
}

std::string label(const std::string &authority, int code)
{
    return authority + ":" + std::to_string(code);
}

void checkReferenceSystems(const HydroCouple::IOutput &provider,
                           const HydroCouple::IInput &consumer, Findings &findings)
{
    const Spatial::ISpatialReferenceSystem *produced = referenceOf(provider);
    const Spatial::ISpatialReferenceSystem *expected = referenceOf(consumer);
    if (!produced || !expected)
        return;

    // Horizontal: two declared systems that differ need a reprojection.
    const bool producedDeclared = produced->authSRID() != 0 && !produced->authName().empty();
    const bool expectedDeclared = expected->authSRID() != 0 && !expected->authName().empty();
    if (producedDeclared && expectedDeclared &&
        (produced->authSRID() != expected->authSRID() ||
         produced->authName() != expected->authName()))
        findings.error(Diagnostic::HorizontalReferenceMismatch,
                       "horizontal reference systems differ (" +
                           label(produced->authName(), produced->authSRID()) + " vs " +
                           label(expected->authName(), expected->authSRID()) +
                           "); wire a reprojecting adapter between them");

    // Vertical: NAVD88 and NGVD29 are ~0.3 m apart across much of the US,
    // and elevations coupled across them are wrong without any error.
    const bool producedVertical =
        produced->verticalAuthSRID() != 0 && !produced->verticalAuthName().empty();
    const bool expectedVertical =
        expected->verticalAuthSRID() != 0 && !expected->verticalAuthName().empty();
    if (producedVertical && expectedVertical)
    {
        if (produced->verticalAuthSRID() != expected->verticalAuthSRID() ||
            produced->verticalAuthName() != expected->verticalAuthName())
            findings.error(Diagnostic::VerticalReferenceMismatch,
                           "vertical datums differ (" +
                               label(produced->verticalAuthName(), produced->verticalAuthSRID()) +
                               " vs " +
                               label(expected->verticalAuthName(), expected->verticalAuthSRID()) +
                               "); elevations need a datum shift");
        else if (produced->verticalDistanceUnits() != expected->verticalDistanceUnits() &&
                 produced->verticalDistanceUnits() != HydroCouple::IUnit::DistanceUnits::Unknown &&
                 expected->verticalDistanceUnits() != HydroCouple::IUnit::DistanceUnits::Unknown)
            findings.error(Diagnostic::VerticalReferenceMismatch,
                           "same vertical datum, different vertical units");
    }
    else if (producedVertical != expectedVertical)
    {
        findings.warning(Diagnostic::VerticalReferenceMismatch,
                         std::string(producedVertical ? "consumer" : "provider") +
                             " declares no vertical datum while the other does; "
                             "elevations cannot be checked");
    }
}

// ── Entities, vector structure, layering ──────────────────────────────────

void compareEntityStructure(Spatial::MeshLocation producedAt, Spatial::MeshLocation expectedAt,
                            Spatial::SpatialDataType producedType,
                            Spatial::SpatialDataType expectedType,
                            Spatial::VectorBasis producedBasis,
                            Spatial::VectorBasis expectedBasis, Findings &findings)
{
    if (producedAt != expectedAt)
        findings.error(Diagnostic::MeshLocationMismatch,
                       std::string("provider values sit on ") + name(producedAt) +
                           ", consumer expects them on " + name(expectedAt));

    if (producedType != expectedType)
    {
        findings.error(Diagnostic::VectorBasisMismatch,
                       std::string("provider values are ") + name(producedType) +
                           ", consumer expects " + name(expectedType));
        return;
    }

    if (producedType != Spatial::SpatialDataType::Vector &&
        producedType != Spatial::SpatialDataType::Tensor)
        return;

    if (producedBasis == Spatial::VectorBasis::Unknown ||
        expectedBasis == Spatial::VectorBasis::Unknown)
        findings.warning(Diagnostic::VectorBasisMismatch,
                         std::string(producedBasis == Spatial::VectorBasis::Unknown
                                         ? "provider"
                                         : "consumer") +
                             " declares no vector basis for " + name(producedType) +
                             " values");
    else if (producedBasis != expectedBasis)
        findings.error(Diagnostic::VectorBasisMismatch,
                       std::string("vector bases differ (") + name(producedBasis) + " vs " +
                           name(expectedBasis) + "); two numbers per entity mean "
                           "different things on the two ends");
}

void checkEntities(const HydroCouple::IOutput &provider,
                   const HydroCouple::IInput &consumer, Findings &findings)
{
    if (const auto *producedSurface =
            dynamic_cast<const Spatial::IPolyhedralSurfaceComponentDataItem *>(&provider))
        if (const auto *expectedSurface =
                dynamic_cast<const Spatial::IPolyhedralSurfaceComponentDataItem *>(&consumer))
            compareEntityStructure(producedSurface->location(), expectedSurface->location(),
                                   producedSurface->meshDataType(), expectedSurface->meshDataType(),
                                   producedSurface->vectorBasis(), expectedSurface->vectorBasis(),
                                   findings);

    if (const auto *producedNetwork =
            dynamic_cast<const Spatial::INetworkComponentDataItem *>(&provider))
        if (const auto *expectedNetwork =
                dynamic_cast<const Spatial::INetworkComponentDataItem *>(&consumer))
            compareEntityStructure(producedNetwork->location(), expectedNetwork->location(),
                                   producedNetwork->networkDataType(),
                                   expectedNetwork->networkDataType(),
                                   producedNetwork->vectorBasis(), expectedNetwork->vectorBasis(),
                                   findings);

    if (const auto *producedGrid =
            dynamic_cast<const Spatial::IRegularGrid2DComponentDataItem *>(&provider))
        if (const auto *expectedGrid =
                dynamic_cast<const Spatial::IRegularGrid2DComponentDataItem *>(&consumer))
            if (producedGrid->location() != expectedGrid->location())
                findings.error(Diagnostic::MeshLocationMismatch,
                               std::string("provider grid values sit on ") +
                                   name(producedGrid->location()) + ", consumer expects " +
                                   name(expectedGrid->location()));

    if (const auto *producedGrid =
            dynamic_cast<const Spatial::IRegularGrid3DComponentDataItem *>(&provider))
        if (const auto *expectedGrid =
                dynamic_cast<const Spatial::IRegularGrid3DComponentDataItem *>(&consumer))
            if (producedGrid->location() != expectedGrid->location())
                findings.error(Diagnostic::MeshLocationMismatch,
                               std::string("provider grid values sit on ") +
                                   name(producedGrid->location()) + ", consumer expects " +
                                   name(expectedGrid->location()));
}

void checkLayering(const HydroCouple::IOutput &provider,
                   const HydroCouple::IInput &consumer, Findings &findings)
{
    const auto *produced = dynamic_cast<const Spatial::ILayering *>(&provider);
    const auto *expected = dynamic_cast<const Spatial::ILayering *>(&consumer);
    if (!produced || !expected)
        return;

    Spatial::IVerticalCoordinate *producedVertical = produced->verticalCoordinate();
    Spatial::IVerticalCoordinate *expectedVertical = expected->verticalCoordinate();
    if (!producedVertical || !expectedVertical)
    {
        findings.error(Diagnostic::LayeringMismatch,
                       std::string(!producedVertical ? "provider" : "consumer") +
                           " is layered but has no vertical coordinate (ILayering "
                           "requires one)");
        return;
    }

    const Spatial::VerticalCoordinateKind a = producedVertical->kind();
    const Spatial::VerticalCoordinateKind b = expectedVertical->kind();
    if (a == Spatial::VerticalCoordinateKind::Unknown ||
        b == Spatial::VerticalCoordinateKind::Unknown)
        findings.warning(Diagnostic::LayeringMismatch,
                         std::string(a == Spatial::VerticalCoordinateKind::Unknown
                                         ? "provider"
                                         : "consumer") +
                             " declares no vertical coordinate kind");
    else if (a != b)
        findings.error(Diagnostic::LayeringMismatch,
                       std::string("layer k means different things (") + name(a) + " vs " +
                           name(b) + "); wire a vertical remapping adapter between them");

    if (producedVertical->layerCount() != expectedVertical->layerCount())
        findings.error(Diagnostic::LayeringMismatch,
                       "layer counts differ (" + std::to_string(producedVertical->layerCount()) +
                           " vs " + std::to_string(expectedVertical->layerCount()) + ")");
}

// ── Dimension roles ───────────────────────────────────────────────────────

void checkDimensionRoles(const HydroCouple::IOutput &provider,
                         const HydroCouple::IInput &consumer, Findings &findings)
{
    using Role = HydroCouple::IDimension::DimensionRole;
    const std::vector<HydroCouple::IDimension *> produced = provider.dimensions();
    const std::vector<HydroCouple::IDimension *> expected = consumer.dimensions();
    if (produced.size() != expected.size())
        return; // Rank is canConsume()'s question, and adapters change it.

    for (size_t axis = 0; axis < produced.size(); ++axis)
    {
        if (!produced[axis] || !expected[axis])
            continue;
        const Role a = produced[axis]->role();
        const Role b = expected[axis]->role();
        if (a == Role::Unknown || b == Role::Unknown || a == Role::Other || b == Role::Other)
            continue;
        if (a != b)
            findings.error(Diagnostic::DimensionRoleMismatch,
                           "axis " + std::to_string(axis) + " is " + name(a) +
                               " on the provider and " + name(b) + " on the consumer");
    }
}

} // namespace

std::vector<ConnectionFinding> validateConnection(const HydroCouple::IOutput &provider,
                                                  const HydroCouple::IInput &consumer)
{
    Findings findings(provider, consumer);
    checkDataKind(provider, consumer, findings);
    checkValueDefinitions(provider, consumer, findings);
    checkTemporal(provider, consumer, findings);
    checkReferenceSystems(provider, consumer, findings);
    checkEntities(provider, consumer, findings);
    checkLayering(provider, consumer, findings);
    checkDimensionRoles(provider, consumer, findings);
    return findings.take();
}

bool hasConnectionError(const std::vector<ConnectionFinding> &findings)
{
    return std::any_of(findings.begin(), findings.end(),
                       [](const ConnectionFinding &finding)
                       { return finding.severity == HydroCouple::ErrorEntry::Severity::Error; });
}

} // namespace HydroCouple::SDK
