// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   polygon.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Polygon, a planar area bounded by an exterior ring with optional
 *         holes.
 *
 * Polygon is the SDK's surface value type and the patch type of
 * PolyhedralSurface and TIN. It does not implement
 * HydroCouple::Spatial::IPolygon; PolygonAdapter (geometryadapters.h) serves
 * one through that interface.
 */

#ifndef HYDROCOUPLESDK_POLYGON_H
#define HYDROCOUPLESDK_POLYGON_H

#include "hydrocouplesdk/export.h"
#include "linestring.h"
#include "envelope.h"

#include <vector>
#include <memory>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

/*!
 * \brief 2D polygon with exterior ring and optional interior holes.
 *
 * The exterior ring must be closed (start == end) and oriented counter-clockwise
 * by convention. Interior holes are clockwise. Supports area and centroid
 * calculations via shoelace formula.
 *
 * The rings are LineString values copied into the polygon; nothing checks
 * them when they are added. The area, centroid and containment calculations
 * use x and y only and expect each ring to repeat its first point as its last
 * (the shoelace sums stop at the last stored point and do not close a ring
 * themselves).
 *
 * \note The implementation disagrees with this description in two places.
 * area() subtracts each hole's *signed* area, so it is correct when a hole
 * runs in the same direction as the exterior ring and adds the hole's area
 * when it runs the other (clockwise) way. centroid() reports half the
 * centroid of a polygon without holes and a further-wrong point for one
 * with holes. See area() and centroid().
 *
 * \threadsafety Not thread-safe: concurrent reads are safe, but adding a
 * hole must not overlap other access to the same polygon.
 */
class HYDROCOUPLESDK_EXPORT Polygon
{
public:
    /*!
     * \brief Constructs an empty polygon with no rings.
     */
    Polygon() = default;

    /*!
     * \brief Constructs a polygon from its exterior ring.
     * \param[in] exterior The boundary; copied. Close it (first point repeated
     *                     last) and orient it counterclockwise; neither is
     *                     checked here (see isValid()).
     */
    explicit Polygon(const LineString &exterior);

    //! Destroys the polygon and its rings.
    ~Polygon() = default;

    // ── Rings ─────────────────────────────────────────────────────────────

    /*!
     * \brief Get exterior ring (boundary).
     * \returns The exterior ring; empty for a default-constructed polygon.
     *          The reference is valid while the polygon exists.
     */
    const LineString &exteriorRing() const noexcept { return m_exterior; }

    /*!
     * \brief Get number of interior holes.
     * \returns The number of rings added with addInteriorRing().
     */
    int numInteriorRings() const noexcept { return (int)m_holes.size(); }

    /*!
     * \brief Get interior ring (hole).
     * \param[in] i Zero-based hole index, from 0 to numInteriorRings() - 1.
     * \returns The hole's ring. For an index out of range this returns the
     *          *exterior* ring rather than failing, so check the index first.
     *          The reference is valid until another hole is added.
     */
    const LineString &interiorRing(int i) const {
        if (i < 0 || i >= (int)m_holes.size()) return m_exterior;
        return m_holes[i];
    }

    /*!
     * \brief Add an interior hole.
     * \param[in] hole The hole's ring; copied, not checked. Close it.
     */
    void addInteriorRing(const LineString &hole) {
        m_holes.push_back(hole);
    }

    // ── Geometry ──────────────────────────────────────────────────────────

    /*!
     * \brief Tests whether the exterior ring is a closed ring of at least
     *        three distinct corners.
     *
     * Only the exterior ring is checked: the holes, the ring orientation and
     * self-intersection are not (the SDK links no geometry engine).
     *
     * \returns True when the exterior ring isClosed() and has at least four
     *          points (three corners plus the repeated first point).
     */
    bool isValid() const noexcept {
        return m_exterior.isClosed() && m_exterior.pointCount() >= 4;
    }

    /*!
     * \brief Returns the bounding box of the exterior ring.
     * \returns The exterior ring's envelope (holes lie inside it), z
     *          included; empty for a polygon with no exterior ring.
     */
    Envelope envelope() const { return m_exterior.envelope(); }

