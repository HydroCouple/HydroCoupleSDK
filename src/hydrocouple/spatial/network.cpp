// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/spatial/network.h"

#include <cmath>

namespace HydroCouple::Spatial {

double NetworkEdge::length(const Network *network) const {
    if (!network || origin < 0 || dest < 0) return 0.0;
    if (origin >= network->vertexCount() || dest >= network->vertexCount()) return 0.0;

    const auto &p0 = network->vertex(origin);
    const auto &p1 = network->vertex(dest);
    double dx = p1.x - p0.x;
    double dy = p1.y - p0.y;
    double dz = p1.z - p0.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Network::Network() : Identity() {
}

int Network::addVertex(double x, double y, double z) {
    Point p(x, y, z);
    int idx = (int)m_vertices.size();
    m_vertices.push_back(p);
    return idx;
}

int Network::addVertex(const Point &p) {
    return addVertex(p.x, p.y, p.z);
}

int Network::addEdge(int originIndex, int destIndex) {
    if (originIndex < 0 || destIndex < 0 ||
        originIndex >= (int)m_vertices.size() || destIndex >= (int)m_vertices.size()) {
        return -1;
    }

    int idx = (int)m_edges.size();
    m_edges.emplace_back(originIndex, destIndex);
    return idx;
}

const Point &Network::vertex(int index) const {
    if (index < 0 || index >= (int)m_vertices.size()) {
        static const Point nullPoint;
        return nullPoint;
    }
    return m_vertices[index];
}

Point &Network::vertex(int index) {
    if (index < 0 || index >= (int)m_vertices.size()) {
        static Point nullPoint;
        return nullPoint;
    }
    return m_vertices[index];
}

const NetworkEdge &Network::edge(int index) const {
    if (index < 0 || index >= (int)m_edges.size()) {
        static const NetworkEdge nullEdge;
        return nullEdge;
    }
    return m_edges[index];
}

double Network::totalLength() const {
    double total = 0.0;
    for (const auto &e : m_edges) {
        total += e.length(this);
    }
    return total;
}

} // namespace HydroCouple::Spatial
