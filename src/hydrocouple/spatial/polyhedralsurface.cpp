// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/spatial/polyhedralsurface.h"

namespace HydroCouple::Spatial {

double PolyhedralSurface::area() const {
    double totalArea = 0.0;
    for (const auto &patch : m_patches) {
        if (patch) totalArea += patch->area();
    }
    return totalArea;
}

std::shared_ptr<Polygon> PolyhedralSurface::findPatch(const Point &p) const {
    // Brute force: check each patch until we find one containing p
    for (const auto &patch : m_patches) {
        if (patch && patch->containsPoint(p)) {
            return patch;
        }
    }
    return nullptr;
}

Point PolyhedralSurface::centroid() const {
    if (isEmpty()) return Point();

    double totalArea = 0.0;
    Point weightedSum;

    for (const auto &patch : m_patches) {
        if (!patch) continue;
        double a = patch->area();
        if (a == 0.0) continue;
        totalArea += a;
        Point c = patch->centroid();
        weightedSum = weightedSum + (c * a);
    }

    if (totalArea == 0.0) return weightedSum;
    return weightedSum * (1.0 / totalArea);
}

} // namespace HydroCouple::Spatial
