// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the meshing tools library (plan §7b): sigma layering and
// terrain-following grids (monotone interfaces, min-thickness, depth
// preservation), extrusion to layered prisms/hexahedra, transfinite +
// Winslow curvilinear grids (positive Jacobians, orthogonality), CDT
// constrained triangulation (topology invariants, holes, densification),
// quad-dominant conversion, and IDW terrain sampling.

#include <gtest/gtest.h>

#include "hydrocouplesdk/tools/sigmagrid.h"
#include "hydrocouplesdk/tools/curvilineargrid.h"
#include "hydrocouplesdk/tools/triangulator.h"
#include "hydrocouplesdk/tools/quadmesher.h"
#include "hydrocouplesdk/tools/terrainsampler.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Tools;
using HydroCouple::SDK::Spatial::Point;

namespace {

/*!
 * Undirected edge count of a CSR mesh (for Euler characteristic checks).
 */
int64_t undirectedEdgeCount(const IO::MeshDefinition &mesh)
{
    std::set<std::pair<int64_t, int64_t>> edges;
    for (int64_t f = 0; f < mesh.faceCount(); ++f)
    {
        const int64_t begin = mesh.faceNodeOffsets[static_cast<size_t>(f)];
        const int64_t end = mesh.faceNodeOffsets[static_cast<size_t>(f) + 1];
        for (int64_t i = begin; i < end; ++i)
        {
            const int64_t a = mesh.faceNodes[static_cast<size_t>(i)];
            const int64_t b = mesh.faceNodes[static_cast<size_t>(
                i + 1 == end ? begin : i + 1)];
            edges.emplace(std::min(a, b), std::max(a, b));
        }
    }
    return static_cast<int64_t>(edges.size());
}

/*!
 * Signed area of face f (positive = counter-clockwise).
 */
double faceSignedArea(const IO::MeshDefinition &mesh, int64_t f)
{
    const int64_t begin = mesh.faceNodeOffsets[static_cast<size_t>(f)];
    const int64_t end   = mesh.faceNodeOffsets[static_cast<size_t>(f) + 1];
    double area         = 0.0;
    for (int64_t i = begin; i < end; ++i)
    {
        const int64_t a = mesh.faceNodes[static_cast<size_t>(i)];
        const int64_t b =
            mesh.faceNodes[static_cast<size_t>(i + 1 == end ? begin : i + 1)];
        area += mesh.nodeX[static_cast<size_t>(a)] *
                    mesh.nodeY[static_cast<size_t>(b)] -
                mesh.nodeX[static_cast<size_t>(b)] *
                    mesh.nodeY[static_cast<size_t>(a)];
    }
    return area / 2.0;
}

} // namespace

// ── Sigma layering ────────────────────────────────────────────────────────

TEST(SigmaLayeringTest, UniformAndStretchedDistributionsValidate)
{
    std::string message;
    const auto uniform = SigmaLayering::uniform(4);
    ASSERT_TRUE(SigmaLayering::validate(uniform, message)) << message;
    EXPECT_DOUBLE_EQ(uniform[2], 0.5);

    const auto surfaceClustered = SigmaLayering::stretched(10, 4.0, 0.0);
    ASSERT_TRUE(SigmaLayering::validate(surfaceClustered, message))
        << message;
    // theta with b=0 clusters interfaces toward the surface: the first
    // layer is thinner than the last.
    EXPECT_LT(surfaceClustered[1] - surfaceClustered[0],
              surfaceClustered[10] - surfaceClustered[9]);

    const auto bedClustered = SigmaLayering::stretched(10, 4.0, 1.0);
    ASSERT_TRUE(SigmaLayering::validate(bedClustered, message)) << message;
    EXPECT_GT(bedClustered[1] - bedClustered[0],
              bedClustered[10] - bedClustered[9]);

    EXPECT_FALSE(SigmaLayering::validate(std::vector<double>{0.0, 0.5, 0.4,
                                                             1.0},
                                         message));
}

