// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the SDK's enforcement of what the interface standard lets items
// and components declare:
//
//  - validateConnection(): each check fires as an Error when both ends declare
//    and contradict each other, as a Warning when a declaration is missing,
//    and stays silent on an honest, matching link;
//  - AbstractWorkflowComponent::validate(): link Errors fail the workflow and
//    reach errors() with their Diagnostic code; link Warnings reach errors()
//    and fail nothing;
//  - setStatus(): a transition the normative table rejects is performed but
//    reported, once per distinct (from, to) pair, as
//    Diagnostic::IllegalStatusTransition.

#include <gtest/gtest.h>

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/component/abstractworkflowcomponent.h"
#include "hydrocouplesdk/component/connectionvalidator.h"
#include "hydrocouplesdk/core/diagnosticcodes.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/unit.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/spatial/layeredmeshexchangeitems.h"
#include "hydrocouplesdk/spatial/meshadapters.h"
#include "hydrocouplesdk/temporal/timeseriesexchangeitems.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <span>
#include <string>
#include <tuple>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;

namespace {

namespace HCS         = HydroCouple::Spatial;
namespace HCT         = HydroCouple::Temporal;
namespace SdkSpatial  = HydroCouple::SDK::Spatial;
namespace SdkTemporal = HydroCouple::SDK::Temporal;
using Severity = ErrorEntry::Severity;
using CStatus  = IModelComponent::ComponentStatus;
using WStatus  = IWorkflowComponent::WorkflowStatus;

// ── Finding helpers ───────────────────────────────────────────────────────

int countOf(const std::vector<ConnectionFinding> &findings, int32_t code, Severity severity)
{
    return static_cast<int>(std::count_if(findings.begin(), findings.end(),
                                          [&](const ConnectionFinding &finding)
                                          {
                                              return finding.code == code &&
                                                     finding.severity == severity;
                                          }));
}

bool has(const std::vector<ConnectionFinding> &findings, int32_t code, Severity severity)
{
    return countOf(findings, code, severity) > 0;
}

std::string dump(const std::vector<ConnectionFinding> &findings)
{
    std::string text;
    for (const auto &finding : findings)
        text += "  [" + std::to_string(static_cast<int>(finding.severity)) + "] " +
                std::to_string(finding.code) + " " + finding.message + "\n";
    return text;
}

// ── 1-D items over explicit value definitions ─────────────────────────────

//! Owns a unit and a quantity over it, since Quantity borrows its unit.
struct OwnedQuantity
{
    explicit OwnedQuantity(Unit *unitIn, ValueKind kind = ValueKind::Intensive)
        : unit(unitIn), quantity(std::make_unique<Quantity>("q", typeid(double), unitIn))
    {
        quantity->setValueKind(kind);
    }

    std::unique_ptr<Unit>     unit;
    std::unique_ptr<Quantity> quantity;
};

struct Link1D
{
    Link1D(ValueDefinition *produced, ValueDefinition *expected)
        : out("out", &outDim, 3, produced, nullptr), in("in", &inDim, 3, expected, nullptr)
    {
    }

    Dimension      outDim{"i", "Index"};
    Dimension      inDim{"j", "Index"};
    Output1DDouble out;
    Input1DDouble  in;

    std::vector<ConnectionFinding> check() const { return validateConnection(out, in); }
};

// ── Spatial test doubles ──────────────────────────────────────────────────

class TestReferenceSystem final : public HCS::ISpatialReferenceSystem
{
public:
    TestReferenceSystem(std::string auth, int srid, std::string verticalAuth = {},
                        int verticalSrid = 0,
                        IUnit::DistanceUnits verticalUnits = IUnit::DistanceUnits::Meters)
        : m_auth(std::move(auth)), m_srid(srid), m_verticalAuth(std::move(verticalAuth)),
          m_verticalSrid(verticalSrid), m_verticalUnits(verticalUnits)
    {
    }

