// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for PolyhedralSurfaceArgument: a mesh as a component argument, from
// a UGRID file and from a live polyhedral-surface data item — the two ways
// the provider pipeline delivers one — plus the refusals that keep bulk out
// of documents.

#include <gtest/gtest.h>

#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/spatial/meshadapters.h"
#include "hydrocouplesdk/spatial/polyhedralsurfaceargument.h"

#ifdef USE_NETCDF
#include "hydrocouplesdk/io/netcdfugridwriter.h"
#endif

#include <filesystem>
#include <string>
#include <vector>

using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Spatial;
using HydroCouple::SDK::IO::MeshDefinition;
using HydroCouple::BufferDescriptor;
using HydroCouple::DataKind;

#ifndef TEST_ARTIFACTS_DIR
#define TEST_ARTIFACTS_DIR "."
#endif

namespace {

//! A square split into a triangle and a quad (the UGRID reader fixture).
MeshDefinition mixedMesh()
{
    MeshDefinition mesh;
    mesh.meshName = "mixed";
    mesh.nodeX = {0.0, 10.0, 10.0, 0.0, 5.0};
    mesh.nodeY = {0.0, 0.0, 10.0, 10.0, 20.0};
    mesh.nodeZ = {1.0, 2.0, 3.0, 4.0, 5.0};
    mesh.faceNodeOffsets = {0, 4, 7};
    mesh.faceNodes = {0, 1, 2, 3, 3, 2, 4};
    mesh.edgeNodes = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    return mesh;
}

/*!
 * The provider's half in miniature: a data item whose surface carries a
 * mesh view — what a mesh generator's output looks like to a consumer.
 */
class SurfaceItem final
    : public AbstractComponentDataItem,
      public virtual HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem
{
public:
    explicit SurfaceItem(MeshDefinition mesh)
        : AbstractComponentDataItem("surface", {}, nullptr, nullptr),
          m_adapter(polyhedralSurfaceFromMesh(mesh), std::move(mesh))
    {}

    [[nodiscard]] std::vector<int64_t> shape() const override { return {}; }
    [[nodiscard]] DataKind dataKind() const override
    {
        return DataKind::Float64;
    }
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &,
                                     std::span<const int64_t>,
                                     std::span<const int64_t>,
                                     std::string *message = nullptr)
        const override
    {
        if (message) *message = "geometry-only item";
        return false;
    }
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &,
                                     std::span<const int64_t>,
                                     std::span<const int64_t>,
                                     std::string *message = nullptr) override
    {
        if (message) *message = "geometry-only item";
        return false;
    }

    [[nodiscard]] HydroCouple::Spatial::MeshLocation
    location() const override
    {
        return HydroCouple::Spatial::MeshLocation::Face;
    }
    [[nodiscard]] HydroCouple::Spatial::SpatialDataType meshDataType()
        const override
    {
        return HydroCouple::Spatial::SpatialDataType::Scalar;
    }
    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    polyhedralSurface() const override
    {
        return const_cast<PolyhedralSurfaceAdapter *>(&m_adapter);
    }
    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return nullptr;
    }
    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        return HydroCouple::Spatial::VectorBasis::Unknown;
    }

private:
    PolyhedralSurfaceAdapter m_adapter;
};

} // namespace

TEST(PolyhedralSurfaceArgumentTest, AdoptsAMeshFromAPolyhedralSurfaceItem)
{
    const SurfaceItem item(mixedMesh());
    PolyhedralSurfaceArgument argument("mesh", nullptr);

    std::string message;
    ASSERT_TRUE(argument.initialize(
        static_cast<const HydroCouple::IComponentDataItem &>(item), message))
        << message;

    EXPECT_TRUE(argument.hasMesh());
    EXPECT_EQ(argument.mesh().nodeCount(), 5);
    EXPECT_EQ(argument.mesh().faceCount(), 2);
    EXPECT_EQ(argument.mesh().faceNodes,
              (std::vector<int64_t>{0, 1, 2, 3, 3, 2, 4}));
    EXPECT_DOUBLE_EQ(argument.mesh().nodeZ[4], 5.0);
}

