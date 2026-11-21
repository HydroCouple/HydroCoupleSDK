// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   geometryadapters.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Adapters exposing the SDK's spatial value types through the
 *         standard's IGeometry family.
 *
 * The SDK's spatial primitives (Point, LineString, Polygon,
 * PolyhedralSurface, Network) are lightweight value types, while the
 * standard's data items hand out interface pointers. Without these adapters
 * every component wanting to publish a geometry data item has to implement
 * `IGeometry` itself — **thirty-two methods** — plus IPoint/IPolygon/INetwork
 * as applicable, and each one would do it slightly differently.
 *
 * Follows the EnvelopeAdapter precedent: the value types stay values, and the
 * adapters own a copy and serve the interface over it.
 *
 * \note The predicates and constructive operations the standard defines
 *       (intersects, buffer, convexHull, …) need a full geometry engine. The
 *       SDK does not link one, so those report false / nullptr rather than
 *       guessing. Consumers that need them — a GIS front end, say — have
 *       GEOS or GDAL to hand and can operate on the WKB these adapters do
 *       produce. Everything a viewer or a writer reads is real.
 *
 * \threadsafety Reads are safe after construction; mutations are not.
 */

#ifndef HYDROCOUPLESDK_SPATIAL_GEOMETRYADAPTERS_H
#define HYDROCOUPLESDK_SPATIAL_GEOMETRYADAPTERS_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/spatial/envelopeadapter.h"
#include "hydrocouplesdk/spatial/linestring.h"
#include "hydrocouplesdk/spatial/point.h"
#include "hydrocouplesdk/spatial/polygon.h"
#include "hydrocouplespatial.h"

#include <memory>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Spatial {

/*!
 * \brief The methods of IGeometry every adapter shares.
 *
 * Subclasses supply the geometry type, the WKB body and the envelope; this
 * base answers the rest of the interface so a concrete adapter stays the few
 * lines that describe its own shape.
 */
class HYDROCOUPLESDK_EXPORT GeometryAdapter
    : public virtual HydroCouple::Spatial::IGeometry
{
public:
    /*!
     * \brief Construct with an identifier and index.
     * \param[in] id    Identifier reported by id().
     * \param[in] index Position within the owning collection.
     * \param[in] srs   Spatial reference (borrowed; may be nullptr).
     */
    GeometryAdapter(std::string id, int64_t index,
                    HydroCouple::Spatial::ISpatialReferenceSystem *srs);

    ~GeometryAdapter() override = default;

    /*!
     * \brief Sets the spatial reference reported by this geometry.
     */
    void setSpatialReferenceSystem(
        HydroCouple::Spatial::ISpatialReferenceSystem *srs) noexcept
    {
        m_srs = srs;
    }

    // ── IGeometry: identity and description ───────────────────────────────

    [[nodiscard]] const std::string &id() const override { return m_id; }
    [[nodiscard]] int64_t index() const override { return m_index; }
    [[nodiscard]] int coordinateDimension() const override { return 2; }

    [[nodiscard]] HydroCouple::Spatial::ISpatialReferenceSystem *
    spatialReferenceSystem() const override
    {
        return m_srs;
    }

    [[nodiscard]] HydroCouple::Spatial::IEnvelope *envelope() const override;

    [[nodiscard]] std::string getWKT() const override;

    [[nodiscard]] bool isSimple() const override { return true; }
    [[nodiscard]] bool is3D() const override { return false; }
    [[nodiscard]] bool isMeasured() const override { return false; }

    // ── IGeometry: relational predicates ──────────────────────────────────
    //
    // Not computed: the SDK links no geometry engine. Reported honestly as
    // "no" rather than guessed, and the WKB above is what a caller with GEOS
    // or GDAL uses instead.

    [[nodiscard]] std::unique_ptr<IGeometry> boundary() const override { return nullptr; }
    [[nodiscard]] bool equals(const IGeometry &) const override
    {
        return false;
    }
    [[nodiscard]] bool disjoint(const IGeometry &) const override
    {
        return false;
    }
    [[nodiscard]] bool intersects(const IGeometry &) const override
    {
        return false;
    }
    [[nodiscard]] bool touches(const IGeometry &) const override
    {
        return false;
    }
    [[nodiscard]] bool crosses(const IGeometry &) const override
    {
        return false;
    }
    [[nodiscard]] bool within(const IGeometry &) const override
    {
        return false;
    }
    [[nodiscard]] bool contains(const IGeometry &) const override
    {
        return false;
    }
    [[nodiscard]] bool overlaps(const IGeometry &) const override
    {
        return false;
    }
    [[nodiscard]] bool relate(const IGeometry &, const std::string &) const override
    {
        return false;
    }
    [[nodiscard]] std::unique_ptr<IGeometry> locateAlong(double) const override
    {
        return nullptr;
    }
    [[nodiscard]] std::unique_ptr<IGeometry> locateBetween(double, double) const override
    {
        return nullptr;
    }
    [[nodiscard]] double distance(const IGeometry &) const override
    {
        return 0.0;
    }
    [[nodiscard]] std::unique_ptr<IGeometry> buffer(double) const override { return nullptr; }
    [[nodiscard]] std::unique_ptr<IGeometry> convexHull() const override { return nullptr; }
    [[nodiscard]] std::unique_ptr<IGeometry> intersection(const IGeometry &) const override
    {
        return nullptr;
    }
    [[nodiscard]] std::unique_ptr<IGeometry> unionG(const IGeometry &) const override
    {
        return nullptr;
    }
    [[nodiscard]] std::unique_ptr<IGeometry> difference(const IGeometry &) const override
    {
        return nullptr;
    }
    [[nodiscard]] std::unique_ptr<IGeometry> symmetricDifference(
        const IGeometry &) const override
    {
        return nullptr;
    }

protected:
    /*!
     * \brief Recomputes the cached envelope from the concrete geometry.
     */
    void setEnvelope(const Envelope &envelope) { m_envelope.setEnvelope(envelope); }

    std::string m_id;
    int64_t m_index = 0;
    HydroCouple::Spatial::ISpatialReferenceSystem *m_srs = nullptr;
    mutable EnvelopeAdapter m_envelope;
};