    [[nodiscard]] int authSRID() const override { return m_srid; }
    [[nodiscard]] const std::string &authName() const override { return m_auth; }
    [[nodiscard]] const std::string &srText() const override { return m_empty; }
    [[nodiscard]] IUnit::DistanceUnits distanceUnits() const override
    {
        return IUnit::DistanceUnits::Meters;
    }
    [[nodiscard]] const std::string &verticalAuthName() const override { return m_verticalAuth; }
    [[nodiscard]] int verticalAuthSRID() const override { return m_verticalSrid; }
    [[nodiscard]] const std::string &verticalSrText() const override { return m_empty; }
    [[nodiscard]] IUnit::DistanceUnits verticalDistanceUnits() const override
    {
        return m_verticalUnits;
    }

private:
    std::string          m_auth;
    int                  m_srid;
    std::string          m_verticalAuth;
    int                  m_verticalSrid;
    IUnit::DistanceUnits m_verticalUnits;
    std::string          m_empty;
};

class TestVerticalCoordinate final : public HCS::IVerticalCoordinate
{
public:
    TestVerticalCoordinate(HCS::VerticalCoordinateKind kind, int64_t layers, int64_t columns)
        : m_kind(kind), m_layers(layers), m_columns(columns),
          m_profile(static_cast<size_t>(columns * (layers + 1)), 0.0)
    {
    }

    [[nodiscard]] HCS::VerticalCoordinateKind kind() const override { return m_kind; }
    [[nodiscard]] int64_t layerCount() const override { return m_layers; }
    [[nodiscard]] int64_t columnCount() const override { return m_columns; }
    [[nodiscard]] bool isTimeVarying() const override { return false; }
    [[nodiscard]] uint64_t geometryEpoch() const override { return 0; }
    [[nodiscard]] double interfaceElevation(int64_t, int64_t interfaceIndex) const override
    {
        return -static_cast<double>(interfaceIndex);
    }
    [[nodiscard]] std::span<const double> interfaceElevations() const override
    {
        return m_profile;
    }

private:
    HCS::VerticalCoordinateKind m_kind;
    int64_t                     m_layers;
    int64_t                     m_columns;
    std::vector<double>         m_profile;
};

constexpr int kCells  = 4;
constexpr int kLayers = 3;

//! Everything one end of a layered link is made of.
struct LayeredEnd
{
    explicit LayeredEnd(TestReferenceSystem srs,
                        HCS::VerticalCoordinateKind kind = HCS::VerticalCoordinateKind::Sigma,
                        int layers = kLayers)
        : reference(std::move(srs)),
          surface(SdkSpatial::PolyhedralSurface{}, &reference),
          vertical(kind, layers, kCells)
    {
    }

    TestReferenceSystem                reference;
    SdkSpatial::PolyhedralSurfaceAdapter surface;
    TestVerticalCoordinate             vertical;
};

TestReferenceSystem utm17NAVD88()
{
    return {"EPSG", 26917, "EPSG", 5703};
}

struct LayeredLink
{
    LayeredLink(LayeredEnd &producer, LayeredEnd &consumer,
                HCS::MeshLocation producedAt = HCS::MeshLocation::Volume,
                HCS::MeshLocation expectedAt = HCS::MeshLocation::Volume,
                HCS::SpatialDataType producedType = HCS::SpatialDataType::Scalar,
                HCS::SpatialDataType expectedType = HCS::SpatialDataType::Scalar)
        : quantity(Unit::lengthInMeters()),
          out("out", &producer.surface, &producer.vertical, producedAt, producedType, kCells,
              static_cast<int>(producer.vertical.layerCount()), quantity.quantity.get(), nullptr),
          in("in", &consumer.surface, &consumer.vertical, expectedAt, expectedType, kCells,
             static_cast<int>(consumer.vertical.layerCount()), quantity.quantity.get(), nullptr)
    {
    }

    OwnedQuantity                         quantity;
    SdkSpatial::LayeredMeshOutput<double> out;
    SdkSpatial::LayeredMeshInput<double>  in;

    std::vector<ConnectionFinding> check() const { return validateConnection(out, in); }
};

} // namespace

// ── A clean link ──────────────────────────────────────────────────────────

TEST(ConnectionValidatorTest, AnHonestMatchingLinkHasNoFindings)
{
    OwnedQuantity flow(Unit::flowInCMS());
    Link1D link(flow.quantity.get(), flow.quantity.get());
    const auto findings = link.check();
    EXPECT_TRUE(findings.empty()) << dump(findings);
    EXPECT_FALSE(hasConnectionError(findings));
}

TEST(ConnectionValidatorTest, EveryMessageNamesTheLink)
{
    OwnedQuantity flow(Unit::flowInCMS());
    Link1D link(flow.quantity.get(), nullptr);
    const auto findings = link.check();
    ASSERT_FALSE(findings.empty());
    for (const auto &finding : findings)
        EXPECT_EQ(finding.message.rfind("link 'out' -> 'in': ", 0), 0u) << finding.message;
}

// ── Data kind ─────────────────────────────────────────────────────────────