TEST(SigmaGridTest, TerrainFollowingWithMinimumThickness)
{
    // Three columns over synthetic terrain; the last is shallow.
    const std::vector<double> bed{-10.0, -6.0, -0.5};
    const std::vector<double> surface{2.0, 2.0, 2.0};
    const auto sigma = SigmaLayering::stretched(5, 3.0, 0.0);

    SigmaGrid grid;
    std::string message;
    ASSERT_TRUE(SigmaGridGenerator::build(bed, surface, sigma,
                                          /*minLayerThickness=*/0.4, grid,
                                          message)) << message;

    ASSERT_EQ(grid.columnCount, 3);
    ASSERT_EQ(grid.layerCount, 5);

    for (int64_t column = 0; column < 3; ++column)
    {
        // Interfaces pinned to surface and bed, strictly decreasing.
        EXPECT_DOUBLE_EQ(grid.z(column, 0),
                         surface[static_cast<size_t>(column)]);
        EXPECT_DOUBLE_EQ(grid.z(column, 5), bed[static_cast<size_t>(column)]);
        double totalThickness = 0.0;
        for (int k = 0; k < 5; ++k)
        {
            EXPECT_LT(grid.z(column, k + 1), grid.z(column, k))
                << "column " << column << " interface " << k;
            totalThickness += grid.thickness(column, k);
        }
        EXPECT_NEAR(totalThickness,
                    surface[static_cast<size_t>(column)] -
                        bed[static_cast<size_t>(column)],
                    1e-9);
    }

    // Deep columns honor the minimum thickness.
    for (int k = 0; k < 5; ++k)
        EXPECT_GE(grid.thickness(0, k), 0.4 - 1e-12);

    // The shallow column (depth 2.5 = 5 × 0.5 ≥ 5 × 0.4) still honors it.
    for (int k = 0; k < 5; ++k)
        EXPECT_GE(grid.thickness(2, k), 0.4 - 1e-12);

    // A column too shallow for the minimum collapses to equal layers.
    SigmaGrid thin;
    ASSERT_TRUE(SigmaGridGenerator::build(
        std::vector<double>{1.0}, std::vector<double>{2.0}, sigma,
        /*minLayerThickness=*/0.4, thin, message)) << message;
    for (int k = 0; k < 5; ++k)
        EXPECT_NEAR(thin.thickness(0, k), 0.2, 1e-12);
}

TEST(SigmaGridTest, ExtrusionBuildsTerrainFollowingPrisms)
{
    // Horizontal mesh: two triangles over four nodes.
    IO::MeshDefinition horizontal;
    horizontal.nodeX = {0.0, 1.0, 1.0, 0.0};
    horizontal.nodeY = {0.0, 0.0, 1.0, 1.0};
    horizontal.faceNodeOffsets = {0, 3, 6};
    horizontal.faceNodes       = {0, 1, 2, 0, 2, 3};

    const std::vector<double> bed{-4.0, -3.0, -2.0, -3.5};
    const std::vector<double> surface{0.0, 0.0, 0.0, 0.0};
    SigmaGrid sigma;
    std::string message;
    ASSERT_TRUE(SigmaGridGenerator::build(bed, surface,
                                          SigmaLayering::uniform(3), 0.0,
                                          sigma, message)) << message;

    LayeredMesh mesh;
    ASSERT_TRUE(SigmaGridGenerator::extrude(horizontal, sigma, mesh, message))
        << message;

    EXPECT_EQ(mesh.layerCount, 3);
    EXPECT_EQ(mesh.nodesPerLayer, 4);
    EXPECT_EQ(mesh.nodeCount(), 16);    // 4 interfaces × 4 nodes
    EXPECT_EQ(mesh.cellCount(), 6);     // 3 layers × 2 faces
    EXPECT_EQ(mesh.cellNodes.size(), 36u); // wedges: 6 nodes each

    // Node elevations follow the terrain: bottom interface equals the bed.
    for (int64_t n = 0; n < 4; ++n)
        EXPECT_DOUBLE_EQ(mesh.nodeZ[static_cast<size_t>(3 * 4 + n)],
                         bed[static_cast<size_t>(n)]);

    // A wedge's top nodes sit exactly one interface above its bottom nodes.
    const int64_t topNode    = mesh.cellNodes[0];
    const int64_t bottomNode = mesh.cellNodes[3];
    EXPECT_EQ(bottomNode - topNode, mesh.nodesPerLayer);
    EXPECT_GT(mesh.nodeZ[static_cast<size_t>(topNode)],
              mesh.nodeZ[static_cast<size_t>(bottomNode)]);
}

// ── Curvilinear grids ─────────────────────────────────────────────────────

