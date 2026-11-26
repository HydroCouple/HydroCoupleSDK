// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   network.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Directed graph network (rivers, flow paths, stream networks).
 *
 * Network and NetworkEdge are value types for a one-dimensional network of
 * vertices joined by directed edges. Network does not implement
 * HydroCouple::Spatial::INetwork; NetworkAdapter (meshadapters.h) serves one
 * through that interface, with a mesh view of its nodes and edges.
 */

#ifndef HYDROCOUPLESDK_NETWORK_H
#define HYDROCOUPLESDK_NETWORK_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/core/identity.h"
#include "point.h"

#include <vector>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

class Network;

/*!
 * \brief Simple directed edge in a network.
 *
 * Represents a directed edge from origin to destination vertex.
 *
 * The edge holds the two vertex indices only; the coordinates live in the
 * Network. A default-constructed edge has both indices -1, which is also
 * what Network::edge() returns for an index out of range.
 */
struct HYDROCOUPLESDK_EXPORT NetworkEdge {
    int origin = -1; //!< Index of the vertex the edge leaves; -1 when unset.
    int dest = -1;   //!< Index of the vertex the edge enters; -1 when unset.

    //! Constructs an unset edge (both indices -1).
    NetworkEdge() = default;

    /*!
     * \brief Constructs an edge between two vertex indices.
     *
     * The indices are not checked here; Network::addEdge() checks them
     * against its vertex list.
     *
     * \param[in] o Index of the origin vertex.
     * \param[in] d Index of the destination vertex.
     */
    NetworkEdge(int o, int d) : origin(o), dest(d) {}

    /*!
     * \brief Returns the straight-line length of the edge in a network.
     * \param[in] network The network whose vertices the indices refer to;
     *                    may be nullptr.
     * \returns The three-dimensional distance between the two vertices, in
     *          coordinate units; 0 when \p network is null or either index is
     *          negative or not below its vertexCount().
     */
    double length(const Network *network) const;
};

/*!
 * \brief Directed graph network.
 *
 * A Network represents a directed graph of vertices and edges, used for modeling
 * linear networks such as river/stream systems, flow paths, and drainage networks.
 *
 * Vertices are stored as Points with x, y, z, m coordinates.
 * Edges store origin/destination vertex indices.
 *
 * Vertices and edges are appended and never removed, so an index, once
 * returned, stays valid for the life of the network. The graph places no
 * other constraint on the edges: self-loops, parallel edges and
 * disconnected parts are all accepted. The identity (id(), caption()) comes
 * from HydroCouple::SDK::Identity; the default id is a generated UUID.
 *
 * \threadsafety Not thread-safe: concurrent reads are safe, but adding
 * vertices or edges must not overlap other access. The out-of-range
 * fallback of the non-const vertex() is one object shared by every network
 * in the process (see vertex()).
 */
class HYDROCOUPLESDK_EXPORT Network : public Identity {
public:
    /*!
     * \brief Construct empty network.
     */
    Network();

    //! Destroys the network and its vertex and edge lists.
    ~Network() override = default;

    // ── Vertex API ─────────────────────────────────────────────────────────

    /*!
     * \brief Add a vertex at (x, y, z).
     * \param[in] x The x coordinate.
     * \param[in] y The y coordinate.
     * \param[in] z The z coordinate (elevation); 0 by default.
     * \return Vertex index, the vertex count before the call; the vertex's
     *         measure is 0.
     */
    int addVertex(double x, double y, double z = 0.0);

    /*!
     * \brief Add a vertex from a Point.
     * \param[in] p The position; its x, y and z are copied and its measure m
     *              is dropped (the stored vertex has m = 0).
     * \return Vertex index
     */
    int addVertex(const Point &p);

    /*!
     * \brief Get number of vertices.
     * \returns The number of vertices added.
     */
    int vertexCount() const noexcept { return (int)m_vertices.size(); }

    /*!
     * \brief Get vertex coordinates by index.
     * \param[in] index Zero-based vertex index, from 0 to vertexCount() - 1.
     * \returns The vertex; for an index out of range, a reference to a
     *          static origin Point. The reference to a real vertex is valid
     *          until the next addVertex().
     */
    const Point &vertex(int index) const;

    /*!
     * \brief Get mutable vertex by index.
     *
     * Writing through the returned reference moves the vertex; edge lengths
     * follow, since edges store indices.
     *
     * \param[in] index Zero-based vertex index, from 0 to vertexCount() - 1.
     * \returns The vertex; for an index out of range, a reference to a
     *          static Point shared by every Network, which a write through
     *          the reference changes for all later out-of-range calls. Check
     *          the index before writing.
     */
    Point &vertex(int index);

    /*!
     * \brief Get all vertices.
     * \returns The vertex list in index order; valid until the next
     *          addVertex().
     */
    const std::vector<Point> &vertices() const noexcept { return m_vertices; }

    // ── Edge API ───────────────────────────────────────────────────────────

    /*!
     * \brief Add a directed edge from origin to destination vertex.
     * \param[in] originIndex Index of the vertex the edge leaves.
     * \param[in] destIndex   Index of the vertex the edge enters; may equal
     *                        \p originIndex.
     * \return Edge index, or -1 (nothing added) when either index is negative
     *         or not below vertexCount().
     */
    int addEdge(int originIndex, int destIndex);

    /*!
     * \brief Get number of edges.
     * \returns The number of edges added.
     */
    int edgeCount() const noexcept { return (int)m_edges.size(); }

    /*!
     * \brief Get edge by index.
     * \param[in] index Zero-based edge index, from 0 to edgeCount() - 1.
     * \returns The edge; for an index out of range, a reference to a static
     *          unset edge (both indices -1).
     */
    const NetworkEdge &edge(int index) const;

    /*!
     * \brief Get all edges.
     * \returns The edge list in index order; valid until the next addEdge().
     */
    const std::vector<NetworkEdge> &edges() const noexcept { return m_edges; }

    // ── Statistics ─────────────────────────────────────────────────────────

    /*!
     * \brief Check if network is empty.
     * \returns True when the network has no vertices.
     */
    bool isEmpty() const noexcept { return m_vertices.empty(); }

    /*!
     * \brief Total length of all edges.
     * \returns The sum of NetworkEdge::length() over the edges, in coordinate
     *          units; 0 for a network without edges.
     */
    double totalLength() const;

private:
    std::vector<Point> m_vertices;   //!< Vertex positions, in index order.
    std::vector<NetworkEdge> m_edges; //!< Edges, in index order.
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_NETWORK_H
