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
 * The adapters take their input when they are constructed and do not follow
 * later changes to it: MeshViewAdapter keeps its own MeshDefinition, and
 * NetworkAdapter and PolyhedralSurfaceAdapter rebuild what they serve from
 * the value they were given. To publish a changed mesh, build a new adapter.
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
 *
 * In fact the constructor takes the definition by value and the adapter owns
 * that copy (pass it with std::move to avoid copying the arrays): the caller's
 * definition need not outlive the adapter, and the spans stay valid for the
 * adapter's life. The node and face arrays are served directly; the edge list
 * is flattened to pairs; and the topology and metrics the standard requires
 * of a mesh with faces are derived once at construction:
 *
 * - Edges implied by face loops but missing from the definition are appended
 *   after the definition's own, which keep their indexes.
 * - faceEdgeOffsets() and faceEdges() list each face's edges in node order,
 *   edge k joining face node k to node k + 1 (the last to the first).
 * - edgeFaces() gives, for edge e, the face on its left (looking from
 *   edgeNodes()[2e] to edgeNodes()[2e+1]) at 2e and the face on its right at
 *   2e + 1, with -1 for the outside of the mesh. It assumes counterclockwise
 *   faces: a face whose loop runs along an edge's direction is placed on its
 *   left.
 * - faceAreas(), faceX() and faceY() are the shoelace area and centroid of
 *   each face in plan, \f$A = \tfrac12 \left|\sum_k (x_k y_{k+1} - x_{k+1} y_k)\right|\f$;
 *   a face of zero area gets the mean of its nodes as its point.
 * - edgeLengths() are plan lengths and edgeNormalX()/edgeNormalY() the unit
 *   normal \f$(\Delta y, -\Delta x)/L\f$, which points from the left face to
 *   the right face; zero for an edge of zero length.
 *
 * Nothing is validated: a face naming a node that does not exist is read out
 * of bounds. Check the definition with MeshDefinition::validate() before
 * adapting one that came from outside the program. A definition without
 * faces (a network or a point cloud) gets edge lengths and normals but empty
 * face spans and an empty edgeFaces().
 */
