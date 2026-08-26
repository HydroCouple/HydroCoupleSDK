// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   meshadapters.cpp
 * \author Caleb Buahin
 * \brief  Implementation of the mesh, network and surface adapters.
 */

#include "hydrocouplesdk/spatial/meshadapters.h"

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
}

// ── VertexAdapter ─────────────────────────────────────────────────────────

VertexAdapter::VertexAdapter(
    const Point &point, unsigned int index,
    HydroCouple::Spatial::ISpatialReferenceSystem *srs)
    : PointAdapter(point, "vertex" + std::to_string(index), index, srs)
{
}

// ── EdgeAdapter ───────────────────────────────────────────────────────────

EdgeAdapter::EdgeAdapter(unsigned int index, VertexAdapter *origin,
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
            network.vertex(i), static_cast<unsigned int>(i), srs));
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
            static_cast<unsigned int>(m_edges.size()),
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

    m_patches.reserve(static_cast<size_t>(surface.numPatches()));

    MeshDefinition mesh;
    mesh.meshName = "surface";

    for (int i = 0; i < surface.numVertices(); ++i)
    {
        const Point vertex = surface.vertex(i);

        m_vertices.push_back(std::make_unique<VertexAdapter>(
            vertex, static_cast<unsigned int>(i), srs));

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
            static_cast<unsigned int>(i), srs));

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

} // namespace HydroCouple::SDK::Spatial
