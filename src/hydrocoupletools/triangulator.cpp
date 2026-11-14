// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "hydrocouplesdk/tools/triangulator.h"

#include <meshkit/cdt.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>

namespace HydroCouple::SDK::Tools {

namespace {

using Point = meshkit::Point2;
using Delaunay = meshkit::ConstrainedDelaunay;

//! A constraint edge between two entries of the point list.
struct Edge
{
    size_t a, b;
};

double distance(const Spatial::Point &a, const Spatial::Point &b)
{
    return std::hypot(b.x - a.x, b.y - a.y);
}

/*!
 * Adds the closed ring's vertices (densified) and its closing edges.
 */
void addRing(const std::vector<Spatial::Point> &ring, double maxEdgeLength,
             std::vector<Point> &points, std::vector<Edge> &edges)
{
    if (ring.size() < 3)
        return;

    std::vector<size_t> ringIndices;
    for (size_t p = 0; p < ring.size(); ++p)
    {
        const Spatial::Point &a = ring[p];
        const Spatial::Point &b = ring[(p + 1) % ring.size()];

        ringIndices.push_back(points.size());
        points.emplace_back(a.x, a.y);

        if (maxEdgeLength > 0.0)
        {
            const int segments =
                static_cast<int>(std::ceil(distance(a, b) / maxEdgeLength));
            for (int s = 1; s < segments; ++s)
            {
                const double t = static_cast<double>(s) / segments;
                ringIndices.push_back(points.size());
                points.emplace_back(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y));
            }
        }
    }
    for (size_t v = 0; v < ringIndices.size(); ++v)
        edges.push_back({ringIndices[v], ringIndices[(v + 1) % ringIndices.size()]});
}

/*!
 * Adds one open constraint segment (densified).
 */
void addSegment(const Spatial::Point &a, const Spatial::Point &b,
                double maxEdgeLength, std::vector<Point> &points,
                std::vector<Edge> &edges)
{
    std::vector<size_t> indices;
    indices.push_back(points.size());
    points.emplace_back(a.x, a.y);
    if (maxEdgeLength > 0.0)
    {
        const int segments =
            static_cast<int>(std::ceil(distance(a, b) / maxEdgeLength));
        for (int s = 1; s < segments; ++s)
        {
            const double t = static_cast<double>(s) / segments;
            indices.push_back(points.size());
            points.emplace_back(a.x + t * (b.x - a.x), a.y + t * (b.y - a.y));
        }
    }
    indices.push_back(points.size());
    points.emplace_back(b.x, b.y);
    for (size_t v = 0; v + 1 < indices.size(); ++v)
        edges.push_back({indices[v], indices[v + 1]});
}

/*!
 * Splits constraint edges where they properly cross, appending each crossing
 * point to \p points.
 *
 * MeshKit recovers a constraint only if no other constraint crosses it, and
 * leaves noding to its caller; the CDT library this replaced resolved
 * crossings itself, so crossing breaklines must keep working. Edges that share
 * an endpoint, or whose end only touches another edge, need no split: MeshKit
 * recovers those through the shared or collinear vertex. Candidate pairs come
 * from a uniform grid; each pair is tested once, in the cell holding the lower
 * corner of the overlap of their boxes.
 */
void nodeCrossings(std::vector<Point> &points, std::vector<Edge> &edges)
{
    if (edges.size() < 2)
        return;

    double minX = points[edges[0].a].x(), maxX = minX;
    double minY = points[edges[0].a].y(), maxY = minY;
    double totalLength = 0.0;
    for (const Edge &e : edges)
    {
        for (size_t v : {e.a, e.b})
        {
            minX = std::min(minX, points[v].x());
            maxX = std::max(maxX, points[v].x());
            minY = std::min(minY, points[v].y());
            maxY = std::max(maxY, points[v].y());
        }
        totalLength += std::hypot(points[e.b].x() - points[e.a].x(),
                                  points[e.b].y() - points[e.a].y());
    }
    const double extent = std::max(maxX - minX, maxY - minY);
    if (!(extent > 0.0))
        return;
    // About one edge per cell, but never more than 4096 cells across.
    const double cell =
        std::max(totalLength / static_cast<double>(edges.size()), extent / 4096.0);
    auto cellOf = [&](double value, double origin) {
        return static_cast<int64_t>(std::floor((value - origin) / cell));
    };
    auto key = [](int64_t i, int64_t j) { return (i << 32) ^ (j & 0xffffffff); };

    struct Box
    {
        double x0, y0, x1, y1;
    };
    std::vector<Box> boxes(edges.size());
    std::unordered_map<int64_t, std::vector<size_t>> grid;
    for (size_t k = 0; k < edges.size(); ++k)
    {
        const Point &p = points[edges[k].a], &q = points[edges[k].b];
        boxes[k] = {std::min(p.x(), q.x()), std::min(p.y(), q.y()),
                    std::max(p.x(), q.x()), std::max(p.y(), q.y())};
        for (int64_t i = cellOf(boxes[k].x0, minX); i <= cellOf(boxes[k].x1, minX); ++i)
            for (int64_t j = cellOf(boxes[k].y0, minY); j <= cellOf(boxes[k].y1, minY); ++j)
                grid[key(i, j)].push_back(k);
    }

    // Crossing points along each edge, as (parameter, point index).
    std::unordered_map<size_t, std::vector<std::pair<double, size_t>>> splits;
    for (const auto &[cellKey, members] : grid)
    {
        for (size_t m = 0; m < members.size(); ++m)
        {
            for (size_t n = m + 1; n < members.size(); ++n)
            {
                const size_t s = members[m], t = members[n];
                const Box &bs = boxes[s], &bt = boxes[t];
                const double ox = std::max(bs.x0, bt.x0), oy = std::max(bs.y0, bt.y0);
                if (ox > std::min(bs.x1, bt.x1) || oy > std::min(bs.y1, bt.y1))
                    continue; // boxes apart
                if (key(cellOf(ox, minX), cellOf(oy, minY)) != cellKey)
                    continue; // this pair is tested in another cell
                const Edge &e = edges[s], &f = edges[t];
                if (e.a == f.a || e.a == f.b || e.b == f.a || e.b == f.b)
                    continue;
                const Point &a = points[e.a], &b = points[e.b];
                const Point &c = points[f.a], &d = points[f.b];
                const double o1 = Delaunay::orientExact(a, b, c);
                const double o2 = Delaunay::orientExact(a, b, d);
                const double o3 = Delaunay::orientExact(c, d, a);
                const double o4 = Delaunay::orientExact(c, d, b);
                if (!((o1 > 0) != (o2 > 0) && o1 != 0 && o2 != 0 &&
                      (o3 > 0) != (o4 > 0) && o3 != 0 && o4 != 0))
                    continue; // no proper crossing
                const double rx = b.x() - a.x(), ry = b.y() - a.y();
                const double sx = d.x() - c.x(), sy = d.y() - c.y();
                const double denominator = rx * sy - ry * sx;
                const double u = ((c.x() - a.x()) * sy - (c.y() - a.y()) * sx) / denominator;
                const double w = ((c.x() - a.x()) * ry - (c.y() - a.y()) * rx) / denominator;
                const size_t crossing = points.size();
                points.emplace_back(a.x() + u * rx, a.y() + u * ry);
                splits[s].push_back({u, crossing});
                splits[t].push_back({w, crossing});
            }
        }
    }
    if (splits.empty())
        return;

    std::vector<Edge> noded;
    noded.reserve(edges.size() + 2 * splits.size());
    for (size_t k = 0; k < edges.size(); ++k)
    {
        auto found = splits.find(k);
        if (found == splits.end())
        {
            noded.push_back(edges[k]);
            continue;
        }
        auto &along = found->second;
        std::sort(along.begin(), along.end());
        size_t from = edges[k].a;
        for (const auto &[parameter, crossing] : along)
        {
            noded.push_back({from, crossing});
            from = crossing;
        }
        noded.push_back({from, edges[k].b});
    }
    edges = std::move(noded);
}

//! Whether \p p lies inside \p ring (crossing-number rule; orientation-free).
bool inside(const std::vector<Spatial::Point> &ring, double x, double y)
{
    bool in = false;
    for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++)
    {
        const Spatial::Point &p = ring[i], &q = ring[j];
        if ((p.y > y) != (q.y > y) &&
            x < (q.x - p.x) * (y - p.y) / (q.y - p.y) + p.x)
            in = !in;
    }
    return in;
}

