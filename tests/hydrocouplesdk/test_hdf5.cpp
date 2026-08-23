// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for HDF5UGRIDWriter: mesh validation, CF/UGRID file structure,
// extend-and-write time appends, fill-value padding, and error reporting —
// verified by reopening the produced file with the HDF5 C API.

#include <gtest/gtest.h>

#include "hydrocouplesdk/io/hdf5ugridwriter.h"
#include "hydrocouplesdk/io/iothread.h"

#include <hdf5.h>

#include <cstring>
#include <filesystem>
#include <memory>

using namespace HydroCouple;
using namespace HydroCouple::SDK::IO;

#ifndef TEST_ARTIFACTS_DIR
#define TEST_ARTIFACTS_DIR "."
#endif

namespace {

std::filesystem::path artifactPath(const std::string &name)
{
    const std::filesystem::path dir{TEST_ARTIFACTS_DIR};
    std::filesystem::create_directories(dir);
    return dir / name;
}

/*!
 * Two-triangle test mesh:
 *
 *   3 ---- 2
 *   | \  1 |
 *   |  \   |
 *   | 0  \ |
 *   0 ---- 1
 */
MeshDefinition twoTriangleMesh()
{
    MeshDefinition mesh;
    mesh.meshName        = "mesh2d";
    mesh.nodeX           = {0.0, 1.0, 1.0, 0.0};
    mesh.nodeY           = {0.0, 0.0, 1.0, 1.0};
    mesh.nodeZ           = {10.0, 11.0, 12.0, 13.0};
    mesh.faceNodeOffsets = {0, 3, 6};
    mesh.faceNodes       = {0, 1, 3, 1, 2, 3};
    mesh.edgeNodes       = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {1, 3}};
    return mesh;
}

FieldSlice doubleField(const std::string &id, std::vector<double> values)
{
    FieldSlice field;
    field.itemId = id;
    field.kind   = DataKind::Float64;
    field.shape  = {static_cast<int64_t>(values.size())};
    field.data.resize(values.size() * sizeof(double));
    std::memcpy(field.data.data(), values.data(), field.data.size());
    return field;
}

std::string readStringAttribute(hid_t object, const char *attrName)
{
    if (H5Aexists(object, attrName) <= 0)
        return {};
    const hid_t attr = H5Aopen(object, attrName, H5P_DEFAULT);
    const hid_t type = H5Aget_type(attr);
    const size_t size = H5Tget_size(type);
    std::string value(size, '\0');
    H5Aread(attr, type, value.data());
    H5Tclose(type);
    H5Aclose(attr);
    while (!value.empty() && value.back() == '\0')
        value.pop_back();
    return value;
}

} // namespace

// ── MeshDefinition validation ─────────────────────────────────────────────

TEST(MeshDefinitionTest, ValidMeshPasses)
{
    std::string message;
    EXPECT_TRUE(twoTriangleMesh().validate(message)) << message;
}

TEST(MeshDefinitionTest, RejectsMismatchedCoordinates)
{
    MeshDefinition mesh = twoTriangleMesh();
    mesh.nodeY.pop_back();
    std::string message;
    EXPECT_FALSE(mesh.validate(message));
    EXPECT_FALSE(message.empty());
}

TEST(MeshDefinitionTest, RejectsOutOfRangeConnectivity)
{
    MeshDefinition mesh = twoTriangleMesh();
    mesh.faceNodes[2] = 99;
    std::string message;
    EXPECT_FALSE(mesh.validate(message));
    EXPECT_NE(message.find("out of range"), std::string::npos);
}

TEST(MeshDefinitionTest, CountsAndWidth)
{
    const MeshDefinition mesh = twoTriangleMesh();
    EXPECT_EQ(mesh.nodeCount(), 4);
    EXPECT_EQ(mesh.faceCount(), 2);
    EXPECT_EQ(mesh.edgeCount(), 5);
    EXPECT_EQ(mesh.maxNodesPerFace(), 3);
}

// ── File structure round trip ─────────────────────────────────────────────

