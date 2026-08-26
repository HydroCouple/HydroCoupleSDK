// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   meshadapters.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  MeshViewAdapter, NetworkAdapter and PolyhedralSurfaceAdapter —
 *         the SDK's mesh and network values through the standard's
 *         interfaces.
 *
 * MeshDefinition was written to mirror IMeshView's vocabulary, so the adapter
 * between them is a set of spans over the same arrays: a component that has a
 * mesh can publish it through the standard without copying or transforming
 * anything.
 *
 * \threadsafety Reads are safe after construction; mutations are not.
 */

#ifndef HYDROCOUPLESDK_SPATIAL_MESHADAPTERS_H
#define HYDROCOUPLESDK_SPATIAL_MESHADAPTERS_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplesdk/spatial/geometryadapters.h"
#include "hydrocouplesdk/spatial/network.h"
#include "hydrocouplesdk/spatial/polyhedralsurface.h"
#include "hydrocouplespatial.h"

#include <memory>
#include <vector>

namespace HydroCouple::SDK::Spatial {

/*!
 * \brief IMeshView over a MeshDefinition.
 *
 * The two describe the same thing in the same layout, so this hands out spans
 * into the definition rather than copying it. The definition must outlive the
 * adapter.
 */
class HYDROCOUPLESDK_EXPORT MeshViewAdapter final
    : public HydroCouple::Spatial::IMeshView
{
public:
    /*!
     * \brief Construct over a copy of \p mesh.
     */
    explicit MeshViewAdapter(HydroCouple::SDK::IO::MeshDefinition mesh);

    ~MeshViewAdapter() override = default;

    /*!
     * \brief The wrapped mesh.
     */
    [[nodiscard]] const HydroCouple::SDK::IO::MeshDefinition &mesh()
        const noexcept
    {
        return m_mesh;
    }

    [[nodiscard]] int64_t nodeCount() const override
    {
        return m_mesh.nodeCount();
    }

    [[nodiscard]] int64_t edgeCount() const override
    {
        return m_mesh.edgeCount();
    }

    [[nodiscard]] int64_t faceCount() const override
    {
        return m_mesh.faceCount();
    }

    [[nodiscard]] std::span<const double> nodeX() const override
    {
        return m_mesh.nodeX;
    }

    [[nodiscard]] std::span<const double> nodeY() const override
    {
        return m_mesh.nodeY;
    }

    [[nodiscard]] std::span<const double> nodeZ() const override
    {
        return m_mesh.nodeZ;
    }

    [[nodiscard]] std::span<const int64_t> faceNodeOffsets() const override
    {
        return m_mesh.faceNodeOffsets;
    }

    [[nodiscard]] std::span<const int64_t> faceNodes() const override
    {
        return m_mesh.faceNodes;
    }

    [[nodiscard]] std::span<const int64_t> edgeNodes() const override
    {
        return m_edgeNodes;
    }

private:
    HydroCouple::SDK::IO::MeshDefinition m_mesh;

    //! Edge endpoints flattened to pairs, because MeshDefinition stores them
    //! as an array of arrays and the interface hands out one flat span.
    std::vector<int64_t> m_edgeNodes;
};

/*!
 * \brief IVertex over a point in a Network.
 */
class HYDROCOUPLESDK_EXPORT VertexAdapter final
    : public PointAdapter,
      public virtual HydroCouple::Spatial::IVertex
{
public:
    /*!
     * \brief Construct at \p point with graph index \p index.
     */
    VertexAdapter(const Point &point, unsigned int index,
                  HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    ~VertexAdapter() override = default;

    [[nodiscard]] unsigned int index() const override
    {
        return PointAdapter::index();
    }

    /*!
     * \brief An edge leaving this vertex, or nullptr.
     *
     * Set by NetworkAdapter once its edges exist.
     */
    [[nodiscard]] HydroCouple::Spatial::IEdge *edge() const override
    {
        return m_edge;
    }

    /*!
     * \brief Records the edge this vertex reports.
     */
    void setEdge(HydroCouple::Spatial::IEdge *edge) noexcept { m_edge = edge; }

private:
    HydroCouple::Spatial::IEdge *m_edge = nullptr;
};

/*!
 * \brief IEdge over a Network edge.
 *
 * \note The quad-edge navigation the interface defines (rot, sym, origNext,
 *       …) needs the topological structure a quad-edge mesh keeps, and the
 *       SDK's Network stores plain endpoint pairs. Those accessors report
 *       nullptr rather than a wrong neighbour; orig(), dest() and index() —
 *       everything a viewer or a writer reads — are real.
 */
class HYDROCOUPLESDK_EXPORT EdgeAdapter final
    : public HydroCouple::Spatial::IEdge
{
public:
    /*!
     * \brief Construct between two vertices.
     */
    EdgeAdapter(unsigned int index, VertexAdapter *origin,
                VertexAdapter *destination);

    ~EdgeAdapter() override = default;

    [[nodiscard]] unsigned int index() const override { return m_index; }

    [[nodiscard]] HydroCouple::Spatial::IVertex *orig() const override;

    [[nodiscard]] HydroCouple::Spatial::IVertex *dest() const override;

    [[nodiscard]] HydroCouple::Spatial::IPolygon *left() const override
    {
        return nullptr;
    }
    [[nodiscard]] HydroCouple::Spatial::IPolygon *right() const override
    {
        return nullptr;
    }
    [[nodiscard]] HydroCouple::Spatial::IPolygon *face() const override
    {
        return nullptr;
    }

    [[nodiscard]] IEdge *rot() const override { return nullptr; }
    [[nodiscard]] IEdge *invRot() const override { return nullptr; }
    [[nodiscard]] IEdge *sym() const override { return nullptr; }
    [[nodiscard]] IEdge *origNext() const override { return nullptr; }
    [[nodiscard]] IEdge *origPrev() const override { return nullptr; }
    [[nodiscard]] IEdge *destNext() const override { return nullptr; }
    [[nodiscard]] IEdge *destPrev() const override { return nullptr; }
    [[nodiscard]] IEdge *leftNext() const override { return nullptr; }
    [[nodiscard]] IEdge *leftPrev() const override { return nullptr; }
    [[nodiscard]] IEdge *rightNext() const override { return nullptr; }
    [[nodiscard]] IEdge *rightPrev() const override { return nullptr; }

private:
    unsigned int m_index = 0;
    VertexAdapter *m_origin = nullptr;
    VertexAdapter *m_destination = nullptr;
};

/*!
 * \brief INetwork over an SDK Network.
 */
class HYDROCOUPLESDK_EXPORT NetworkAdapter final
    : public HydroCouple::SDK::Identity,
      public virtual HydroCouple::Spatial::INetwork
{
public:
    /*!
     * \brief Construct over a copy of \p network.
     */
    explicit NetworkAdapter(
        const Network &network,
        HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    ~NetworkAdapter() override = default;

    [[nodiscard]] int64_t edgeCount() const override
    {
        return static_cast<int64_t>(m_edges.size());
    }

    [[nodiscard]] HydroCouple::Spatial::IEdge *edge(int64_t index) const override;

    [[nodiscard]] int64_t vertexCount() const override
    {
        return static_cast<int64_t>(m_vertices.size());
    }

    [[nodiscard]] HydroCouple::Spatial::IVertex *vertex(
        int64_t index) const override;

    /*!
     * \brief A mesh view of the network: its vertices as nodes, its edges as
     *        edges, and no faces.
     */
    [[nodiscard]] const HydroCouple::Spatial::IMeshView *meshView()
        const override
    {
        return m_meshView.get();
    }

private:
    std::vector<std::unique_ptr<VertexAdapter>> m_vertices;
    std::vector<std::unique_ptr<EdgeAdapter>> m_edges;
    std::unique_ptr<MeshViewAdapter> m_meshView;
};

/*!
 * \brief A PolyhedralSurface over \p mesh: its nodes as vertices, its faces
 *        as patches.
 *
 * The bridge between the SDK's serialization vocabulary and its geometry
 * one. MeshDefinition is a structure of arrays because that is what a UGRID
 * file is; IPolyhedralSurface hands out objects because that is what a
 * consumer draws and picks against. Something has to cross between them, and
 * doing it here means a reopened run, a mesh read from a file and a mesh a
 * component published all arrive at a consumer the same way.
 *
 * A mesh with no faces converts to a surface with vertices and no patches,
 * which is what a point cloud or a set of network nodes *is* — not an error
 * and not a reason to invent connectivity.
 *
 * \param[in] mesh The mesh to convert; copied into the result.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT PolyhedralSurface polyhedralSurfaceFromMesh(
    const HydroCouple::SDK::IO::MeshDefinition &mesh);

/*!
 * \brief IPolyhedralSurface over an SDK PolyhedralSurface.
 */
class HYDROCOUPLESDK_EXPORT PolyhedralSurfaceAdapter final
    : public GeometryAdapter,
      public virtual HydroCouple::Spatial::IPolyhedralSurface
{
public:
    /*!
     * \brief Construct over a copy of \p surface.
     *
     * \note The mesh view of a surface built this way carries its nodes and
     *       no connectivity. A PolyhedralSurface records polygons, not which
     *       vertex indices each polygon used, so faces and edges cannot be
     *       recovered from it without matching coordinates back to vertices
     *       — which guesses, and guesses wrong wherever two vertices
     *       coincide. A caller that *has* the connectivity should pass it,
     *       through the constructor below.
     */
    explicit PolyhedralSurfaceAdapter(
        const PolyhedralSurface &surface,
        HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    /*!
     * \brief Construct over \p surface, with \p mesh as its bulk view.
     *
     * For the caller that built the surface from a mesh and still holds it.
     * The standard calls meshView() "the accessor partitioners,
     * interpolating adapters, IO writers, and device staging must use", so a
     * view reporting no faces and no edges for a surface that has both is
     * not a lesser answer — it is a wrong one, and it is wrong in a way that
     * looks like an empty mesh rather than like a missing feature.
     *
     * \param[in] surface The surface, as patches and vertices.
     * \param[in] mesh    The connectivity it was built from; moved.
     * \param[in] srs     Spatial reference, borrowed.
     */
    PolyhedralSurfaceAdapter(
        const PolyhedralSurface &surface,
        HydroCouple::SDK::IO::MeshDefinition mesh,
        HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    ~PolyhedralSurfaceAdapter() override = default;

    [[nodiscard]] int64_t patchCount() const override
    {
        return static_cast<int64_t>(m_patches.size());
    }

    [[nodiscard]] HydroCouple::Spatial::IPolygon *patch(
        int64_t index) const override;

    [[nodiscard]] int64_t vertexCount() const override
    {
        return static_cast<int64_t>(m_vertices.size());
    }

    [[nodiscard]] HydroCouple::Spatial::IVertex *vertex(
        int64_t index) const override;

    /*!
     * \brief The polygons bounding \p polygon.
     *
     * \note Needs adjacency the SDK's PolyhedralSurface does not keep;
     *       reports nullptr rather than a wrong neighbourhood.
     */
    [[nodiscard]] HydroCouple::Spatial::IMultiPolygon *boundingPolygons(
        const HydroCouple::Spatial::IPolygon *) const override
    {
        return nullptr;
    }

    [[nodiscard]] bool isClosed() const override { return false; }

    [[nodiscard]] const HydroCouple::Spatial::IMeshView *meshView()
        const override
    {
        return m_meshView.get();
    }

    // ── IGeometry / ISurface ──────────────────────────────────────────────

    [[nodiscard]] int dimension() const override { return 2; }

    [[nodiscard]] GeometryType geometryType() const override
    {
        return GeometryType::PolyhedralSurface;
    }

    [[nodiscard]] std::vector<unsigned char> getWKB() const override;

    [[nodiscard]] bool isEmpty() const override { return m_patches.empty(); }

    [[nodiscard]] double area() const override { return m_area; }

    [[nodiscard]] HydroCouple::Spatial::IPoint *centroid() const override
    {
        return m_centroid.get();
    }

    [[nodiscard]] HydroCouple::Spatial::IPoint *pointOnSurface() const override
    {
        return m_centroid.get();
    }

    [[nodiscard]] HydroCouple::Spatial::IMultiCurve *boundaryMultiCurve()
        const override
    {
        return nullptr;
    }

private:
    double m_area = 0.0;
    std::unique_ptr<PointAdapter> m_centroid;
    std::vector<std::unique_ptr<PolygonAdapter>> m_patches;
    std::vector<std::unique_ptr<VertexAdapter>> m_vertices;
    std::unique_ptr<MeshViewAdapter> m_meshView;
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_SPATIAL_MESHADAPTERS_H
