// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/spatial/polygon.h"

#include <cmath>
#include <algorithm>

namespace HydroCouple::Spatial {

Polygon::Polygon(const LineString &exterior)
    : m_exterior(exterior)
{
}

int Polygon::pointCount() const noexcept {
    int count = m_exterior.pointCount();
    for (const auto &hole : m_holes) {
        count += hole.pointCount();
    }
    return count;
}

double Polygon::ringSignedArea(const LineString &ring) const {
    int n = ring.pointCount();
    if (n < 3) return 0.0;

    const auto &x = ring.getX();
    const auto &y = ring.getY();

    double area = 0.0;
    for (int i = 0; i < n - 1; ++i) {
        area += (x[i+1] - x[i]) * (y[i+1] + y[i]);
    }
    return -area / 2.0;  // Shoelace formula
}

double Polygon::area() const {
    double totalArea = ringSignedArea(m_exterior);

    // Subtract hole areas (which are negative when calculated)
    for (const auto &hole : m_holes) {
        totalArea -= ringSignedArea(hole);
    }

    return std::abs(totalArea);
}

Point Polygon::ringCentroid(const LineString &ring) const {
    int n = ring.pointCount();
    if (n < 3) return Point();

    const auto &x = ring.getX();
    const auto &y = ring.getY();

    double cx = 0.0, cy = 0.0, a = 0.0;

    for (int i = 0; i < n - 1; ++i) {
        double cross = x[i] * y[i+1] - x[i+1] * y[i];
        cx += (x[i] + x[i+1]) * cross;
        cy += (y[i] + y[i+1]) * cross;
        a += cross;
    }

    if (std::abs(a) < 1e-10) return ring.centroid();
    return Point(cx / (6.0 * a), cy / (6.0 * a));
}

Point Polygon::centroid() const {
    double totalArea = ringSignedArea(m_exterior);
    if (std::abs(totalArea) < 1e-10) return m_exterior.centroid();

    Point cg = ringCentroid(m_exterior) * totalArea;

    // Subtract hole contributions
    for (const auto &hole : m_holes) {
        double holeArea = ringSignedArea(hole);
        cg = cg + (ringCentroid(hole) * (-holeArea));
    }

    return cg * (1.0 / totalArea);
}

bool Polygon::containsPoint(const Point &p) const {
    // Ray casting algorithm: cast ray to +infinity in X direction
    // Count edge crossings; odd = inside, even = outside

    auto rayIntersects = [](const LineString &ring, const Point &p) -> int {
        int n = ring.pointCount();
        int crossings = 0;

        const auto &x = ring.getX();
        const auto &y = ring.getY();

        for (int i = 0; i < n - 1; ++i) {
            double x1 = x[i], y1 = y[i];
            double x2 = x[i+1], y2 = y[i+1];

            // Check if ray crosses this edge
            if ((y1 <= p.y && p.y < y2) || (y2 <= p.y && p.y < y1)) {
                // Compute X intersection of horizontal ray at p.y
                double xIntersect = x1 + (p.y - y1) * (x2 - x1) / (y2 - y1);
                if (p.x < xIntersect) {
                    crossings++;
                }
            }
        }

        return crossings;
    };

    // Check exterior ring
    if (rayIntersects(m_exterior, p) % 2 == 0) {
        return false;  // Outside
    }

    // Check interior rings (holes)
    for (const auto &hole : m_holes) {
        if (rayIntersects(hole, p) % 2 == 1) {
            return false;  // Inside a hole
        }
    }

    return true;  // Inside exterior, outside all holes
}

} // namespace HydroCouple::Spatial
