// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 18: meshing with HydroCoupleTools. A boundary-fitted
 *         curvilinear grid with Winslow smoothing, a constrained Delaunay
 *         triangulation with a hole and a breakline, its quad-dominant
 *         conversion, and a watched, canceled run; every result is checked.
 */

#include "hydrocouplesdk/tools/curvilineargrid.h"
#include "hydrocouplesdk/tools/quadmesher.h"
#include "hydrocouplesdk/tools/triangulator.h"

#include <cmath>
#include <cstdio>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Tools;
using HydroCouple::SDK::Spatial::Point;

namespace {

constexpr double kPi = 3.14159265358979323846;

int fail(const std::string &what)
{
    std::fprintf(stderr, "FAILED: %s\n", what.c_str());
    return 1;
}

//! Signed area of face f (positive: counter-clockwise).
double signedArea(const IO::MeshDefinition &mesh, int64_t f)
{
    const int64_t begin = mesh.faceNodeOffsets[f], end = mesh.faceNodeOffsets[f + 1];
    double area = 0.0;
    for (int64_t i = begin; i < end; ++i)
    {
        const int64_t a = mesh.faceNodes[i], b = mesh.faceNodes[i + 1 == end ? begin : i + 1];
        area += mesh.nodeX[a] * mesh.nodeY[b] - mesh.nodeX[b] * mesh.nodeY[a];
    }
    return area / 2.0;
}

//! Distinct undirected edges, for the Euler characteristic V - E + F.
int64_t edgeCount(const IO::MeshDefinition &mesh)
{
    std::set<std::pair<int64_t, int64_t>> edges;
    for (int64_t f = 0; f < mesh.faceCount(); ++f)
    {
        const int64_t begin = mesh.faceNodeOffsets[f], end = mesh.faceNodeOffsets[f + 1];
        for (int64_t i = begin; i < end; ++i)
        {
            const int64_t a = mesh.faceNodes[i], b = mesh.faceNodes[i + 1 == end ? begin : i + 1];
            edges.emplace(std::min(a, b), std::max(a, b));
        }
    }
    return static_cast<int64_t>(edges.size());
}

} // namespace

