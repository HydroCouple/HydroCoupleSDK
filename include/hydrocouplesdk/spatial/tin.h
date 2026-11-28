// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   tin.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  TIN, a triangulated irregular network: a polyhedral surface of
 *         triangles.
 *
 * TIN is a value type. It does not implement HydroCouple::Spatial::ITIN; the
 * SDK has no ITIN adapter, so a component that publishes a TIN through the
 * standard serves it as an IPolyhedralSurface with PolyhedralSurfaceAdapter
 * or implements ITIN itself.
 */

#ifndef HYDROCOUPLESDK_TIN_H
#define HYDROCOUPLESDK_TIN_H

#include "hydrocouplesdk/export.h"
#include "polyhedralsurface.h"

#include <vector>
#include <array>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

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
 * Each triangle is stored as a Polygon whose exterior ring is its three
 * corners plus the first repeated. The operations are linear searches over
 * the triangles (and, for triangleVertices() and numEdges(), over the vertex
 * list); there is no neighbor structure, so "efficient" above describes the
 * intent, not the current cost.
 *
 * \note interpolateZ() and findContainingTriangle() are wrong in the current
 * implementation: the barycentric weights they compute are all 1, so
 * interpolateZ() returns the sum of the corner elevations rather than the
 * interpolated one. See findContainingTriangle().
 *
 * \threadsafety Not thread-safe: concurrent reads are safe, but adding
 * triangles or vertices must not overlap other access to the same TIN.
 */
class HYDROCOUPLESDK_EXPORT TIN : public PolyhedralSurface
{
public:
    /*!
     * \brief Construct empty TIN.
     */
    TIN() = default;

    //! Destroys the TIN and releases its triangles.
    ~TIN() override = default;

    // ── Triangle-specific API ──────────────────────────────────────────────

    /*!
     * \brief Add a triangle from three points.
     *
     * Appends the three points to the vertex list as new vertices, even when
     * a vertex at the same position already exists, and then calls
     * addTriangleByIndex(). Use addTriangleByIndex() with shared vertices to
     * build a connected mesh without duplicates. The corner order is kept;
     * orientation is not checked.
     *
     * \param[in] p0 First corner.
     * \param[in] p1 Second corner.
     * \param[in] p2 Third corner.
     * \return Triangle index
     */
    int addTriangle(const Point &p0, const Point &p1, const Point &p2);

    /*!
     * \brief Add a triangle from three vertex indices.
     *
     * Copies the three vertices' coordinates into a closed ring
     * (v0, v1, v2, v0) and adds it as a patch.
     *
     * \param[in] v0 Index of the first corner in the vertex list.
     * \param[in] v1 Index of the second corner.
     * \param[in] v2 Index of the third corner.
     * \return Triangle index, or -1 (nothing added) when any index is
     *         outside 0 to numVertices() - 1.
     */
    int addTriangleByIndex(int v0, int v1, int v2);

    /*!
     * \brief Get triangle vertices (3 indices).
     *
     * The TIN does not store which vertices a triangle used, so the indices
     * are recovered by matching each corner's coordinates against the vertex
     * list with Point::equals() (absolute tolerance 1e-10 on x, y and z).
     * When several vertices share a position the highest matching index is
     * returned. Costs one pass over the vertex list.
     *
     * \param[in] triIndex Zero-based triangle index.
     * \returns The indices of the three corners in ring order; all -1 when
     *          \p triIndex is out of range, and -1 for any corner whose
     *          position matches no vertex.
     */
    std::array<int, 3> triangleVertices(int triIndex) const;

    /*!
     * \brief Get number of triangles (same as numPatches).
     * \returns numPatches().
     */
    int numTriangles() const noexcept { return numPatches(); }

    /*!
     * \brief Get triangle at index.
     * \param[in] i Zero-based triangle index.
     * \returns patch(i): the triangle as a Polygon (shared), or nullptr when
     *          \p i is out of range.
     */
    std::shared_ptr<Polygon> triangle(int i) const { return patch(i); }

    // ── Mesh statistics ───────────────────────────────────────────────────

    /*!
     * \brief Count edges (each internal edge shared by 2 triangles).
     *
     * Counts the distinct unordered vertex pairs over all triangles, with
     * the vertices found by triangleVertices(); triangles with an unmatched
     * corner are skipped. Because corners are matched by position, an edge
     * shared by two triangles built with addTriangle() (and therefore with
     * duplicate vertices) is still counted once. Costs one pass over the
     * vertex list per triangle.
     *
     * \returns The number of distinct edges.
     */
    int numEdges() const;

    /*!
     * \brief Tests whether every patch is a triangle.
     *
     * Orientation and degeneracy are not checked.
     *
     * \returns True when every patch is non-null and its exterior ring has
     *          exactly four points (three corners plus the closing point);
     *          true for an empty TIN.
     */
    bool isValid() const;

    /*!
     * \brief Get average triangle area.
     * \returns area() divided by numTriangles(), in squared coordinate
     *          units; 0 for an empty TIN.
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
     *
     * The value returned is \f$ \lambda_0 z_0 + \lambda_1 z_1 + \lambda_2 z_2 \f$
     * with the weights from findContainingTriangle(), which are currently all
     * 1: inside a triangle this returns \f$ z_0 + z_1 + z_2 \f$, not the
     * interpolated elevation.
     *
     * \param[in] x Plan x of the query point.
     * \param[in] y Plan y of the query point.
     * \returns The value above, or NaN when no triangle contains (x, y).
     */
    double interpolateZ(double x, double y) const;

    /*!
     * \brief Find containing triangle and barycentric coordinates.
     *
     * Searches the triangles in order and takes the first one that contains
     * (x, y) under Polygon::containsPoint(), skipping triangles whose area is
     * below 1e-10. Each weight is meant to be the area of the sub-triangle
     * opposite a corner divided by the triangle's area, but the helper that
     * computes those areas ignores the query point (see barycentric()), so
     * the three weights all come out as 1.
     *
     * \param[in]  x       Plan x of the query point.
     * \param[in]  y       Plan y of the query point.
     * \param[out] lambda0 Weight of the first corner; currently 1. Left
     *                     unchanged when no triangle is found.
     * \param[out] lambda1 Weight of the second corner; currently 1.
     * \param[out] lambda2 Weight of the third corner; currently 1.
     * \return Triangle index, or -1 if point is outside
     */
    int findContainingTriangle(double x, double y,
                                double &lambda0, double &lambda1, double &lambda2) const;

private:
    /*!
     * \brief Tests whether a patch has the shape of a triangle.
     * \param[in] patch The patch; may be null.
     * \returns True when \p patch is non-null and its exterior ring has four
     *          points.
     */
    bool isTriangle(const std::shared_ptr<Polygon> &patch) const {
        return patch && patch->exteriorRing().pointCount() == 4;  // closed ring: 4 points
    }

    /*!
     * \brief Returns the area of the triangle (p0, p1, p2).
     *
     * Half the magnitude of the three-dimensional cross product of
     * (p1 - p0) and (p2 - p0). The first parameter is not used, which is the
     * cause of the equal weights findContainingTriangle() reports: every call
     * there passes a permutation of the same three corners.
     *
     * \param[in] p  Unused.
     * \param[in] p0 First corner.
     * \param[in] p1 Second corner.
     * \param[in] p2 Third corner.
     * \returns The triangle's area in squared coordinate units, never
     *          negative.
     */
    double barycentric(const Point &p, const Point &p0,
                      const Point &p1, const Point &p2) const;
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_TIN_H