/*!
 * \brief IPoint over an SDK Point value.
 *
 * Not final: a network vertex is a point that also knows an edge, so
 * VertexAdapter builds on this rather than repeating it.
 */
class HYDROCOUPLESDK_EXPORT PointAdapter
    : public GeometryAdapter,
      public virtual HydroCouple::Spatial::IPoint
{
public:
    /*!
     * \brief Construct over a copy of \p point.
     */
    PointAdapter(const Point &point, std::string id = "point",
                 int64_t index = 0,
                 HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    ~PointAdapter() override = default;

    /*!
     * \brief The wrapped point value.
     */
    [[nodiscard]] const Point &point() const noexcept { return m_point; }

    [[nodiscard]] int dimension() const override { return 0; }

    [[nodiscard]] GeometryType geometryType() const override
    {
        return GeometryType::Point;
    }

    [[nodiscard]] std::vector<unsigned char> getWKB() const override;

    [[nodiscard]] bool isEmpty() const override { return false; }

    [[nodiscard]] double x() const override { return m_point.x; }
    [[nodiscard]] double y() const override { return m_point.y; }
    [[nodiscard]] double z() const override { return m_point.z; }
    [[nodiscard]] double m() const override { return m_point.m; }

private:
    Point m_point;
};

/*!
 * \brief ILineString over an SDK LineString value.
 */
class HYDROCOUPLESDK_EXPORT LineStringAdapter final
    : public GeometryAdapter,
      public virtual HydroCouple::Spatial::ILineString
{
public:
    /*!
     * \brief Construct over a copy of \p line.
     */
    LineStringAdapter(
        const LineString &line, std::string id = "linestring",
        int64_t index = 0,
        HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    ~LineStringAdapter() override = default;

    // Owns its point adapters, so it is not copyable. Declared explicitly because MSVC
    // instantiates every implicit member of an exported class, and the implicit copy of the
    // vector of unique_ptr below does not compile.
    LineStringAdapter(const LineStringAdapter &) = delete;
    LineStringAdapter &operator=(const LineStringAdapter &) = delete;

    /*!
     * \brief The wrapped line value.
     */
    [[nodiscard]] const LineString &lineString() const noexcept
    {
        return m_line;
    }

    [[nodiscard]] int dimension() const override { return 1; }

    [[nodiscard]] GeometryType geometryType() const override
    {
        return GeometryType::LineString;
    }

    [[nodiscard]] std::vector<unsigned char> getWKB() const override;

    [[nodiscard]] bool isEmpty() const override { return m_line.isEmpty(); }

    // ── ICurve / ILineString ──────────────────────────────────────────────

    [[nodiscard]] int64_t pointCount() const override
    {
        return m_line.pointCount();
    }

    [[nodiscard]] HydroCouple::Spatial::IPoint *point(int64_t index) const override;

    [[nodiscard]] double length() const override { return m_line.length(); }

    [[nodiscard]] HydroCouple::Spatial::IPoint *startPoint() const override;

    [[nodiscard]] HydroCouple::Spatial::IPoint *endPoint() const override;

    [[nodiscard]] bool isClosed() const override { return m_line.isClosed(); }

    [[nodiscard]] bool isRing() const override
    {
        return m_line.isClosed() && m_line.pointCount() > 3;
    }

private:
    void rebuildPoints();

    LineString m_line;
    std::vector<std::unique_ptr<PointAdapter>> m_points;
};

/*!
 * \brief IPolygon over an SDK Polygon value.
 */
class HYDROCOUPLESDK_EXPORT PolygonAdapter final
    : public GeometryAdapter,
      public virtual HydroCouple::Spatial::IPolygon
{
public:
    /*!
     * \brief Construct over a copy of \p polygon.
     */
    PolygonAdapter(
        const Polygon &polygon, std::string id = "polygon",
        int64_t index = 0,
        HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    ~PolygonAdapter() override = default;

    // Owns its ring adapters, so it is not copyable (see LineStringAdapter).
    PolygonAdapter(const PolygonAdapter &) = delete;
    PolygonAdapter &operator=(const PolygonAdapter &) = delete;

    /*!
     * \brief The wrapped polygon value.
     */
    [[nodiscard]] const Polygon &polygon() const noexcept { return m_polygon; }

    [[nodiscard]] int dimension() const override { return 2; }

    [[nodiscard]] GeometryType geometryType() const override
    {
        return GeometryType::Polygon;
    }

    [[nodiscard]] std::vector<unsigned char> getWKB() const override;

    [[nodiscard]] bool isEmpty() const override
    {
        return m_polygon.exteriorRing().isEmpty();
    }

    // ── ISurface / IPolygon ───────────────────────────────────────────────

    [[nodiscard]] HydroCouple::Spatial::ILineString *exteriorRing()
        const override;

    [[nodiscard]] int64_t interiorRingCount() const override
    {
        return m_polygon.numInteriorRings();
    }

    [[nodiscard]] HydroCouple::Spatial::ILineString *interiorRing(
        int64_t index) const override;

    [[nodiscard]] double area() const override { return m_polygon.area(); }

    //! Newly constructed, caller-owned (interface ownership convention).
    [[nodiscard]] std::unique_ptr<HydroCouple::Spatial::IPoint> centroid() const override;

    //! Newly constructed, caller-owned (interface ownership convention).
    [[nodiscard]] std::unique_ptr<HydroCouple::Spatial::IPoint> pointOnSurface() const override;

    [[nodiscard]] std::unique_ptr<HydroCouple::Spatial::IMultiCurve> boundaryMultiCurve()
        const override
    {
        return nullptr;
    }

    /*!
     * \brief An adjacent edge, when a surface set one.
     *
     * A polygon standing on its own has no adjacency; the surface that owns
     * it records one through setSurface().
     */
    [[nodiscard]] HydroCouple::Spatial::IEdge *edge() const override
    {
        return m_edge;
    }

    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    polyhedralSurface() const override
    {
        return m_surface;
    }

    /*!
     * \brief Records the surface this polygon is a patch of.
     */
    void setSurface(HydroCouple::Spatial::IPolyhedralSurface *surface,
                    HydroCouple::Spatial::IEdge *edge = nullptr) noexcept
    {
        m_surface = surface;
        m_edge = edge;
    }

private:
    HydroCouple::Spatial::IPolyhedralSurface *m_surface = nullptr;
    HydroCouple::Spatial::IEdge *m_edge = nullptr;
    Polygon m_polygon;
    std::unique_ptr<LineStringAdapter> m_exterior;
    std::vector<std::unique_ptr<LineStringAdapter>> m_interiors;
    std::unique_ptr<PointAdapter> m_centroid;
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_SPATIAL_GEOMETRYADAPTERS_H