class HYDROCOUPLESDK_EXPORT MeshViewAdapter final
    : public HydroCouple::Spatial::IMeshView
{
public:
    /*!
     * \brief Construct over a copy of \p mesh.
     * \param[in] mesh The mesh; taken by value and owned (move it in to avoid
     *                 a copy). Its edges, if any, keep their indexes.
     */
    explicit MeshViewAdapter(HydroCouple::SDK::IO::MeshDefinition mesh);

    //! Destroys the adapter and its mesh.
    ~MeshViewAdapter() override = default;

    /*!
     * \brief The wrapped mesh.
     * \returns The owned definition as given; the edges derived from face
     *          loops are not added to it.
     */
    [[nodiscard]] const HydroCouple::SDK::IO::MeshDefinition &mesh()
        const noexcept
    {
        return m_mesh;
    }

    /*!
     * \brief The number of nodes (IMeshView::nodeCount()).
     * \returns MeshDefinition::nodeCount(), the length of nodeX.
     */
    [[nodiscard]] int64_t nodeCount() const override
    {
        return m_mesh.nodeCount();
    }

    /*!
     * \brief The number of edges (IMeshView::edgeCount()).
     * \returns The definition's edges plus those derived from face loops.
     */
    [[nodiscard]] int64_t edgeCount() const override
    {
        // Includes the edges derived from face loops (see
        // deriveTopologyAndMetrics()), so it always agrees with edgeNodes().
        return static_cast<int64_t>(m_edgeNodes.size()) / 2;
    }

    /*!
     * \brief The number of faces (IMeshView::faceCount()).
     * \returns MeshDefinition::faceCount(); 0 for a network or point cloud.
     */
    [[nodiscard]] int64_t faceCount() const override
    {
        return m_mesh.faceCount();
    }

    /*!
     * \brief Node x coordinates (IMeshView::nodeX()).
     * \returns A span over the definition's nodeX, nodeCount() values.
     */
    [[nodiscard]] std::span<const double> nodeX() const override
    {
        return m_mesh.nodeX;
    }

    /*!
     * \brief Node y coordinates (IMeshView::nodeY()).
     * \returns A span over the definition's nodeY.
     */
    [[nodiscard]] std::span<const double> nodeY() const override
    {
        return m_mesh.nodeY;
    }

    /*!
     * \brief Node elevations (IMeshView::nodeZ()).
     * \returns A span over the definition's nodeZ: nodeCount() values, or
     *          empty for a two-dimensional mesh.
     */
    [[nodiscard]] std::span<const double> nodeZ() const override
    {
        return m_mesh.nodeZ;
    }

    /*!
     * \brief CSR offsets of the face-node lists (IMeshView::faceNodeOffsets()).
     * \returns A span over the definition's faceNodeOffsets: faceCount() + 1
     *          values, or empty without faces.
     */
    [[nodiscard]] std::span<const int64_t> faceNodeOffsets() const override
    {
        return m_mesh.faceNodeOffsets;
    }

    /*!
     * \brief Concatenated face-node lists (IMeshView::faceNodes()).
     * \returns A span over the definition's faceNodes, in the order given
     *          (counterclockwise per face by the standard's convention).
     */
    [[nodiscard]] std::span<const int64_t> faceNodes() const override
    {
        return m_mesh.faceNodes;
    }

    /*!
     * \brief Edge endpoints as flat pairs (IMeshView::edgeNodes()).
     * \returns 2·edgeCount() node indexes: the definition's edges first, in
     *          their order, then the derived ones.
     */
    [[nodiscard]] std::span<const int64_t> edgeNodes() const override
    {
        return m_edgeNodes;
    }

    // Topology and metrics derived once at construction (see the .cpp): a
    // definition that carries no edges gets them from its face loops, so a
    // surface always answers edgeFaces() and a conservative remap on either
    // side of a coupling integrates over the same areas.

    /*!
     * \brief CSR offsets of the face-edge lists (IMeshView::faceEdgeOffsets()).
     * \returns faceCount() + 1 values; empty without faces.
     */
    [[nodiscard]] std::span<const int64_t> faceEdgeOffsets() const override
    {
        return m_faceEdgeOffsets;
    }

    /*!
     * \brief Concatenated face-edge lists (IMeshView::faceEdges()).
     * \returns Each face's edge indexes in node order; empty without faces.
     */
    [[nodiscard]] std::span<const int64_t> faceEdges() const override
    {
        return m_faceEdges;
    }

    /*!
     * \brief Left and right face of every edge (IMeshView::edgeFaces()).
     * \returns 2·edgeCount() face indexes, -1 for the outside; empty without
     *          faces. An edge of the definition that no face uses has -1 on
     *          both sides.
     */
    [[nodiscard]] std::span<const int64_t> edgeFaces() const override
    {
        return m_edgeFaces;
    }

    /*!
     * \brief Face centroid x (IMeshView::faceX()).
     * \returns faceCount() values; empty without faces.
     */
    [[nodiscard]] std::span<const double> faceX() const override { return m_faceX; }
    /*!
     * \brief Face centroid y (IMeshView::faceY()).
     * \returns faceCount() values; empty without faces.
     */
    [[nodiscard]] std::span<const double> faceY() const override { return m_faceY; }
    /*!
     * \brief Face plan areas (IMeshView::faceAreas()).
     * \returns faceCount() values in squared coordinate units, never
     *          negative; empty without faces.
     */
    [[nodiscard]] std::span<const double> faceAreas() const override { return m_faceAreas; }
    /*!
     * \brief Edge plan lengths (IMeshView::edgeLengths()).
     * \returns edgeCount() values in coordinate units (z is ignored); 0 for
     *          an edge naming a node that does not exist.
     */
    [[nodiscard]] std::span<const double> edgeLengths() const override { return m_edgeLengths; }
    /*!
     * \brief x of each edge's unit normal (IMeshView::edgeNormalX()).
     * \returns edgeCount() values, \f$\Delta y / L\f$.
     */
    [[nodiscard]] std::span<const double> edgeNormalX() const override { return m_edgeNormalX; }
    /*!
     * \brief y of each edge's unit normal (IMeshView::edgeNormalY()).
     * \returns edgeCount() values, \f$-\Delta x / L\f$.
     */
    [[nodiscard]] std::span<const double> edgeNormalY() const override { return m_edgeNormalY; }

private:
    /*!
     * \brief Derives the face-edge and edge-face topology and the metric
     *        arrays from m_mesh, appending implied edges to m_edgeNodes.
     */
    void deriveTopologyAndMetrics();

    HydroCouple::SDK::IO::MeshDefinition m_mesh; //!< The owned definition.

    //! Edge endpoints flattened to pairs, because MeshDefinition stores them
    //! as an array of arrays and the interface hands out one flat span.
    std::vector<int64_t> m_edgeNodes;
    std::vector<int64_t> m_faceEdgeOffsets; //!< CSR offsets into m_faceEdges.
    std::vector<int64_t> m_faceEdges;       //!< Edge indexes of every face.
    std::vector<int64_t> m_edgeFaces;       //!< Left and right face of every edge.
    std::vector<double> m_faceX, m_faceY, m_faceAreas;
    std::vector<double> m_edgeLengths, m_edgeNormalX, m_edgeNormalY;
};

