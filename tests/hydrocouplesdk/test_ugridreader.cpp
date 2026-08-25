// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for readUGRIDMesh: the counterpart the writers lacked. The fixtures
// are written by the SDK's own NetCDFUGRIDWriter, so a round trip proves the
// pair agree — and the index-convention tests are written with the NetCDF C
// API directly, because a file this codebase produced can only ever exercise
// the conventions this codebase chose.

#include <gtest/gtest.h>

#include "hydrocouplesdk/io/netcdfugridwriter.h"
#include "hydrocouplesdk/io/ugridreader.h"

#include <netcdf.h>

#include <filesystem>
#include <string>
#include <vector>

using namespace HydroCouple::SDK::IO;

#ifndef TEST_ARTIFACTS_DIR
#define TEST_ARTIFACTS_DIR "."
#endif

namespace {

std::filesystem::path artifact(const std::string &name)
{
    const std::filesystem::path dir{TEST_ARTIFACTS_DIR};
    std::error_code code;
    std::filesystem::create_directories(dir, code);

    return dir / name;
}

//! A square split into a triangle and a quad, so faces differ in width.
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

} // namespace

TEST(UGRIDReaderTest, ReportsSupportInThisBuild)
{
    // NetCDF is optional at build time, and this test target exists only
    // when it is on.
    EXPECT_TRUE(ugridReadSupported());
}

TEST(UGRIDReaderTest, RoundTripsAMeshThroughTheWriter)
{
    const std::filesystem::path path = artifact("ugrid_roundtrip.nc");
    std::filesystem::remove(path);

    const MeshDefinition original = mixedMesh();

    {
        NetCDFUGRIDWriter writer(path, original);
        ASSERT_EQ(writer.initialize(), 0);
        writer.finalize();
    }

    std::string message;
    MeshDefinition read;

    ASSERT_TRUE(readUGRIDMesh(path, "", read, message)) << message;

    EXPECT_EQ(read.meshName, original.meshName);
    EXPECT_EQ(read.nodeCount(), original.nodeCount());
    EXPECT_EQ(read.faceCount(), original.faceCount());
    EXPECT_EQ(read.edgeCount(), original.edgeCount());

    EXPECT_EQ(read.nodeX, original.nodeX);
    EXPECT_EQ(read.nodeY, original.nodeY);
    EXPECT_EQ(read.nodeZ, original.nodeZ);

    // The connectivity survives the rectangular on-disk form: a triangle
    // stored in a four-wide array comes back three nodes long, not four.
    EXPECT_EQ(read.faceNodeOffsets, original.faceNodeOffsets);
    EXPECT_EQ(read.faceNodes, original.faceNodes);
    EXPECT_EQ(read.edgeNodes, original.edgeNodes);
}

TEST(UGRIDReaderTest, ListsTheMeshesAFileHolds)
{
    const std::filesystem::path path = artifact("ugrid_names.nc");
    std::filesystem::remove(path);

    {
        NetCDFUGRIDWriter writer(path, mixedMesh());
        ASSERT_EQ(writer.initialize(), 0);
        writer.finalize();
    }

    std::string message;
    const std::vector<std::string> names = ugridMeshNames(path, message);

    ASSERT_EQ(names.size(), 1u) << message;
    EXPECT_EQ(names.front(), "mixed");
}

TEST(UGRIDReaderTest, ReadsANodeOnlyMesh)
{
    const std::filesystem::path path = artifact("ugrid_nodes.nc");
    std::filesystem::remove(path);

    MeshDefinition cloud;
    cloud.meshName = "cloud";
    cloud.nodeX = {1.0, 2.0, 3.0};
    cloud.nodeY = {4.0, 5.0, 6.0};

    {
        NetCDFUGRIDWriter writer(path, cloud);
        ASSERT_EQ(writer.initialize(), 0);
        writer.finalize();
    }

    std::string message;
    MeshDefinition read;

    // A node-only mesh is valid — a point cloud, or the nodes of a 1-D
    // network — and must not be mistaken for a malformed one.
    ASSERT_TRUE(readUGRIDMesh(path, "", read, message)) << message;

    EXPECT_EQ(read.nodeCount(), 3);
    EXPECT_EQ(read.faceCount(), 0);
    EXPECT_EQ(read.edgeCount(), 0);
}

