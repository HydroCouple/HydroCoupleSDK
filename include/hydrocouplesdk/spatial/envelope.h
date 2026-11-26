// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   envelope.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Envelope, the axis-aligned bounding box of the SDK geometries.
 *
 * Every SDK geometry reports its extent as an Envelope value. The standard's
 * HydroCouple::Spatial::IEnvelope is served over one by EnvelopeAdapter
 * (envelopeadapter.h).
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
 * \brief Axis-aligned bounding box with minimum and maximum x, y and z.
 *
 * Represents the smallest box, with sides parallel to the coordinate axes,
 * that contains a geometry. The six bounds are public data members in the
 * units of the geometry's coordinates.
 *
 * A default-constructed envelope is empty: each minimum is the largest finite
 * double and each maximum the lowest, so isValid() is false and the first
 * expandToInclude() call sets all six bounds to the included point or box.
 * The two-dimensional constructor sets both z bounds to 0, which makes a
 * valid box of zero depth; contains() and intersects() test z as well, so
 * such a box contains only points with z = 0.
 *
 * Envelope is a value type and does not implement
 * HydroCouple::Spatial::IEnvelope; EnvelopeAdapter does.
 *
 * \threadsafety Not thread-safe: concurrent reads are safe, but any
 * mutation (expandToInclude(), assignment to a bound) must not overlap other
 * access to the same envelope.
 */
class HYDROCOUPLESDK_EXPORT Envelope
{
public:
    /*!
     * \brief Constructs an empty envelope, for which isValid() is false.
     *
     * Each minimum starts at the largest finite double and each maximum at
     * the lowest, so the first expandToInclude() takes its argument's bounds.
     * width(), height(), depth(), area() and volume() of an empty envelope
     * are not meaningful (width() is minus infinity).
     */
    Envelope() noexcept
        : minX(std::numeric_limits<double>::max()),
          maxX(std::numeric_limits<double>::lowest()),
          minY(std::numeric_limits<double>::max()),
          maxY(std::numeric_limits<double>::lowest()),
          minZ(std::numeric_limits<double>::max()),
          maxZ(std::numeric_limits<double>::lowest()) {}

    /*!
     * \brief Constructs a two-dimensional envelope from its plan bounds.
     *
     * Both z bounds are set to 0. The arguments are stored as given; the
     * constructor does not reorder them, so passing a maximum where a
     * minimum belongs makes an envelope whose isValid() is false.
     *
     * \param[in] minX_ Smallest x.
     * \param[in] minY_ Smallest y.
     * \param[in] maxX_ Largest x.
     * \param[in] maxY_ Largest y.
     */
    Envelope(double minX_, double minY_, double maxX_, double maxY_) noexcept
        : minX(minX_), maxX(maxX_), minY(minY_), maxY(maxY_),
          minZ(0.0), maxZ(0.0) {}

    /*!
     * \brief Constructs a three-dimensional envelope from its bounds.
     *
     * The arguments are stored as given, without reordering.
     *
     * \param[in] minX_ Smallest x.
     * \param[in] minY_ Smallest y.
     * \param[in] minZ_ Smallest z.
     * \param[in] maxX_ Largest x.
     * \param[in] maxY_ Largest y.
     * \param[in] maxZ_ Largest z.
     */
    Envelope(double minX_, double minY_, double minZ_,
             double maxX_, double maxY_, double maxZ_) noexcept
        : minX(minX_), maxX(maxX_), minY(minY_), maxY(maxY_),
          minZ(minZ_), maxZ(maxZ_) {}

    /*!
     * \brief Constructs the smallest envelope containing two points.
     *
     * The points may be any two opposite corners; each bound is the minimum
     * or maximum of the two coordinates, z included.
     *
     * \param[in] p1 One corner.
     * \param[in] p2 The opposite corner.
     */
    Envelope(const Point &p1, const Point &p2) noexcept {
        minX = std::min(p1.x, p2.x);
        maxX = std::max(p1.x, p2.x);
        minY = std::min(p1.y, p2.y);
        maxY = std::max(p1.y, p2.y);
        minZ = std::min(p1.z, p2.z);
        maxZ = std::max(p1.z, p2.z);
    }