TEST(ConnectionValidatorTest, DifferentElementTypesAreAnError)
{
    OwnedQuantity flow(Unit::flowInCMS());
    Dimension a("i", "Index"), b("j", "Index");
    Output1DDouble out("out", &a, 3, flow.quantity.get(), nullptr);
    Input1DInt     in("in", &b, 3, flow.quantity.get(), nullptr);
    const auto findings = validateConnection(out, in);
    EXPECT_TRUE(has(findings, Diagnostic::DataKindMismatch, Severity::Error)) << dump(findings);
}

// ── Value definitions ─────────────────────────────────────────────────────

TEST(ConnectionValidatorTest, AMissingValueDefinitionIsAWarning)
{
    OwnedQuantity flow(Unit::flowInCMS());
    Link1D link(flow.quantity.get(), nullptr);
    const auto findings = link.check();
    EXPECT_TRUE(has(findings, Diagnostic::ValueDefinitionMismatch, Severity::Warning))
        << dump(findings);
    EXPECT_FALSE(hasConnectionError(findings)) << dump(findings);
}

TEST(ConnectionValidatorTest, ValueKindsThatDifferAreAnErrorAndMissingOnesAWarning)
{
    OwnedQuantity intensive(Unit::flowInCMS(), ValueKind::Intensive);
    OwnedQuantity extensive(Unit::flowInCMS(), ValueKind::Extensive);
    OwnedQuantity unknown(Unit::flowInCMS(), ValueKind::Unknown);

    EXPECT_TRUE(has(Link1D(intensive.quantity.get(), extensive.quantity.get()).check(),
                    Diagnostic::ValueKindMismatch, Severity::Error));

    const auto oneSided = Link1D(unknown.quantity.get(), extensive.quantity.get()).check();
    EXPECT_TRUE(has(oneSided, Diagnostic::ValueKindMismatch, Severity::Warning)) << dump(oneSided);
    EXPECT_FALSE(hasConnectionError(oneSided)) << dump(oneSided);

    const auto neither = Link1D(unknown.quantity.get(), unknown.quantity.get()).check();
    EXPECT_TRUE(has(neither, Diagnostic::ValueKindMismatch, Severity::Warning)) << dump(neither);
}

TEST(ConnectionValidatorTest, AQuantityConsumedAsAQualityIsAnError)
{
    OwnedQuantity flow(Unit::flowInCMS());
    Quality landUse("land use", {"water", "forest"});
    landUse.setValueKind(ValueKind::Intensive);
    EXPECT_TRUE(has(Link1D(flow.quantity.get(), &landUse).check(),
                    Diagnostic::ValueDefinitionMismatch, Severity::Error));
}

TEST(ConnectionValidatorTest, QualitiesWhoseCategoriesDifferAreAnError)
{
    Quality a("land use", {"water", "forest"});
    Quality b("land use", {"forest", "water"});
    Quality c("land use", {"water", "forest"});
    for (Quality *quality : {&a, &b, &c})
        quality->setValueKind(ValueKind::Intensive);

    // Same labels, different order: index 0 means water on one end and
    // forest on the other.
    EXPECT_TRUE(has(Link1D(&a, &b).check(), Diagnostic::ValueDefinitionMismatch, Severity::Error));
    EXPECT_TRUE(Link1D(&a, &c).check().empty());
}

TEST(ConnectionValidatorTest, UnitsOfOneDimensionButAnotherScaleAreAnError)
{
    OwnedQuantity cms(Unit::flowInCMS());
    OwnedQuantity cfs(Unit::flowInCFS());
    const auto findings = Link1D(cms.quantity.get(), cfs.quantity.get()).check();
    ASSERT_TRUE(has(findings, Diagnostic::UnitMismatch, Severity::Error)) << dump(findings);
    EXPECT_NE(findings.front().message.find("conversion"), std::string::npos)
        << findings.front().message;
}

TEST(ConnectionValidatorTest, UnitsOfDifferentDimensionsAreAnError)
{
    OwnedQuantity flow(Unit::flowInCMS());
    OwnedQuantity length(Unit::lengthInMeters());
    EXPECT_TRUE(has(Link1D(flow.quantity.get(), length.quantity.get()).check(),
                    Diagnostic::UnitMismatch, Severity::Error));
}

TEST(ConnectionValidatorTest, ADimensionlessUnitAgainstADimensionedOneIsAWarning)
{
    // Unit-less is also what an undeclared unit looks like.
    OwnedQuantity flow(Unit::flowInCMS());
    OwnedQuantity coefficient(Unit::unitlessCoefficient());
    const auto findings = Link1D(coefficient.quantity.get(), flow.quantity.get()).check();
    EXPECT_TRUE(has(findings, Diagnostic::UnitMismatch, Severity::Warning)) << dump(findings);
    EXPECT_FALSE(hasConnectionError(findings)) << dump(findings);
}

