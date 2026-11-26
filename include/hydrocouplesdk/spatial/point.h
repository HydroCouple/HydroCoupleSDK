// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   point.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Point, the SDK's two- and three-dimensional coordinate value type.
 *
 * Point is the building block of every other SDK geometry (LineString,
 * Polygon, PolyhedralSurface, TIN, Network) and doubles as a 3-D vector for
 * the small amount of vector algebra those classes need. It is a plain value:
 * it does not implement HydroCouple::Spatial::IPoint. A component that must
 * hand a point to a consumer through the standard wraps it in a PointAdapter
 * (geometryadapters.h).
 */

#ifndef HYDROCOUPLESDK_POINT_H
#define HYDROCOUPLESDK_POINT_H

#include "hydrocouplesdk/export.h"

#include <cmath>
#include <array>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

/*!
 * \brief A coordinate with x, y, z and a measure m, stored as four doubles.
 *
 * The four coordinates are public data members, so a Point is an aggregate in
 * practice: 32 bytes on the stack, no heap allocation, freely copied. The
 * coordinates carry no unit or reference system of their own; they are in
 * whatever system the geometry that holds them declares (see the
 * spatial-reference pointer on the adapters). A two-dimensional point has
 * z = 0, and the measure m defaults to 0.
 *
 * The vector operations (operator-(), operator+(), operator*(), dot(),
 * cross(), magnitude(), normalized()) treat the point as the 3-D vector
 * (x, y, z): they ignore m, and every Point they return has m = 0.
 *
 * Point is not an implementation of HydroCouple::Spatial::IPoint; use
 * PointAdapter to serve one through that interface.
 *
 * \threadsafety Concurrent reads are safe. Writing a coordinate while
 * another thread reads the same Point is a data race.
 */
class HYDROCOUPLESDK_EXPORT Point
{
public:
    /*!
     * \brief Constructs the origin, (0, 0, 0) with measure 0.
     */
    Point() noexcept : x(0.0), y(0.0), z(0.0), m(0.0) {}

    /*!
     * \brief Constructs a two-dimensional point; z and m are set to 0.
     * \param[in] x_ The x coordinate.
     * \param[in] y_ The y coordinate.
     */
    Point(double x_, double y_) noexcept
        : x(x_), y(y_), z(0.0), m(0.0) {}

    /*!
     * \brief Constructs a three-dimensional point; m is set to 0.
     * \param[in] x_ The x coordinate.
     * \param[in] y_ The y coordinate.
     * \param[in] z_ The z coordinate (an elevation, positive up, when the
     *               owning geometry's reference system declares a vertical
     *               datum).
     */
    Point(double x_, double y_, double z_) noexcept
        : x(x_), y(y_), z(z_), m(0.0) {}

    /*!
     * \brief Constructs a three-dimensional point with a measure.
     * \param[in] x_ The x coordinate.
     * \param[in] y_ The y coordinate.
     * \param[in] z_ The z coordinate.
     * \param[in] m_ The measure: a value attached to the position, such as a
     *               river station or a distance along a line, in whatever
     *               units the owner uses.
     */
    Point(double x_, double y_, double z_, double m_) noexcept
        : x(x_), y(y_), z(z_), m(m_) {}

    //! Destroys the point; it owns nothing.
    ~Point() = default;

    // ── Coordinate access ────────────────────────────────────────────────

    double x, y, z, m;

    /*!
     * \brief Returns the squared Euclidean distance to another point in three
     *        dimensions.
     *
     * Uses x, y and z; m is ignored. For two-dimensional points (z = 0 on
     * both) this is the plan distance squared. Cheaper than distance() when
     * only comparisons are needed.
     *
     * \param[in] other The point to measure to.
     * \returns (dx² + dy² + dz²) in the squared units of the coordinates.
     */
    double distanceSq(const Point &other) const noexcept {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return dx*dx + dy*dy + dz*dz;
    }

