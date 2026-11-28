// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin
#include <gtest/gtest.h>
#include "hydrocouplesdk/io/meshpackage.h"
#include "hydrocouplesdk/spatial/polyhedralsurfaceargument.h"
#include <fstream>
using namespace HydroCouple::SDK::IO;
namespace {
std::filesystem::path artifact(const std::string &name) {
    return std::filesystem::path(TEST_ARTIFACTS_DIR) / name;
}
std::filesystem::path source(const std::string &name, const std::string &text) {
    const auto path = artifact(name);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream(path) << text;
    return path;
}
MeshImportOptions options() {
    MeshImportOptions result;
    result.horizontalCrs = "LOCAL";
    result.verticalDatum = "synthetic-bed";
    result.elevationKind = "bed-elevation";
    return result;
}
const char *mixed =
    "[2D_VERTICES]\n0 0 -5\n10 0 -5\n10 10 -5\n0 10 -5\n20 0 -5\n20 10 -5\n"
    "[2D_TRIANGLES]\n1 4 2 0.03 5 apron\n4 5 2 0.04 5 apron\n"
    "[2D_QUADS]\n0 3 2 1 0.02 5 corridor\n"
    "[2D_BOUNDARY_CONDITIONS]\n2 2 WALL * * bank\n"
    "[2D_TRIANGLE_NODE_MAP]\n2 OF1 0.65 2\n";
}

TEST(MeshPackage, PreservesMixedTopologyAttributesAndEndpointIdentity) {
    MeshPackage package;
    std::string message;
    ASSERT_TRUE(importSurfaceMesh(source("mixed_swmm.2dm", mixed), options(), package, message)) << message;
    EXPECT_EQ(package.mesh.faceNodeOffsets, (std::vector<int64_t>{0,3,6,10}));
    EXPECT_EQ(package.faceIds, (std::vector<std::string>{"0","1","2"}));
    EXPECT_EQ(package.regionTags[2], "corridor");
    EXPECT_EQ(package.roughness[1], 0.04);
    EXPECT_EQ(package.initialDepth, (std::vector<double>{5,5,5}));
    ASSERT_EQ(package.boundaries.size(), 1u);
    EXPECT_EQ(package.boundaries[0].node0, 1);
    EXPECT_EQ(package.boundaries[0].node1, 0);
    EXPECT_EQ(package.boundaries[0].group, "bank");
    EXPECT_EQ(package.sourceSections["[2D_TRIANGLE_NODE_MAP]"][0][1], "OF1");
}

TEST(MeshPackage, DetectsSmsByContentAndPreservesSparseIds) {
    MeshPackage package;
    std::string message;
    const auto file = source("sms_named_swmm.inp",
        "MESH2D\nE4Q 99 10 20 30 40 mat\nE3T 700 20 50 30 sand\n"
        "ND 10 0 0 -5\nND 20 10 0 -5\nND 30 10 10 -5\nND 40 0 10 -5\nND 50 20 0 -5\n");
    ASSERT_TRUE(importSurfaceMesh(file, options(), package, message)) << message;
    EXPECT_EQ(package.faceIds, (std::vector<std::string>{"700", "99"}));
    EXPECT_EQ(package.nodeIds[4], "50");
    EXPECT_EQ(package.mesh.faceNodeOffsets, (std::vector<int64_t>{0,3,7}));
    EXPECT_TRUE(package.initialDepth.empty());
}

TEST(MeshPackage, ConvertsUnitsAndRefusesMissingDatumAndInvalidCellsAtomically) {
    std::string message;
    MeshPackage package;
    auto unitOptions = options();
    unitOptions.horizontalUnits = "ft";
    unitOptions.verticalUnits = "us-survey-ft";
    unitOptions.depthUnits = "ft";
    ASSERT_TRUE(importSurfaceMesh(source("units.2dm", mixed), unitOptions, package, message)) << message;
    EXPECT_DOUBLE_EQ(package.mesh.nodeX[1], 3.048);
    EXPECT_NEAR(package.mesh.nodeZ[0], -5 * 1200.0 / 3937.0, 1e-14);
    EXPECT_DOUBLE_EQ(package.initialDepth[0], 1.524);
    const auto original = package.mesh.nodeX;
    unitOptions.verticalDatum.clear();
    EXPECT_FALSE(importSurfaceMesh(source("no_datum.2dm", mixed), unitOptions, package, message));
    EXPECT_EQ(package.mesh.nodeX, original);
    EXPECT_FALSE(importSurfaceMesh(source("bad_depth.2dm",
        "[2D_VERTICES]\n0 0 -5\n1 0 -5\n0 1 -5\n[2D_TRIANGLES]\n0 1 2 0.03 -0.1\n"),
        options(), package, message));
    EXPECT_EQ(package.mesh.nodeX, original);
}

