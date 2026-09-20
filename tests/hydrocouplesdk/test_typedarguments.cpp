// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// U2-S: typed arguments answer their typed interfaces.
//
// An IArgument is an IComponentDataItem, so an argument that holds a time
// series or a mesh can say so rather than leaving a consumer to infer it
// from a rank-1 shape or a shape of { nodes, 3 }. A configurator reading
// the interface gets a time series; one reading only the shape gets a
// column of numbers and has to guess that the first axis is time. The
// guessing is what this removes.
//
// The static_asserts below are not decoration. -fsyntax-only will happily
// compile a class that still has a pure virtual — the error appears only
// where something instantiates one — and adding an interface to a class
// is precisely the change that leaves one behind.

#include <gtest/gtest.h>

#include "hydrocouplesdk/spatial/polyhedralsurfaceargument.h"
#include "hydrocouplesdk/temporal/timeseriesargument.h"

#include "hydrocouplespatial.h"
#include "hydrocoupletemporal.h"

#include <string>
#include <type_traits>
#include <vector>

using namespace HydroCouple::SDK;
using HydroCouple::SDK::IO::MeshDefinition;
using HydroCouple::SDK::Spatial::PolyhedralSurfaceArgument;
using HydroCouple::SDK::Temporal::TimeSeriesArgumentDouble;

// ── what the types are ──────────────────────────────────────────────────

static_assert(!std::is_abstract_v<TimeSeriesArgumentDouble>,
              "TimeSeriesArgumentDouble has an unimplemented pure virtual");

static_assert(!std::is_abstract_v<PolyhedralSurfaceArgument>,
              "PolyhedralSurfaceArgument has an unimplemented pure virtual");

static_assert(
    std::is_base_of_v<HydroCouple::Temporal::ITimeSeriesComponentDataItem,
                      TimeSeriesArgumentDouble>,
    "a time-series argument must answer ITimeSeriesComponentDataItem");

static_assert(
    std::is_base_of_v<
        HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem,
        PolyhedralSurfaceArgument>,
    "a mesh argument must answer IPolyhedralSurfaceComponentDataItem");

// Still arguments. Adding an interface must not cost the one they had.
static_assert(std::is_base_of_v<HydroCouple::IArgument,
                                TimeSeriesArgumentDouble>,
              "and must still be an argument");

static_assert(std::is_base_of_v<HydroCouple::IArgument,
                                PolyhedralSurfaceArgument>,
              "and must still be an argument");

namespace {

//! A square split into a triangle and a quad.
MeshDefinition smallMesh()
{
    MeshDefinition mesh;
    mesh.meshName = "small";
    mesh.nodeX = {0.0, 1.0, 1.0, 0.0};
    mesh.nodeY = {0.0, 0.0, 1.0, 1.0};
    mesh.nodeZ = {0.0, 0.0, 0.0, 0.0};
    mesh.faceNodeOffsets = {0, 3, 7};
    mesh.faceNodes = {0, 1, 2, 0, 2, 3, 0};

    return mesh;
}

std::string timeSeriesJson()
{
    return R"({"times":[2451545.0,2451546.0,2451547.0],
               "values":[1.0,2.0,3.0]})";
}

} // namespace

// ── the time series ─────────────────────────────────────────────────────

TEST(TypedArgumentTest, ATimeSeriesArgumentReportsItsTimes)
{
    Dimension timeDimension("time", "Time");
    TimeSeriesArgumentDouble argument("series", "Series", nullptr,
                                      &timeDimension);

    std::string message;
    ASSERT_TRUE(argument.initialize(
        timeSeriesJson(), HydroCouple::IArgument::ArgumentInputType::String,
        message))
        << message;

    auto *item =
        dynamic_cast<HydroCouple::Temporal::ITimeSeriesComponentDataItem *>(
            &argument);

    // The cast is the whole point: this is what a configurator does.
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->timeCount(), 3);
    ASSERT_EQ(item->times().size(), 3u);
    EXPECT_DOUBLE_EQ(item->times()[0], 2451545.0);
    EXPECT_DOUBLE_EQ(item->times()[2], 2451547.0);

    EXPECT_EQ(item->timeDimension(), &timeDimension);

    ASSERT_NE(item->time(0), nullptr);
    EXPECT_DOUBLE_EQ(item->time(0)->julianDay(), 2451545.0);
    EXPECT_DOUBLE_EQ(item->time(2)->julianDay(), 2451547.0);

    // Out of range answers nothing rather than the nearest thing.
    EXPECT_EQ(item->time(-1), nullptr);
    EXPECT_EQ(item->time(3), nullptr);

    ASSERT_NE(item->timeSpan(), nullptr);
    EXPECT_DOUBLE_EQ(item->timeSpan()->duration(), 2.0);
}

