// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   linestring.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  LineString geometry with polyline topology.
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
 * \threadsafety Not thread-safe.
 */
class HYDROCOUPLESDK_EXPORT LineString
{
public:
    /*!
     * \brief Construct empty linestring.
     */
    LineString() = default;

    /*!
     * \brief Construct from a vector of points.
     */
    explicit LineString(const std::vector<Point> &points);

    ~LineString() = default;

    // ── Point access ──────────────────────────────────────────────────────

    /*!
     * \brief Get number of points.
     */
    int pointCount() const noexcept { return (int)m_x.size(); }

    /*!
     * \brief Get point at index.
     */
    Point pointAt(int i) const noexcept {
        if (i < 0 || i >= (int)m_x.size()) return Point();
        return Point(m_x[i], m_y[i], m_z[i], m_m[i]);
    }

    /*!
     * \brief Get first point.
     */
    Point startPoint() const noexcept { return pointAt(0); }

    /*!
     * \brief Get last point.
     */
    Point endPoint() const noexcept { return pointAt(pointCount() - 1); }

    /*!
     * \brief Add a point to the end.
     */
    void addPoint(const Point &p);

    /*!
     * \brief Add points from vector.
     */
    void addPoints(const std::vector<Point> &points) {
        for (const auto &p : points) addPoint(p);
    }

    // ── Coordinate access (SoA) ───────────────────────────────────────────

    /*!
     * \brief Direct access to X coordinates (for SIMD).
     */
    const std::vector<double> &getX() const noexcept { return m_x; }
    const std::vector<double> &getY() const noexcept { return m_y; }
    const std::vector<double> &getZ() const noexcept { return m_z; }
    const std::vector<double> &getM() const noexcept { return m_m; }

    // ── Geometry ──────────────────────────────────────────────────────────

    /*!
     * \brief Check if linestring is empty.
     */
    bool isEmpty() const noexcept { return m_x.empty(); }

    /*!
     * \brief Check if linestring is closed (start == end).
     */
    bool isClosed() const noexcept {
        return pointCount() >= 2 && startPoint().equals(endPoint());
    }

    /*!
     * \brief Get length (sum of segment lengths).
     */
    double length() const;

    /*!
     * \brief Get bounding envelope.
     */
    Envelope envelope() const;

    /*!
     * \brief Get centroid (average of all points).
     */
    Point centroid() const;

    /*!
     * \brief Check if M-values are present.
     */
    bool hasM() const noexcept { return !m_m.empty(); }

private:
    std::vector<double> m_x, m_y, m_z, m_m;  // SoA storage
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_LINESTRING_H