// ── Dimension roles ───────────────────────────────────────────────────────

// ── Quantity names (ABI 5 vocabulary binding) ─────────────────────────────

TEST(ConnectionValidatorTest, DifferentlyNamedQuantitiesAreOnlyAWarning)
{
    OwnedQuantity produced(Unit::flowInCMS(), ValueKind::Flux);
    OwnedQuantity expected(Unit::flowInCMS(), ValueKind::Flux);
    produced.quantity->setStandardName("water_volume_transport_in_river_channel");
    expected.quantity->setStandardName("surface_runoff_flux");
    Link1D link(produced.quantity.get(), expected.quantity.get());
    const auto findings = link.check();
    EXPECT_TRUE(has(findings, Diagnostic::QuantityNameMismatch, Severity::Warning))
        << dump(findings);
    EXPECT_FALSE(hasConnectionError(findings)) << dump(findings);
}

TEST(ConnectionValidatorTest, ConceptUrisDecideOverStandardNames)
{
    OwnedQuantity produced(Unit::flowInCMS(), ValueKind::Flux);
    OwnedQuantity expected(Unit::flowInCMS(), ValueKind::Flux);
    const char *uri = "http://qudt.org/vocab/quantitykind/VolumeFlowRate";
    produced.quantity->setConceptURI(uri);
    expected.quantity->setConceptURI(uri);
    produced.quantity->setStandardName("water_volume_transport_in_river_channel");
    expected.quantity->setStandardName("surface_runoff_flux");
    Link1D link(produced.quantity.get(), expected.quantity.get());
    EXPECT_EQ(countOf(link.check(), Diagnostic::QuantityNameMismatch, Severity::Warning), 0);
}

TEST(ConnectionValidatorTest, AnUndeclaredQuantityNameSaysNothing)
{
    OwnedQuantity produced(Unit::flowInCMS(), ValueKind::Flux);
    OwnedQuantity expected(Unit::flowInCMS(), ValueKind::Flux);
    produced.quantity->setStandardName("water_volume_transport_in_river_channel");
    Link1D link(produced.quantity.get(), expected.quantity.get());
    EXPECT_EQ(countOf(link.check(), Diagnostic::QuantityNameMismatch, Severity::Warning), 0);
}

TEST(ConnectionValidatorTest, AnAxisThatMeansDifferentThingsIsAnError)
{
    OwnedQuantity flow(Unit::flowInCMS());
    Link1D link(flow.quantity.get(), flow.quantity.get());
    link.outDim.setRole(IDimension::DimensionRole::Time);
    link.inDim.setRole(IDimension::DimensionRole::Entity);
    EXPECT_TRUE(has(link.check(), Diagnostic::DimensionRoleMismatch, Severity::Error));

    // Undeclared on either side is not a contradiction.
    link.inDim.setRole(IDimension::DimensionRole::Unknown);
    EXPECT_TRUE(link.check().empty()) << dump(link.check());
}

// ── Temporal semantics ────────────────────────────────────────────────────

namespace {

struct TemporalLink
{
    TemporalLink()
        : flow(Unit::flowInCMS()),
          out("out", {&outTime}, flow.quantity.get(), nullptr),
          in("in", {&inTime}, flow.quantity.get(), nullptr)
    {
        out.setTimeKind(HCT::TimeKind::Instantaneous);
        in.setTimeKind(HCT::TimeKind::Instantaneous);
    }

    OwnedQuantity                     flow;
    Dimension                         outTime{"time", "Time"};
    Dimension                         inTime{"time", "Time"};
    SdkTemporal::TimeSeriesOutputDouble out;
    SdkTemporal::TimeSeriesInputDouble  in;

    std::vector<ConnectionFinding> check() const { return validateConnection(out, in); }
};

} // namespace

TEST(ConnectionValidatorTest, SdkTimeSeriesItemsDeclareThatTheyFabricateNothing)
{
    TemporalLink link;
    EXPECT_EQ(link.out.timeInterpolation(), HCT::TimeInterpolation::None);
    EXPECT_EQ(link.out.timeExtrapolation(), HCT::TimeExtrapolation::Refuse);
    EXPECT_EQ(link.in.timeInterpolation(), HCT::TimeInterpolation::None);
    EXPECT_EQ(link.in.timeExtrapolation(), HCT::TimeExtrapolation::Refuse);
    EXPECT_TRUE(link.check().empty()) << dump(link.check());
}