int main()
{
    std::string message;

    //! [curvilinear]
    // A channel bend: a quarter annulus between radii 10 m and 20 m. ξ runs
    // across the channel (south: the radius at angle 0), η along it.
    const int ni = 6, nj = 13;
    std::vector<Point> south, north, west, east;
    for (int i = 0; i < ni; ++i)
    {
        const double r = 10.0 + 10.0 * i / (ni - 1);
        south.emplace_back(r, 0.0);
        north.emplace_back(0.0, r);
    }
    for (int j = 0; j < nj; ++j)
    {
        const double angle = 0.5 * kPi * j / (nj - 1);
        west.emplace_back(10.0 * std::cos(angle), 10.0 * std::sin(angle));
        east.emplace_back(20.0 * std::cos(angle), 20.0 * std::sin(angle));
    }
    StructuredGrid2D bend;
    if (!CurvilinearGridGenerator::transfinite(south, north, west, east, bend, message))
        return fail(message);
    const double jacobian0 = CurvilinearGridGenerator::minCornerJacobian(bend);
    const double skew0 = CurvilinearGridGenerator::maxOrthogonalityDeviation(bend);

    // Disturb the interior, then let Winslow smoothing repair it.
    StructuredGrid2D disturbed = bend;
    for (int j = 1; j < nj - 1; ++j)
        for (int i = 1; i < ni - 1; ++i)
            disturbed.X(i, j) += 0.4 * ((i * 7 + j * 3) % 5 - 2);
    const double skewDisturbed = CurvilinearGridGenerator::maxOrthogonalityDeviation(disturbed);
    const double lastMove = CurvilinearGridGenerator::winslowSmooth(disturbed, 300);
    const double skewSmoothed = CurvilinearGridGenerator::maxOrthogonalityDeviation(disturbed);
    std::printf("transfinite: min corner Jacobian %.4f m2, max skew %.3f deg\n", jacobian0, skew0);
    std::printf("disturbed: max skew %.3f deg; after 300 Winslow sweeps %.3f deg "
                "(last move %.2e m)\n", skewDisturbed, skewSmoothed, lastMove);
    if (!(jacobian0 > 0.0) || !(skewSmoothed < skewDisturbed) ||
        !(CurvilinearGridGenerator::minCornerJacobian(disturbed) > 0.0))
        return fail("curvilinear grid quality");

    const IO::MeshDefinition quads = CurvilinearGridGenerator::toMeshDefinition(bend, "bend");
    for (int64_t f = 0; f < quads.faceCount(); ++f)
        if (!(signedArea(quads, f) > 0.0))
            return fail("a quad is not counter-clockwise");
    std::printf("bend mesh: %lld nodes, %lld quads\n",
                static_cast<long long>(quads.nodeCount()),
                static_cast<long long>(quads.faceCount()));
    //! [curvilinear]

    //! [triangulate]
    // A 40 m x 20 m floodplain with a building (a hole) and a levee crest
    // (a breakline the triangles must not cross).
    TriangulationInput plain;
    plain.boundary = {{0, 0}, {40, 0}, {40, 20}, {0, 20}};
    plain.holes = {{{10, 8}, {14, 8}, {14, 12}, {10, 12}}};
    plain.constraintSegments = {{Point(20, 2), Point(30, 18)}};
    plain.maxEdgeLength = 4.0;
    IO::MeshDefinition triangles;
    if (!Triangulator::triangulate(plain, triangles, message))
        return fail(message);
    // One hole: V - E + F = 1 - holes = 0.
    const int64_t euler = triangles.nodeCount() - edgeCount(triangles) + triangles.faceCount();
    std::printf("triangulation: %lld nodes, %lld triangles, V - E + F = %lld\n",
                static_cast<long long>(triangles.nodeCount()),
                static_cast<long long>(triangles.faceCount()), static_cast<long long>(euler));
    if (euler != 0)
        return fail("the Euler characteristic of a domain with one hole is 0");
    for (int64_t f = 0; f < triangles.faceCount(); ++f)
        if (!(signedArea(triangles, f) > 0.0))
            return fail("a triangle is not counter-clockwise");
    //! [triangulate]

    //! [quads]
    IO::MeshDefinition mixed;
    if (!QuadMesher::triToQuadDominant(triangles, /*minQuality=*/0.5, mixed, message))
        return fail(message);
    int64_t quadCount = 0;
    for (int64_t f = 0; f < mixed.faceCount(); ++f)
        quadCount += mixed.faceNodeOffsets[f + 1] - mixed.faceNodeOffsets[f] == 4;
    std::printf("quad-dominant: %lld quads + %lld triangles from %lld triangles\n",
                static_cast<long long>(quadCount),
                static_cast<long long>(mixed.faceCount() - quadCount),
                static_cast<long long>(triangles.faceCount()));
    if (2 * quadCount + (mixed.faceCount() - quadCount) != triangles.faceCount())
        return fail("every merge replaces exactly two triangles");
    // Corners counter-clockwise; the same rhombus listed clockwise scores 0.
    const double square  = QuadMesher::quadQuality(0, 0, 1, 0, 1, 1, 0, 1);
    const double rhombus = QuadMesher::quadQuality(0, 0, 2, -1, 4, 0, 2, 1);
    const double reversed = QuadMesher::quadQuality(0, 0, 2, 1, 4, 0, 2, -1);
    std::printf("quality: unit square %.3f, 4 x 2 rhombus %.3f, same rhombus clockwise %.3f\n",
                square, rhombus, reversed);
    if (square != 1.0 || std::abs(rhombus - (1.0 - (90.0 - 2.0 * std::atan(0.5) * 180.0 / kPi) / 90.0)) > 1e-12 ||
        reversed != 0.0)
        return fail("quad quality");
    //! [quads]

    //! [progress]
    // Watch a run, then stop one: a canceled run hands back nothing.
    int calls = 0;
    std::string lastStage;
    IO::MeshDefinition watched;
    const bool built = Triangulator::triangulate(
        plain, watched, message, [&](double fraction, const char *stage) {
            ++calls;
            if (lastStage != stage)
                std::printf("  %4.2f %s\n", fraction, stage);
            lastStage = stage;
            return true;
        });
    if (!built || watched.faceNodes != triangles.faceNodes)
        return fail("watching a run must not change it");

    IO::MeshDefinition stopped = triangles; // holds a mesh before the call
    const bool finished = Triangulator::triangulate(
        plain, stopped, message,
        [](double, const char *stage) { return std::string(stage) != "inserting edges"; });
    std::printf("stopped at \"inserting edges\": returned %s, message \"%s\", %lld faces left\n",
                finished ? "true" : "false", message.c_str(),
                static_cast<long long>(stopped.faceCount()));
    if (finished || stopped.faceCount() != 0 || stopped.nodeCount() != 0)
        return fail("a canceled run should leave the output empty");
    //! [progress]
    return 0;
}