TEST(PolyhedralSurfaceArgumentTest, TheDataPlaneShowsNodeCoordinates)
{
    PolyhedralSurfaceArgument argument("mesh", nullptr);
    std::string message;
    ASSERT_TRUE(argument.setMesh(mixedMesh(), message)) << message;

    EXPECT_EQ(argument.shape(), (std::vector<int64_t>{5, 3}));
    std::vector<double> values(15, -1.0);
    const std::vector<int64_t> shape{5, 3};
    const BufferDescriptor destination = HydroCouple::SDK::detail::hostDescriptor(
        values.data(), DataKind::Float64, shape);
    const std::vector<int64_t> start{0, 0};
    ASSERT_TRUE(argument.getValuesInto(destination, start, shape, &message))
        << message;
    EXPECT_DOUBLE_EQ(values[0], 0.0);   // node 0 x
    EXPECT_DOUBLE_EQ(values[3], 10.0);  // node 1 x
    EXPECT_DOUBLE_EQ(values[13], 20.0); // node 4 y
    EXPECT_DOUBLE_EQ(values[14], 5.0);  // node 4 z
}

TEST(PolyhedralSurfaceArgumentTest, RefusesAnInlineMeshPayload)
{
    PolyhedralSurfaceArgument argument("mesh", nullptr);
    std::string message;
    EXPECT_FALSE(argument.initialize(
        R"({"nodeX": [0, 1], "nodeY": [0, 1]})",
        HydroCouple::IArgument::ArgumentInputType::JSON, message));
    EXPECT_NE(message.find("never inlined"), std::string::npos) << message;
}

TEST(PolyhedralSurfaceArgumentTest, AMeshFromALiveOutputRefusesToSerialize)
{
    PolyhedralSurfaceArgument argument("mesh", nullptr);
    std::string message;
    ASSERT_TRUE(argument.setMesh(mixedMesh(), message)) << message;

    std::string value;
    EXPECT_FALSE(argument.serialize(
        HydroCouple::IArgument::ArgumentInputType::JSON, value, message));
    EXPECT_NE(message.find("@from"), std::string::npos) << message;
}

TEST(PolyhedralSurfaceArgumentTest, RefusesAnIncoherentMesh)
{
    PolyhedralSurfaceArgument argument("mesh", nullptr);
    MeshDefinition broken = mixedMesh();
    broken.nodeY.pop_back();
    std::string message;
    EXPECT_FALSE(argument.setMesh(broken, message));
    EXPECT_FALSE(message.empty());
    EXPECT_FALSE(argument.hasMesh());
}

TEST(PolyhedralSurfaceArgumentTest, AnotherMeshArgumentIsAdoptedWholesale)
{
    PolyhedralSurfaceArgument source("mesh", nullptr);
    std::string message;
    ASSERT_TRUE(source.setMesh(mixedMesh(), message)) << message;

    PolyhedralSurfaceArgument target("mesh", nullptr);
    ASSERT_TRUE(target.initialize(
        static_cast<const HydroCouple::IComponentDataItem &>(source),
        message))
        << message;
    EXPECT_EQ(target.mesh().faceNodes, source.mesh().faceNodes);
}

#ifdef USE_NETCDF

TEST(PolyhedralSurfaceArgumentTest, ReadsAUGRIDFileAndSerializesItsReference)
{
    const std::filesystem::path dir{TEST_ARTIFACTS_DIR};
    std::error_code code;
    std::filesystem::create_directories(dir, code);
    const std::filesystem::path path = dir / "mesh_argument_fixture.nc";

    {
        HydroCouple::SDK::IO::NetCDFUGRIDWriter writer(path, mixedMesh());
        ASSERT_EQ(writer.initialize(), 0) << writer.errorMessage();
        ASSERT_EQ(writer.finalize(), 0) << writer.errorMessage();
    }

    PolyhedralSurfaceArgument argument("mesh", nullptr);
    std::string message;
    ASSERT_TRUE(argument.initialize(
        path.string(), HydroCouple::IArgument::ArgumentInputType::File,
        message))
        << message;
    EXPECT_EQ(argument.mesh().nodeCount(), 5);
    EXPECT_EQ(argument.mesh().faceCount(), 2);

    // The document gets the reference, not the mesh.
    std::string value;
    ASSERT_TRUE(argument.serialize(
        HydroCouple::IArgument::ArgumentInputType::JSON, value, message))
        << message;
    const auto payload = nlohmann::json::parse(value);
    ASSERT_TRUE(payload.contains("@uri"));
    EXPECT_EQ(payload["@uri"], path.string());
    EXPECT_FALSE(payload.contains("nodeX"));
}

#endif // USE_NETCDF
