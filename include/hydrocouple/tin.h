// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   tin.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Triangulated Irregular Network (TIN) mesh.
 */

#ifndef HYDROCOUPLESDK_TIN_H
#define HYDROCOUPLESDK_TIN_H

#include "hydrocouplesdk.h"
#include "polyhedralsurface.h"

#include <vector>
#include <array>

namespace HydroCouple::Spatial {

/*!
 * \brief Triangulated Irregular Network (TIN).
 *
 * A PolyhedralSurface where all patches are triangles (3-vertex polygons).
 * Supports efficient triangle mesh operations: neighbor lookup, interpolation,
 * element search.
 *
 * Future: Guibas-Stolfi quad-edge topology for O(1) navigation.
 * Current: Polygon-based storage (simpler, works with existing geometry classes).
 *
 * \threadsafety Not thread-safe.
 */
class HYDROCOUPLESDK_EXPORT TIN : public PolyhedralSurface
{
public:
    /*!
     * \brief Construct empty TIN.
     */
    TIN() = default;

    ~TIN() override = default;

    // ── Triangle-specific API ──────────────────────────────────────────────

    /*!
     * \brief Add a triangle from three points.
     * \return Triangle index
     */
    int addTriangle(const Point &p0, const Point &p1, const Point &p2);

    /*!
     * \brief Add a triangle from three vertex indices.
     * \return Triangle index
     */
    int addTriangleByIndex(int v0, int v1, int v2);

    /*!
     * \brief Get triangle vertices (3 indices).
     */
    std::array<int, 3> triangleVertices(int triIndex) const;

    /*!
     * \brief Get number of triangles (same as numPatches).
     */
    int numTriangles() const noexcept { return numPatches(); }

    /*!
     * \brief Get triangle at index.
     */
    std::shared_ptr<Polygon> triangle(int i) const { return patch(i); }

    // ── Mesh statistics ───────────────────────────────────────────────────

    /*!
     * \brief Count edges (each internal edge shared by 2 triangles).
     */
    int numEdges() const;

    /*!
     * \brief Check mesh validity (all triangles, proper orientation).
     */
    bool isValid() const;

    /*!
     * \brief Get average triangle area.
     */
    double averageTriangleArea() const {
        if (isEmpty()) return 0.0;
        return area() / numTriangles();
    }

    // ── Interpolation ──────────────────────────────────────────────────────

    /*!
     * \brief Linear interpolation of Z at (x, y).
     *
     * Finds triangle containing (x, y) and interpolates Z using
     * barycentric coordinates. Returns NaN if point is outside mesh.
     */
    double interpolateZ(double x, double y) const;

    /*!
     * \brief Find containing triangle and barycentric coordinates.
     *
     * \return Triangle index, or -1 if point is outside
     */
    int findContainingTriangle(double x, double y,
                                double &lambda0, double &lambda1, double &lambda2) const;

private:
    bool isTriangle(const std::shared_ptr<Polygon> &patch) const {
        return patch && patch->exteriorRing().pointCount() == 4;  // closed ring: 4 points
    }

    double barycentric(const Point &p, const Point &p0,
                      const Point &p1, const Point &p2) const;
};

} // namespace HydroCouple::Spatial

#endif // HYDROCOUPLESDK_TIN_H