class HDF5UGRIDWriterTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_path = artifactPath("io_ugrid.h5");
        std::filesystem::remove(m_path);
    }

    /*! Writes three time levels of node depth + face flow. */
    void writeThreeLevels()
    {
        HDF5UGRIDWriter writer(m_path, twoTriangleMesh());
        writer.addField("depth", MeshLocation::Node, "m", "sea_floor_depth");
        writer.addField("flow", MeshLocation::Face, "m3 s-1");
        ASSERT_EQ(writer.initialize(), 0) << writer.errorMessage();

        for (int step = 0; step < 3; ++step)
        {
            Snapshot snapshot(2451545.0 + step);
            snapshot.addField(doubleField(
                "depth", {step + 0.1, step + 0.2, step + 0.3, step + 0.4}));
            snapshot.addField(doubleField("flow", {step * 10.0, step * 20.0}));
            ASSERT_EQ(writer.write(snapshot), 0) << writer.errorMessage();
        }
        ASSERT_EQ(writer.finalize(), 0) << writer.errorMessage();
    }

    std::filesystem::path m_path;
};

TEST_F(HDF5UGRIDWriterTest, WritesConventionsAndTopology)
{
    writeThreeLevels();

    const hid_t file = H5Fopen(m_path.string().c_str(), H5F_ACC_RDONLY,
                               H5P_DEFAULT);
    ASSERT_GE(file, 0);

    EXPECT_EQ(readStringAttribute(file, "Conventions"), "CF-1.11 UGRID-1.0");

    const hid_t topology = H5Dopen2(file, "mesh2d", H5P_DEFAULT);
    ASSERT_GE(topology, 0);
    EXPECT_EQ(readStringAttribute(topology, "cf_role"), "mesh_topology");
    EXPECT_EQ(readStringAttribute(topology, "node_coordinates"),
              "mesh2d_node_x mesh2d_node_y");
    EXPECT_EQ(readStringAttribute(topology, "face_node_connectivity"),
              "mesh2d_face_nodes");
    H5Dclose(topology);
    H5Fclose(file);
}

TEST_F(HDF5UGRIDWriterTest, WritesCoordinatesAndPaddedConnectivity)
{
    writeThreeLevels();

    const hid_t file = H5Fopen(m_path.string().c_str(), H5F_ACC_RDONLY,
                               H5P_DEFAULT);
    ASSERT_GE(file, 0);

    // Node x coordinates round-trip.
    {
        const hid_t dataset = H5Dopen2(file, "mesh2d_node_x", H5P_DEFAULT);
        ASSERT_GE(dataset, 0);
        double x[4] = {};
        H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, x);
        EXPECT_DOUBLE_EQ(x[1], 1.0);
        EXPECT_DOUBLE_EQ(x[3], 0.0);
        H5Dclose(dataset);
    }
    // Connectivity is [2, 3] with start_index 0 (fully packed triangles).
    {
        const hid_t dataset = H5Dopen2(file, "mesh2d_face_nodes", H5P_DEFAULT);
        ASSERT_GE(dataset, 0);
        const hid_t space = H5Dget_space(dataset);
        hsize_t dims[2] = {};
        H5Sget_simple_extent_dims(space, dims, nullptr);
        EXPECT_EQ(dims[0], 2u);
        EXPECT_EQ(dims[1], 3u);
        int64_t connectivity[6] = {};
        H5Dread(dataset, H5T_NATIVE_INT64, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                connectivity);
        EXPECT_EQ(connectivity[0], 0);
        EXPECT_EQ(connectivity[5], 3);
        EXPECT_EQ(readStringAttribute(dataset, "cf_role"),
                  "face_node_connectivity");
        H5Sclose(space);
        H5Dclose(dataset);
    }
    H5Fclose(file);
}

