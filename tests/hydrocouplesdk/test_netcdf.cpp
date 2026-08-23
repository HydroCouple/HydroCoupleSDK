// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for NetCDFUGRIDWriter: CF/UGRID structure, unlimited time appends,
// fill-value-padded connectivity, and error paths — verified by reopening
// the file with the NetCDF C API.

#include <gtest/gtest.h>

#include "hydrocouplesdk/io/netcdfugridwriter.h"
#include "hydrocouplesdk/io/iothread.h"

#include <netcdf.h>

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

MeshDefinition twoTriangleMesh()
{
    MeshDefinition mesh;
    mesh.meshName        = "mesh2d";
    mesh.nodeX           = {0.0, 1.0, 1.0, 0.0};
    mesh.nodeY           = {0.0, 0.0, 1.0, 1.0};
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

std::string readTextAttribute(int ncid, int varid, const char *attribute)
{
    size_t length = 0;
    if (nc_inq_attlen(ncid, varid, attribute, &length) != NC_NOERR)
        return {};
    std::string value(length, '\0');
    nc_get_att_text(ncid, varid, attribute, value.data());
    return value;
}

std::filesystem::path writeThreeLevels(const std::string &fileName)
{
    const auto path = artifactPath(fileName);
    std::filesystem::remove(path);

    NetCDFUGRIDWriter writer(path, twoTriangleMesh());
    writer.addField("depth", MeshLocation::Node, "m", "sea_floor_depth");
    writer.addField("flow", MeshLocation::Face, "m3 s-1");
    EXPECT_EQ(writer.initialize(), 0) << writer.errorMessage();

    for (int step = 0; step < 3; ++step)
    {
        Snapshot snapshot(2451545.0 + step);
        snapshot.addField(doubleField(
            "depth", {step + 0.1, step + 0.2, step + 0.3, step + 0.4}));
        snapshot.addField(doubleField("flow", {step * 10.0, step * 20.0}));
        EXPECT_EQ(writer.write(snapshot), 0) << writer.errorMessage();
    }
    EXPECT_EQ(writer.finalize(), 0) << writer.errorMessage();
    return path;
}

} // namespace

TEST(NetCDFUGRIDWriterTest, WritesConventionsAndTopology)
{
    const auto path = writeThreeLevels("io_ugrid.nc");

    int ncid = -1;
    ASSERT_EQ(nc_open(path.string().c_str(), NC_NOWRITE, &ncid), NC_NOERR);

    EXPECT_EQ(readTextAttribute(ncid, NC_GLOBAL, "Conventions"),
              "CF-1.11 UGRID-1.0");

    int topologyVar = -1;
    ASSERT_EQ(nc_inq_varid(ncid, "mesh2d", &topologyVar), NC_NOERR);
    EXPECT_EQ(readTextAttribute(ncid, topologyVar, "cf_role"),
              "mesh_topology");
    EXPECT_EQ(readTextAttribute(ncid, topologyVar, "face_node_connectivity"),
              "mesh2d_face_nodes");
    nc_close(ncid);
}

TEST(NetCDFUGRIDWriterTest, AppendsUnlimitedTimeAndFieldValues)
{
    const auto path = writeThreeLevels("io_ugrid.nc");

    int ncid = -1;
    ASSERT_EQ(nc_open(path.string().c_str(), NC_NOWRITE, &ncid), NC_NOERR);

    int timeDim = -1;
    size_t timeLength = 0;
    ASSERT_EQ(nc_inq_dimid(ncid, "time", &timeDim), NC_NOERR);
    ASSERT_EQ(nc_inq_dimlen(ncid, timeDim, &timeLength), NC_NOERR);
    EXPECT_EQ(timeLength, 3u);

    int depthVar = -1;
    ASSERT_EQ(nc_inq_varid(ncid, "depth", &depthVar), NC_NOERR);
    EXPECT_EQ(readTextAttribute(ncid, depthVar, "mesh"), "mesh2d");
    EXPECT_EQ(readTextAttribute(ncid, depthVar, "location"), "node");
    double depth[12] = {};
    ASSERT_EQ(nc_get_var_double(ncid, depthVar, depth), NC_NOERR);
    EXPECT_DOUBLE_EQ(depth[8], 2.1);  // t=2, node 0
    EXPECT_DOUBLE_EQ(depth[11], 2.4); // t=2, node 3

    int timeVar = -1;
    ASSERT_EQ(nc_inq_varid(ncid, "time", &timeVar), NC_NOERR);
    double times[3] = {};
    ASSERT_EQ(nc_get_var_double(ncid, timeVar, times), NC_NOERR);
    EXPECT_DOUBLE_EQ(times[2], 2451547.0);
    nc_close(ncid);
}

