// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   network.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Directed graph network (rivers, flow paths, stream networks).
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
 */
struct HYDROCOUPLESDK_EXPORT NetworkEdge {
    int origin = -1;
    int dest = -1;

    NetworkEdge() = default;
    NetworkEdge(int o, int d) : origin(o), dest(d) {}

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
 * \threadsafety Not thread-safe.
 */
class HYDROCOUPLESDK_EXPORT Network : public Identity {
public:
    /*!
     * \brief Construct empty network.
     */
    Network();

    ~Network() override = default;

    // ── Vertex API ─────────────────────────────────────────────────────────

    /*!
     * \brief Add a vertex at (x, y, z).
     * \return Vertex index
     */
    int addVertex(double x, double y, double z = 0.0);

    /*!
     * \brief Add a vertex from a Point.
     * \return Vertex index
     */
    int addVertex(const Point &p);

    /*!
     * \brief Get number of vertices.
     */
    int vertexCount() const noexcept { return (int)m_vertices.size(); }

    /*!
     * \brief Get vertex coordinates by index.
     */
    const Point &vertex(int index) const;

    /*!
     * \brief Get mutable vertex by index.
     */
    Point &vertex(int index);

    /*!
     * \brief Get all vertices.
     */
    const std::vector<Point> &vertices() const noexcept { return m_vertices; }

    // ── Edge API ───────────────────────────────────────────────────────────

    /*!
     * \brief Add a directed edge from origin to destination vertex.
     * \return Edge index
     */
    int addEdge(int originIndex, int destIndex);

    /*!
     * \brief Get number of edges.
     */
    int edgeCount() const noexcept { return (int)m_edges.size(); }

    /*!
     * \brief Get edge by index.
     */
    const NetworkEdge &edge(int index) const;

    /*!
     * \brief Get all edges.
     */
    const std::vector<NetworkEdge> &edges() const noexcept { return m_edges; }

    // ── Statistics ─────────────────────────────────────────────────────────

    /*!
     * \brief Check if network is empty.
     */
    bool isEmpty() const noexcept { return m_vertices.empty(); }

    /*!
     * \brief Total length of all edges.
     */
    double totalLength() const;

private:
    std::vector<Point> m_vertices;
    std::vector<NetworkEdge> m_edges;
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_NETWORK_H
