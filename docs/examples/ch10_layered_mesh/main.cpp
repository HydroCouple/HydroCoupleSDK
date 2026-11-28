// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 10: layered meshes. A sigma vertical coordinate, a
 *         LayeredMeshOutput connected to a LayeredMeshInput, and what
 *         canConsume() and validateConnection() say when the two ends
 *         disagree about layers or vertical datums.
 *
 * Exits nonzero when any value is not the one the guide states.
 */

#include "hydrocouplesdk/component/connectionvalidator.h"
#include "hydrocouplesdk/core/unit.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplesdk/spatial/layeredmeshexchangeitems.h"
#include "hydrocouplesdk/spatial/meshadapters.h"

#include <cmath>
#include <cstdio>
#include <memory>
#include <span>
#include <string>
#include <vector>

using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Spatial;
namespace HCS = HydroCouple::Spatial;
using HydroCouple::SDK::IO::MeshDefinition;

namespace {

int failures = 0;

void check(bool ok, const char *what)
{
    if (!ok)
    {
        std::printf("FAILED: %s\n", what);
        ++failures;
    }
}

bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

//! A horizontal reference with a declared vertical datum.
class Reference final : public HCS::ISpatialReferenceSystem
{
public:
    Reference(int srid, int verticalSrid) : m_srid(srid), m_verticalSrid(verticalSrid) {}
    int authSRID() const override { return m_srid; }
    const std::string &authName() const override { return m_epsg; }
    const std::string &srText() const override { return m_empty; }
    HydroCouple::IUnit::DistanceUnits distanceUnits() const override
    {
        return HydroCouple::IUnit::DistanceUnits::Meters;
    }
    const std::string &verticalAuthName() const override { return m_epsg; }
    int verticalAuthSRID() const override { return m_verticalSrid; }
    const std::string &verticalSrText() const override { return m_empty; }
    HydroCouple::IUnit::DistanceUnits verticalDistanceUnits() const override
    {
        return HydroCouple::IUnit::DistanceUnits::Meters;
    }

private:
    int m_srid, m_verticalSrid;
    std::string m_epsg = "EPSG", m_empty;
};

//! [sigma]
//! Terrain-following layers: each column's interfaces divide the depth
//! between the free surface and the bed equally. Elevations are positive up;
//! interface 0 is the surface.
class SigmaCoordinate final : public HCS::IVerticalCoordinate
{
public:
    SigmaCoordinate(int64_t layers, std::vector<double> bed, double stage,
                    HCS::VerticalCoordinateKind kind = HCS::VerticalCoordinateKind::Sigma)
        : m_layers(layers), m_bed(std::move(bed)), m_stage(stage), m_kind(kind)
    {
        rebuild();
    }

    //! Moves the free surface; every interface moves with it.
    void setStage(double stage)
    {
        m_stage = stage;
        rebuild();
        ++m_epoch;
    }

    HCS::VerticalCoordinateKind kind() const override { return m_kind; }
    int64_t layerCount() const override { return m_layers; }
    int64_t columnCount() const override { return static_cast<int64_t>(m_bed.size()); }
    bool isTimeVarying() const override { return true; }
    uint64_t geometryEpoch() const override { return m_epoch; }

    double interfaceElevation(int64_t column, int64_t interfaceIndex) const override
    {
        return m_profile[static_cast<size_t>(column * (m_layers + 1) + interfaceIndex)];
    }

    std::span<const double> interfaceElevations() const override { return m_profile; }

private:
    void rebuild()
    {
        m_profile.assign(m_bed.size() * static_cast<size_t>(m_layers + 1), 0.0);
        for (size_t c = 0; c < m_bed.size(); ++c)
            for (int64_t k = 0; k <= m_layers; ++k)
                m_profile[c * static_cast<size_t>(m_layers + 1) + static_cast<size_t>(k)] =
                    m_stage + (m_bed[c] - m_stage) * static_cast<double>(k) /
                                  static_cast<double>(m_layers);
    }

    int64_t m_layers;
    std::vector<double> m_bed;
    double m_stage;
    HCS::VerticalCoordinateKind m_kind;
    uint64_t m_epoch = 0;
    std::vector<double> m_profile;
};
//! [sigma]

MeshDefinition twoCellMesh()
{
    MeshDefinition mesh;
    mesh.nodeX = {0.0, 10.0, 10.0, 0.0, 5.0};
    mesh.nodeY = {0.0, 0.0, 10.0, 10.0, 20.0};
    mesh.faceNodeOffsets = {0, 4, 7};
    mesh.faceNodes = {0, 1, 2, 3, 3, 2, 4};
    return mesh;
}

void print(const char *title, const std::vector<ConnectionFinding> &findings)
{
    std::printf("%s: %zu finding(s)%s\n", title, findings.size(),
                hasConnectionError(findings) ? ", has errors" : "");
    for (const ConnectionFinding &finding : findings)
        std::printf("  %s %s\n",
                    finding.severity == HydroCouple::ErrorEntry::Severity::Error ? "error  "
                                                                                 : "warning",
                    finding.message.c_str());
}

} // namespace

