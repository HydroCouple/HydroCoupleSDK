// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   envelope.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Axis-aligned bounding box for geometries.
 */

#ifndef HYDROCOUPLESDK_ENVELOPE_H
#define HYDROCOUPLESDK_ENVELOPE_H

#include "hydrocouplesdk/export.h"
#include "point.h"

#include <limits>
#include <algorithm>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

/*!
 * \brief Axis-aligned bounding box with min/max (x, y, z).
 *
 * Represents the smallest axis-aligned box containing a geometry.
 * Supports 2D and 3D. Undefined if minX > maxX (call isValid() to check).
 *
 * \threadsafety Not thread-safe.
 */
class HYDROCOUPLESDK_EXPORT Envelope
{
public:
    /*!
     * \brief Construct empty envelope (invalid).
     */
    Envelope() noexcept
        : minX(std::numeric_limits<double>::max()),
          maxX(std::numeric_limits<double>::lowest()),
          minY(std::numeric_limits<double>::max()),
          maxY(std::numeric_limits<double>::lowest()),
          minZ(std::numeric_limits<double>::max()),
          maxZ(std::numeric_limits<double>::lowest()) {}

    /*!
     * \brief Construct from 2D bounds.
     */
    Envelope(double minX_, double minY_, double maxX_, double maxY_) noexcept
        : minX(minX_), maxX(maxX_), minY(minY_), maxY(maxY_),
          minZ(0.0), maxZ(0.0) {}

    /*!
     * \brief Construct from 3D bounds.
     */
    Envelope(double minX_, double minY_, double minZ_,
             double maxX_, double maxY_, double maxZ_) noexcept
        : minX(minX_), maxX(maxX_), minY(minY_), maxY(maxY_),
          minZ(minZ_), maxZ(maxZ_) {}

    /*!
     * \brief Construct from two points.
     */
    Envelope(const Point &p1, const Point &p2) noexcept {
        minX = std::min(p1.x, p2.x);
        maxX = std::max(p1.x, p2.x);
        minY = std::min(p1.y, p2.y);
        maxY = std::max(p1.y, p2.y);
        minZ = std::min(p1.z, p2.z);
        maxZ = std::max(p1.z, p2.z);
    }

    ~Envelope() = default;

    // ── Bounds ────────────────────────────────────────────────────────────

    double minX, maxX, minY, maxY, minZ, maxZ;

    /*!
     * \brief Check if envelope is valid (minX <= maxX, etc.).
     */
    bool isValid() const noexcept {
        return minX <= maxX && minY <= maxY && minZ <= maxZ;
    }

    /*!
     * \brief Expand envelope to include a point.
     */
    void expandToInclude(const Point &p) noexcept {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
        minZ = std::min(minZ, p.z);
        maxZ = std::max(maxZ, p.z);
    }

    /*!
     * \brief Expand envelope to include another envelope.
     */
    void expandToInclude(const Envelope &other) noexcept {
        minX = std::min(minX, other.minX);
        maxX = std::max(maxX, other.maxX);
        minY = std::min(minY, other.minY);
        maxY = std::max(maxY, other.maxY);
        minZ = std::min(minZ, other.minZ);
        maxZ = std::max(maxZ, other.maxZ);
    }

    /*!
     * \brief Check if point is inside envelope.
     */
    bool contains(const Point &p) const noexcept {
        return p.x >= minX && p.x <= maxX &&
               p.y >= minY && p.y <= maxY &&
               p.z >= minZ && p.z <= maxZ;
    }

    /*!
     * \brief Check if another envelope overlaps this one.
     */
    bool intersects(const Envelope &other) const noexcept {
        return !(minX > other.maxX || maxX < other.minX ||
                 minY > other.maxY || maxY < other.minY ||
                 minZ > other.maxZ || maxZ < other.minZ);
    }

    /*!
     * \brief Get center point.
     */
    Point center() const noexcept {
        return Point((minX + maxX) / 2.0, (minY + maxY) / 2.0, (minZ + maxZ) / 2.0);
    }

    /*!
     * \brief Get width (maxX - minX).
     */
    double width() const noexcept { return maxX - minX; }

    /*!
     * \brief Get height (maxY - minY).
     */
    double height() const noexcept { return maxY - minY; }

    /*!
     * \brief Get depth (maxZ - minZ).
     */
    double depth() const noexcept { return maxZ - minZ; }

    /*!
     * \brief Get area (width × height).
     */
    double area() const noexcept { return width() * height(); }

    /*!
     * \brief Get volume (width × height × depth).
     */
    double volume() const noexcept { return width() * height() * depth(); }
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_ENVELOPE_H
