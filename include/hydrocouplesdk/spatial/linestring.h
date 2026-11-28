// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   linestring.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  LineString, an ordered sequence of points joined by straight
 *         segments.
 *
 * LineString is the SDK's polyline value type and the ring type of Polygon.
 * It does not implement HydroCouple::Spatial::ILineString; LineStringAdapter
 * (geometryadapters.h) serves one through that interface.
 */

#ifndef HYDROCOUPLESDK_LINESTRING_H
#define HYDROCOUPLESDK_LINESTRING_H

#include "hydrocouplesdk/export.h"
#include "point.h"
#include "envelope.h"

#include <vector>
#include <cmath>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

/*!
 * \brief Ordered sequence of 2D/3D points forming a polyline.
 *
 * Stores coordinates in Structure-of-Arrays (SoA) format:
 * - Separate std::vector<double> for x, y, z, m
 * - Contiguous memory, SIMD-friendly for bulk operations
 * - No dynamic Point allocation
 *
 * Supports optional M-values for parametric representation (river km, etc.).
 *
 * Points are appended with addPoint() or addPoints() and cannot be removed or
 * edited in place; build a new line to change one. The four coordinate arrays
 * always have the same length, pointCount(). Consecutive points are joined by
 * straight segments in three dimensions: length() includes the change in z.
 * A ring (a closed line, as Polygon uses) repeats its first point as its last.
 *
 * \threadsafety Not thread-safe: concurrent reads are safe, but adding
 * points must not overlap other access to the same line.
 */
class HYDROCOUPLESDK_EXPORT LineString
{
public:
    /*!
     * \brief Constructs an empty line with no points.
     */
    LineString() = default;

    /*!
     * \brief Constructs a line from points, in order.
     * \param[in] points The vertices; copied, including their z and m.
     */
    explicit LineString(const std::vector<Point> &points);

    //! Destroys the line and its coordinate arrays.
    ~LineString() = default;

    // ── Point access ──────────────────────────────────────────────────────

    /*!
     * \brief Returns the number of points.
     * \returns The length of each coordinate array; 0 for an empty line.
     */
    int pointCount() const noexcept { return (int)m_x.size(); }

    /*!
     * \brief Returns a copy of the point at an index.
     * \param[in] i Zero-based index, from 0 to pointCount() - 1.
     * \returns The point with its x, y, z and m; the origin Point() when \p i
     *          is out of range, rather than an error.
     */
    Point pointAt(int i) const noexcept {
        if (i < 0 || i >= (int)m_x.size()) return Point();
        return Point(m_x[i], m_y[i], m_z[i], m_m[i]);
    }

    /*!
     * \brief Returns the first point.
     * \returns pointAt(0); the origin for an empty line.
     */
    Point startPoint() const noexcept { return pointAt(0); }

    /*!
     * \brief Returns the last point.
     * \returns pointAt(pointCount() - 1); the origin for an empty line.
     */
    Point endPoint() const noexcept { return pointAt(pointCount() - 1); }

    /*!
     * \brief Appends a point to the end of the line.
     * \param[in] p The point; its x, y, z and m are copied.
     */
    void addPoint(const Point &p);

    /*!
     * \brief Appends points to the end of the line, in order.
     * \param[in] points The points to append; each is copied.
     */
    void addPoints(const std::vector<Point> &points) {
        for (const auto &p : points) addPoint(p);
    }

    // ── Coordinate access (SoA) ───────────────────────────────────────────

    /*!
     * \brief Direct access to X coordinates (for SIMD).
     * \returns The x coordinates, pointCount() elements, in point order.
     *          The reference is valid until the line is modified or
     *          destroyed.
     */
    const std::vector<double> &getX() const noexcept { return m_x; }
    /*!
     * \brief Direct access to Y coordinates.
     * \returns The y coordinates, pointCount() elements; valid as getX() is.
     */
    const std::vector<double> &getY() const noexcept { return m_y; }
    /*!
     * \brief Direct access to Z coordinates.
     * \returns The z coordinates, pointCount() elements (0 for points added
     *          without z); valid as getX() is.
     */
    const std::vector<double> &getZ() const noexcept { return m_z; }
    /*!
     * \brief Direct access to the measures.
     * \returns The measures, pointCount() elements (0 where none was set);
     *          valid as getX() is.
     */
    const std::vector<double> &getM() const noexcept { return m_m; }

    // ── Geometry ──────────────────────────────────────────────────────────

    /*!
     * \brief Tests whether the line has no points.
     * \returns True when pointCount() is 0.
     */
    bool isEmpty() const noexcept { return m_x.empty(); }

    /*!
     * \brief Tests whether the line ends where it starts.
     * \returns True when the line has at least two points and the first and
     *          last are equal under Point::equals() (absolute tolerance
     *          1e-10 on x, y and z).
     */
    bool isClosed() const noexcept {
        return pointCount() >= 2 && startPoint().equals(endPoint());
    }

    /*!
     * \brief Returns the length of the line.
     * \returns The sum of the three-dimensional lengths of its segments, in
     *          coordinate units; 0 for fewer than two points.
     */
    double length() const;

    /*!
     * \brief Returns the bounding box of the points.
     * \returns The smallest envelope containing every point, z included; an
     *          empty Envelope (isValid() false) for an empty line.
     */
    Envelope envelope() const;

    /*!
     * \brief Returns the mean of the points.
     *
     * This is the vertex average, not the length-weighted centroid of the
     * line: closely spaced points pull it toward themselves, and the repeated
     * closing point of a ring is counted twice.
     *
     * \returns The average x, y and z of all points, with m = 0; the origin
     *          for an empty line.
     */
    Point centroid() const;

    /*!
     * \brief Reports whether the line holds a measure array.
     *
     * The measure array grows with every point added (a point without a
     * measure contributes m = 0), so this is true for every non-empty line
     * whether or not any measure was set. It does not detect measures that
     * are all zero.
     *
     * \returns True when the line has at least one point.
     */
    bool hasM() const noexcept { return !m_m.empty(); }

private:
    std::vector<double> m_x, m_y, m_z, m_m;  // SoA storage
};

/*! \var std::vector<double> HydroCouple::SDK::Spatial::LineString::m_x
 *  \brief x coordinate of each point, in order (SoA storage).
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::LineString::m_y
 *  \brief y coordinate of each point; same length as m_x.
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::LineString::m_z
 *  \brief z coordinate of each point; same length as m_x.
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::LineString::m_m
 *  \brief Measure of each point; same length as m_x.
 */

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_LINESTRING_H