TEST(UGRIDReaderTest, ConvertsAOneBasedFileToZeroBased)
{
    // Written by hand with the NetCDF API, because our own writer always
    // emits start_index 0 — a file it produced could never exercise this.
    //
    // Classic netCDF-3, deliberately: it needs no HDF5, so this fixture is
    // written by the same library the reader uses, and it also proves the
    // reader is not tied to the NetCDF-4 files the SDK's writer produces.
    const std::filesystem::path path = artifact("ugrid_one_based.nc");
    std::filesystem::remove(path);

    int ncid = -1;
    ASSERT_EQ(nc_create(path.string().c_str(), NC_CLOBBER, &ncid), NC_NOERR);

    int nodeDim = -1;
    int faceDim = -1;
    int cornerDim = -1;
    ASSERT_EQ(nc_def_dim(ncid, "node", 4, &nodeDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "face", 1, &faceDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "corner", 4, &cornerDim), NC_NOERR);

    int topologyVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "grid", NC_INT, 0, nullptr, &topologyVar),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "cf_role", 13,
                              "mesh_topology"),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "node_coordinates", 13,
                              "node_x node_y"),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "face_node_connectivity", 10,
                              "face_nodes"),
              NC_NOERR);

    int nodeXVar = -1;
    int nodeYVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "node_x", NC_DOUBLE, 1, &nodeDim, &nodeXVar),
              NC_NOERR);
    ASSERT_EQ(nc_def_var(ncid, "node_y", NC_DOUBLE, 1, &nodeDim, &nodeYVar),
              NC_NOERR);

    // NC_INT, not NC_INT64: classic netCDF has no 64-bit integer type, and a
    // reader that only understood the widest one would fail on every file
    // written before NetCDF-4.
    const int faceDims[2] = {faceDim, cornerDim};
    int faceVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "face_nodes", NC_INT, 2, faceDims, &faceVar),
              NC_NOERR);

    const int startIndex = 1;
    ASSERT_EQ(nc_put_att_int(ncid, faceVar, "start_index", NC_INT, 1,
                             &startIndex),
              NC_NOERR);

    const int fill = -999;
    ASSERT_EQ(nc_put_att_int(ncid, faceVar, "_FillValue", NC_INT, 1, &fill),
              NC_NOERR);

    ASSERT_EQ(nc_enddef(ncid), NC_NOERR);

    const double xs[4] = {0.0, 1.0, 1.0, 0.0};
    const double ys[4] = {0.0, 0.0, 1.0, 1.0};
    ASSERT_EQ(nc_put_var_double(ncid, nodeXVar, xs), NC_NOERR);
    ASSERT_EQ(nc_put_var_double(ncid, nodeYVar, ys), NC_NOERR);

    // One triangle in a four-wide array: nodes 1, 2, 3 one-based, then the
    // fill value.
    const int faces[4] = {1, 2, 3, fill};
    ASSERT_EQ(nc_put_var_int(ncid, faceVar, faces), NC_NOERR);

    ASSERT_EQ(nc_close(ncid), NC_NOERR);

    std::string message;
    MeshDefinition read;

    ASSERT_TRUE(readUGRIDMesh(path, "", read, message)) << message;

    EXPECT_EQ(read.meshName, "grid");
    EXPECT_EQ(read.nodeCount(), 4);
    ASSERT_EQ(read.faceCount(), 1);

    // Converted once, here: an off-by-one in connectivity draws a mesh that
    // is subtly wrong everywhere rather than obviously wrong anywhere.
    const std::vector<int64_t> expected = {0, 1, 2};
    EXPECT_EQ(read.faceNodes, expected)
        << "the file's one-based indices were not converted";
    EXPECT_EQ(read.faceNodeOffsets.back(), 3)
        << "the fill padding was read as a node";
}

TEST(UGRIDReaderTest, ExplainsWhatItCannotRead)
{
    std::string message;
    MeshDefinition mesh;

    EXPECT_FALSE(readUGRIDMesh(artifact("does_not_exist.nc"), "", mesh,
                               message));
    EXPECT_FALSE(message.empty()) << "a failure with no explanation";

    // A file with no topology variable is not a UGRID file, and saying so is
    // more useful than returning an empty mesh.
    const std::filesystem::path bare = artifact("ugrid_bare.nc");
    std::filesystem::remove(bare);

    int ncid = -1;
    ASSERT_EQ(nc_create(bare.string().c_str(), NC_CLOBBER, &ncid), NC_NOERR);
    ASSERT_EQ(nc_close(ncid), NC_NOERR);

    message.clear();
    EXPECT_FALSE(readUGRIDMesh(bare, "", mesh, message));
    EXPECT_FALSE(message.empty());

    // Naming a mesh the file does not hold must not silently read another.
    message.clear();
    const std::filesystem::path path = artifact("ugrid_roundtrip.nc");

    if (std::filesystem::exists(path))
    {
        EXPECT_FALSE(readUGRIDMesh(path, "no_such_mesh", mesh, message));
        EXPECT_FALSE(message.empty());
    }
}