/*! \var std::vector<double> HydroCouple::SDK::Spatial::MeshViewAdapter::m_faceX
 *  \brief Centroid x of every face.
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::MeshViewAdapter::m_faceY
 *  \brief Centroid y of every face.
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::MeshViewAdapter::m_faceAreas
 *  \brief Plan area of every face.
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::MeshViewAdapter::m_edgeLengths
 *  \brief Plan length of every edge.
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::MeshViewAdapter::m_edgeNormalX
 *  \brief x of every edge's unit normal.
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::MeshViewAdapter::m_edgeNormalY
 *  \brief y of every edge's unit normal.
 */

/*!
 * \brief IVertex over a point in a Network.
 *
 * A PointAdapter with the id "vertexI", I being its index, that can also name one edge
 * leaving it. NetworkAdapter and PolyhedralSurfaceAdapter own their vertex
 * adapters; the pointers they hand out stay valid for the owner's life.
 */
class HYDROCOUPLESDK_EXPORT VertexAdapter final
    : public PointAdapter,
      public virtual HydroCouple::Spatial::IVertex
{
public:
    /*!
     * \brief Construct at \p point with graph index \p index.
     * \param[in] point The position; copied.
     * \param[in] index The vertex's index in its network or surface; also
     *                  forms its id.
     * \param[in] srs   Spatial reference (borrowed; may be nullptr).
     */
    VertexAdapter(const Point &point, int64_t index,
                  HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    //! Destroys the adapter; the edge is not owned.
    ~VertexAdapter() override = default;

    /*!
     * \brief The vertex's index (IGeometry::index()).
     * \returns The index given at construction.
     */
    [[nodiscard]] int64_t index() const override
    {
        return PointAdapter::index();
    }

    /*!
     * \brief An edge leaving this vertex, or nullptr.
     *
     * Set by NetworkAdapter once its edges exist.
     *
     * \returns The first edge of the network whose origin is this vertex;
     *          nullptr for a vertex no edge leaves (an outlet, for example)
     *          and for every vertex of a PolyhedralSurfaceAdapter.
     */
    [[nodiscard]] HydroCouple::Spatial::IEdge *edge() const override
    {
        return m_edge;
    }

    /*!
     * \brief Records the edge this vertex reports.
     * \param[in] edge An edge leaving this vertex (borrowed; may be nullptr).
     */
    void setEdge(HydroCouple::Spatial::IEdge *edge) noexcept { m_edge = edge; }

private:
    HydroCouple::Spatial::IEdge *m_edge = nullptr; //!< Borrowed outgoing edge; may be nullptr.
};

/*!
 * \brief IEdge over a Network edge.
 *
 * \note The quad-edge navigation the interface defines (rot, sym, origNext,
 *       …) needs the topological structure a quad-edge mesh keeps, and the
 *       SDK's Network stores plain endpoint pairs. Those accessors report
 *       nullptr rather than a wrong neighbor; orig(), dest() and index() —
 *       everything a viewer or a writer reads — are real.
 *
 * The interface documents the navigation accessors as never returning null;
 * this adapter does return null, so a consumer that walks a quad-edge
 * structure cannot use it. A network edge has no faces either.
 */
class HYDROCOUPLESDK_EXPORT EdgeAdapter final
    : public HydroCouple::Spatial::IEdge
{
public:
    /*!
     * \brief Construct between two vertices.
     * \param[in] index       The edge's index in its network.
     * \param[in] origin      The vertex the edge leaves (borrowed).
     * \param[in] destination The vertex the edge enters (borrowed).
     */
    EdgeAdapter(int64_t index, VertexAdapter *origin,
                VertexAdapter *destination);

    //! Destroys the adapter; the vertices are not owned.
    ~EdgeAdapter() override = default;

    /*!
     * \brief The edge's index (IEdge::index()).
     * \returns The index given at construction, which is also its position
     *          in the owning network's meshView()->edgeNodes().
     */
    [[nodiscard]] int64_t index() const override { return m_index; }

    /*!
     * \brief The vertex the edge leaves (IEdge::orig()).
     * \returns The borrowed origin vertex.
     */
    [[nodiscard]] HydroCouple::Spatial::IVertex *orig() const override;

    /*!
     * \brief The vertex the edge enters (IEdge::dest()).
     * \returns The borrowed destination vertex.
     */
    [[nodiscard]] HydroCouple::Spatial::IVertex *dest() const override;

    /*!
     * \brief The face on the left (IEdge::left()).
     * \returns nullptr; a network edge has no faces.
     */
    [[nodiscard]] HydroCouple::Spatial::IPolygon *left() const override
    {
        return nullptr;
    }
    /*!
     * \brief The face on the right (IEdge::right()).
     * \returns nullptr; a network edge has no faces.
     */
    [[nodiscard]] HydroCouple::Spatial::IPolygon *right() const override
    {
        return nullptr;
    }
    /*!
     * \brief The target face of a dual edge (IEdge::face()).
     * \returns nullptr.
     */
    [[nodiscard]] HydroCouple::Spatial::IPolygon *face() const override
    {
        return nullptr;
    }

    /*!
     * \brief Quad-edge dual, right to left (IEdge::rot()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *rot() const override { return nullptr; }
    /*!
     * \brief Quad-edge dual, left to right (IEdge::invRot()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *invRot() const override { return nullptr; }
    /*!
     * \brief The reversed edge (IEdge::sym()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *sym() const override { return nullptr; }
    /*!
     * \brief Next edge counterclockwise around the origin (IEdge::origNext()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *origNext() const override { return nullptr; }
    /*!
     * \brief Next edge clockwise around the origin (IEdge::origPrev()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *origPrev() const override { return nullptr; }
    /*!
     * \brief Next edge counterclockwise into the destination (IEdge::destNext()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *destNext() const override { return nullptr; }
    /*!
     * \brief Next edge clockwise into the destination (IEdge::destPrev()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *destPrev() const override { return nullptr; }
    /*!
     * \brief Next edge around the left face (IEdge::leftNext()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *leftNext() const override { return nullptr; }
    /*!
     * \brief Previous edge around the left face (IEdge::leftPrev()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *leftPrev() const override { return nullptr; }
    /*!
     * \brief Next edge around the right face (IEdge::rightNext()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *rightNext() const override { return nullptr; }
    /*!
     * \brief Previous edge around the right face (IEdge::rightPrev()); not kept.
     * \returns nullptr.
     */
    [[nodiscard]] IEdge *rightPrev() const override { return nullptr; }

private:
    int64_t m_index = 0;                       //!< The edge's index.
    VertexAdapter *m_origin = nullptr;         //!< Borrowed origin vertex.
    VertexAdapter *m_destination = nullptr;    //!< Borrowed destination vertex.
};

/*!
 * \brief INetwork over an SDK Network.
 *
 * Copies the network at construction into owned vertex and edge adapters and
 * a MeshViewAdapter; later changes to the Network are not seen. The adapter
 * takes the network's id and caption. An edge that names a vertex outside the
 * network is skipped, and the edges kept are renumbered consecutively, so
 * edge indexes differ from the Network's when any edge was skipped. Each
 * vertex reports, as IVertex::edge(), the first kept edge that leaves it.
 *
 * The mesh view carries the vertices' x and y as nodes (not z: nodeZ() is
 * empty), the kept edges in order, and no faces; its edge lengths are plan
 * lengths, unlike NetworkEdge::length(), which includes z.
 *
 * The adapter cannot be copied or moved (Identity base).
 */
class HYDROCOUPLESDK_EXPORT NetworkAdapter final
    : public HydroCouple::SDK::Identity,
      public virtual HydroCouple::Spatial::INetwork
{
public:
    /*!
     * \brief Construct over a copy of \p network.
     * \param[in] network The network; read once, not retained.
     * \param[in] srs     Spatial reference given to every vertex (borrowed;
     *                    may be nullptr). The connection validator reads a
     *                    network item's reference from its first vertex.
     */
    explicit NetworkAdapter(
        const Network &network,
        HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    //! Destroys the adapter and its vertex, edge and mesh-view adapters.
    ~NetworkAdapter() override = default;

    /*!
     * \brief The number of edges (INetwork::edgeCount()).
     * \returns The number of edges kept (well-formed edges of the network).
     */
    [[nodiscard]] int64_t edgeCount() const override
    {
        return static_cast<int64_t>(m_edges.size());
    }

    /*!
     * \brief The edge at an index (INetwork::edge()).
     * \param[in] index Zero-based edge index.
     * \returns A non-owning pointer valid for the life of the adapter;
     *          nullptr when \p index is out of range.
     */
    [[nodiscard]] HydroCouple::Spatial::IEdge *edge(int64_t index) const override;

    /*!
     * \brief The number of vertices (INetwork::vertexCount()).
     * \returns The network's vertex count at construction.
     */
    [[nodiscard]] int64_t vertexCount() const override
    {
        return static_cast<int64_t>(m_vertices.size());
    }

    /*!
     * \brief The vertex at an index (INetwork::vertex()).
     * \param[in] index Zero-based vertex index, the Network's own.
     * \returns A non-owning pointer valid for the life of the adapter;
     *          nullptr when \p index is out of range.
     */
    [[nodiscard]] HydroCouple::Spatial::IVertex *vertex(
        int64_t index) const override;

    /*!
     * \brief A mesh view of the network: its vertices as nodes, its edges as
     *        edges, and no faces.
     * \returns A non-owning pointer, never nullptr, valid for the life of the
     *          adapter.
     */
    [[nodiscard]] const HydroCouple::Spatial::IMeshView *meshView()
        const override
    {
        return m_meshView.get();
    }

private:
    std::vector<std::unique_ptr<VertexAdapter>> m_vertices; //!< One adapter per network vertex.
    std::vector<std::unique_ptr<EdgeAdapter>> m_edges;      //!< One adapter per kept edge.
    std::unique_ptr<MeshViewAdapter> m_meshView;            //!< The bulk view.
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
 * Each face becomes a Polygon whose exterior ring is its nodes in order with
 * the first repeated last. Node z is used when nodeZ has one value per node
 * and dropped otherwise. The result does not record which nodes a patch
 * used; pass \p mesh along to PolyhedralSurfaceAdapter's second constructor
 * to keep the connectivity. Indexes are not checked (see MeshViewAdapter).
 *
 * \param[in] mesh The mesh to convert; copied into the result.
 * \returns The surface, with nodeCount() vertices and faceCount() patches.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT PolyhedralSurface polyhedralSurfaceFromMesh(
    const HydroCouple::SDK::IO::MeshDefinition &mesh);

/*!
 * \brief IPolyhedralSurface over an SDK PolyhedralSurface.
 *
 * Copies the surface at construction: one PolygonAdapter per non-null patch
 * (id "patchI" for patch I, told through setSurface() that it belongs to this
 * adapter), one VertexAdapter per vertex (with no edge), and the area,
 * envelope and centroid of the surface. The adapter's own id is always
 * "surface" with index 0. Null patches are skipped, so patch(k) is the k-th
 * non-null patch while its index() keeps the original position.
 *
 * Which mesh view it serves depends on the constructor: the MeshDefinition
 * passed to the second one, or, from the first, a view of the vertices alone
 * with no faces and no edges (see the note there). The second constructor is
 * the one to use; PolyhedralSurfaceArgument does.
 *
 * \note boundingPolygons() returns nullptr and isClosed() false: the surface
 * keeps no adjacency. The centroid inherits PolyhedralSurface::centroid()'s
 * error (half the true value), and getWKT() writes
 * "GEOMETRYCOLLECTION EMPTY"; getWKB() is complete.
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
     *
     * \param[in] surface The surface; read once, not retained.
     * \param[in] srs     Spatial reference (borrowed; may be nullptr), given
     *                    to the patches and vertices too.
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
     * A \p mesh with no nodes is ignored and the vertex-only view of the
     * first constructor is used instead. \p mesh is not checked against
     * \p surface; the caller keeps them consistent.
     *
     * \param[in] surface The surface, as patches and vertices.
     * \param[in] mesh    The connectivity it was built from; moved.
     * \param[in] srs     Spatial reference, borrowed.
     */
    PolyhedralSurfaceAdapter(
        const PolyhedralSurface &surface,
        HydroCouple::SDK::IO::MeshDefinition mesh,
        HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    //! Destroys the adapter and everything it built.
    ~PolyhedralSurfaceAdapter() override = default;

    /*!
     * \brief The number of patches (IPolyhedralSurface::patchCount()).
     * \returns The number of non-null patches of the surface.
     */
    [[nodiscard]] int64_t patchCount() const override
    {
        return static_cast<int64_t>(m_patches.size());
    }

    /*!
     * \brief The patch at an index (IPolyhedralSurface::patch()).
     * \param[in] index Zero-based patch index.
     * \returns A non-owning pointer valid for the life of the adapter, whose
     *          polyhedralSurface() is this adapter; nullptr when \p index is
     *          out of range.
     */
    [[nodiscard]] HydroCouple::Spatial::IPolygon *patch(
        int64_t index) const override;

    /*!
     * \brief The number of vertices (IPolyhedralSurface::vertexCount()).
     * \returns The surface's vertex count at construction.
     */
    [[nodiscard]] int64_t vertexCount() const override
    {
        return static_cast<int64_t>(m_vertices.size());
    }

    /*!
     * \brief The vertex at an index (IPolyhedralSurface::vertex()).
     * \param[in] index Zero-based vertex index.
     * \returns A non-owning pointer valid for the life of the adapter, whose
     *          edge() is nullptr; nullptr when \p index is out of range.
     */
    [[nodiscard]] HydroCouple::Spatial::IVertex *vertex(
        int64_t index) const override;

    /*!
     * \brief The polygons bounding \p polygon.
     *
     * \note Needs adjacency the SDK's PolyhedralSurface does not keep;
     *       reports nullptr rather than a wrong neighborhood.
     *
     * \returns nullptr, whatever the argument; use meshView()->edgeFaces()
     *          for neighbors.
     */
    [[nodiscard]] std::unique_ptr<HydroCouple::Spatial::IMultiPolygon> boundingPolygons(
        const HydroCouple::Spatial::IPolygon *) const override
    {
        return nullptr;
    }

    /*!
     * \brief Whether the surface bounds a solid (IPolyhedralSurface::isClosed()).
     * \returns Always false; closure is not computed.
     */
    [[nodiscard]] bool isClosed() const override { return false; }

    /*!
     * \brief The bulk view (IPolyhedralSurface::meshView()).
     * \returns A non-owning pointer, never nullptr, valid for the life of the
     *          adapter: the supplied MeshDefinition, or the vertex-only view.
     */
    [[nodiscard]] const HydroCouple::Spatial::IMeshView *meshView()
        const override
    {
        return m_meshView.get();
    }

    // ── IGeometry / ISurface ──────────────────────────────────────────────

    /*!
     * \brief The topological dimension (IGeometry::dimension()).
     * \returns 2.
     */
    [[nodiscard]] int dimension() const override { return 2; }

    /*!
     * \brief The geometry type (IGeometry::geometryType()).
     * \returns GeometryType::PolyhedralSurface.
     */
    [[nodiscard]] GeometryType geometryType() const override
    {
        return GeometryType::PolyhedralSurface;
    }

    /*!
     * \brief Well-known binary of the surface (IGeometry::getWKB()).
     * \returns The little-endian marker 1, the type code 15, the patch count
     *          as uint32, then each patch as a complete polygon record (see
     *          PolygonAdapter::getWKB()).
     */
    [[nodiscard]] std::vector<unsigned char> getWKB() const override;

    /*!
     * \brief Whether the surface has no patches (IGeometry::isEmpty()).
     * \returns True when patchCount() is 0.
     */
    [[nodiscard]] bool isEmpty() const override { return m_patches.empty(); }

    /*!
     * \brief The plan area (ISurface::area()).
     * \returns PolyhedralSurface::area() of the surface at construction.
     */
    [[nodiscard]] double area() const override { return m_area; }

    //! Newly constructed, caller-owned (interface ownership convention).
    /*!
     * \returns A new PointAdapter ("surface:centroid") at
     *          PolyhedralSurface::centroid() of the surface at construction;
     *          currently half the true centroid. Never nullptr.
     */
    [[nodiscard]] std::unique_ptr<HydroCouple::Spatial::IPoint> centroid() const override;

    /*!
     * \brief A point on the surface (ISurface::pointOnSurface()).
     * \returns centroid(), which is not guaranteed to lie on the surface.
     */
    [[nodiscard]] std::unique_ptr<HydroCouple::Spatial::IPoint> pointOnSurface() const override
    {
        return centroid();
    }

    /*!
     * \brief The boundary as a multi-curve (ISurface::boundaryMultiCurve());
     *        not computed.
     * \returns nullptr.
     */
    [[nodiscard]] std::unique_ptr<HydroCouple::Spatial::IMultiCurve> boundaryMultiCurve()
        const override
    {
        return nullptr;
    }

private:
    double m_area = 0.0;                                     //!< Area of the surface at construction.
    Point m_centroidPoint;                                   //!< Centroid of the surface at construction.
    std::unique_ptr<PointAdapter> m_centroid;                //!< Built at construction; centroid() does not use it.
    std::vector<std::unique_ptr<PolygonAdapter>> m_patches;  //!< One adapter per non-null patch.
    std::vector<std::unique_ptr<VertexAdapter>> m_vertices;  //!< One adapter per vertex.
    std::unique_ptr<MeshViewAdapter> m_meshView;             //!< The bulk view.
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_SPATIAL_MESHADAPTERS_H
