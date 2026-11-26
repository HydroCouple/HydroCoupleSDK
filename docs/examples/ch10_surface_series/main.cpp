// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 10: a mesh as a polyhedral surface, its bulk mesh view, and
 *         face values that vary in time in a
 *         TimeSeriesPolyhedralSurfaceComponentDataItem; a TIN value.
 *
 * The mesh is a 10 m square cell with a triangle on top (the layout the SDK's
 * UGRID tests use). Exits nonzero when any value is not the one the guide
 * states.
 */

#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplesdk/spatial/meshadapters.h"
#include "hydrocouplesdk/spatial/tin.h"
#include "hydrocouplesdk/spatiotemporal/timepolyhedralsurfacecomponentdataitem.h"
#include "hydrocouplesdk/temporal/timedata.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple::SDK::Spatial;
namespace HCS = HydroCouple::Spatial;
using HydroCouple::SDK::IO::MeshDefinition;
using HydroCouple::SDK::SpatioTemporal::TimeSeriesPolyhedralSurfaceComponentDataItemDouble;
using HydroCouple::SDK::Temporal::TimeData;

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

} // namespace

int main()
{
    //! [mesh]
    // Node coordinates (m) and bed elevations; faces counterclockwise.
    MeshDefinition mesh;
    mesh.meshName = "floodplain";
    mesh.nodeX = {0.0, 10.0, 10.0, 0.0, 5.0};
    mesh.nodeY = {0.0, 0.0, 10.0, 10.0, 20.0};
    mesh.nodeZ = {1.0, 1.0, 1.5, 1.5, 2.0};
    mesh.faceNodeOffsets = {0, 4, 7};        // face 0: 4 nodes, face 1: 3
    mesh.faceNodes = {0, 1, 2, 3, 3, 2, 4};  // a quad, then a triangle
    std::string problem;
    if (!mesh.validate(problem))
    {
        std::printf("FAILED: mesh: %s\n", problem.c_str());
        return 1;
    }

    // Patches and vertices for drawing and picking, plus the connectivity
    // for the bulk view: the second constructor keeps both.
    PolyhedralSurfaceAdapter surface(polyhedralSurfaceFromMesh(mesh), mesh);
    const HCS::IMeshView *view = surface.meshView();
    //! [mesh]

    std::printf("surface: %lld patches, %lld vertices, area %.1f m2, patch 1 ring %lld points\n",
                static_cast<long long>(surface.patchCount()),
                static_cast<long long>(surface.vertexCount()), surface.area(),
                static_cast<long long>(surface.patch(1)->exteriorRing()->pointCount()));
    std::printf("view: %lld nodes, %lld faces, %lld edges (none were given)\n",
                static_cast<long long>(view->nodeCount()),
                static_cast<long long>(view->faceCount()),
                static_cast<long long>(view->edgeCount()));
    for (int64_t e = 0; e < view->edgeCount(); ++e)
        std::printf("  edge %lld: nodes %lld-%lld, left face %lld, right face %lld, "
                    "length %.3f, normal (%.3f, %.3f)\n",
                    static_cast<long long>(e), static_cast<long long>(view->edgeNodes()[2 * e]),
                    static_cast<long long>(view->edgeNodes()[2 * e + 1]),
                    static_cast<long long>(view->edgeFaces()[2 * e]),
                    static_cast<long long>(view->edgeFaces()[2 * e + 1]),
                    view->edgeLengths()[e], view->edgeNormalX()[e] + 0.0, view->edgeNormalY()[e] + 0.0);
    for (int64_t f = 0; f < view->faceCount(); ++f)
        std::printf("  face %lld: area %.1f, centroid (%.3f, %.3f)\n", static_cast<long long>(f),
                    view->faceAreas()[f], view->faceX()[f], view->faceY()[f]);

    check(surface.patch(1)->polyhedralSurface() == &surface, "patches name their surface");
    check(near(surface.area(), 150.0), "surface area");
    check(view->edgeCount() == 6, "four quad edges plus two more from the triangle");
    check(view->edgeFaces()[2 * 2] == 0 && view->edgeFaces()[2 * 2 + 1] == 1,
          "the shared edge has the quad on its left and the triangle on its right");
    check(view->edgeFaces()[2 * 0 + 1] == -1, "a boundary edge has -1 on its outer side");
    check(near(view->faceAreas()[1], 50.0) && near(view->faceY()[1], 40.0 / 3.0),
          "triangle area and centroid");
    check(near(view->edgeNormalX()[2], 0.0) && near(view->edgeNormalY()[2], 1.0),
          "the shared edge's normal points from the quad into the triangle");

    //! [series]
    // Water depth (m) on each face at three times.
    TimeSeriesPolyhedralSurfaceComponentDataItemDouble depth(
        "depth", &surface, HCS::MeshLocation::Face, HCS::SpatialDataType::Scalar,
        static_cast<int>(view->faceCount()));
    depth.setTimeKind(HydroCouple::Temporal::TimeKind::Instantaneous);
    depth.addTimeStep(std::make_shared<TimeData>("t0", 2026, 6, 1, 0), {0.00, 0.00});
    depth.addTimeStep(std::make_shared<TimeData>("t1", 2026, 6, 1, 1), {0.20, 0.05});
    depth.addTimeStep(std::make_shared<TimeData>("t2", 2026, 6, 1, 2), {0.35, 0.10});

    // Stored water at each time: depth times the face areas the view
    // publishes, so both sides of a coupling integrate over the same areas.
    std::vector<double> stored;
    for (int64_t t = 0; t < depth.timeCount(); ++t)
    {
        std::array<double, 2> row{};
        const std::array<int64_t, 2> start{t, 0}, count{1, 2};
        const std::array<int64_t, 1> bufferShape{2};
        std::string message;
        if (!depth.getValuesInto(HydroCouple::SDK::detail::hostDescriptor(
                                     row.data(), HydroCouple::DataKind::Float64, bufferShape),
                                 start, count, &message))
        {
            std::printf("FAILED: %s\n", message.c_str());
            return 1;
        }
        double volume = 0.0;
        for (int64_t f = 0; f < view->faceCount(); ++f)
            volume += row[static_cast<size_t>(f)] * view->faceAreas()[f];
        stored.push_back(volume);
    }
    //! [series]

    std::printf("depth item: dimensions \"%s\" x \"%s\", shape {%lld, %lld}, "
                "stored water %.1f, %.1f, %.1f m3\n",
                depth.timeDimension()->id().c_str(), depth.entityDimension()->id().c_str(),
                static_cast<long long>(depth.shape()[0]),
                static_cast<long long>(depth.shape()[1]), stored[0], stored[1], stored[2]);
    check(depth.entityDimension()->id() == "faces", "face values get a faces dimension");
    check(near(stored[1], 22.5) && near(stored[2], 40.0), "stored water");
    check(depth.polyhedralSurface() == &surface, "the item borrows the surface");

    //! [tin]
    // The same square as a TIN of two triangles sharing the vertex list.
    TIN tin;
    const int a = tin.addVertex(Point(0.0, 0.0, 1.0));
    const int b = tin.addVertex(Point(10.0, 0.0, 1.0));
    const int c = tin.addVertex(Point(10.0, 10.0, 1.5));
    const int d = tin.addVertex(Point(0.0, 10.0, 1.5));
    tin.addTriangleByIndex(a, b, c);
    tin.addTriangleByIndex(a, c, d);
    const int refused = tin.addTriangleByIndex(a, b, 9);
    const auto corners = tin.triangleVertices(1);
    //! [tin]

    std::printf("tin: %d triangles, %d edges, valid %d, area %.1f, mean triangle %.1f, "
                "triangle 1 corners %d %d %d, refused index %d\n",
                tin.numTriangles(), tin.numEdges(), tin.isValid(), tin.area(),
                tin.averageTriangleArea(), corners[0], corners[1], corners[2], refused);
    check(tin.numEdges() == 5 && tin.isValid() && near(tin.area(), 100.0), "tin statistics");
    check(corners[0] == a && corners[1] == c && corners[2] == d, "corners recovered by position");
    check(refused == -1 && tin.numTriangles() == 2, "a missing vertex index is refused");

    std::printf("%s\n", failures == 0 ? "all checks passed" : "checks FAILED");
    return failures == 0 ? 0 : 1;
}
