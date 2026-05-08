// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/spatial/linestring.h"

#include <cmath>
#include <algorithm>

namespace HydroCouple::Spatial {

LineString::LineString(const std::vector<Point> &points) {
    addPoints(points);
}

void LineString::addPoint(const Point &p) {
    m_x.push_back(p.x);
    m_y.push_back(p.y);
    m_z.push_back(p.z);
    m_m.push_back(p.m);
}

double LineString::length() const {
    if (pointCount() < 2) return 0.0;

    double total = 0.0;
    int n = pointCount() - 1;

#ifdef _OPENMP
#pragma omp simd reduction(+:total)
#endif
    for (int i = 0; i < n; ++i) {
        double dx = m_x[i+1] - m_x[i];
        double dy = m_y[i+1] - m_y[i];
        double dz = m_z[i+1] - m_z[i];
        total += std::sqrt(dx*dx + dy*dy + dz*dz);
    }

    return total;
}

Envelope LineString::envelope() const {
    if (isEmpty()) return Envelope();

    double minX = m_x[0], maxX = m_x[0];
    double minY = m_y[0], maxY = m_y[0];
    double minZ = m_z[0], maxZ = m_z[0];

#ifdef _OPENMP
#pragma omp simd reduction(min:minX,minY,minZ) reduction(max:maxX,maxY,maxZ)
#endif
    for (int i = 1; i < (int)m_x.size(); ++i) {
        minX = std::min(minX, m_x[i]);
        maxX = std::max(maxX, m_x[i]);
        minY = std::min(minY, m_y[i]);
        maxY = std::max(maxY, m_y[i]);
        minZ = std::min(minZ, m_z[i]);
        maxZ = std::max(maxZ, m_z[i]);
    }

    return Envelope(minX, minY, minZ, maxX, maxY, maxZ);
}

Point LineString::centroid() const {
    if (isEmpty()) return Point();

    double sumX = 0.0, sumY = 0.0, sumZ = 0.0;

#ifdef _OPENMP
#pragma omp simd reduction(+:sumX,sumY,sumZ)
#endif
    for (int i = 0; i < (int)m_x.size(); ++i) {
        sumX += m_x[i];
        sumY += m_y[i];
        sumZ += m_z[i];
    }

    int n = pointCount();
    return Point(sumX / n, sumY / n, sumZ / n);
}

} // namespace HydroCouple::Spatial
