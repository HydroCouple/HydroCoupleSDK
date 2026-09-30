// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   meshadapters.cpp
 * \author Caleb Buahin
 * \brief  Implementation of the mesh, network and surface adapters.
 */

#include "hydrocouplesdk/spatial/meshadapters.h"
#include <cmath>
#include <map>
#include <utility>

#include <cstdint>
#include <string>

namespace HydroCouple::SDK::Spatial {

using HydroCouple::SDK::IO::MeshDefinition;

// ── MeshDefinition → PolyhedralSurface ───────────────────────────────────

PolyhedralSurface polyhedralSurfaceFromMesh(const MeshDefinition &mesh)
{
    PolyhedralSurface surface;

    const bool hasZ = mesh.nodeZ.size() == mesh.nodeX.size();

    const auto nodeAt = [&](int64_t node)
    {
        const size_t index = static_cast<size_t>(node);

        return hasZ ? Point(mesh.nodeX[index], mesh.nodeY[index],
                            mesh.nodeZ[index])
                    : Point(mesh.nodeX[index], mesh.nodeY[index]);
    };

    for (int64_t node = 0; node < mesh.nodeCount(); ++node)
        surface.addVertex(nodeAt(node));

    for (int64_t face = 0; face < mesh.faceCount(); ++face)
    {
        const int64_t begin = mesh.faceNodeOffsets[static_cast<size_t>(face)];
        const int64_t end = mesh.faceNodeOffsets[static_cast<size_t>(face) + 1];

        LineString ring;

        for (int64_t slot = begin; slot < end; ++slot)
            ring.addPoint(nodeAt(mesh.faceNodes[static_cast<size_t>(slot)]));

        // Closed explicitly: UGRID connectivity lists a face's nodes once,
        // and a ring whose last point is not its first is not a ring.
        if (end > begin)
            ring.addPoint(nodeAt(mesh.faceNodes[static_cast<size_t>(begin)]));

        surface.addPatch(std::make_shared<Polygon>(ring));
    }

    return surface;
}

// ── MeshViewAdapter ───────────────────────────────────────────────────────

MeshViewAdapter::MeshViewAdapter(MeshDefinition mesh) : m_mesh(std::move(mesh))
{
    // MeshDefinition keeps edge endpoints as pairs while the interface hands
    // out one flat span, so this is the single place the two differ.
    m_edgeNodes.reserve(m_mesh.edgeNodes.size() * 2);

    for (const std::array<int64_t, 2> &edge : m_mesh.edgeNodes)
    {
        m_edgeNodes.push_back(edge[0]);
        m_edgeNodes.push_back(edge[1]);
    }

    deriveTopologyAndMetrics();
}

void MeshViewAdapter::deriveTopologyAndMetrics()
{
    const int64_t nFaces = m_mesh.faceCount();
    const int64_t nNodes = m_mesh.nodeCount();

    // Edge lookup by unordered node pair; edges the definition already holds
    // keep their indexes, edges only implied by face loops are appended.
    std::map<std::pair<int64_t, int64_t>, int64_t> edgeOf;
    auto key = [](int64_t a, int64_t b) {
        return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
    };
    for (int64_t e = 0; e < static_cast<int64_t>(m_edgeNodes.size()) / 2; ++e)
        edgeOf.emplace(key(m_edgeNodes[2 * e], m_edgeNodes[2 * e + 1]), e);

    if (nFaces > 0)
    {
        m_faceEdgeOffsets.assign(1, 0);
        m_faceEdges.reserve(m_mesh.faceNodes.size());
        for (int64_t f = 0; f < nFaces; ++f)
        {
            const int64_t begin = m_mesh.faceNodeOffsets[static_cast<size_t>(f)];
            const int64_t end   = m_mesh.faceNodeOffsets[static_cast<size_t>(f) + 1];
            for (int64_t k = begin; k < end; ++k)
            {
                const int64_t a = m_mesh.faceNodes[static_cast<size_t>(k)];
                const int64_t b = m_mesh.faceNodes[static_cast<size_t>(k + 1 < end ? k + 1 : begin)];
                auto it = edgeOf.find(key(a, b));
                if (it == edgeOf.end())
                {
                    const int64_t e = static_cast<int64_t>(m_edgeNodes.size()) / 2;
                    m_edgeNodes.push_back(a);
                    m_edgeNodes.push_back(b);
                    it = edgeOf.emplace(key(a, b), e).first;
                }
                m_faceEdges.push_back(it->second);
            }
            m_faceEdgeOffsets.push_back(static_cast<int64_t>(m_faceEdges.size()));
        }

        // A ccw face lies to the left of each of its directed boundary edges;
        // if the stored edge runs the other way the face is on its right.
        const int64_t nEdges = static_cast<int64_t>(m_edgeNodes.size()) / 2;
        m_edgeFaces.assign(static_cast<size_t>(2 * nEdges), -1);
        for (int64_t f = 0; f < nFaces; ++f)
        {
            const int64_t begin = m_mesh.faceNodeOffsets[static_cast<size_t>(f)];
            const int64_t end   = m_mesh.faceNodeOffsets[static_cast<size_t>(f) + 1];
            for (int64_t k = begin; k < end; ++k)
            {
                const int64_t a = m_mesh.faceNodes[static_cast<size_t>(k)];
                const int64_t e = m_faceEdges[static_cast<size_t>(m_faceEdgeOffsets[static_cast<size_t>(f)] + (k - begin))];
                const bool sameDirection = m_edgeNodes[static_cast<size_t>(2 * e)] == a;
                m_edgeFaces[static_cast<size_t>(2 * e + (sameDirection ? 0 : 1))] = f;
            }
        }

        // Metrics: shoelace area and centroid per face.
        m_faceX.resize(static_cast<size_t>(nFaces));
        m_faceY.resize(static_cast<size_t>(nFaces));
        m_faceAreas.resize(static_cast<size_t>(nFaces));
        for (int64_t f = 0; f < nFaces; ++f)
        {
            const int64_t begin = m_mesh.faceNodeOffsets[static_cast<size_t>(f)];
            const int64_t end   = m_mesh.faceNodeOffsets[static_cast<size_t>(f) + 1];
            double twiceArea = 0.0, cx = 0.0, cy = 0.0;
            for (int64_t k = begin; k < end; ++k)
            {
                const int64_t a = m_mesh.faceNodes[static_cast<size_t>(k)];
                const int64_t b = m_mesh.faceNodes[static_cast<size_t>(k + 1 < end ? k + 1 : begin)];
                const double xa = m_mesh.nodeX[static_cast<size_t>(a)], ya = m_mesh.nodeY[static_cast<size_t>(a)];
                const double xb = m_mesh.nodeX[static_cast<size_t>(b)], yb = m_mesh.nodeY[static_cast<size_t>(b)];
                const double cross = xa * yb - xb * ya;
                twiceArea += cross;
                cx += (xa + xb) * cross;
                cy += (ya + yb) * cross;
            }
            const double area = 0.5 * twiceArea;
            m_faceAreas[static_cast<size_t>(f)] = std::abs(area);
            if (std::abs(area) > 0.0)
            {
                m_faceX[static_cast<size_t>(f)] = cx / (6.0 * area);
                m_faceY[static_cast<size_t>(f)] = cy / (6.0 * area);
            }
            else
            {
                // Degenerate face: fall back to the vertex mean.
                double sx = 0.0, sy = 0.0;
                for (int64_t k = begin; k < end; ++k)
                {
                    const int64_t a = m_mesh.faceNodes[static_cast<size_t>(k)];
                    sx += m_mesh.nodeX[static_cast<size_t>(a)];
                    sy += m_mesh.nodeY[static_cast<size_t>(a)];
                }
                const double n = static_cast<double>(end - begin);
                m_faceX[static_cast<size_t>(f)] = n > 0 ? sx / n : 0.0;
                m_faceY[static_cast<size_t>(f)] = n > 0 ? sy / n : 0.0;
            }
        }
    }

    // Edge lengths and unit normals (left face -> right face).
    const int64_t nEdges = static_cast<int64_t>(m_edgeNodes.size()) / 2;
    m_edgeLengths.resize(static_cast<size_t>(nEdges));
    m_edgeNormalX.resize(static_cast<size_t>(nEdges));
    m_edgeNormalY.resize(static_cast<size_t>(nEdges));
    for (int64_t e = 0; e < nEdges; ++e)
    {
        const int64_t a = m_edgeNodes[static_cast<size_t>(2 * e)];
        const int64_t b = m_edgeNodes[static_cast<size_t>(2 * e + 1)];
        if (a < 0 || b < 0 || a >= nNodes || b >= nNodes)
            continue;
        const double dx = m_mesh.nodeX[static_cast<size_t>(b)] - m_mesh.nodeX[static_cast<size_t>(a)];
        const double dy = m_mesh.nodeY[static_cast<size_t>(b)] - m_mesh.nodeY[static_cast<size_t>(a)];
        const double len = std::sqrt(dx * dx + dy * dy);
        m_edgeLengths[static_cast<size_t>(e)] = len;
        m_edgeNormalX[static_cast<size_t>(e)] = len > 0.0 ? dy / len : 0.0;
        m_edgeNormalY[static_cast<size_t>(e)] = len > 0.0 ? -dx / len : 0.0;
    }
}

// ── VertexAdapter ─────────────────────────────────────────────────────────

VertexAdapter::VertexAdapter(
    const Point &point, int64_t index,
    HydroCouple::Spatial::ISpatialReferenceSystem *srs)
    : PointAdapter(point, "vertex" + std::to_string(index), index, srs)
{
}

// ── EdgeAdapter ───────────────────────────────────────────────────────────

EdgeAdapter::EdgeAdapter(int64_t index, VertexAdapter *origin,
                         VertexAdapter *destination)
    : m_index(index), m_origin(origin), m_destination(destination)
{
}

HydroCouple::Spatial::IVertex *EdgeAdapter::orig() const
{
    return m_origin;
}

HydroCouple::Spatial::IVertex *EdgeAdapter::dest() const
{
    return m_destination;
}

// ── NetworkAdapter ────────────────────────────────────────────────────────

NetworkAdapter::NetworkAdapter(
    const Network &network, HydroCouple::Spatial::ISpatialReferenceSystem *srs)
    : Identity(network.id(), network.caption())
{
    m_vertices.reserve(static_cast<size_t>(network.vertexCount()));

    for (int i = 0; i < network.vertexCount(); ++i)
    {
        m_vertices.push_back(std::make_unique<VertexAdapter>(
            network.vertex(i), static_cast<int64_t>(i), srs));
    }

    m_edges.reserve(static_cast<size_t>(network.edgeCount()));

    MeshDefinition mesh;
    mesh.meshName = network.id().empty() ? "network" : network.id();
    mesh.nodeX.reserve(m_vertices.size());
    mesh.nodeY.reserve(m_vertices.size());

    for (const std::unique_ptr<VertexAdapter> &vertex : m_vertices)
    {
        mesh.nodeX.push_back(vertex->x());
        mesh.nodeY.push_back(vertex->y());
    }

    for (int i = 0; i < network.edgeCount(); ++i)
    {
        const NetworkEdge &edge = network.edge(i);

        // An edge naming a vertex outside the graph is a malformed network;
        // it is skipped rather than dereferenced.
        if (edge.origin < 0 || edge.dest < 0
            || edge.origin >= static_cast<int>(m_vertices.size())
            || edge.dest >= static_cast<int>(m_vertices.size()))
        {
            continue;
        }

        m_edges.push_back(std::make_unique<EdgeAdapter>(
            static_cast<int64_t>(m_edges.size()),
            m_vertices[static_cast<size_t>(edge.origin)].get(),
            m_vertices[static_cast<size_t>(edge.dest)].get()));

        // Each vertex reports one edge leaving it, which is the first that
        // happens to touch it.
        VertexAdapter *origin = m_vertices[static_cast<size_t>(edge.origin)].get();

        if (!origin->edge())
        {
            origin->setEdge(m_edges.back().get());
        }

        mesh.edgeNodes.push_back({static_cast<int64_t>(edge.origin),
                                  static_cast<int64_t>(edge.dest)});
    }

    m_meshView = std::make_unique<MeshViewAdapter>(std::move(mesh));
}

HydroCouple::Spatial::IEdge *NetworkAdapter::edge(int64_t index) const
{
    return index >= 0 && index < static_cast<int64_t>(m_edges.size())
               ? m_edges[static_cast<size_t>(index)].get()
               : nullptr;
}

HydroCouple::Spatial::IVertex *NetworkAdapter::vertex(int64_t index) const
{
    return index >= 0 && index < static_cast<int64_t>(m_vertices.size())
               ? m_vertices[static_cast<size_t>(index)].get()
               : nullptr;
}

// ── PolyhedralSurfaceAdapter ──────────────────────────────────────────────

PolyhedralSurfaceAdapter::PolyhedralSurfaceAdapter(
    const PolyhedralSurface &surface,
    HydroCouple::Spatial::ISpatialReferenceSystem *srs)
    : PolyhedralSurfaceAdapter(surface, MeshDefinition{}, srs)
{
}

PolyhedralSurfaceAdapter::PolyhedralSurfaceAdapter(
    const PolyhedralSurface &surface, MeshDefinition supplied,
    HydroCouple::Spatial::ISpatialReferenceSystem *srs)
    : GeometryAdapter("surface", 0, srs)
{
    setEnvelope(surface.envelope());
    m_area = surface.area();
    m_centroid = std::make_unique<PointAdapter>(surface.centroid(),
                                                "surface:centroid", 0, srs);
    m_centroidPoint = surface.centroid();

    m_patches.reserve(static_cast<size_t>(surface.numPatches()));

    MeshDefinition mesh;
    mesh.meshName = "surface";

    for (int i = 0; i < surface.numVertices(); ++i)
    {
        const Point vertex = surface.vertex(i);

        m_vertices.push_back(std::make_unique<VertexAdapter>(
            vertex, static_cast<int64_t>(i), srs));

        mesh.nodeX.push_back(vertex.x);
        mesh.nodeY.push_back(vertex.y);
        mesh.nodeZ.push_back(vertex.z);
    }

    for (int i = 0; i < surface.numPatches(); ++i)
    {
        const std::shared_ptr<Polygon> patch = surface.patch(i);

        if (!patch)
        {
            continue;
        }

        m_patches.push_back(std::make_unique<PolygonAdapter>(
            *patch, "patch" + std::to_string(i),
            static_cast<int64_t>(i), srs));

        // So a patch can name the surface it belongs to, which is what the
        // standard's IPolygon::polyhedralSurface() is for.
        m_patches.back()->setSurface(this);
    }

    // The caller's connectivity when there is any, because it is the only
    // place faces and edges survive: the vertex walk above rebuilds the
    // nodes and nothing else.
    m_meshView = std::make_unique<MeshViewAdapter>(
        supplied.nodeCount() > 0 ? std::move(supplied) : std::move(mesh));
}

std::vector<unsigned char> PolyhedralSurfaceAdapter::getWKB() const
{
    // WKB type 15: a count of patches, each a complete polygon record.
    std::vector<unsigned char> bytes;

    bytes.push_back(1);

    const uint32_t type = 15;
    const auto *typeBytes = reinterpret_cast<const unsigned char *>(&type);
    bytes.insert(bytes.end(), typeBytes, typeBytes + sizeof(type));

    const auto count = static_cast<uint32_t>(m_patches.size());
    const auto *countBytes = reinterpret_cast<const unsigned char *>(&count);
    bytes.insert(bytes.end(), countBytes, countBytes + sizeof(count));

    for (const std::unique_ptr<PolygonAdapter> &patch : m_patches)
    {
        const std::vector<unsigned char> patchBytes = patch->getWKB();
        bytes.insert(bytes.end(), patchBytes.begin(), patchBytes.end());
    }

    return bytes;
}

HydroCouple::Spatial::IPolygon *PolyhedralSurfaceAdapter::patch(
    int64_t index) const
{
    return index >= 0 && index < static_cast<int64_t>(m_patches.size())
               ? m_patches[static_cast<size_t>(index)].get()
               : nullptr;
}

HydroCouple::Spatial::IVertex *PolyhedralSurfaceAdapter::vertex(
    int64_t index) const
{
    return index >= 0 && index < static_cast<int64_t>(m_vertices.size())
               ? m_vertices[static_cast<size_t>(index)].get()
               : nullptr;
}

std::unique_ptr<HydroCouple::Spatial::IPoint>
PolyhedralSurfaceAdapter::centroid() const
{
    return std::make_unique<PointAdapter>(m_centroidPoint, "surface:centroid", 0,
                                          spatialReferenceSystem());
}

} // namespace HydroCouple::SDK::Spatial
