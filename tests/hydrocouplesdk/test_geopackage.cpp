// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for GeoPackageWriter: gpkg conformance tables and pragmas, node
// feature layer with GeoPackageBinary points, result time series with
// buffered transactional inserts, and finalize() aggregates — verified by
// reopening the file with the SQLite C API.

#include <gtest/gtest.h>

#include "hydrocouplesdk/io/geopackagewriter.h"
#include "hydrocouplesdk/io/iothread.h"

#include <sqlite3.h>

#include <cmath>
#include <cstring>
#include <filesystem>
#include <memory>

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

/*! Runs a scalar query returning a double (NAN on failure). */
double scalarQuery(sqlite3 *db, const std::string &sql)
{
    sqlite3_stmt *stmt = nullptr;
    double value = std::nan("");
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK &&
        sqlite3_step(stmt) == SQLITE_ROW)
        value = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);
    return value;
}

/*! Writer producing a mesh + two fields over three time levels. */
std::filesystem::path writeStandardPackage(const std::string &fileName,
                                           int flushRows = 5000)
{
    const auto path = artifactPath(fileName);
    std::filesystem::remove(path);

    GeoPackageWriter writer(path, "test-simulation", flushRows);
    writer.setMesh(twoTriangleMesh());
    writer.addField("depth", "m", "water depth at nodes");

    EXPECT_EQ(writer.initialize(), 0) << writer.errorMessage();
    for (int step = 0; step < 3; ++step)
    {
        Snapshot snapshot(100.0 + step);
        snapshot.addField(doubleField(
            "depth", {step + 0.1, step + 0.2, step + 0.3, step + 0.4}));
        snapshot.addField(doubleField("flow", {step * 10.0, step * 20.0}));
        EXPECT_EQ(writer.write(snapshot), 0) << writer.errorMessage();
    }
    EXPECT_EQ(writer.finalize(), 0) << writer.errorMessage();
    return path;
}

} // namespace

// ── Conformance ───────────────────────────────────────────────────────────

TEST(GeoPackageWriterTest, WritesGpkgConformance)
{
    const auto path = writeStandardPackage("io_results.gpkg");

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open_v2(path.string().c_str(), &db,
                              SQLITE_OPEN_READONLY, nullptr), SQLITE_OK);

    // 'GPKG' application id and mandatory SRS rows.
    EXPECT_EQ(static_cast<int64_t>(scalarQuery(db, "PRAGMA application_id")),
              0x47504B47);
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT COUNT(*) FROM gpkg_spatial_ref_sys "
                        "WHERE srs_id IN (-1, 0, 4326)"), 3.0);
    // Contents advertise the feature and attribute tables.
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT COUNT(*) FROM gpkg_contents "
                        "WHERE table_name='mesh_nodes' "
                        "AND data_type='features'"), 1.0);
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT COUNT(*) FROM gpkg_geometry_columns "
                        "WHERE table_name='mesh_nodes' "
                        "AND geometry_type_name='POINT'"), 1.0);
    // Database integrity.
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(db, "PRAGMA integrity_check", -1, &stmt,
                                 nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    EXPECT_STREQ(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)),
                 "ok");
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

TEST(GeoPackageWriterTest, NodeFeaturesCarryGeoPackageBinaryPoints)
{
    const auto path = writeStandardPackage("io_results.gpkg");

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open_v2(path.string().c_str(), &db,
                              SQLITE_OPEN_READONLY, nullptr), SQLITE_OK);

    EXPECT_DOUBLE_EQ(scalarQuery(db, "SELECT COUNT(*) FROM mesh_nodes"), 4.0);

    // Node 1 sits at (1, 0): check the GP header magic and the WKB payload.
    sqlite3_stmt *stmt = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(
                  db, "SELECT geom FROM mesh_nodes WHERE node_index=1", -1,
                  &stmt, nullptr), SQLITE_OK);
    ASSERT_EQ(sqlite3_step(stmt), SQLITE_ROW);
    const auto *blob =
        static_cast<const uint8_t *>(sqlite3_column_blob(stmt, 0));
    const int size = sqlite3_column_bytes(stmt, 0);
    ASSERT_EQ(size, 29);
    EXPECT_EQ(blob[0], 'G');
    EXPECT_EQ(blob[1], 'P');
    double x = 0.0, y = 0.0;
    std::memcpy(&x, blob + 13, 8);
    std::memcpy(&y, blob + 21, 8);
    EXPECT_DOUBLE_EQ(x, 1.0);
    EXPECT_DOUBLE_EQ(y, 0.0);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}