    //! Destroys the envelope; it owns nothing.
    ~Envelope() = default;

    // ── Bounds ────────────────────────────────────────────────────────────

    double minX, maxX, minY, maxY, minZ, maxZ;

    /*!
     * \brief Tests whether the envelope describes a box.
     * \returns True when minX <= maxX, minY <= maxY and minZ <= maxZ; false
     *          for a default-constructed (empty) envelope.
     */
    bool isValid() const noexcept {
        return minX <= maxX && minY <= maxY && minZ <= maxZ;
    }

    /*!
     * \brief Grows the envelope, if needed, to contain a point.
     *
     * All three axes are expanded, so including a point with z ≠ 0 in a
     * two-dimensional envelope (z bounds 0) gives it depth.
     *
     * \param[in] p The point to include.
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
     * \brief Grows the envelope, if needed, to contain another envelope.
     *
     * Including an empty envelope leaves this one unchanged, because its
     * minima are larger and its maxima smaller than any real bound.
     *
     * \param[in] other The envelope to include.
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
     * \brief Tests whether a point lies inside or on the boundary of the box.
     * \param[in] p The point to test.
     * \returns True when every coordinate of \p p, z included, lies within
     *          the inclusive bounds of its axis.
     */
    bool contains(const Point &p) const noexcept {
        return p.x >= minX && p.x <= maxX &&
               p.y >= minY && p.y <= maxY &&
               p.z >= minZ && p.z <= maxZ;
    }

    /*!
     * \brief Tests whether two boxes share at least one point.
     *
     * Boxes that only touch along a face, an edge or a corner intersect.
     *
     * \param[in] other The envelope to test against.
     * \returns True unless the boxes are separated along x, y or z.
     */
    bool intersects(const Envelope &other) const noexcept {
        return !(minX > other.maxX || maxX < other.minX ||
                 minY > other.maxY || maxY < other.minY ||
                 minZ > other.maxZ || maxZ < other.minZ);
    }

    /*!
     * \brief Returns the center of the box.
     * \returns The midpoint of each axis, with m = 0.
     */
    Point center() const noexcept {
        return Point((minX + maxX) / 2.0, (minY + maxY) / 2.0, (minZ + maxZ) / 2.0);
    }

    /*!
     * \brief Returns the extent along x.
     * \returns maxX - minX.
     */
    double width() const noexcept { return maxX - minX; }

    /*!
     * \brief Returns the extent along y.
     * \returns maxY - minY.
     */
    double height() const noexcept { return maxY - minY; }

    /*!
     * \brief Returns the extent along z.
     * \returns maxZ - minZ; 0 for a two-dimensional envelope.
     */
    double depth() const noexcept { return maxZ - minZ; }

    /*!
     * \brief Returns the plan area of the box.
     * \returns width() × height(), in squared coordinate units.
     */
    double area() const noexcept { return width() * height(); }

    /*!
     * \brief Returns the volume of the box.
     * \returns width() × height() × depth(); 0 for a two-dimensional
     *          envelope.
     */
    double volume() const noexcept { return width() * height() * depth(); }
};

/*! \var double HydroCouple::SDK::Spatial::Envelope::minX
 *  \brief Smallest x in the box.
 */
/*! \var double HydroCouple::SDK::Spatial::Envelope::maxX
 *  \brief Largest x in the box.
 */
/*! \var double HydroCouple::SDK::Spatial::Envelope::minY
 *  \brief Smallest y in the box.
 */
/*! \var double HydroCouple::SDK::Spatial::Envelope::maxY
 *  \brief Largest y in the box.
 */
/*! \var double HydroCouple::SDK::Spatial::Envelope::minZ
 *  \brief Smallest z in the box; 0 for a two-dimensional envelope.
 */
/*! \var double HydroCouple::SDK::Spatial::Envelope::maxZ
 *  \brief Largest z in the box; 0 for a two-dimensional envelope.
 */

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_ENVELOPE_H