TEST(CurvilinearGridTest, TransfiniteQuarterAnnulusIsValid)
{
    // Quarter annulus, right-handed parameterization: ξ = radius (1 → 2),
    // η = angle (0 → 90°).
    const int ni = 5, nj = 9;
    std::vector<Point> south, north, west, east;
    for (int i = 0; i < ni; ++i)
    {
        const double radius = 1.0 + static_cast<double>(i) / (ni - 1);
        south.emplace_back(radius, 0.0);  // angle 0
        north.emplace_back(0.0, radius);  // angle 90°
    }
    for (int j = 0; j < nj; ++j)
    {
        const double angle =
            (3.14159265358979323846 / 2.0) * j / (nj - 1);
        west.emplace_back(std::cos(angle), std::sin(angle));        // r = 1
        east.emplace_back(2.0 * std::cos(angle), 2.0 * std::sin(angle));
    }

    StructuredGrid2D grid;
    std::string message;
    ASSERT_TRUE(CurvilinearGridGenerator::transfinite(south, north, west,
                                                      east, grid, message))
        << message;

    EXPECT_GT(CurvilinearGridGenerator::minCornerJacobian(grid), 0.0);
    EXPECT_LT(CurvilinearGridGenerator::maxOrthogonalityDeviation(grid),
              15.0);

    // Winslow smoothing recovers a deliberately perturbed interior.
    StructuredGrid2D perturbed = grid;
    for (int j = 1; j < perturbed.nj - 1; ++j)
    {
        for (int i = 1; i < perturbed.ni - 1; ++i)
        {
            perturbed.X(i, j) += 0.03 * ((i * 7 + j * 3) % 5 - 2);
            perturbed.Y(i, j) += 0.03 * ((i * 3 + j * 5) % 5 - 2);
        }
    }
    const double deviationPerturbed =
        CurvilinearGridGenerator::maxOrthogonalityDeviation(perturbed);
    CurvilinearGridGenerator::winslowSmooth(perturbed, 200);
    EXPECT_GT(CurvilinearGridGenerator::minCornerJacobian(perturbed), 0.0);
    EXPECT_LT(CurvilinearGridGenerator::maxOrthogonalityDeviation(perturbed),
              deviationPerturbed);

    // UGRID export: (ni-1)(nj-1) counter-clockwise quads.
    const IO::MeshDefinition mesh =
        CurvilinearGridGenerator::toMeshDefinition(grid, "annulus");
    ASSERT_TRUE(mesh.validate(message)) << message;
    EXPECT_EQ(mesh.faceCount(), (ni - 1) * (nj - 1));
    for (int64_t f = 0; f < mesh.faceCount(); ++f)
        EXPECT_GT(faceSignedArea(mesh, f), 0.0);
}

TEST(CurvilinearGridTest, RejectsInconsistentCorners)
{
    std::vector<Point> south{{0, 0}, {1, 0}};
    std::vector<Point> north{{0, 1}, {1, 1}};
    std::vector<Point> west{{0, 0}, {0, 1}};
    std::vector<Point> badEast{{5, 0}, {1, 1}}; // wrong SE corner

    StructuredGrid2D grid;
    std::string message;
    EXPECT_FALSE(CurvilinearGridGenerator::transfinite(south, north, west,
                                                       badEast, grid,
                                                       message));
    EXPECT_NE(message.find("corners"), std::string::npos);
}

TEST(CurvilinearGridTest, GradedExtrusionTo3D)
{
    std::vector<Point> south{{0, 0}, {1, 0}, {2, 0}};
    std::vector<Point> north{{0, 1}, {1, 1}, {2, 1}};
    std::vector<Point> west{{0, 0}, {0, 1}};
    std::vector<Point> east{{2, 0}, {2, 1}};
    StructuredGrid2D grid;
    std::string message;
    ASSERT_TRUE(CurvilinearGridGenerator::transfinite(south, north, west,
                                                      east, grid, message));

    // Grading from a stretched sigma distribution mapped onto [-10, 0].
    const auto sigma = SigmaLayering::stretched(4, 3.0, 0.0);
    std::vector<double> zLevels;
    for (double s : sigma)
        zLevels.push_back(0.0 - 10.0 * s);

    StructuredGrid3D volume;
    ASSERT_TRUE(CurvilinearGridGenerator::extrude(grid, zLevels, volume,
                                                  message)) << message;
    EXPECT_EQ(volume.nk, 5);
    EXPECT_EQ(volume.x.size(), static_cast<size_t>(3 * 2 * 5));
    EXPECT_DOUBLE_EQ(volume.z.front(), 0.0);
    EXPECT_DOUBLE_EQ(volume.z.back(), -10.0);

    std::vector<double> nonMonotone{0.0, -1.0, -0.5};
    EXPECT_FALSE(CurvilinearGridGenerator::extrude(grid, nonMonotone, volume,
                                                   message));
}