// ── Results ───────────────────────────────────────────────────────────────

TEST(GeoPackageWriterTest, WritesTimeSeriesAndAutoRegistersVariables)
{
    const auto path = writeStandardPackage("io_results.gpkg");

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open_v2(path.string().c_str(), &db,
                              SQLITE_OPEN_READONLY, nullptr), SQLITE_OK);

    // 3 levels × (4 depth + 2 flow) rows.
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT COUNT(*) FROM result_timeseries"), 18.0);
    // Pre-registered metadata survives; unknown field auto-registered.
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT COUNT(*) FROM variables "
                        "WHERE item_id='depth' AND units='m'"), 1.0);
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT COUNT(*) FROM variables "
                        "WHERE item_id='flow'"), 1.0);
    // Value spot check: depth node 3 at the last level is 2.4.
    EXPECT_DOUBLE_EQ(
        scalarQuery(db,
                    "SELECT value FROM result_timeseries r "
                    "JOIN variables v ON v.id=r.variable_id "
                    "WHERE v.item_id='depth' AND r.object_index=3 "
                    "AND r.elapsed_time=102.0"), 2.4);
    sqlite3_close(db);
}

TEST(GeoPackageWriterTest, FinalizeComputesSummaryAggregates)
{
    const auto path = writeStandardPackage("io_results.gpkg");

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open_v2(path.string().c_str(), &db,
                              SQLITE_OPEN_READONLY, nullptr), SQLITE_OK);

    // flow values: {0,0, 10,20, 20,40} → count 6, min 0, max 40, mean 15.
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT count FROM result_summary s "
                        "JOIN variables v ON v.id=s.variable_id "
                        "WHERE v.item_id='flow'"), 6.0);
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT max FROM result_summary s "
                        "JOIN variables v ON v.id=s.variable_id "
                        "WHERE v.item_id='flow'"), 40.0);
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT mean FROM result_summary s "
                        "JOIN variables v ON v.id=s.variable_id "
                        "WHERE v.item_id='flow'"), 15.0);
    sqlite3_close(db);
}

TEST(GeoPackageWriterTest, BufferedInsertsFlushAcrossTransactions)
{
    // Tiny flush threshold forces many transaction rotations; every row must
    // still land.
    const auto path = writeStandardPackage("io_results_flush.gpkg",
                                           /*flushRows=*/3);
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open_v2(path.string().c_str(), &db,
                              SQLITE_OPEN_READONLY, nullptr), SQLITE_OK);
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT COUNT(*) FROM result_timeseries"), 18.0);
    sqlite3_close(db);
}

TEST(GeoPackageWriterTest, InvalidMeshFailsInitialize)
{
    MeshDefinition mesh = twoTriangleMesh();
    mesh.faceNodes[0] = 99;
    GeoPackageWriter writer(artifactPath("io_bad.gpkg"));
    writer.setMesh(mesh);
    EXPECT_NE(writer.initialize(), 0);
    EXPECT_FALSE(writer.errorMessage().empty());
}

TEST(GeoPackageWriterTest, RunsBehindIOThread)
{
    const auto path = artifactPath("io_results_threaded.gpkg");
    std::filesystem::remove(path);

    auto writer = std::make_shared<GeoPackageWriter>(path, "threaded");
    IOThread io(4);
    io.addWriter(writer);
    ASSERT_TRUE(io.start());
    for (int step = 0; step < 5; ++step)
    {
        Snapshot snapshot(200.0 + step);
        snapshot.addField(doubleField("stage", {1.0, 2.0, 3.0}));
        ASSERT_TRUE(io.enqueue(std::move(snapshot)));
    }
    io.finish();
    ASSERT_EQ(io.errorCode(), 0) << io.errorMessage();

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open_v2(path.string().c_str(), &db,
                              SQLITE_OPEN_READONLY, nullptr), SQLITE_OK);
    EXPECT_DOUBLE_EQ(
        scalarQuery(db, "SELECT COUNT(*) FROM result_timeseries"), 15.0);
    sqlite3_close(db);
}
