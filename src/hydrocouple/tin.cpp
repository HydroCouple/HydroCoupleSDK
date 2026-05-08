// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/tin.h"

#include <cmath>
#include <algorithm>

namespace HydroCouple::Spatial {

int TIN::addTriangle(const Point &p0, const Point &p1, const Point &p2) {
    int v0 = addVertex(p0);
    int v1 = addVertex(p1);
    int v2 = addVertex(p2);
    return addTriangleByIndex(v0, v1, v2);
}

int TIN::addTriangleByIndex(int v0, int v1, int v2) {
    if (v0 < 0 || v1 < 0 || v2 < 0 ||
        v0 >= numVertices() || v1 >= numVertices() || v2 >= numVertices()) {
        return -1;
    }

    Point p0 = vertex(v0);
    Point p1 = vertex(v1);
    Point p2 = vertex(v2);

    // Create triangle as a closed linestring (4 points: p0, p1, p2, p0)
    LineString ring;
    ring.addPoint(p0);
    ring.addPoint(p1);
    ring.addPoint(p2);
    ring.addPoint(p0);  // Close the ring

    auto poly = std::make_shared<Polygon>(ring);
    int triIndex = numPatches();
    addPatch(poly);
    return triIndex;
}

std::array<int, 3> TIN::triangleVertices(int triIndex) const {
    std::array<int, 3> result = {-1, -1, -1};

    auto tri = triangle(triIndex);
    if (!tri || tri->exteriorRing().pointCount() < 4) {
        return result;
    }

    const auto &ring = tri->exteriorRing();
    Point p0 = ring.pointAt(0);
    Point p1 = ring.pointAt(1);
    Point p2 = ring.pointAt(2);

    // Find vertex indices by matching coordinates
    for (int i = 0; i < numVertices(); ++i) {
        Point v = vertex(i);
        if (v.equals(p0)) result[0] = i;
        else if (v.equals(p1)) result[1] = i;
        else if (v.equals(p2)) result[2] = i;
    }

    return result;
}

int TIN::numEdges() const {
    // Count unique edges: 3 edges per triangle, but internal edges are shared
    // For a valid triangulation: edges = 3*triangles - boundary_edges
    // Rough estimate: ~3*triangles / 2 for interior-heavy mesh
    int edges = 0;
    std::set<std::pair<int,int>> uniqueEdges;

    for (int t = 0; t < numTriangles(); ++t) {
        auto verts = triangleVertices(t);
        if (verts[0] >= 0 && verts[1] >= 0 && verts[2] >= 0) {
            // Add edges in sorted order to detect duplicates
            int v0 = verts[0], v1 = verts[1], v2 = verts[2];
            uniqueEdges.insert({std::min(v0,v1), std::max(v0,v1)});
            uniqueEdges.insert({std::min(v1,v2), std::max(v1,v2)});
            uniqueEdges.insert({std::min(v2,v0), std::max(v2,v0)});
        }
    }

    return (int)uniqueEdges.size();
}

bool TIN::isValid() const {
    for (int t = 0; t < numPatches(); ++t) {
        if (!isTriangle(patch(t))) return false;
    }
    return true;
}

double TIN::barycentric(const Point &p, const Point &p0,
                        const Point &p1, const Point &p2) const {
    // Compute area of triangle p0-p1-p2 using cross product
    Point v0 = p1 - p0;
    Point v1 = p2 - p0;
    return std::abs(v0.cross(v1).magnitude()) / 2.0;
}

int TIN::findContainingTriangle(double x, double y,
                                 double &lambda0, double &lambda1, double &lambda2) const {
    Point p(x, y, 0.0);

    for (int t = 0; t < numTriangles(); ++t) {
        auto tri = triangle(t);
        if (!tri || !isTriangle(tri)) continue;

        if (tri->containsPoint(p)) {
            const auto &ring = tri->exteriorRing();
            Point p0 = ring.pointAt(0);
            Point p1 = ring.pointAt(1);
            Point p2 = ring.pointAt(2);

            double areaTotal = barycentric(p0, p0, p1, p2);
            if (areaTotal < 1e-10) continue;

            double area0 = barycentric(p, p1, p2, p0);
            double area1 = barycentric(p, p2, p0, p1);
            double area2 = barycentric(p, p0, p1, p2);

            lambda0 = area0 / areaTotal;
            lambda1 = area1 / areaTotal;
            lambda2 = area2 / areaTotal;

            return t;
        }
    }

    return -1;  // Outside mesh
}

double TIN::interpolateZ(double x, double y) const {
    double lambda0, lambda1, lambda2;
    int triIndex = findContainingTriangle(x, y, lambda0, lambda1, lambda2);

    if (triIndex < 0) return std::nan("");

    auto tri = triangle(triIndex);
    const auto &ring = tri->exteriorRing();
    Point p0 = ring.pointAt(0);
    Point p1 = ring.pointAt(1);
    Point p2 = ring.pointAt(2);

    double z = lambda0 * p0.z + lambda1 * p1.z + lambda2 * p2.z;
    return z;
}

} // namespace HydroCouple::Spatial
