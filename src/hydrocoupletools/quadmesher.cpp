// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "hydrocouplesdk/tools/quadmesher.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <vector>

namespace HydroCouple::SDK::Tools {

namespace {

//! One pair of adjacent triangles that could merge into a quad.
struct MergeCandidate
{
    double  quality; //!< QuadMesher::quadQuality() of the merged quad.
    int64_t face1;   //!< First triangle.
    int64_t face2;   //!< Second triangle (across the shared edge).
    int64_t quad[4]; //!< Counter-clockwise node indexes.
};

//! An undirected edge's key: the larger node index in the high 32 bits, the
//! smaller in the low 32 (indexes must be below 2^32).
uint64_t edgeKey(int64_t a, int64_t b)
{
    const uint64_t lo = static_cast<uint64_t>(std::min(a, b));
    const uint64_t hi = static_cast<uint64_t>(std::max(a, b));
    return (hi << 32) | lo;
}

//! Calls \p progress if there is one and reports whether to carry on.
bool carryOn(const MeshProgress &progress, double fraction, const char *stage)
{
    return !progress || progress(fraction, stage);
}

//! What a canceled merge leaves behind: nothing, and a reason.
bool cancelled(IO::MeshDefinition &out, std::string &message)
{
    out     = IO::MeshDefinition{};
    message = "cancelled";
    return false;
}

} // namespace

double QuadMesher::quadQuality(double ax, double ay, double bx, double by,
                               double cx, double cy, double dx, double dy)
{
    const double px[4] = {ax, bx, cx, dx};
    const double py[4] = {ay, by, cy, dy};

    double maxDeviation = 0.0;
    for (int corner = 0; corner < 4; ++corner)
    {
        const int prev = (corner + 3) % 4;
        const int next = (corner + 1) % 4;
        const double inX  = px[prev] - px[corner];
        const double inY  = py[prev] - py[corner];
        const double outX = px[next] - px[corner];
        const double outY = py[next] - py[corner];

        // Convexity for a counter-clockwise quad: outgoing × incoming > 0.
        const double crossProduct = outX * inY - outY * inX;
        if (crossProduct <= 0.0)
            return 0.0;

        const double magnitudes =
            std::hypot(inX, inY) * std::hypot(outX, outY);
        if (magnitudes <= 0.0)
            return 0.0;
        const double cosAngle =
            std::clamp((inX * outX + inY * outY) / magnitudes, -1.0, 1.0);
        const double angleDeg =
            std::acos(cosAngle) * 180.0 / 3.14159265358979323846;
        maxDeviation = std::max(maxDeviation, std::abs(angleDeg - 90.0));
    }
    return std::max(0.0, 1.0 - maxDeviation / 90.0);
}

bool QuadMesher::triToQuadDominant(const IO::MeshDefinition &triangleMesh,
                                   double minQuality, IO::MeshDefinition &out,
                                   std::string &message)
{
    return triToQuadDominant(triangleMesh, minQuality, out, message,
                             MeshProgress{});
}

bool QuadMesher::triToQuadDominant(const IO::MeshDefinition &triangleMesh,
                                   double minQuality, IO::MeshDefinition &out,
                                   std::string &message,
                                   const MeshProgress &progress)
{
    if (!triangleMesh.validate(message))
        return false;
    const int64_t faces = triangleMesh.faceCount();
    if (faces == 0)
    {
        message = "input mesh has no faces";
        return false;
    }
    for (int64_t f = 0; f < faces; ++f)
    {
        if (triangleMesh.faceNodeOffsets[static_cast<size_t>(f) + 1] -
                triangleMesh.faceNodeOffsets[static_cast<size_t>(f)] !=
            3)
        {
            message = "face " + std::to_string(f) + " is not a triangle";
            return false;
        }
    }
    minQuality = std::clamp(minQuality, 0.0, 1.0);

    if (!carryOn(progress, 0.0, "indexing edges"))
        return cancelled(out, message);

    // Directed half-edge record per face edge: key → (face, from, to, opposite).
    struct HalfEdge
    {
        int64_t face;
        int64_t from;
        int64_t to;
        int64_t opposite;
    };
    std::unordered_map<uint64_t, std::vector<HalfEdge>> edgeFaces;
    edgeFaces.reserve(static_cast<size_t>(faces) * 3);

    auto faceNode = [&](int64_t f, int corner) {
        return triangleMesh
            .faceNodes[static_cast<size_t>(
                triangleMesh.faceNodeOffsets[static_cast<size_t>(f)] +
                corner)];
    };

    for (int64_t f = 0; f < faces; ++f)
    {
        const int64_t n0 = faceNode(f, 0);
        const int64_t n1 = faceNode(f, 1);
        const int64_t n2 = faceNode(f, 2);
        edgeFaces[edgeKey(n0, n1)].push_back({f, n0, n1, n2});
        edgeFaces[edgeKey(n1, n2)].push_back({f, n1, n2, n0});
        edgeFaces[edgeKey(n2, n0)].push_back({f, n2, n0, n1});
    }

    if (!carryOn(progress, 0.30, "scoring pairs"))
        return cancelled(out, message);

    std::vector<MergeCandidate> candidates;
    size_t                      scored = 0;
    for (const auto &[key, records] : edgeFaces)
    {
        if ((++scored & 0xFFFu) == 0
            && !carryOn(progress,
                        0.30 + 0.30 * double(scored) / double(edgeFaces.size()),
                        "scoring pairs"))
            return cancelled(out, message);

        if (records.size() != 2)
            continue; // boundary or non-manifold edge
        const HalfEdge &first  = records[0];
        const HalfEdge &second = records[1];
        if (first.face == second.face)
            continue;

        // Consistent orientation means the second face traverses the edge
        // in reverse: first u→v, second v→u. Quad CCW = (u, x, v, w) with
        // w opposite in the first face and x opposite in the second.
        const int64_t u = first.from;
        const int64_t v = first.to;
        const int64_t w = first.opposite;
        const int64_t x = second.opposite;
        if (second.from != v || second.to != u)
            continue; // inconsistent winding — leave as triangles

        const double quality = quadQuality(
            triangleMesh.nodeX[static_cast<size_t>(u)],
            triangleMesh.nodeY[static_cast<size_t>(u)],
            triangleMesh.nodeX[static_cast<size_t>(x)],
            triangleMesh.nodeY[static_cast<size_t>(x)],
            triangleMesh.nodeX[static_cast<size_t>(v)],
            triangleMesh.nodeY[static_cast<size_t>(v)],
            triangleMesh.nodeX[static_cast<size_t>(w)],
            triangleMesh.nodeY[static_cast<size_t>(w)]);
        if (quality >= minQuality && quality > 0.0)
            candidates.push_back(
                {quality, first.face, second.face, {u, x, v, w}});
    }

    std::sort(candidates.begin(), candidates.end(),
              [](const MergeCandidate &a, const MergeCandidate &b) {
                  return a.quality > b.quality;
              });

    if (!carryOn(progress, 0.60, "merging"))
        return cancelled(out, message);

    std::vector<bool> merged(static_cast<size_t>(faces), false);
    out          = IO::MeshDefinition{};
    out.meshName = triangleMesh.meshName;
    out.nodeX    = triangleMesh.nodeX;
    out.nodeY    = triangleMesh.nodeY;
    out.nodeZ    = triangleMesh.nodeZ;
    out.faceNodeOffsets.push_back(0);

    size_t considered = 0;
    for (const auto &candidate : candidates)
    {
        if ((++considered & 0xFFFu) == 0
            && !carryOn(progress,
                        0.60 + 0.30 * double(considered)
                                 / double(candidates.size()),
                        "merging"))
            return cancelled(out, message);

        if (merged[static_cast<size_t>(candidate.face1)] ||
            merged[static_cast<size_t>(candidate.face2)])
            continue;
        merged[static_cast<size_t>(candidate.face1)] = true;
        merged[static_cast<size_t>(candidate.face2)] = true;
        out.faceNodes.insert(out.faceNodes.end(), candidate.quad,
                             candidate.quad + 4);
        out.faceNodeOffsets.push_back(
            static_cast<int64_t>(out.faceNodes.size()));
    }

    if (!carryOn(progress, 0.90, "copying triangles"))
        return cancelled(out, message);

    for (int64_t f = 0; f < faces; ++f)
    {
        if (merged[static_cast<size_t>(f)])
            continue;
        out.faceNodes.push_back(faceNode(f, 0));
        out.faceNodes.push_back(faceNode(f, 1));
        out.faceNodes.push_back(faceNode(f, 2));
        out.faceNodeOffsets.push_back(
            static_cast<int64_t>(out.faceNodes.size()));
    }

    carryOn(progress, 1.0, "done");

    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Tools