    /*!
     * \brief Returns the Euclidean distance to another point in three
     *        dimensions.
     * \param[in] other The point to measure to.
     * \returns The square root of distanceSq(), in the units of the
     *          coordinates; never negative.
     */
    double distance(const Point &other) const noexcept {
        return std::sqrt(distanceSq(other));
    }

    /*!
     * \brief Returns the vector from \p other to this point.
     * \param[in] other The point subtracted.
     * \returns (x - other.x, y - other.y, z - other.z) with m = 0.
     */
    Point operator-(const Point &other) const noexcept {
        return Point(x - other.x, y - other.y, z - other.z);
    }

    /*!
     * \brief Returns the component-wise sum of this point and \p other.
     * \param[in] other The vector added.
     * \returns (x + other.x, y + other.y, z + other.z) with m = 0.
     */
    Point operator+(const Point &other) const noexcept {
        return Point(x + other.x, y + other.y, z + other.z);
    }

    /*!
     * \brief Returns this point scaled by a factor.
     * \param[in] scalar The factor applied to x, y and z.
     * \returns (x·scalar, y·scalar, z·scalar) with m = 0.
     */
    Point operator*(double scalar) const noexcept {
        return Point(x * scalar, y * scalar, z * scalar);
    }

    /*!
     * \brief Returns the dot product of this point and \p other as 3-D
     *        vectors.
     * \param[in] other The second vector.
     * \returns x·other.x + y·other.y + z·other.z.
     */
    double dot(const Point &other) const noexcept {
        return x*other.x + y*other.y + z*other.z;
    }

    /*!
     * \brief Returns the cross product of this point and \p other as 3-D
     *        vectors.
     *
     * For two vectors in the xy plane the result points along z and its z
     * component is twice the signed area of the triangle they span (positive
     * when \p other lies counterclockwise from this vector).
     *
     * \param[in] other The second vector.
     * \returns this × other, with m = 0.
     */
    Point cross(const Point &other) const noexcept {
        return Point(
            y*other.z - z*other.y,
            z*other.x - x*other.z,
            x*other.y - y*other.x
        );
    }

    /*!
     * \brief Returns the length of this point as a 3-D vector.
     * \returns sqrt(x² + y² + z²); m is ignored.
     */
    double magnitude() const noexcept {
        return std::sqrt(x*x + y*y + z*z);
    }

    /*!
     * \brief Returns the unit vector in the direction of this point.
     * \returns This vector divided by magnitude(), with m = 0; the zero
     *          vector (0, 0, 0) when the magnitude is exactly zero, rather
     *          than a division by zero.
     */
    Point normalized() const noexcept {
        double mag = magnitude();
        if (mag == 0.0) return Point(0.0, 0.0, 0.0);
        return Point(x/mag, y/mag, z/mag);
    }

    /*!
     * \brief Tests whether two points coincide within an absolute tolerance.
     *
     * Compares x, y and z separately; m is not compared. The test is strict:
     * a difference equal to \p tolerance counts as different.
     *
     * \param[in] other     The point to compare with.
     * \param[in] tolerance Largest absolute difference per coordinate that
     *                      still counts as equal, in coordinate units; must be
     *                      positive for any two points to compare equal.
     * \returns True when |x - other.x|, |y - other.y| and |z - other.z| are
     *          all less than \p tolerance.
     */
    bool equals(const Point &other, double tolerance = 1e-10) const noexcept {
        return std::abs(x - other.x) < tolerance &&
               std::abs(y - other.y) < tolerance &&
               std::abs(z - other.z) < tolerance;
    }
};

/*! \var double HydroCouple::SDK::Spatial::Point::x
 *  \brief The x coordinate (easting or longitude), in the units of the
 *         owning geometry's reference system.
 */
/*! \var double HydroCouple::SDK::Spatial::Point::y
 *  \brief The y coordinate (northing or latitude), in the units of the
 *         owning geometry's reference system.
 */
/*! \var double HydroCouple::SDK::Spatial::Point::z
 *  \brief The z coordinate; 0 for a two-dimensional point.
 */
/*! \var double HydroCouple::SDK::Spatial::Point::m
 *  \brief The measure attached to the position; 0 unless set.
 */

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_POINT_H