TEST(TypedArgumentTest, ReloadingASeriesDoesNotHandOutTheOldDates)
{
    // The cache validates itself per slot rather than being invalidated by
    // whichever loader ran, because there are four loaders and a fifth
    // would silently skip the invalidation. This is the gate that would
    // fail if that validation were ever replaced by a size check alone:
    // both series below have three times.
    Dimension timeDimension("time", "Time");
    TimeSeriesArgumentDouble argument("series", "Series", nullptr,
                                      &timeDimension);

    std::string message;
    ASSERT_TRUE(argument.initialize(
        timeSeriesJson(), HydroCouple::IArgument::ArgumentInputType::String,
        message))
        << message;

    auto *item =
        dynamic_cast<HydroCouple::Temporal::ITimeSeriesComponentDataItem *>(
            &argument);
    ASSERT_NE(item, nullptr);

    // Read first, so a cache exists to go stale.
    ASSERT_NE(item->time(1), nullptr);
    EXPECT_DOUBLE_EQ(item->time(1)->julianDay(), 2451546.0);
    const double firstSpan = item->timeSpan()->duration();

    ASSERT_TRUE(argument.initialize(
        R"({"times":[2460000.0,2460010.0,2460020.0],
            "values":[9.0,8.0,7.0]})",
        HydroCouple::IArgument::ArgumentInputType::String, message))
        << message;

    EXPECT_EQ(item->timeCount(), 3);
    ASSERT_NE(item->time(1), nullptr);
    EXPECT_DOUBLE_EQ(item->time(1)->julianDay(), 2460010.0)
        << "a reloaded series handed out the previous series' dates";

    EXPECT_NE(item->timeSpan()->duration(), firstSpan);
    EXPECT_DOUBLE_EQ(item->timeSpan()->duration(), 20.0);
}

TEST(TypedArgumentTest, AnEmptySeriesCoversNothingRatherThanRefusingToSay)
{
    Dimension timeDimension("time", "Time");
    TimeSeriesArgumentDouble argument("series", "Series", nullptr,
                                      &timeDimension);

    auto *item =
        dynamic_cast<HydroCouple::Temporal::ITimeSeriesComponentDataItem *>(
            &argument);
    ASSERT_NE(item, nullptr);

    EXPECT_EQ(item->timeCount(), 0);
    EXPECT_TRUE(item->times().empty());
    EXPECT_EQ(item->time(0), nullptr);

    // A span rather than a null pointer: a caller asking what an unloaded
    // series covers should get "nothing", not something to test for.
    ASSERT_NE(item->timeSpan(), nullptr);
    EXPECT_DOUBLE_EQ(item->timeSpan()->duration(), 0.0);
}

// ── the surface ─────────────────────────────────────────────────────────

TEST(TypedArgumentTest, AMeshArgumentReportsItsSurface)
{
    PolyhedralSurfaceArgument argument("mesh", nullptr);

    std::string message;
    ASSERT_TRUE(argument.setMesh(smallMesh(), message)) << message;

    auto *item = dynamic_cast<
        HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem *>(
        &argument);
    ASSERT_NE(item, nullptr);

    HydroCouple::Spatial::IPolyhedralSurface *surface =
        item->polyhedralSurface();
    ASSERT_NE(surface, nullptr);

    // Faces, not just a point cloud. A surface built without the
    // connectivity reports none, and that reads as an empty mesh rather
    // than as a missing feature.
    EXPECT_EQ(surface->patchCount(), 2);
    EXPECT_EQ(surface->vertexCount(), 4);

    // The vertex axis is the argument's own node dimension, not a second
    // one that could disagree about how many vertices there are.
    ASSERT_NE(item->vertexDimension(), nullptr);
    EXPECT_EQ(item->vertexDimension()->id(), "node");

    ASSERT_NE(item->patchDimension(), nullptr);
    ASSERT_NE(item->edgeDimension(), nullptr);
    EXPECT_NE(item->patchDimension(), item->edgeDimension());

    EXPECT_EQ(item->meshDataObjectType(),
              HydroCouple::Spatial::MeshDataObjectType::Vertex);
    EXPECT_EQ(item->meshDataType(),
              HydroCouple::Spatial::SpatialDataType::Vector);
}

TEST(TypedArgumentTest, NoMeshIsNotAnEmptySurface)
{
    PolyhedralSurfaceArgument argument("mesh", nullptr);

    auto *item = dynamic_cast<
        HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem *>(
        &argument);
    ASSERT_NE(item, nullptr);

    // A caller handed an empty surface would report zero faces as a fact
    // about the argument rather than as "nothing loaded yet", and act on
    // it — by drawing nothing, and by saying the mesh is degenerate.
    EXPECT_EQ(item->polyhedralSurface(), nullptr);
}

TEST(TypedArgumentTest, ReplacingAMeshReplacesTheSurfaceItBuilt)
{
    // Every assignment to the mesh goes through adoptMesh() so that this
    // cannot be forgotten at one of the four call sites. Without that,
    // the second surface below would still describe the first mesh — a
    // stale answer rather than a crash, which is the kind of bug that
    // survives a long time.
    PolyhedralSurfaceArgument argument("mesh", nullptr);

    std::string message;
    ASSERT_TRUE(argument.setMesh(smallMesh(), message)) << message;

    auto *item = dynamic_cast<
        HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem *>(
        &argument);
    ASSERT_NE(item, nullptr);

    ASSERT_NE(item->polyhedralSurface(), nullptr);
    EXPECT_EQ(item->polyhedralSurface()->patchCount(), 2);

    MeshDefinition single;
    single.meshName = "single";
    single.nodeX = {0.0, 2.0, 2.0};
    single.nodeY = {0.0, 0.0, 2.0};
    single.nodeZ = {0.0, 0.0, 0.0};
    single.faceNodeOffsets = {0, 3};
    single.faceNodes = {0, 1, 2};

    ASSERT_TRUE(argument.setMesh(single, message)) << message;

    ASSERT_NE(item->polyhedralSurface(), nullptr);
    EXPECT_EQ(item->polyhedralSurface()->patchCount(), 1)
        << "the surface still describes the mesh that was replaced";
    EXPECT_EQ(item->polyhedralSurface()->vertexCount(), 3);
}