TEST(ConnectionValidatorTest, MeansReadAsInstantsAreAnError)
{
    TemporalLink link;
    link.out.setTimeKind(HCT::TimeKind::IntervalMean, 1.0);
    EXPECT_TRUE(has(link.check(), Diagnostic::TimeKindMismatch, Severity::Error));
}

TEST(ConnectionValidatorTest, MeansOverDifferentIntervalsAreAnError)
{
    TemporalLink link;
    link.out.setTimeKind(HCT::TimeKind::IntervalMean, 1.0);
    link.in.setTimeKind(HCT::TimeKind::IntervalMean, 1.0 / 24.0);
    EXPECT_TRUE(has(link.check(), Diagnostic::TimeKindMismatch, Severity::Error));

    link.in.setTimeKind(HCT::TimeKind::IntervalMean, 1.0);
    EXPECT_TRUE(link.check().empty()) << dump(link.check());
}

TEST(ConnectionValidatorTest, AnUndeclaredTimeKindIsAWarning)
{
    TemporalLink link;
    link.out.setTimeKind(HCT::TimeKind::Unknown);
    const auto findings = link.check();
    EXPECT_TRUE(has(findings, Diagnostic::TimeKindMismatch, Severity::Warning)) << dump(findings);
    EXPECT_FALSE(hasConnectionError(findings)) << dump(findings);
}

TEST(ConnectionValidatorTest, InterpolationTheConsumerDoesNotAcceptIsAnError)
{
    TemporalLink link;
    link.out.setTimeInterpolation(HCT::TimeInterpolation::Linear);
    link.in.setTimeInterpolation(HCT::TimeInterpolation::Previous);
    EXPECT_TRUE(has(link.check(), Diagnostic::TemporalPolicyMismatch, Severity::Error));

    // A consumer declaring Linear accepts Linear...
    link.in.setTimeInterpolation(HCT::TimeInterpolation::Linear);
    EXPECT_TRUE(link.check().empty()) << dump(link.check());

    // ...and None, from a provider that never fabricates.
    link.out.setTimeInterpolation(HCT::TimeInterpolation::None);
    EXPECT_TRUE(link.check().empty()) << dump(link.check());
}

TEST(ConnectionValidatorTest, ExtrapolationTheConsumerDoesNotAcceptIsAnError)
{
    TemporalLink link;
    link.out.setTimeExtrapolation(HCT::TimeExtrapolation::HoldLast);
    EXPECT_TRUE(has(link.check(), Diagnostic::TemporalPolicyMismatch, Severity::Error));

    link.in.setTimeExtrapolation(HCT::TimeExtrapolation::HoldLast);
    EXPECT_TRUE(link.check().empty()) << dump(link.check());
}

TEST(ConnectionValidatorTest, BothEndsUndeclaredIsAnErrorByTheStandardsOwnRule)
{
    TemporalLink link;
    link.out.setTimeInterpolation(HCT::TimeInterpolation::Unknown);
    link.in.setTimeInterpolation(HCT::TimeInterpolation::Unknown);
    link.out.setTimeExtrapolation(HCT::TimeExtrapolation::Unknown);
    link.in.setTimeExtrapolation(HCT::TimeExtrapolation::Unknown);
    EXPECT_EQ(countOf(link.check(), Diagnostic::TemporalPolicyMismatch, Severity::Error), 2)
        << dump(link.check());
}

TEST(ConnectionValidatorTest, OneEndUndeclaredIsAWarning)
{
    TemporalLink link;
    link.in.setTimeInterpolation(HCT::TimeInterpolation::Unknown);
    const auto findings = link.check();
    EXPECT_TRUE(has(findings, Diagnostic::TemporalPolicyMismatch, Severity::Warning))
        << dump(findings);
    EXPECT_FALSE(hasConnectionError(findings)) << dump(findings);
}

// ── Reference systems, entities, layering (over layered mesh items) ───────

TEST(ConnectionValidatorTest, MatchingLayeredItemsHaveNoFindings)
{
    LayeredEnd producer(utm17NAVD88()), consumer(utm17NAVD88());
    LayeredLink link(producer, consumer);
    EXPECT_TRUE(link.check().empty()) << dump(link.check());
}