// ── Triangulation ─────────────────────────────────────────────────────────

TEST(TriangulatorTest, SquareWithHoleSatisfiesEuler)
{
    TriangulationInput input;
    input.boundary = {{0, 0}, {10, 0}, {10, 10}, {0, 10}};
    input.holes.push_back({{4, 4}, {6, 4}, {6, 6}, {4, 6}});
    input.maxEdgeLength = 2.0;

    IO::MeshDefinition mesh;
    std::string message;
    ASSERT_TRUE(Triangulator::triangulate(input, mesh, message)) << message;
    ASSERT_TRUE(mesh.validate(message)) << message;
    EXPECT_GT(mesh.faceCount(), 0);

    // Every face is a counter-clockwise triangle.
    EXPECT_EQ(mesh.maxNodesPerFace(), 3);
    for (int64_t f = 0; f < mesh.faceCount(); ++f)
        EXPECT_GT(faceSignedArea(mesh, f), 0.0);

    // Planar subdivision with one hole: V − E + F = 2 − holes, counting
    // only interior faces (F here excludes the outer and hole faces):
    // V − E + (F + 2) = 2 ⇒ V − E + F = 1 − holes.
    const int64_t vertices = mesh.nodeCount();
    const int64_t edges    = undirectedEdgeCount(mesh);
    EXPECT_EQ(vertices - edges + mesh.faceCount(), 1 - 1);

    // Densification honored the max edge length on the boundary.
    double longestBoundaryEdge = 0.0;
    for (int64_t f = 0; f < mesh.faceCount(); ++f)
    {
        const int64_t begin = mesh.faceNodeOffsets[static_cast<size_t>(f)];
        for (int corner = 0; corner < 3; ++corner)
        {
            const int64_t a = mesh.faceNodes[static_cast<size_t>(
                begin + corner)];
            const int64_t b = mesh.faceNodes[static_cast<size_t>(
                begin + (corner + 1) % 3)];
            const bool aOnBoundary =
                mesh.nodeX[static_cast<size_t>(a)] == 0.0 ||
                mesh.nodeX[static_cast<size_t>(a)] == 10.0;
            const bool bOnBoundary =
                mesh.nodeX[static_cast<size_t>(b)] == 0.0 ||
                mesh.nodeX[static_cast<size_t>(b)] == 10.0;
            if (aOnBoundary && bOnBoundary)
                longestBoundaryEdge = std::max(
                    longestBoundaryEdge,
                    std::hypot(mesh.nodeX[static_cast<size_t>(b)] -
                                   mesh.nodeX[static_cast<size_t>(a)],
                               mesh.nodeY[static_cast<size_t>(b)] -
                                   mesh.nodeY[static_cast<size_t>(a)]));
        }
    }
    EXPECT_LE(longestBoundaryEdge, 2.0 * std::sqrt(2.0) + 1e-9);
}

TEST(TriangulatorTest, ConstraintSegmentsAreConformedTo)
{
    TriangulationInput input;
    input.boundary = {{0, 0}, {4, 0}, {4, 4}, {0, 4}};
    input.constraintSegments.push_back(
        {Point{1.0, 2.0}, Point{3.0, 2.0}}); // interior breakline

    IO::MeshDefinition mesh;
    std::string message;
    ASSERT_TRUE(Triangulator::triangulate(input, mesh, message)) << message;

    // The breakline endpoints exist as nodes.
    auto hasNode = [&](double x, double y) {
        for (size_t n = 0; n < mesh.nodeX.size(); ++n)
        {
            if (mesh.nodeX[n] == x && mesh.nodeY[n] == y)
                return true;
        }
        return false;
    };
    EXPECT_TRUE(hasNode(1.0, 2.0));
    EXPECT_TRUE(hasNode(3.0, 2.0));

    EXPECT_FALSE(Triangulator::triangulate(TriangulationInput{}, mesh,
                                           message));
}