TEST(NetCDFUGRIDWriterTest, ConnectivityIsPackedWithStartIndexZero)
{
    const auto path = writeThreeLevels("io_ugrid.nc");

    int ncid = -1;
    ASSERT_EQ(nc_open(path.string().c_str(), NC_NOWRITE, &ncid), NC_NOERR);
    int faceNodesVar = -1;
    ASSERT_EQ(nc_inq_varid(ncid, "mesh2d_face_nodes", &faceNodesVar),
              NC_NOERR);
    long long connectivity[6] = {};
    ASSERT_EQ(nc_get_var_longlong(ncid, faceNodesVar, connectivity),
              NC_NOERR);
    EXPECT_EQ(connectivity[0], 0);
    EXPECT_EQ(connectivity[5], 3);
    EXPECT_EQ(readTextAttribute(ncid, faceNodesVar, "cf_role"),
              "face_node_connectivity");
    nc_close(ncid);
}

TEST(NetCDFUGRIDWriterTest, MissingRegisteredFieldFailsWithMessage)
{
    NetCDFUGRIDWriter writer(artifactPath("io_ugrid_missing.nc"),
                             twoTriangleMesh());
    writer.addField("depth", MeshLocation::Node);
    ASSERT_EQ(writer.initialize(), 0) << writer.errorMessage();
    Snapshot snapshot(0.0);
    EXPECT_NE(writer.write(snapshot), 0);
    EXPECT_NE(writer.errorMessage().find("depth"), std::string::npos);
    (void)writer.finalize();
}

TEST(NetCDFUGRIDWriterTest, InvalidMeshFailsInitialize)
{
    MeshDefinition mesh = twoTriangleMesh();
    mesh.faceNodes[0] = -3;
    NetCDFUGRIDWriter writer(artifactPath("io_ugrid_bad.nc"), mesh);
    EXPECT_NE(writer.initialize(), 0);
    EXPECT_FALSE(writer.errorMessage().empty());
}

TEST(NetCDFUGRIDWriterTest, RunsBehindIOThread)
{
    const auto path = artifactPath("io_ugrid_threaded.nc");
    std::filesystem::remove(path);
    auto writer =
        std::make_shared<NetCDFUGRIDWriter>(path, twoTriangleMesh());
    writer->addField("depth", MeshLocation::Node);

    IOThread io(4);
    io.addWriter(writer);
    ASSERT_TRUE(io.start());
    for (int step = 0; step < 5; ++step)
    {
        Snapshot snapshot(300.0 + step);
        snapshot.addField(doubleField("depth", {1.0, 2.0, 3.0, 4.0}));
        ASSERT_TRUE(io.enqueue(std::move(snapshot)));
    }
    io.finish();
    ASSERT_EQ(io.errorCode(), 0) << io.errorMessage();

    int ncid = -1;
    ASSERT_EQ(nc_open(path.string().c_str(), NC_NOWRITE, &ncid), NC_NOERR);
    int timeDim = -1;
    size_t timeLength = 0;
    nc_inq_dimid(ncid, "time", &timeDim);
    nc_inq_dimlen(ncid, timeDim, &timeLength);
    EXPECT_EQ(timeLength, 5u);
    nc_close(ncid);
}