TEST(ConnectionValidatorTest, LayeredItemsDeclareWhatTheirAxesMean)
{
    LayeredEnd end(utm17NAVD88());
    LayeredLink link(end, end);
    const auto dimensions = link.out.dimensions();
    ASSERT_EQ(dimensions.size(), 2u);
    EXPECT_EQ(dimensions[0]->role(), IDimension::DimensionRole::Entity);
    EXPECT_EQ(dimensions[1]->role(), IDimension::DimensionRole::Layer);
    EXPECT_EQ(link.out.layerDimension(), dimensions[1]);
    EXPECT_EQ(link.out.entityDimension(), dimensions[0]);
}

TEST(ConnectionValidatorTest, DifferentHorizontalSystemsAreAnError)
{
    LayeredEnd producer(utm17NAVD88());
    LayeredEnd consumer(TestReferenceSystem{"EPSG", 4326, "EPSG", 5703});
    EXPECT_TRUE(has(LayeredLink(producer, consumer).check(),
                    Diagnostic::HorizontalReferenceMismatch, Severity::Error));
}

TEST(ConnectionValidatorTest, DifferentVerticalDatumsAreAnError)
{
    // NAVD88 against NGVD29: about a foot apart over much of the US.
    LayeredEnd producer(utm17NAVD88());
    LayeredEnd consumer(TestReferenceSystem{"EPSG", 26917, "EPSG", 5702});
    const auto findings = LayeredLink(producer, consumer).check();
    EXPECT_TRUE(has(findings, Diagnostic::VerticalReferenceMismatch, Severity::Error))
        << dump(findings);
}

TEST(ConnectionValidatorTest, OneDatumInDifferentVerticalUnitsIsAnError)
{
    LayeredEnd producer(utm17NAVD88());
    LayeredEnd consumer(
        TestReferenceSystem{"EPSG", 26917, "EPSG", 5703, IUnit::DistanceUnits::Feet});
    EXPECT_TRUE(has(LayeredLink(producer, consumer).check(),
                    Diagnostic::VerticalReferenceMismatch, Severity::Error));
}

TEST(ConnectionValidatorTest, AVerticalDatumOnOneSideOnlyIsAWarning)
{
    LayeredEnd producer(utm17NAVD88());
    LayeredEnd consumer(TestReferenceSystem{"EPSG", 26917});
    const auto findings = LayeredLink(producer, consumer).check();
    EXPECT_TRUE(has(findings, Diagnostic::VerticalReferenceMismatch, Severity::Warning))
        << dump(findings);
    EXPECT_FALSE(hasConnectionError(findings)) << dump(findings);
}

TEST(ConnectionValidatorTest, ValuesOnDifferentEntitiesAreAnError)
{
    LayeredEnd producer(utm17NAVD88()), consumer(utm17NAVD88());
    EXPECT_TRUE(has(LayeredLink(producer, consumer, HCS::MeshLocation::Volume,
                                HCS::MeshLocation::Face)
                        .check(),
                    Diagnostic::MeshLocationMismatch, Severity::Error));
}

TEST(ConnectionValidatorTest, VectorsReadAsScalarsAreAnError)
{
    LayeredEnd producer(utm17NAVD88()), consumer(utm17NAVD88());
    EXPECT_TRUE(has(LayeredLink(producer, consumer, HCS::MeshLocation::Volume,
                                HCS::MeshLocation::Volume, HCS::SpatialDataType::Vector,
                                HCS::SpatialDataType::Scalar)
                        .check(),
                    Diagnostic::VectorBasisMismatch, Severity::Error));
}

TEST(ConnectionValidatorTest, VectorsInDifferentBasesAreAnErrorAndAnUndeclaredBasisAWarning)
{
    LayeredEnd producer(utm17NAVD88()), consumer(utm17NAVD88());
    LayeredLink link(producer, consumer, HCS::MeshLocation::Face, HCS::MeshLocation::Face,
                     HCS::SpatialDataType::Vector, HCS::SpatialDataType::Vector);

    link.out.setVectorBasis(HCS::VectorBasis::EastNorthUp);
    const auto undeclared = link.check();
    EXPECT_TRUE(has(undeclared, Diagnostic::VectorBasisMismatch, Severity::Warning))
        << dump(undeclared);
    EXPECT_FALSE(hasConnectionError(undeclared)) << dump(undeclared);

    link.in.setVectorBasis(HCS::VectorBasis::NormalTangential);
    EXPECT_TRUE(has(link.check(), Diagnostic::VectorBasisMismatch, Severity::Error));

    link.in.setVectorBasis(HCS::VectorBasis::EastNorthUp);
    EXPECT_TRUE(link.check().empty()) << dump(link.check());
}