int main()
{
    //! [link]
    // Temperature is intensive: an adapter averages it rather than summing.
    Unit celsius("degree Celsius", 1.0, 273.15);
    celsius.setSymbol("degC");
    celsius.dimensionsInternal()->setPower(
        HydroCouple::IUnitDimensions::FundamentalUnitDimension::Temperature, 1.0);
    Quantity temperatureQuantity("Water temperature", typeid(double), &celsius);
    temperatureQuantity.setValueKind(HydroCouple::ValueKind::Intensive);

    // Both models share the horizontal system and the NAVD88 datum.
    Reference utmNavd88(26917, 5703);
    PolyhedralSurfaceAdapter surface(polyhedralSurfaceFromMesh(twoCellMesh()), twoCellMesh(),
                                     &utmNavd88);
    SigmaCoordinate lakeLayers(3, {-6.0, -3.0}, 0.0);   // bed per face, stage 0 m
    SigmaCoordinate modelLayers(3, {-6.0, -3.0}, 0.0);

    LayeredMeshOutputDouble produced("temperature", &surface, &lakeLayers,
                                     HCS::MeshLocation::Volume, HCS::SpatialDataType::Scalar,
                                     2, 3, &temperatureQuantity, nullptr);
    LayeredMeshInputDouble consumed("temperature_in", &surface, &modelLayers,
                                    HCS::MeshLocation::Volume, HCS::SpatialDataType::Scalar,
                                    2, 3, &temperatureQuantity, nullptr);

    // Values (e, k): warm surface layer, cooler below.
    const double warm[2][3] = {{24.0, 20.0, 15.0}, {23.5, 21.0, 18.0}};
    for (int e = 0; e < 2; ++e)
        for (int k = 0; k < 3; ++k)
            produced(e, k) = warm[e][k];

    const bool connected = consumed.setProvider(&produced);
    consumed.retrieveValuesFromProvider();
    const std::vector<ConnectionFinding> clean = validateConnection(produced, consumed);
    //! [link]

    std::printf("output: shape {%lld, %lld}, axes \"%s\" \"%s\"; layer 1 of face 0 spans "
                "%.1f to %.1f m\n",
                static_cast<long long>(produced.shape()[0]),
                static_cast<long long>(produced.shape()[1]),
                produced.entityDimension()->id().c_str(),
                produced.layerDimension()->id().c_str(),
                produced.verticalCoordinate()->interfaceElevation(0, 1),
                produced.verticalCoordinate()->interfaceElevation(0, 2));
    std::printf("connected %d; input face 1 layers %.1f %.1f %.1f\n", connected,
                consumed(1, 0), consumed(1, 1), consumed(1, 2));
    print("matching link", clean);

    check(connected, "a matching provider is accepted");
    check(near(consumed(1, 2), 18.0), "values arrive entity-major");
    check(!hasConnectionError(clean), "the matching link has no errors");
    check(near(produced.verticalCoordinate()->interfaceElevation(0, 1), -2.0),
          "sigma interface 1 of face 0");

    //! [live]
    // The coordinate is borrowed, not copied: raise the surface and the item
    // reports the new elevations, with a new epoch for caches to compare.
    const uint64_t before = produced.verticalCoordinate()->geometryEpoch();
    lakeLayers.setStage(1.5);
    //! [live]
    std::printf("after the stage rises 1.5 m: interface 0 at %.2f m, interface 1 at %.2f m, "
                "epoch %llu -> %llu\n",
                produced.verticalCoordinate()->interfaceElevation(0, 0),
                produced.verticalCoordinate()->interfaceElevation(0, 1),
                static_cast<unsigned long long>(before),
                static_cast<unsigned long long>(produced.verticalCoordinate()->geometryEpoch()));
    check(near(produced.verticalCoordinate()->interfaceElevation(0, 0), 1.5) &&
              produced.verticalCoordinate()->geometryEpoch() == before + 1,
          "the vertical coordinate is live");

    //! [mismatch]
    // Four layers where the provider has three: refused before any value moves.
    SigmaCoordinate fourLayers(4, {-6.0, -3.0}, 0.0);
    LayeredMeshInputDouble tooMany("too_many", &surface, &fourLayers,
                                   HCS::MeshLocation::Volume, HCS::SpatialDataType::Scalar, 2,
                                   4, &temperatureQuantity, nullptr);
    std::string why;
    const bool accepted = tooMany.canConsume(&produced, why);

    // Same layer count, but z-levels and an NGVD29 datum (EPSG 7968): the
    // shapes match and the meaning does not.
    Reference utmNgvd29(26917, 7968);
    PolyhedralSurfaceAdapter otherSurface(polyhedralSurfaceFromMesh(twoCellMesh()),
                                          twoCellMesh(), &utmNgvd29);
    SigmaCoordinate zLevels(3, {-6.0, -3.0}, 0.0, HCS::VerticalCoordinateKind::ZLevel);
    LayeredMeshInputDouble zModel("z_model", &otherSurface, &zLevels,
                                  HCS::MeshLocation::Volume, HCS::SpatialDataType::Scalar, 2,
                                  3, &temperatureQuantity, nullptr);
    const std::vector<ConnectionFinding> wrong = validateConnection(produced, zModel);
    //! [mismatch]

    std::printf("four-layer input: canConsume %d, \"%s\"\n", accepted, why.c_str());
    print("z-level model on NGVD29", wrong);
    check(!accepted && why == "Provider has 3 layers, this input has 4", "layer count refused");
    check(zModel.canConsume(&produced, why), "canConsume does not compare elevations or datums");
    check(hasConnectionError(wrong) && wrong.size() == 2, "datum and coordinate kind are errors");

    std::printf("%s\n", failures == 0 ? "all checks passed" : "checks FAILED");
    return failures == 0 ? 0 : 1;
}