#ifdef HYDROCOUPLESDK_HAVE_NETCDF
TEST(MeshPackage, RoundTripsManifestAndArgumentWithTamperDetection) {
    MeshPackage package, restored;
    std::string message;
    ASSERT_TRUE(importSurfaceMesh(source("roundtrip_input.2dm", mixed), options(), package, message)) << message;
    auto manifest = artifact("roundtrip.mesh.json"), nc = manifest;
    nc.replace_extension(".nc");
    std::filesystem::remove(manifest); std::filesystem::remove(nc);
    ASSERT_TRUE(writeMeshPackage(manifest, package, message)) << message;
    EXPECT_FALSE(writeMeshPackage(manifest, package, message));
    ASSERT_TRUE(readMeshPackage(manifest, restored, message)) << message;
    EXPECT_EQ(restored.mesh.faceNodes, package.mesh.faceNodes);
    EXPECT_EQ(restored.nodeIds, package.nodeIds);
    EXPECT_EQ(restored.faceIds, package.faceIds);
    EXPECT_EQ(restored.sourceSections, package.sourceSections);
    HydroCouple::SDK::Spatial::PolyhedralSurfaceArgument argument("mesh", nullptr);
    ASSERT_TRUE(argument.initialize(manifest.string(), HydroCouple::IArgument::ArgumentInputType::File, message)) << message;
    ASSERT_TRUE(argument.meshPackage().has_value());
    EXPECT_EQ(argument.meshPackage()->boundaries[0].node0, 1);
    nlohmann::json data;
    { std::ifstream in(manifest); in >> data; }
    data["geometry_fingerprint"] = "invalid";
    { std::ofstream out(manifest); out << data; }
    EXPECT_FALSE(argument.initialize(manifest.string(), HydroCouple::IArgument::ArgumentInputType::File, message));
    EXPECT_EQ(argument.mesh().faceCount(), 3);
    EXPECT_TRUE(argument.meshPackage().has_value());
}
#endif

TEST(MeshPackage, DeclaredUnitsMustAgreeWithImportAndHeadersArePreserved) {
    auto opt = options();
    MeshPackage package;
    std::string message;
    const auto path = source("declared_units.2dm", std::string(";; UNITS: SI (m)\n;; SOURCE_CRS: LOCAL\n") + mixed);
    ASSERT_TRUE(importSurfaceMesh(path, opt, package, message)) << message;
    EXPECT_EQ(package.provenance["source_headers"]["UNITS"], "SI (m)");
    opt.horizontalUnits = "ft";
    EXPECT_FALSE(importSurfaceMesh(path, opt, package, message));
    EXPECT_NE(message.find("disagrees"), std::string::npos);
}

TEST(MeshPackage, ImportsBellingeSingularMetreHeaderWithoutChangingCoordinates) {
    MeshPackage package;
    std::string message;
    ASSERT_TRUE(importSurfaceMesh(source("bellinge_units.2dm", std::string(";; UNITS: metre\n") + mixed),
        options(), package, message)) << message;
    EXPECT_DOUBLE_EQ(package.mesh.nodeX[1], 10.0);
    EXPECT_EQ(package.provenance["source_headers"]["UNITS"], "metre");
}

TEST(MeshPackage, PublishesNamedLocalDatumAndMetricSpatialReference) {
    MeshPackage package;
    package.horizontalCrs="LOCAL"; package.verticalDatum="synthetic-bed";
    package.elevationKind="bed-elevation";
    MeshPackageReference reference(package);
    EXPECT_NE(reference.srText().find("LOCAL_CS"), std::string::npos);
    EXPECT_NE(reference.verticalSrText().find("synthetic-bed"), std::string::npos);
    EXPECT_EQ(reference.distanceUnits(), HydroCouple::IUnit::DistanceUnits::Meters);
    EXPECT_EQ(reference.verticalDistanceUnits(), HydroCouple::IUnit::DistanceUnits::Meters);
    EXPECT_EQ(reference.verticalAuthSRID(),0);
}

#ifdef USE_GDAL
TEST(MeshPackage, NormalizesFeetProjectionMetadataWithMetreCoordinates) {
    auto opt = options();
    opt.horizontalCrs = "EPSG:2263";
    opt.horizontalUnits = "us-survey-ft";
    MeshPackage package;
    std::string message;
    ASSERT_TRUE(importSurfaceMesh(source("feet_projection.2dm", mixed), opt, package, message)) << message;
    EXPECT_EQ(package.provenance["source_horizontal_crs"], "EPSG:2263");
    EXPECT_NE(package.horizontalCrs.find("metre"), std::string::npos);
    EXPECT_NEAR(package.mesh.nodeX[1], 10 * 1200.0 / 3937.0, 1e-13);
}
#endif
