// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for NetCDFUGRIDWriter: CF/UGRID structure, unlimited time appends,
// fill-value-padded connectivity, and error paths — verified by reopening
// the file with the NetCDF C API.

#include <gtest/gtest.h>

#include "hydrocouplesdk/io/netcdfugridwriter.h"
#include "hydrocouplesdk/io/iothread.h"
#include "hydrocouplesdk/io/resultreader.h"

#include <netcdf.h>

// Only to print HDF5's error stack when a NetCDF-4 file fails to open; the
// tests themselves never touch the HDF5 API. The test target links HDF5
// under the same condition.
#ifdef HYDROCOUPLESDK_HAVE_HDF5
#include <hdf5.h>
#endif

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>

using namespace HydroCouple;
using namespace HydroCouple::SDK::IO;

#ifndef TEST_ARTIFACTS_DIR
#define TEST_ARTIFACTS_DIR "."
#endif

namespace {

/*!
 * \brief Where a test writes \p name, under a path only that test uses.
 *
 * Namespaced by the running case, because ctest registers every gtest case
 * as a test of its own and `-jN` runs them as concurrent processes over this
 * one directory. Several cases here ask for the same file name and each
 * writes it before reading it back, so in parallel they truncated and
 * removed each other's files mid-read — which is why the parallel failing
 * set moved from run to run and the suite could only be judged at `-j 1`.
 *
 * Still a real path under tests/artifacts, so what a test checked can be
 * opened afterwards, and now it also says which case produced it.
 */
std::filesystem::path artifactPath(const std::string &name)
{
    const std::filesystem::path dir{TEST_ARTIFACTS_DIR};
    std::filesystem::create_directories(dir);

    // Null outside a running test (a suite-wide fixture, say); the shared
    // name is then the best available and no worse than before.
    if (const auto *info =
            ::testing::UnitTest::GetInstance()->current_test_info())
    {
        return dir / (std::string(info->test_suite_name()) + "." + info->name()
                      + "." + name);
    }

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

/*!
 * \brief Opens a file this suite just wrote, reporting *why* on failure.
 *
 * A bare ASSERT_EQ on nc_open prints only the numeric code, which for a
 * NetCDF-4 file is usually NC_EHDFERR (-101) -- "error at HDF5 layer" and
 * nothing more. The file exists and the writer reported success, so the
 * interesting question is what the netCDF and HDF5 layers have to say. This
 * prints nc_strerror plus the HDF5 error stack that netCDF suppresses by
 * default, so a CI log is enough to diagnose the failure.
 */
int openForRead(const std::filesystem::path &path, int &ncid)
{
    const int status = nc_open(path.string().c_str(), NC_NOWRITE, &ncid);
    if (status != NC_NOERR)
    {
        std::error_code ec;
        const auto size = std::filesystem::file_size(path, ec);
        ADD_FAILURE() << "nc_open(" << path.string() << ") -> " << status
                      << " (" << nc_strerror(status) << ")\n"
                      << "  file " << (ec ? "size unavailable"
                                          : "exists, " + std::to_string(size)
                                                + " bytes")
                      << "\n  netcdf " << nc_inq_libvers();
#ifdef HYDROCOUPLESDK_HAVE_HDF5
        std::fprintf(stderr, "  HDF5 error stack (if any):\n");
        H5Eprint2(H5E_DEFAULT, stderr);
#endif

        // The same file read from inside libHydroCoupleSDK rather than from
        // here. This suite once failed exactly that way round -- nc_open
        // returning NC_EHDFERR on a file the SDK's reader opened happily --
        // because the executable linked its own copy of the static netcdf
        // and hdf5 archives beside the copies already in the shared library,
        // and two copies of HDF5 do not share its globals. The test target
        // now takes those headers without their libraries, so there is one
        // instance and it is the one that wrote the file.
        //
        // The comparison stays because it is what told us that, and it is
        // the same question any future failure here raises:
        //
        //   SDK read OK  -> the file is fine and the bytes are on disk;
        //                   the failure belongs to *this* executable's
        //                   library instance, not to the writer. Check
        //                   whether a second netcdf/hdf5 has crept back
        //                   into the link ("ignoring duplicate libraries").
        //   SDK read bad -> the writer really did produce something
        //                   unreadable, and the flush/close path is the
        //                   place to look.
        std::string readerMessage;
        auto reader = HydroCouple::SDK::IO::createResultReader("netcdf_ugrid",
                                                               readerMessage);
        if (!reader)
        {
            std::fprintf(stderr, "  SDK reader unavailable: %s\n",
                         readerMessage.c_str());
        }
        else if (reader->open(path, readerMessage))
        {
            std::fprintf(stderr,
                         "  SDK reader opened the SAME file successfully.\n"
                         "  => the file is valid; a second netcdf/hdf5\n"
                         "     instance in this executable is the problem.\n"
                         "     Check the test target's link for a duplicate\n"
                         "     static netcdf/hdf5.\n");
        }
        else
        {
            std::fprintf(stderr,
                         "  SDK reader ALSO failed: %s\n"
                         "  => the written file is genuinely unreadable.\n",
                         readerMessage.c_str());
        }
    }
    return status;
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
    ASSERT_EQ(openForRead(path, ncid), NC_NOERR)
        << "nc_open failed on a file this suite just wrote";

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
    ASSERT_EQ(openForRead(path, ncid), NC_NOERR)
        << "nc_open failed on a file this suite just wrote";

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
    ASSERT_EQ(openForRead(path, ncid), NC_NOERR)
        << "nc_open failed on a file this suite just wrote";
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
    ASSERT_EQ(openForRead(path, ncid), NC_NOERR)
        << "nc_open failed on a file this suite just wrote";
    int timeDim = -1;
    size_t timeLength = 0;
    nc_inq_dimid(ncid, "time", &timeDim);
    nc_inq_dimlen(ncid, timeDim, &timeLength);
    EXPECT_EQ(timeLength, 5u);
    nc_close(ncid);
}