TEST(TriangulatorTest, CrossingBreaklinesAreSplitAtTheirCrossing)
{
    // MeshKit recovers only non-crossing constraints, so the tools node
    // crossing segments first, as the CDT library they replaced did.
    TriangulationInput input;
    input.boundary = {{0, 0}, {4, 0}, {4, 4}, {0, 4}};
    input.constraintSegments.push_back({Point{1.0, 1.0}, Point{3.0, 3.0}});
    input.constraintSegments.push_back({Point{1.0, 3.0}, Point{3.0, 1.0}});

    IO::MeshDefinition mesh;
    std::string message;
    ASSERT_TRUE(Triangulator::triangulate(input, mesh, message)) << message;
    ASSERT_TRUE(mesh.validate(message)) << message;

    bool crossing = false;
    for (size_t n = 0; n < mesh.nodeX.size(); ++n)
        crossing = crossing || (std::abs(mesh.nodeX[n] - 2.0) < 1e-12 &&
                                std::abs(mesh.nodeY[n] - 2.0) < 1e-12);
    EXPECT_TRUE(crossing) << "no node at the crossing (2, 2)";

    // A simply connected domain: V − E + F = 1.
    EXPECT_EQ(mesh.nodeCount() - undirectedEdgeCount(mesh) + mesh.faceCount(), 1);
    for (int64_t f = 0; f < mesh.faceCount(); ++f)
        EXPECT_GT(faceSignedArea(mesh, f), 0.0);
}

