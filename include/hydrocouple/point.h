// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   point.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  2D/3D point geometry.
 */

#ifndef HYDROCOUPLESDK_POINT_H
#define HYDROCOUPLESDK_POINT_H

#include "hydrocouplesdk.h"

#include <cmath>
#include <array>

namespace HydroCouple::Spatial {

/*!
 * \brief 3D point with x, y, z, optional m (measure).
 *
 * Stores coordinates on the stack (24 bytes: 3 × 8-byte doubles).
 * No heap allocation. Thread-safe for reads. Supports M-value (measure)
 * for parametric representation.
 *
 * \threadsafety Immutable read access is thread-safe. Mutations are not.
 */
class HYDROCOUPLESDK_EXPORT Point
{
public:
    /*!
     * \brief Construct origin (0, 0, 0).
     */
    Point() noexcept : x(0.0), y(0.0), z(0.0), m(0.0) {}

    /*!
     * \brief Construct from (x, y) coordinates.
     */
    Point(double x_, double y_) noexcept
        : x(x_), y(y_), z(0.0), m(0.0) {}

    /*!
     * \brief Construct from (x, y, z) coordinates.
     */
    Point(double x_, double y_, double z_) noexcept
        : x(x_), y(y_), z(z_), m(0.0) {}

    /*!
     * \brief Construct from (x, y, z, m) coordinates.
     */
    Point(double x_, double y_, double z_, double m_) noexcept
        : x(x_), y(y_), z(z_), m(m_) {}

    ~Point() = default;

    // ── Coordinate access ────────────────────────────────────────────────

    double x, y, z, m;

    /*!
     * \brief Squared distance to another point.
     */
    double distanceSq(const Point &other) const noexcept {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return dx*dx + dy*dy + dz*dz;
    }

    /*!
     * \brief Euclidean distance to another point.
     */
    double distance(const Point &other) const noexcept {
        return std::sqrt(distanceSq(other));
    }

    /*!
     * \brief Vector to another point.
     */
    Point operator-(const Point &other) const noexcept {
        return Point(x - other.x, y - other.y, z - other.z);
    }

    /*!
     * \brief Add vector.
     */
    Point operator+(const Point &other) const noexcept {
        return Point(x + other.x, y + other.y, z + other.z);
    }

    /*!
     * \brief Scalar multiplication.
     */
    Point operator*(double scalar) const noexcept {
        return Point(x * scalar, y * scalar, z * scalar);
    }

    /*!
     * \brief Dot product.
     */
    double dot(const Point &other) const noexcept {
        return x*other.x + y*other.y + z*other.z;
    }

    /*!
     * \brief Cross product (returns 3D vector).
     */
    Point cross(const Point &other) const noexcept {
        return Point(
            y*other.z - z*other.y,
            z*other.x - x*other.z,
            x*other.y - y*other.x
        );
    }

    /*!
     * \brief Magnitude of vector.
     */
    double magnitude() const noexcept {
        return std::sqrt(x*x + y*y + z*z);
    }

    /*!
     * \brief Normalized vector (unit length).
     */
    Point normalized() const noexcept {
        double mag = magnitude();
        if (mag == 0.0) return Point(0.0, 0.0, 0.0);
        return Point(x/mag, y/mag, z/mag);
    }

    /*!
     * \brief Equality comparison (with tolerance for floating-point).
     */
    bool equals(const Point &other, double tolerance = 1e-10) const noexcept {
        return std::abs(x - other.x) < tolerance &&
               std::abs(y - other.y) < tolerance &&
               std::abs(z - other.z) < tolerance;
    }
};

} // namespace HydroCouple::Spatial

#endif // HYDROCOUPLESDK_POINT_H