/*!
 * Calls \p progress if there is one and reports whether to carry on.
 */
bool carryOn(const MeshProgress &progress, double fraction, const char *stage)
{
    return !progress || progress(fraction, stage);
}

//! What a cancelled run leaves behind: nothing, and a reason.
bool cancelled(IO::MeshDefinition &out, std::string &message)
{
    out     = IO::MeshDefinition{};
    message = "cancelled";
    return false;
}

} // namespace

bool Triangulator::triangulate(const TriangulationInput &input,
                               IO::MeshDefinition &out, std::string &message)
{
    return triangulate(input, out, message, MeshProgress{});
}

bool Triangulator::triangulate(const TriangulationInput &input,
                               IO::MeshDefinition &out, std::string &message,
                               const MeshProgress &progress)
{
    if (input.boundary.size() < 3)
    {
        message = "boundary ring needs at least 3 points";
        return false;
    }

    std::vector<Point> points;
    std::vector<Edge>  edges;

    if (!carryOn(progress, 0.0, "densifying"))
        return cancelled(out, message);

    addRing(input.boundary, input.maxEdgeLength, points, edges);
    for (const auto &hole : input.holes)
    {
        if (hole.size() < 3)
        {
            message = "hole ring needs at least 3 points";
            return false;
        }
        addRing(hole, input.maxEdgeLength, points, edges);

        // Between rings, not within one: densifying a ring is a tight loop
        // of appends, and the run's real cost is the insertion below.
        if (!carryOn(progress, 0.05, "densifying"))
            return cancelled(out, message);
    }
    for (const auto &segment : input.constraintSegments)
    {
        addSegment(segment[0], segment[1], input.maxEdgeLength, points, edges);

        if (!carryOn(progress, 0.10, "densifying"))
            return cancelled(out, message);
    }
    nodeCrossings(points, edges);
    for (const auto &point : input.interiorPoints)
        points.emplace_back(point.x, point.y);

    // Vertex insertion: MeshKit polls for cancellation and reports progress
    // between batches, so a stop lands within one batch.
    if (!carryOn(progress, 0.20, "inserting vertices"))
        return cancelled(out, message);
    Delaunay                   triangulation;
    Delaunay::Vector<int>      vertexOfPoint;
    bool                       stop = false;
    double                     reached = 0.20;
    const double               total = static_cast<double>(points.size());
    const bool built = triangulation.build(
        points, &vertexOfPoint, [&] { return stop; },
        [&](std::uint64_t inserted) {
            reached = std::max(reached, 0.20 + 0.30 * std::min(1.0, double(inserted) / total));
            if (!carryOn(progress, reached, "inserting vertices"))
                stop = true;
        });
    if (stop)
        return cancelled(out, message);
    if (!built)
    {
        message = "triangulation failed: " + triangulation.errorMsg();
        return false;
    }

    // Edge insertion is this library's own loop over MeshKit's recovery.
    if (!carryOn(progress, 0.50, "inserting edges"))
        return cancelled(out, message);
    for (size_t k = 0; k < edges.size(); ++k)
    {
        if ((k & 0xFFFu) == 0xFFFu &&
            !carryOn(progress, 0.50 + 0.30 * double(k) / double(edges.size()),
                     "inserting edges"))
            return cancelled(out, message);
        const int a = vertexOfPoint[edges[k].a];
        const int b = vertexOfPoint[edges[k].b];
        if (a != b && !triangulation.insertConstraint(a, b))
        {
            const Point &p = points[edges[k].a], &q = points[edges[k].b];
            message = "triangulation failed: constraint (" + std::to_string(p.x()) + ", " +
                      std::to_string(p.y()) + ")-(" + std::to_string(q.x()) + ", " +
                      std::to_string(q.y()) + ") could not be recovered";
            if (!triangulation.errorMsg().empty())
                message += ": " + triangulation.errorMsg();
            return false;
        }
    }

    // Keep the triangles inside the boundary and outside every hole. The
    // rings are constraints, so no kept triangle straddles one and its
    // centroid decides.
    if (!carryOn(progress, 0.80, "removing holes"))
        return cancelled(out, message);
    int realVertices = 0;
    for (int v : vertexOfPoint)
        realVertices = std::max(realVertices, v + 1);
    std::vector<std::array<int, 3>> kept;
    for (const auto &triangle : triangulation.triangles())
    {
        if (!triangle.alive || triangle.v[0] >= realVertices ||
            triangle.v[1] >= realVertices || triangle.v[2] >= realVertices)
            continue; // removed, or touching the enclosing super triangle
        const auto &vertices = triangulation.vertices();
        const double cx = (vertices[triangle.v[0]].x() + vertices[triangle.v[1]].x() +
                           vertices[triangle.v[2]].x()) / 3.0;
        const double cy = (vertices[triangle.v[0]].y() + vertices[triangle.v[1]].y() +
                           vertices[triangle.v[2]].y()) / 3.0;
        if (!inside(input.boundary, cx, cy))
            continue;
        bool inHole = false;
        for (const auto &hole : input.holes)
            inHole = inHole || inside(hole, cx, cy);
        if (!inHole)
            kept.push_back({triangle.v[0], triangle.v[1], triangle.v[2]});
    }

    if (!carryOn(progress, 0.90, "extracting"))
        return cancelled(out, message);

    // Nodes are the distinct input points in first-occurrence order (MeshKit
    // numbers them so), followed by any crossing points.
    out = IO::MeshDefinition{};
    out.meshName = "triangulation";
    out.nodeX.reserve(static_cast<size_t>(realVertices));
    out.nodeY.reserve(static_cast<size_t>(realVertices));
    for (int v = 0; v < realVertices; ++v)
    {
        out.nodeX.push_back(triangulation.vertices()[v].x());
        out.nodeY.push_back(triangulation.vertices()[v].y());
    }
    out.faceNodeOffsets.push_back(0);
    size_t extracted = 0;
    for (const auto &triangle : kept)
    {
        // Often enough to answer a click, rarely enough not to cost
        // anything on a mesh of millions.
        if ((++extracted & 0xFFFu) == 0 &&
            !carryOn(progress, 0.90 + 0.10 * double(extracted) / double(kept.size()),
                     "extracting"))
            return cancelled(out, message);

        // MeshKit triangles are counter-clockwise.
        for (int corner : triangle)
            out.faceNodes.push_back(static_cast<int64_t>(corner));
        out.faceNodeOffsets.push_back(static_cast<int64_t>(out.faceNodes.size()));
    }

    if (out.faceCount() == 0)
    {
        message = "triangulation produced no triangles";
        return false;
    }
    // The last word is a report, not a question: the mesh is built, and a
    // caller that says stop now is answered by ignoring it rather than by
    // throwing away work that is already done.
    carryOn(progress, 1.0, "done");

    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Tools