TEST(TriangulatorTest, VertexInsertionCanBeCancelled)
{
    // The phase the CDT library could not interrupt: MeshKit asks between
    // batches of vertices, so a stop asked for during it lands there.
    TriangulationInput input;
    input.boundary = {{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    for (int i = 1; i < 100; ++i)
        for (int j = 1; j < 100; ++j)
            input.interiorPoints.push_back({i + 0.25 * (j % 3), j + 0.25 * (i % 3)});

    int insertionCalls = 0;
    IO::MeshDefinition mesh;
    std::string message;
    EXPECT_FALSE(Triangulator::triangulate(
        input, mesh, message, [&](double, const char *stage) {
            if (std::string(stage) != "inserting vertices")
                return true;
            return ++insertionCalls < 2; // stop at the first report from inside
        }));
    EXPECT_EQ(message, "cancelled");
    EXPECT_EQ(mesh.faceCount(), 0);
    EXPECT_GE(insertionCalls, 2) << "insertion never reported from inside";
}

// ── Quad-dominant meshing ─────────────────────────────────────────────────

TEST(QuadMesherTest, StructuredTrianglePairsMergeToQuads)
{
    // 3×3 node grid split into 8 right triangles (consistent CCW winding).
    IO::MeshDefinition triangles;
    triangles.nodeX = {0, 1, 2, 0, 1, 2, 0, 1, 2};
    triangles.nodeY = {0, 0, 0, 1, 1, 1, 2, 2, 2};
    triangles.faceNodeOffsets.push_back(0);
    auto addTriangle = [&](int64_t a, int64_t b, int64_t c) {
        triangles.faceNodes.insert(triangles.faceNodes.end(), {a, b, c});
        triangles.faceNodeOffsets.push_back(
            static_cast<int64_t>(triangles.faceNodes.size()));
    };
    for (int j = 0; j < 2; ++j)
    {
        for (int i = 0; i < 2; ++i)
        {
            const int64_t n00 = j * 3 + i, n10 = n00 + 1;
            const int64_t n01 = n00 + 3, n11 = n01 + 1;
            addTriangle(n00, n10, n11);
            addTriangle(n00, n11, n01);
        }
    }

    IO::MeshDefinition mixed;
    std::string message;
    ASSERT_TRUE(QuadMesher::triToQuadDominant(triangles, 0.2, mixed,
                                              message)) << message;
    ASSERT_TRUE(mixed.validate(message)) << message;

    // Every diagonal pair merges: 4 quads, no leftover triangles.
    EXPECT_EQ(mixed.faceCount(), 4);
    EXPECT_EQ(mixed.maxNodesPerFace(), 4);
    for (int64_t f = 0; f < mixed.faceCount(); ++f)
        EXPECT_GT(faceSignedArea(mixed, f), 0.0);

    // Unit-square quads from right-triangle pairs score 0.5 (45° corners).
    EXPECT_DOUBLE_EQ(QuadMesher::quadQuality(0, 0, 1, 0, 1, 1, 0, 1), 1.0);
    EXPECT_DOUBLE_EQ(QuadMesher::quadQuality(0, 0, 1, 1, 0, 2, -1, 1), 1.0);
    EXPECT_DOUBLE_EQ(QuadMesher::quadQuality(0, 0, 2, 0, 1, 1, 2, 2),
                     0.0); // non-convex

    // A skewed pair (45°/135° parallelogram, quality 0.5) stays triangular
    // under a stricter threshold.
    IO::MeshDefinition skewed;
    skewed.nodeX           = {0, 1, 2, 1};
    skewed.nodeY           = {0, 0, 1, 1};
    skewed.faceNodeOffsets = {0, 3, 6};
    skewed.faceNodes       = {0, 1, 2, 0, 2, 3};
    IO::MeshDefinition untouched;
    ASSERT_TRUE(QuadMesher::triToQuadDominant(skewed, 0.8, untouched,
                                              message)) << message;
    EXPECT_EQ(untouched.faceCount(), 2);
    EXPECT_EQ(untouched.maxNodesPerFace(), 3);
    IO::MeshDefinition mergedPair;
    ASSERT_TRUE(QuadMesher::triToQuadDominant(skewed, 0.4, mergedPair,
                                              message)) << message;
    EXPECT_EQ(mergedPair.faceCount(), 1);
}

// ── Watching a meshing run, and stopping one ──────────────────────────────
//
// The tools are static functions that return when they are done, which is
// all a batch caller needs. A caller driving a window needs to say that work
// is under way and to abandon it, so both take an optional callback. What
// these check is that watching a run does not change its answer, that a
// refusal to carry on leaves nothing behind rather than half a mesh, and
// that the phases are reported in the order they happen.

namespace {

//! Records what a run reported, and can refuse to carry on at a chosen stage.
struct ProgressRecorder
{
    std::vector<std::pair<double, std::string>> calls;

    //! Stage to stop at; empty carries on to the end.
    std::string stopAt;

    MeshProgress callback()
    {
        return [this](double fraction, const char *stage) {
            calls.emplace_back(fraction, stage);
            return stopAt.empty() || stage != stopAt;
        };
    }

    [[nodiscard]] bool reported(const std::string &stage) const
    {
        return std::any_of(calls.begin(), calls.end(),
                           [&](const auto &call) {
                               return call.second == stage;
                           });
    }
};

TriangulationInput squareDomain()
{
    TriangulationInput input;
    input.boundary = {{0.0, 0.0}, {10.0, 0.0}, {10.0, 10.0}, {0.0, 10.0}};
    input.holes.push_back({{4.0, 4.0}, {6.0, 4.0}, {6.0, 6.0}, {4.0, 6.0}});
    input.constraintSegments.push_back({Point{1.0, 1.0}, Point{9.0, 1.0}});
    input.maxEdgeLength = 1.0;
    return input;
}

} // namespace

TEST(MeshProgressTest, ATriangulationReportsItsPhasesInOrderAndEndsAtOne)
{
    ProgressRecorder recorder;
    IO::MeshDefinition mesh;
    std::string        message;

    ASSERT_TRUE(Triangulator::triangulate(squareDomain(), mesh, message,
                                          recorder.callback()))
        << message;

    ASSERT_FALSE(recorder.calls.empty());

    // Every phase the caller can be told about, and in the order they
    // happen: a bar that goes backwards is worse than no bar.
    EXPECT_TRUE(recorder.reported("densifying"));
    EXPECT_TRUE(recorder.reported("inserting vertices"));
    EXPECT_TRUE(recorder.reported("inserting edges"));
    EXPECT_TRUE(recorder.reported("removing holes"));
    EXPECT_TRUE(recorder.reported("extracting"));

    for (size_t call = 1; call < recorder.calls.size(); ++call)
        EXPECT_GE(recorder.calls[call].first, recorder.calls[call - 1].first)
            << "the fraction went backwards at call " << call;

    EXPECT_DOUBLE_EQ(recorder.calls.front().first, 0.0);
    EXPECT_DOUBLE_EQ(recorder.calls.back().first, 1.0);
    EXPECT_EQ(recorder.calls.back().second, "done");
}

TEST(MeshProgressTest, WatchingATriangulationDoesNotChangeIt)
{
    IO::MeshDefinition watched;
    IO::MeshDefinition unwatched;
    std::string        message;

    ProgressRecorder recorder;
    ASSERT_TRUE(Triangulator::triangulate(squareDomain(), watched, message,
                                          recorder.callback()))
        << message;
    ASSERT_TRUE(Triangulator::triangulate(squareDomain(), unwatched, message))
        << message;

    // Observation is not participation: the same domain gives the same mesh
    // whether or not anybody was looking.
    EXPECT_EQ(watched.nodeX, unwatched.nodeX);
    EXPECT_EQ(watched.nodeY, unwatched.nodeY);
    EXPECT_EQ(watched.faceNodes, unwatched.faceNodes);
    EXPECT_EQ(watched.faceNodeOffsets, unwatched.faceNodeOffsets);

    // And an empty callback is exactly the unwatched call.
    IO::MeshDefinition empty;
    ASSERT_TRUE(Triangulator::triangulate(squareDomain(), empty, message,
                                          MeshProgress{}))
        << message;
    EXPECT_EQ(empty.faceNodes, unwatched.faceNodes);
}

TEST(MeshProgressTest, StoppingATriangulationHandsBackNothing)
{
    ProgressRecorder recorder;
    recorder.stopAt = "densifying";

    // Handed an output that already holds a mesh, which is what a caller
    // meshing twice into one variable does. Given a fresh one, "hands back
    // nothing" is true before the call and proves nothing about it.
    IO::MeshDefinition mesh;
    std::string        message;
    ASSERT_TRUE(Triangulator::triangulate(squareDomain(), mesh, message))
        << message;
    ASSERT_GT(mesh.faceCount(), 0);

    EXPECT_FALSE(Triangulator::triangulate(squareDomain(), mesh, message,
                                           recorder.callback()));
    EXPECT_EQ(message, "cancelled");
    EXPECT_EQ(mesh.nodeCount(), 0);
    EXPECT_EQ(mesh.faceCount(), 0);
}

TEST(MeshProgressTest, StoppingAfterTheMeshIsBuiltStillHandsBackNothing)
{
    // The last chance to stop comes after CDT has done the expensive part,
    // and taking it must still leave nothing behind: a caller that abandoned
    // a run and then read the mesh anyway would get geometry from a run the
    // user cancelled.
    ProgressRecorder recorder;
    recorder.stopAt = "extracting";

    IO::MeshDefinition mesh;
    std::string        message;
    ASSERT_TRUE(Triangulator::triangulate(squareDomain(), mesh, message))
        << message;
    ASSERT_GT(mesh.faceCount(), 0);

    EXPECT_FALSE(Triangulator::triangulate(squareDomain(), mesh, message,
                                           recorder.callback()));
    EXPECT_EQ(message, "cancelled");
    EXPECT_EQ(mesh.nodeCount(), 0);
    EXPECT_EQ(mesh.faceCount(), 0);
}

TEST(MeshProgressTest, TheQuadMergeIsWatchedAndCanBeStopped)
{
    IO::MeshDefinition triangles;
    std::string        message;
    ASSERT_TRUE(Triangulator::triangulate(squareDomain(), triangles, message))
        << message;

    ProgressRecorder recorder;
    IO::MeshDefinition watched;
    ASSERT_TRUE(QuadMesher::triToQuadDominant(triangles, 0.3, watched,
                                              message, recorder.callback()))
        << message;

    EXPECT_TRUE(recorder.reported("indexing edges"));
    EXPECT_TRUE(recorder.reported("scoring pairs"));
    EXPECT_TRUE(recorder.reported("merging"));
    EXPECT_DOUBLE_EQ(recorder.calls.back().first, 1.0);

    IO::MeshDefinition unwatched;
    ASSERT_TRUE(QuadMesher::triToQuadDominant(triangles, 0.3, unwatched,
                                              message))
        << message;
    EXPECT_EQ(watched.faceNodes, unwatched.faceNodes);

    // Every phase of the merge is this library's own loop, so a stop asked
    // for anywhere in it is taken there and then.
    ProgressRecorder stopper;
    stopper.stopAt = "merging";

    IO::MeshDefinition abandoned = watched;
    ASSERT_GT(abandoned.faceCount(), 0);

    EXPECT_FALSE(QuadMesher::triToQuadDominant(triangles, 0.3, abandoned,
                                               message, stopper.callback()));
    EXPECT_EQ(message, "cancelled");
    EXPECT_EQ(abandoned.faceCount(), 0);
    EXPECT_EQ(abandoned.nodeCount(), 0);
}

// ── Terrain sampling ──────────────────────────────────────────────────────

TEST(TerrainSamplerTest, IdwInterpolatesAndHonorsRadius)
{
    const std::vector<Point> samples{
        {0.0, 0.0, 10.0}, {10.0, 0.0, 20.0}, {0.0, 10.0, 30.0}};

    // Exact at a sample; bounded between extremes elsewhere.
    std::vector<double> x{0.0, 5.0};
    std::vector<double> y{0.0, 5.0};
    std::vector<double> z(2, 0.0);
    std::string message;
    ASSERT_TRUE(TerrainSampler::idw(samples, 2.0, x, y, z, message))
        << message;
    EXPECT_DOUBLE_EQ(z[0], 10.0);
    EXPECT_GT(z[1], 10.0);
    EXPECT_LT(z[1], 30.0);

    // Radius excludes every sample → diagnosed failure.
    std::vector<double> farX{100.0}, farY{100.0}, farZ(1);
    EXPECT_FALSE(TerrainSampler::idw(samples, 2.0, farX, farY, farZ, message,
                                     /*searchRadius=*/1.0));
    EXPECT_FALSE(message.empty());

    EXPECT_FALSE(TerrainSampler::idw({}, 2.0, x, y, z, message));
}

// ── End to end: triangulate → sample terrain → sigma → extrude ────────────

TEST(ToolsIntegrationTest, GeneratorToLayeredMeshPipeline)
{
    TriangulationInput domain;
    domain.boundary      = {{0, 0}, {8, 0}, {8, 8}, {0, 8}};
    domain.maxEdgeLength = 3.0;

    IO::MeshDefinition surfaceMesh;
    std::string message;
    ASSERT_TRUE(Triangulator::triangulate(domain, surfaceMesh, message))
        << message;

    // Bed elevations sampled from scattered survey points.
    const std::vector<Point> survey{
        {0, 0, -5.0}, {8, 0, -3.0}, {8, 8, -6.0}, {0, 8, -4.0}, {4, 4, -8.0}};
    std::vector<double> bed(static_cast<size_t>(surfaceMesh.nodeCount()));
    ASSERT_TRUE(TerrainSampler::idw(survey, 2.0, surfaceMesh.nodeX,
                                    surfaceMesh.nodeY, bed, message))
        << message;

    const std::vector<double> surface(bed.size(), 1.0);
    SigmaGrid sigma;
    ASSERT_TRUE(SigmaGridGenerator::build(bed, surface,
                                          SigmaLayering::stretched(4, 2.0),
                                          0.2, sigma, message)) << message;

    LayeredMesh volume;
    ASSERT_TRUE(SigmaGridGenerator::extrude(surfaceMesh, sigma, volume,
                                            message)) << message;
    EXPECT_EQ(volume.cellCount(), 4 * surfaceMesh.faceCount());
    EXPECT_EQ(volume.nodeCount(), 5 * surfaceMesh.nodeCount());

    // Every cell's bottom nodes are strictly below its top nodes.
    for (int64_t cell = 0; cell < volume.cellCount(); ++cell)
    {
        const int64_t begin =
            volume.cellNodeOffsets[static_cast<size_t>(cell)];
        const int64_t end =
            volume.cellNodeOffsets[static_cast<size_t>(cell) + 1];
        const int64_t half = (end - begin) / 2;
        for (int64_t i = 0; i < half; ++i)
        {
            const int64_t top =
                volume.cellNodes[static_cast<size_t>(begin + i)];
            const int64_t bottom =
                volume.cellNodes[static_cast<size_t>(begin + half + i)];
            EXPECT_GT(volume.nodeZ[static_cast<size_t>(top)],
                      volume.nodeZ[static_cast<size_t>(bottom)]);
        }
    }
}
