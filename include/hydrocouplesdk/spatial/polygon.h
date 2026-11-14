// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   polygon.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Polygon geometry with exterior ring and holes.
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
 * \threadsafety Not thread-safe.
 */
class HYDROCOUPLESDK_EXPORT Polygon
{
public:
    /*!
     * \brief Construct empty polygon.
     */
    Polygon() = default;

    /*!
     * \brief Construct from exterior ring.
     */
    explicit Polygon(const LineString &exterior);

    ~Polygon() = default;

    // ── Rings ─────────────────────────────────────────────────────────────

    /*!
     * \brief Get exterior ring (boundary).
     */
    const LineString &exteriorRing() const noexcept { return m_exterior; }

    /*!
     * \brief Get number of interior holes.
     */
    int numInteriorRings() const noexcept { return (int)m_holes.size(); }

    /*!
     * \brief Get interior ring (hole).
     */
    const LineString &interiorRing(int i) const {
        if (i < 0 || i >= (int)m_holes.size()) return m_exterior;
        return m_holes[i];
    }

    /*!
     * \brief Add an interior hole.
     */
    void addInteriorRing(const LineString &hole) {
        m_holes.push_back(hole);
    }

    // ── Geometry ──────────────────────────────────────────────────────────

    /*!
     * \brief Check if polygon is valid (closed rings, no self-intersection).
     */
    bool isValid() const noexcept {
        return m_exterior.isClosed() && m_exterior.pointCount() >= 4;
    }

    /*!
     * \brief Get bounding envelope.
     */
    Envelope envelope() const { return m_exterior.envelope(); }

    /*!
     * \brief Calculate polygon area using shoelace formula.
     *
     * Positive area = counter-clockwise exterior ring.
     * Interior holes subtract from area.
     */
    double area() const;

    /*!
     * \brief Calculate centroid weighted by area.
     */
    Point centroid() const;

    /*!
     * \brief Check if point is inside polygon (ray casting algorithm).
     */
    bool containsPoint(const Point &p) const;

    /*!
     * \brief Get number of points (exterior + holes).
     */
    int pointCount() const noexcept;

private:
    LineString m_exterior;
    std::vector<LineString> m_holes;

    double ringSignedArea(const LineString &ring) const;
    Point ringCentroid(const LineString &ring) const;
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_POLYGON_H