TEST_F(HDF5UGRIDWriterTest, AppendsTimeLevelsWithFieldTags)
{
    writeThreeLevels();

    const hid_t file = H5Fopen(m_path.string().c_str(), H5F_ACC_RDONLY,
                               H5P_DEFAULT);
    ASSERT_GE(file, 0);

    // /time grew to 3 by extend-and-write.
    {
        const hid_t dataset = H5Dopen2(file, "time", H5P_DEFAULT);
        ASSERT_GE(dataset, 0);
        const hid_t space = H5Dget_space(dataset);
        hsize_t dims[1] = {};
        H5Sget_simple_extent_dims(space, dims, nullptr);
        EXPECT_EQ(dims[0], 3u);
        double times[3] = {};
        H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                times);
        EXPECT_DOUBLE_EQ(times[2], 2451547.0);
        EXPECT_EQ(readStringAttribute(dataset, "calendar"), "julian");
        H5Sclose(space);
        H5Dclose(dataset);
    }
    // depth is [3, 4] tagged mesh/location; last row round-trips.
    {
        const hid_t dataset = H5Dopen2(file, "depth", H5P_DEFAULT);
        ASSERT_GE(dataset, 0);
        EXPECT_EQ(readStringAttribute(dataset, "mesh"), "mesh2d");
        EXPECT_EQ(readStringAttribute(dataset, "location"), "node");
        EXPECT_EQ(readStringAttribute(dataset, "units"), "m");
        const hid_t space = H5Dget_space(dataset);
        hsize_t dims[2] = {};
        H5Sget_simple_extent_dims(space, dims, nullptr);
        EXPECT_EQ(dims[0], 3u);
        EXPECT_EQ(dims[1], 4u);
        double values[12] = {};
        H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT,
                values);
        EXPECT_DOUBLE_EQ(values[8], 2.1);  // t=2, node 0
        EXPECT_DOUBLE_EQ(values[11], 2.4); // t=2, node 3
        H5Sclose(space);
        H5Dclose(dataset);
    }
    // flow is on faces.
    {
        const hid_t dataset = H5Dopen2(file, "flow", H5P_DEFAULT);
        ASSERT_GE(dataset, 0);
        EXPECT_EQ(readStringAttribute(dataset, "location"), "face");
        H5Dclose(dataset);
    }
    H5Fclose(file);
}

TEST_F(HDF5UGRIDWriterTest, MissingRegisteredFieldFailsWithMessage)
{
    HDF5UGRIDWriter writer(m_path, twoTriangleMesh());
    writer.addField("depth", MeshLocation::Node);
    ASSERT_EQ(writer.initialize(), 0) << writer.errorMessage();

    Snapshot snapshot(0.0); // no fields
    EXPECT_NE(writer.write(snapshot), 0);
    EXPECT_NE(writer.errorMessage().find("depth"), std::string::npos);
    (void)writer.finalize();
}

TEST_F(HDF5UGRIDWriterTest, WrongElementCountFailsWithMessage)
{
    HDF5UGRIDWriter writer(m_path, twoTriangleMesh());
    writer.addField("depth", MeshLocation::Node);
    ASSERT_EQ(writer.initialize(), 0) << writer.errorMessage();

    Snapshot snapshot(0.0);
    snapshot.addField(doubleField("depth", {1.0, 2.0})); // mesh has 4 nodes
    EXPECT_NE(writer.write(snapshot), 0);
    EXPECT_NE(writer.errorMessage().find("4"), std::string::npos);
    (void)writer.finalize();
}

TEST_F(HDF5UGRIDWriterTest, InvalidMeshFailsInitialize)
{
    MeshDefinition mesh = twoTriangleMesh();
    mesh.faceNodes[0] = -5;
    HDF5UGRIDWriter writer(m_path, mesh);
    EXPECT_NE(writer.initialize(), 0);
    EXPECT_FALSE(writer.errorMessage().empty());
}

TEST_F(HDF5UGRIDWriterTest, RunsBehindIOThread)
{
    auto writer = std::make_shared<HDF5UGRIDWriter>(
        artifactPath("io_ugrid_threaded.h5"), twoTriangleMesh());
    writer->addField("depth", MeshLocation::Node);

    IOThread io(4);
    io.addWriter(writer);
    ASSERT_TRUE(io.start());
    for (int step = 0; step < 5; ++step)
    {
        Snapshot snapshot(100.0 + step);
        snapshot.addField(doubleField("depth", {1.0, 2.0, 3.0, 4.0}));
        ASSERT_TRUE(io.enqueue(std::move(snapshot)));
    }
    io.finish();
    ASSERT_EQ(io.errorCode(), 0) << io.errorMessage();

    const hid_t file =
        H5Fopen(writer->path().string().c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    ASSERT_GE(file, 0);
    const hid_t dataset = H5Dopen2(file, "depth", H5P_DEFAULT);
    const hid_t space = H5Dget_space(dataset);
    hsize_t dims[2] = {};
    H5Sget_simple_extent_dims(space, dims, nullptr);
    EXPECT_EQ(dims[0], 5u);
    H5Sclose(space);
    H5Dclose(dataset);
    H5Fclose(file);
}
