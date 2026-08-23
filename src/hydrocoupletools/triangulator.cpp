// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "hydrocouplesdk/tools/triangulator.h"

#include <CDT.h>

#include <cmath>

namespace HydroCouple::SDK::Tools {

namespace {

double distance(const Spatial::Point &a, const Spatial::Point &b)
{
    return std::hypot(b.x - a.x, b.y - a.y);
}

/*!
 * Adds the closed ring's vertices (densified) and its closing edges.
 */
void addRing(const std::vector<Spatial::Point> &ring, double maxEdgeLength,
             std::vector<CDT::V2d<double>> &vertices,
             std::vector<CDT::Edge> &edges)
{
    if (ring.size() < 3)
        return;

    std::vector<CDT::VertInd> ringIndices;
    for (size_t p = 0; p < ring.size(); ++p)
    {
        const Spatial::Point &a = ring[p];
        const Spatial::Point &b = ring[(p + 1) % ring.size()];

        ringIndices.push_back(static_cast<CDT::VertInd>(vertices.size()));
        vertices.push_back({a.x, a.y});

        if (maxEdgeLength > 0.0)
        {
            const double length = distance(a, b);
            const int segments =
                static_cast<int>(std::ceil(length / maxEdgeLength));
            for (int s = 1; s < segments; ++s)
            {
                const double t = static_cast<double>(s) / segments;
                ringIndices.push_back(
                    static_cast<CDT::VertInd>(vertices.size()));
                vertices.push_back(
                    {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)});
            }
        }
    }
    for (size_t v = 0; v < ringIndices.size(); ++v)
        edges.emplace_back(ringIndices[v],
                           ringIndices[(v + 1) % ringIndices.size()]);
}

/*!
 * Adds one open constraint segment (densified).
 */
void addSegment(const Spatial::Point &a, const Spatial::Point &b,
                double maxEdgeLength,
                std::vector<CDT::V2d<double>> &vertices,
                std::vector<CDT::Edge> &edges)
{
    std::vector<CDT::VertInd> indices;
    indices.push_back(static_cast<CDT::VertInd>(vertices.size()));
    vertices.push_back({a.x, a.y});
    if (maxEdgeLength > 0.0)
    {
        const double length = distance(a, b);
        const int segments =
            static_cast<int>(std::ceil(length / maxEdgeLength));
        for (int s = 1; s < segments; ++s)
        {
            const double t = static_cast<double>(s) / segments;
            indices.push_back(static_cast<CDT::VertInd>(vertices.size()));
            vertices.push_back(
                {a.x + t * (b.x - a.x), a.y + t * (b.y - a.y)});
        }
    }
    indices.push_back(static_cast<CDT::VertInd>(vertices.size()));
    vertices.push_back({b.x, b.y});
    for (size_t v = 0; v + 1 < indices.size(); ++v)
        edges.emplace_back(indices[v], indices[v + 1]);
}

} // namespace

bool Triangulator::triangulate(const TriangulationInput &input,
                               IO::MeshDefinition &out, std::string &message)
{
    if (input.boundary.size() < 3)
    {
        message = "boundary ring needs at least 3 points";
        return false;
    }

    std::vector<CDT::V2d<double>> vertices;
    std::vector<CDT::Edge>        edges;

    addRing(input.boundary, input.maxEdgeLength, vertices, edges);
    for (const auto &hole : input.holes)
    {
        if (hole.size() < 3)
        {
            message = "hole ring needs at least 3 points";
            return false;
        }
        addRing(hole, input.maxEdgeLength, vertices, edges);
    }
    for (const auto &segment : input.constraintSegments)
        addSegment(segment[0], segment[1], input.maxEdgeLength, vertices,
                   edges);
    for (const auto &point : input.interiorPoints)
        vertices.push_back({point.x, point.y});

    try
    {
        CDT::Triangulation<double> triangulation(
            CDT::VertexInsertionOrder::Auto,
            CDT::IntersectingConstraintEdges::TryResolve,
            /*minDistToConstraintEdge=*/0.0);

        CDT::RemoveDuplicatesAndRemapEdges(vertices, edges);
        triangulation.insertVertices(vertices);
        triangulation.insertEdges(edges);
        triangulation.eraseOuterTrianglesAndHoles();

        out = IO::MeshDefinition{};
        out.meshName = "triangulation";
        out.nodeX.reserve(triangulation.vertices.size());
        out.nodeY.reserve(triangulation.vertices.size());
        for (const auto &vertex : triangulation.vertices)
        {
            out.nodeX.push_back(vertex.x);
            out.nodeY.push_back(vertex.y);
        }
        out.faceNodeOffsets.push_back(0);
        for (const auto &triangle : triangulation.triangles)
        {
            // CDT triangles are counter-clockwise.
            out.faceNodes.push_back(
                static_cast<int64_t>(triangle.vertices[0]));
            out.faceNodes.push_back(
                static_cast<int64_t>(triangle.vertices[1]));
            out.faceNodes.push_back(
                static_cast<int64_t>(triangle.vertices[2]));
            out.faceNodeOffsets.push_back(
                static_cast<int64_t>(out.faceNodes.size()));
        }
    }
    catch (const std::exception &exception)
    {
        message = std::string("triangulation failed: ") + exception.what();
        return false;
    }

    if (out.faceCount() == 0)
    {
        message = "triangulation produced no triangles";
        return false;
    }
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Tools