TEST(ConnectionValidatorTest, LayersThatMeanDifferentThingsAreAnError)
{
    LayeredEnd sigma(utm17NAVD88(), HCS::VerticalCoordinateKind::Sigma);
    LayeredEnd zLevel(utm17NAVD88(), HCS::VerticalCoordinateKind::ZLevel);
    const auto findings = LayeredLink(sigma, zLevel).check();
    ASSERT_TRUE(has(findings, Diagnostic::LayeringMismatch, Severity::Error)) << dump(findings);
}

TEST(ConnectionValidatorTest, DifferentLayerCountsAreAnError)
{
    LayeredEnd three(utm17NAVD88(), HCS::VerticalCoordinateKind::Sigma, 3);
    LayeredEnd five(utm17NAVD88(), HCS::VerticalCoordinateKind::Sigma, 5);
    EXPECT_TRUE(has(LayeredLink(three, five).check(), Diagnostic::LayeringMismatch,
                    Severity::Error));
}

TEST(ConnectionValidatorTest, AnUndeclaredVerticalCoordinateKindIsAWarning)
{
    LayeredEnd declared(utm17NAVD88(), HCS::VerticalCoordinateKind::Sigma);
    LayeredEnd undeclared(utm17NAVD88(), HCS::VerticalCoordinateKind::Unknown);
    const auto findings = LayeredLink(declared, undeclared).check();
    EXPECT_TRUE(has(findings, Diagnostic::LayeringMismatch, Severity::Warning)) << dump(findings);
    EXPECT_FALSE(hasConnectionError(findings)) << dump(findings);
}

// ── Workflow validate() ───────────────────────────────────────────────────

namespace {

//! One output and one input over value definitions the test chooses.
class PortComponent : public AbstractModelComponent
{
public:
    PortComponent(std::string_view id, ValueDefinition *produced, ValueDefinition *expected)
        : AbstractModelComponent(id), m_produced(produced), m_expected(expected)
    {
    }

    std::vector<std::string> validate() override
    {
        setStatus(CStatus::Validating);
        setStatus(CStatus::Valid);
        return {};
    }
    void prepare() override
    {
        setStatus(CStatus::Preparing);
        setStatus(CStatus::Updated);
    }
    void update(const std::vector<IOutput *> & = {}) override
    {
        setStatus(CStatus::Updating);
        setStatus(CStatus::Done);
    }
    void finish() override
    {
        setStatus(CStatus::Finishing);
        setStatus(CStatus::Finished);
    }

    // Exposed so the lifecycle tests can make transitions a real component
    // would only make by mistake.
    using AbstractModelComponent::setStatus;

    Input1DDouble  *in  = nullptr;
    Output1DDouble *out = nullptr;

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        in = new Input1DDouble("in", &m_dim, 1, m_expected, this);
        addInput(in);
    }
    void createOutputs() override
    {
        out = new Output1DDouble("out", &m_dim, 1, m_produced, this);
        addOutput(out);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension        m_dim{"i", "Index"};
    ValueDefinition *m_produced;
    ValueDefinition *m_expected;
};

class PlainWorkflow : public AbstractWorkflowComponent
{
public:
    using AbstractWorkflowComponent::AbstractWorkflowComponent;
    using AbstractWorkflowComponent::setStatus;

protected:
    bool performStep(std::string &message) override
    {
        message.clear();
        return true;
    }
};

std::vector<ErrorEntry> entriesWithCode(const std::vector<ErrorEntry> &entries, int32_t code)
{
    std::vector<ErrorEntry> matching;
    std::copy_if(entries.begin(), entries.end(), std::back_inserter(matching),
                 [code](const ErrorEntry &entry) { return entry.code == code; });
    return matching;
}

} // namespace

TEST(WorkflowConnectionTest, AContradictoryLinkFailsValidationWithItsCode)
{
    OwnedQuantity cms(Unit::flowInCMS());
    OwnedQuantity cfs(Unit::flowInCFS());
    PortComponent upstream("up", cms.quantity.get(), cms.quantity.get());
    PortComponent downstream("down", cfs.quantity.get(), cfs.quantity.get());

    PlainWorkflow workflow("wf");
    ASSERT_TRUE(workflow.addModelComponent(&upstream));
    ASSERT_TRUE(workflow.addModelComponent(&downstream));
    workflow.initialize();
    ASSERT_EQ(workflow.status(), WStatus::Initialized);
    ASSERT_TRUE(downstream.in->setProvider(upstream.out));
    upstream.out->addConsumer(downstream.in);

    const auto problems = workflow.validate();
    ASSERT_EQ(problems.size(), 1u);
    EXPECT_NE(problems[0].find("link 'out' -> 'in'"), std::string::npos) << problems[0];
    EXPECT_EQ(workflow.status(), WStatus::Invalid);

    const auto units = entriesWithCode(workflow.errors(), Diagnostic::UnitMismatch);
    ASSERT_EQ(units.size(), 1u);
    EXPECT_EQ(units[0].severity, Severity::Error);
}