    /*!
     * \brief Calculate polygon area using shoelace formula.
     *
     * Positive area = counter-clockwise exterior ring.
     * Interior holes subtract from area.
     *
     * Computed in plan (x, y only) as
     * \f$ \left| S_e - \sum_h S_h \right| \f$, where \f$S\f$ is a ring's
     * signed shoelace area (positive counterclockwise). A hole
     * therefore reduces the area only when it is wound in the same direction
     * as the exterior ring; a clockwise hole inside a counterclockwise
     * exterior, as the class convention describes, adds its area instead.
     *
     * \returns The area in squared coordinate units, never negative; 0 when
     *          the exterior ring has fewer than three points.
     */
    double area() const;

    /*!
     * \brief Calculate centroid weighted by area.
     *
     * Combines the exterior ring e and the holes h as
     * \f[ c = \frac{S_e\,r(e) - \sum_h S_h\,r(h)}{S_e} \f]
     * where \f$S\f$ is a ring's signed area (ringSignedArea()) and
     * \f$r\f$ the value ringCentroid() returns. Two things make this differ
     * from the area-weighted centroid the name promises: ringCentroid()
     * returns half of a ring's centroid, so even a polygon without holes
     * reports half its centroid (a unit square at the origin gives
     * (0.25, 0.25), not (0.5, 0.5)); and with holes the divisor is the
     * exterior ring's area, not the net area. When the exterior ring's area
     * is below 1e-10 the vertex mean of the exterior ring is returned
     * instead.
     *
     * \returns The value above in plan, with z = 0 and m = 0, except in the
     *          degenerate case, where z is the vertex mean of z.
     */
    Point centroid() const;

    /*!
     * \brief Check if point is inside polygon (ray casting algorithm).
     *
     * Casts a ray from \p p toward +x and counts the crossings of each ring,
     * in plan (z is ignored). \p p is inside when the exterior ring is
     * crossed an odd number of times and no hole is. Points exactly on a
     * ring follow the half-open rule of the crossing test (an edge includes
     * its lower endpoint in y and excludes its upper one), so a point on the
     * boundary may test either way.
     *
     * \param[in] p The point to test.
     * \returns True when \p p is inside the exterior ring and outside every
     *          hole.
     */
    bool containsPoint(const Point &p) const;

    /*!
     * \brief Get number of points (exterior + holes).
     * \returns The points of the exterior ring plus those of every hole,
     *          counting each ring's repeated closing point.
     */
    int pointCount() const noexcept;

private:
    LineString m_exterior;
    std::vector<LineString> m_holes;

    /*!
     * \brief Returns a ring's signed area by the shoelace (trapezoid) sum.
     *
     * \f$ S = -\tfrac{1}{2} \sum_{i=0}^{n-2} (x_{i+1} - x_i)(y_{i+1} + y_i) \f$
     * over the n stored points; the ring is not closed implicitly.
     *
     * \param[in] ring A closed ring; only x and y are used.
     * \returns The area, positive for a counterclockwise ring and negative
     *          for a clockwise one; 0 for fewer than three points.
     */
    double ringSignedArea(const LineString &ring) const;

    /*!
     * \brief Returns the shoelace centroid of one ring.
     * \param[in] ring A closed ring; only x and y are used.
     * \returns The ring's centroid in plan; the origin for fewer than three
     *          points and the ring's vertex mean when the summed cross
     *          products are below 1e-10. The divisor is six times the summed
     *          cross products, which is twice the shoelace centroid's divisor
     *          of six times the area, so the coordinates returned are half the
     *          ring's centroid (a unit square at the origin gives (0.25, 0.25)).
     */
    Point ringCentroid(const LineString &ring) const;
};

/*! \var LineString HydroCouple::SDK::Spatial::Polygon::m_exterior
 *  \brief The exterior ring, as given to the constructor.
 */
/*! \var std::vector<LineString> HydroCouple::SDK::Spatial::Polygon::m_holes
 *  \brief The interior rings, in the order they were added.
 */

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_POLYGON_H