TEST(WorkflowConnectionTest, AnUnderdeclaredLinkWarnsAndStillValidates)
{
    OwnedQuantity cms(Unit::flowInCMS());
    PortComponent upstream("up", cms.quantity.get(), cms.quantity.get());
    PortComponent downstream("down", cms.quantity.get(), nullptr);

    PlainWorkflow workflow("wf");
    ASSERT_TRUE(workflow.addModelComponent(&upstream));
    ASSERT_TRUE(workflow.addModelComponent(&downstream));
    workflow.initialize();
    ASSERT_TRUE(downstream.in->setProvider(upstream.out));
    upstream.out->addConsumer(downstream.in);

    EXPECT_TRUE(workflow.validate().empty());
    EXPECT_EQ(workflow.status(), WStatus::Validated);

    const auto warnings =
        entriesWithCode(workflow.errors(), Diagnostic::ValueDefinitionMismatch);
    ASSERT_EQ(warnings.size(), 1u);
    EXPECT_EQ(warnings[0].severity, Severity::Warning);
}

// ── Lifecycle enforcement ─────────────────────────────────────────────────

TEST(LifecycleEnforcementTest, AnIllegalComponentTransitionIsPerformedAndReportedOnce)
{
    OwnedQuantity cms(Unit::flowInCMS());
    PortComponent component("c", cms.quantity.get(), cms.quantity.get());

    component.setStatus(CStatus::Updated); // Created → Updated: illegal
    EXPECT_EQ(component.status(), CStatus::Updated) << "performed, not refused";
    component.setStatus(CStatus::Updated); // a refresh, not a transition
    component.setStatus(CStatus::Created); // Updated → Created: illegal
    component.setStatus(CStatus::Updated); // Created → Updated again: already reported

    const auto illegal =
        entriesWithCode(component.errors(), Diagnostic::IllegalStatusTransition);
    ASSERT_EQ(illegal.size(), 2u);
    EXPECT_EQ(illegal[0].severity, Severity::Error);
    EXPECT_NE(illegal[0].message.find("Created -> Updated"), std::string::npos)
        << illegal[0].message;
    EXPECT_NE(illegal[1].message.find("Updated -> Created"), std::string::npos)
        << illegal[1].message;
}

TEST(LifecycleEnforcementTest, AComponentMayFailFromAnyStatusButFinished)
{
    OwnedQuantity cms(Unit::flowInCMS());
    PortComponent resting("resting", cms.quantity.get(), cms.quantity.get());
    resting.initialize();
    ASSERT_EQ(resting.status(), CStatus::Initialized);
    resting.setStatus(CStatus::Failed, "peer died");
    EXPECT_TRUE(
        entriesWithCode(resting.errors(), Diagnostic::IllegalStatusTransition).empty());

    PortComponent finished("finished", cms.quantity.get(), cms.quantity.get());
    finished.initialize();
    std::ignore = finished.validate();
    finished.prepare();
    finished.update();
    finished.finish();
    ASSERT_EQ(finished.status(), CStatus::Finished);
    EXPECT_TRUE(
        entriesWithCode(finished.errors(), Diagnostic::IllegalStatusTransition).empty());
    finished.setStatus(CStatus::Failed, "too late");
    EXPECT_EQ(entriesWithCode(finished.errors(), Diagnostic::IllegalStatusTransition).size(), 1u);
}

TEST(LifecycleEnforcementTest, AnIllegalWorkflowTransitionIsReported)
{
    PlainWorkflow workflow("wf");
    workflow.setStatus(WStatus::Updated); // Created → Updated: illegal
    EXPECT_EQ(workflow.status(), WStatus::Updated);
    workflow.setStatus(WStatus::Failed, "peer died"); // legal from anywhere but Finished
    const auto illegal =
        entriesWithCode(workflow.errors(), Diagnostic::IllegalStatusTransition);
    ASSERT_EQ(illegal.size(), 1u);
    EXPECT_NE(illegal[0].message.find("Created -> Updated"), std::string::npos)
        << illegal[0].message;
}
