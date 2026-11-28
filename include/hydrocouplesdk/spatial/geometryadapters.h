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
 * The adapters are two-dimensional on the wire: getWKB() and getWKT() write
 * x and y only, coordinateDimension() is 2 and is3D() is false, even though
 * IPoint::z() and the envelope report the z each point holds.
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
 *
 * What it answers: the identity (id(), index()), the borrowed spatial
 * reference, the cached envelope, WKT text for points, lines and polygons,
 * and fixed values for the properties the SDK does not compute (isSimple()
 * true, is3D() and isMeasured() false). Every relational predicate returns
 * false, distance() returns 0, and every constructive operation returns
 * nullptr, as the file comment explains. The reference system is borrowed:
 * it must outlive the adapter, and nothing copies it.
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

    //! Destroys the adapter; the reference system is not owned.
    ~GeometryAdapter() override = default;

    /*!
     * \brief Sets the spatial reference reported by this geometry.
     *
     * Changes this adapter only. The point adapters of a line and the ring
     * adapters of a polygon were given the reference at construction and
     * keep it, so construct with the reference rather than setting it later
     * when the parts must agree.
     *
     * \param[in] srs The reference system (borrowed; may be nullptr).
     */
    void setSpatialReferenceSystem(
        HydroCouple::Spatial::ISpatialReferenceSystem *srs) noexcept
    {
        m_srs = srs;
    }

    // ── IGeometry: identity and description ───────────────────────────────

    /*!
     * \brief The identifier given at construction (IGeometry::id()).
     * \returns The id; parts made by a composite adapter carry derived ids
     *          such as "line:0" or "poly:exterior".
     */
    [[nodiscard]] const std::string &id() const override { return m_id; }
    /*!
     * \brief The position in the owning collection (IGeometry::index()).
     * \returns The index given at construction.
     */
    [[nodiscard]] int64_t index() const override { return m_index; }
    /*!
     * \brief The coordinate dimension (IGeometry::coordinateDimension()).
     * \returns Always 2, because the adapters serialize x and y only.
     */
    [[nodiscard]] int coordinateDimension() const override { return 2; }

    /*!
     * \brief The spatial reference system (IGeometry::spatialReferenceSystem()).
     * \returns The borrowed reference given at construction or by
     *          setSpatialReferenceSystem(); nullptr when none was given.
     */
    [[nodiscard]] HydroCouple::Spatial::ISpatialReferenceSystem *
    spatialReferenceSystem() const override
    {
        return m_srs;
    }

    /*!
     * \brief The bounding box (IGeometry::envelope()).
     * \returns A non-owning pointer to the adapter's own EnvelopeAdapter,
     *          never nullptr and valid for the life of the adapter. Its
     *          bounds are those of the wrapped value at construction, z
     *          included.
     */
    [[nodiscard]] HydroCouple::Spatial::IEnvelope *envelope() const override;

    /*!
     * \brief Well-known text of the geometry (IGeometry::getWKT()).
     *
     * Writes "POINT (x y)", "LINESTRING (x y, ...)" or
     * "POLYGON ((exterior), (hole), ...)" according to geometryType(), and
     * "GEOMETRYCOLLECTION EMPTY" for any other type (a
     * PolyhedralSurfaceAdapter among them). Coordinates are written with the
     * default stream precision of six significant digits, so large projected
     * coordinates lose digits (4512345.67 becomes 4.51235e+06); use getWKB()
     * where precision matters.
     *
     * \returns The text described above.
     */
    [[nodiscard]] std::string getWKT() const override;

    /*!
     * \brief Whether the geometry is simple (IGeometry::isSimple()).
     * \returns Always true; self-intersection is not checked.
     */
    [[nodiscard]] bool isSimple() const override { return true; }
    /*!
     * \brief Whether the geometry has z (IGeometry::is3D()).
     * \returns Always false, matching the two-dimensional WKB and WKT.
     */
    [[nodiscard]] bool is3D() const override { return false; }
    /*!
     * \brief Whether the geometry has measures (IGeometry::isMeasured()).
     * \returns Always false.
     */
    [[nodiscard]] bool isMeasured() const override { return false; }

    // ── IGeometry: relational predicates ──────────────────────────────────
    //
    // Not computed: the SDK links no geometry engine. Reported honestly as
    // "no" rather than guessed, and the WKB above is what a caller with GEOS
    // or GDAL uses instead.

    /*!
     * \brief The boundary (IGeometry::boundary()); not computed.
     * \returns nullptr.
     */
    [[nodiscard]] std::unique_ptr<IGeometry> boundary() const override { return nullptr; }
    /*!
     * \brief Spatial equality (IGeometry::equals()); not computed.
     * \returns false, whatever the argument.
     */
    [[nodiscard]] bool equals(const IGeometry &) const override
    {
        return false;
    }
    /*!
     * \brief Disjointness (IGeometry::disjoint()); not computed.
     * \returns false, whatever the argument.
     */
    [[nodiscard]] bool disjoint(const IGeometry &) const override
    {
        return false;
    }
    /*!
     * \brief Intersection test (IGeometry::intersects()); not computed.
     * \returns false, whatever the argument.
     */
    [[nodiscard]] bool intersects(const IGeometry &) const override
    {
        return false;
    }
    /*!
     * \brief Touch test (IGeometry::touches()); not computed.
     * \returns false, whatever the argument.
     */
    [[nodiscard]] bool touches(const IGeometry &) const override
    {
        return false;
    }
    /*!
     * \brief Crossing test (IGeometry::crosses()); not computed.
     * \returns false, whatever the argument.
     */
    [[nodiscard]] bool crosses(const IGeometry &) const override
    {
        return false;
    }
    /*!
     * \brief Containment by another geometry (IGeometry::within()); not
     *        computed.
     * \returns false, whatever the argument.
     */
    [[nodiscard]] bool within(const IGeometry &) const override
    {
        return false;
    }
    /*!
     * \brief Containment of another geometry (IGeometry::contains()); not
     *        computed.
     * \returns false, whatever the argument.
     */
    [[nodiscard]] bool contains(const IGeometry &) const override
    {
        return false;
    }
    /*!
     * \brief Overlap test (IGeometry::overlaps()); not computed.
     * \returns false, whatever the argument.
     */
    [[nodiscard]] bool overlaps(const IGeometry &) const override
    {
        return false;
    }
    /*!
     * \brief DE-9IM relation test (IGeometry::relate()); not computed.
     * \returns false, whatever the arguments.
     */
    [[nodiscard]] bool relate(const IGeometry &, const std::string &) const override
    {
        return false;
    }
    /*!
     * \brief Measure-based location (IGeometry::locateAlong()); not computed.
     * \returns nullptr.
     */
    [[nodiscard]] std::unique_ptr<IGeometry> locateAlong(double) const override
    {
        return nullptr;
    }
    /*!
     * \brief Measure-range location (IGeometry::locateBetween()); not
     *        computed.
     * \returns nullptr.
     */
    [[nodiscard]] std::unique_ptr<IGeometry> locateBetween(double, double) const override
    {
        return nullptr;
    }
    /*!
     * \brief Shortest distance (IGeometry::distance()); not computed.
     * \returns 0, whatever the argument; do not read it as "touching".
     */
    [[nodiscard]] double distance(const IGeometry &) const override
    {
        return 0.0;
    }
    /*!
     * \brief Buffer (IGeometry::buffer()); not computed.
     * \returns nullptr.
     */
    [[nodiscard]] std::unique_ptr<IGeometry> buffer(double) const override { return nullptr; }
    /*!
     * \brief Convex hull (IGeometry::convexHull()); not computed.
     * \returns nullptr.
     */
    [[nodiscard]] std::unique_ptr<IGeometry> convexHull() const override { return nullptr; }
    /*!
     * \brief Intersection (IGeometry::intersection()); not computed.
     * \returns nullptr.
     */
    [[nodiscard]] std::unique_ptr<IGeometry> intersection(const IGeometry &) const override
    {
        return nullptr;
    }
    /*!
     * \brief Union (IGeometry::unionG()); not computed.
     * \returns nullptr.
     */
    [[nodiscard]] std::unique_ptr<IGeometry> unionG(const IGeometry &) const override
    {
        return nullptr;
    }
    /*!
     * \brief Difference (IGeometry::difference()); not computed.
     * \returns nullptr.
     */
    [[nodiscard]] std::unique_ptr<IGeometry> difference(const IGeometry &) const override
    {
        return nullptr;
    }
    /*!
     * \brief Symmetric difference (IGeometry::symmetricDifference()); not
     *        computed.
     * \returns nullptr.
     */
    [[nodiscard]] std::unique_ptr<IGeometry> symmetricDifference(
        const IGeometry &) const override
    {
        return nullptr;
    }

protected:
    /*!
     * \brief Recomputes the cached envelope from the concrete geometry.
     *
     * Called by each subclass constructor with its value's envelope.
     *
     * \param[in] envelope The bounds envelope() reports from now on; copied.
     */
    void setEnvelope(const Envelope &envelope) { m_envelope.setEnvelope(envelope); }

    std::string m_id;                                         //!< The identifier id() returns.
    int64_t m_index = 0;                                      //!< The index index() returns.
    HydroCouple::Spatial::ISpatialReferenceSystem *m_srs = nullptr; //!< Borrowed reference system; may be nullptr.
    mutable EnvelopeAdapter m_envelope;                       //!< The envelope envelope() points to.
};

/*!
 * \brief IPoint over an SDK Point value.
 *
 * Not final: a network vertex is a point that also knows an edge, so
 * VertexAdapter builds on this rather than repeating it.
 *
 * The adapter holds a copy of the point; its envelope is the point itself, a
 * box of zero size (not an empty one).
 */
class HYDROCOUPLESDK_EXPORT PointAdapter
    : public GeometryAdapter,
      public virtual HydroCouple::Spatial::IPoint
{
public:
    /*!
     * \brief Construct over a copy of \p point.
     * \param[in] point The point; copied, z and m included.
     * \param[in] id    Identifier reported by id(); "point" by default.
     * \param[in] index Position within the owning collection.
     * \param[in] srs   Spatial reference (borrowed; may be nullptr).
     */
    PointAdapter(const Point &point, std::string id = "point",
                 int64_t index = 0,
                 HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    //! Destroys the adapter.
    ~PointAdapter() override = default;

    /*!
     * \brief The wrapped point value.
     * \returns The stored copy; valid for the life of the adapter.
     */
    [[nodiscard]] const Point &point() const noexcept { return m_point; }

    /*!
     * \brief The topological dimension (IGeometry::dimension()).
     * \returns 0.
     */
    [[nodiscard]] int dimension() const override { return 0; }

    /*!
     * \brief The geometry type (IGeometry::geometryType()).
     * \returns GeometryType::Point, also for a point with z.
     */
    [[nodiscard]] GeometryType geometryType() const override
    {
        return GeometryType::Point;
    }

    /*!
     * \brief Well-known binary of the point (IGeometry::getWKB()).
     * \returns 21 bytes: the little-endian marker 1, the type code 1 as a
     *          little-endian uint32, then x and y as doubles in the host's
     *          byte order (little-endian on every supported platform).
     */
    [[nodiscard]] std::vector<unsigned char> getWKB() const override;

    /*!
     * \brief Whether the point is empty (IGeometry::isEmpty()).
     * \returns Always false; a PointAdapter always holds a point.
     */
    [[nodiscard]] bool isEmpty() const override { return false; }

    /*!
     * \brief The x coordinate (IPoint::x()).
     * \returns Point::x of the stored point.
     */
    [[nodiscard]] double x() const override { return m_point.x; }
    /*!
     * \brief The y coordinate (IPoint::y()).
     * \returns Point::y of the stored point.
     */
    [[nodiscard]] double y() const override { return m_point.y; }
    /*!
     * \brief The z coordinate (IPoint::z()).
     * \returns Point::z of the stored point; 0 for a two-dimensional point.
     */
    [[nodiscard]] double z() const override { return m_point.z; }
    /*!
     * \brief The measure (IPoint::m()).
     * \returns Point::m of the stored point; 0 when none was set.
     */
    [[nodiscard]] double m() const override { return m_point.m; }

private:
    Point m_point; //!< The wrapped point, owned.
};

/*!
 * \brief ILineString over an SDK LineString value.
 *
 * Holds a copy of the line and one PointAdapter per vertex, built at
 * construction (ids "ID:I" for the line id ID and point index I, indexes I, the same reference system), so the
 * IPoint pointers point() returns stay valid for the life of the adapter.
 * Polygon rings are served through this class too.
 */
class HYDROCOUPLESDK_EXPORT LineStringAdapter final
    : public GeometryAdapter,
      public virtual HydroCouple::Spatial::ILineString
{
public:
    /*!
     * \brief Construct over a copy of \p line.
     * \param[in] line  The line; copied.
     * \param[in] id    Identifier reported by id(); "linestring" by default.
     * \param[in] index Position within the owning collection.
     * \param[in] srs   Spatial reference (borrowed; may be nullptr), also
     *                  given to every point adapter.
     */
    LineStringAdapter(
        const LineString &line, std::string id = "linestring",
        int64_t index = 0,
        HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    //! Destroys the adapter and its point adapters.
    ~LineStringAdapter() override = default;

    // Owns its point adapters, so it is not copyable. Declared explicitly because MSVC
    // instantiates every implicit member of an exported class, and the implicit copy of the
    // vector of unique_ptr below does not compile.
    //! Not copyable: the adapter owns its point adapters.
    LineStringAdapter(const LineStringAdapter &) = delete;
    /*!
     * \brief Not assignable: the adapter owns its point adapters.
     * \returns Not callable.
     */
    LineStringAdapter &operator=(const LineStringAdapter &) = delete;

    /*!
     * \brief The wrapped line value.
     * \returns The stored copy; valid for the life of the adapter.
     */
    [[nodiscard]] const LineString &lineString() const noexcept
    {
        return m_line;
    }

    /*!
     * \brief The topological dimension (IGeometry::dimension()).
     * \returns 1.
     */
    [[nodiscard]] int dimension() const override { return 1; }

    /*!
     * \brief The geometry type (IGeometry::geometryType()).
     * \returns GeometryType::LineString.
     */
    [[nodiscard]] GeometryType geometryType() const override
    {
        return GeometryType::LineString;
    }

    /*!
     * \brief Well-known binary of the line (IGeometry::getWKB()).
     * \returns 9 + 16·pointCount() bytes: the little-endian marker 1, the
     *          type code 2, the point count as uint32, then x and y of each
     *          point.
     */
    [[nodiscard]] std::vector<unsigned char> getWKB() const override;

    /*!
     * \brief Whether the line has no points (IGeometry::isEmpty()).
     * \returns LineString::isEmpty() of the stored line.
     */
    [[nodiscard]] bool isEmpty() const override { return m_line.isEmpty(); }

    // ── ICurve / ILineString ──────────────────────────────────────────────

    /*!
     * \brief The number of points (ILineString::pointCount()).
     * \returns LineString::pointCount() of the stored line.
     */
    [[nodiscard]] int64_t pointCount() const override
    {
        return m_line.pointCount();
    }

    /*!
     * \brief The point at an index (ILineString::point()).
     * \param[in] index Zero-based point index.
     * \returns A non-owning pointer to the vertex's PointAdapter, valid for
     *          the life of this adapter; nullptr when \p index is out of
     *          range.
     */
    [[nodiscard]] HydroCouple::Spatial::IPoint *point(int64_t index) const override;

    /*!
     * \brief The length (ICurve::length()).
     * \returns LineString::length(): the three-dimensional length, in
     *          coordinate units.
     */
    [[nodiscard]] double length() const override { return m_line.length(); }

    /*!
     * \brief The first point (ICurve::startPoint()).
     * \returns point(0), or nullptr for an empty line.
     */
    [[nodiscard]] HydroCouple::Spatial::IPoint *startPoint() const override;

    /*!
     * \brief The last point (ICurve::endPoint()).
     * \returns point(pointCount() - 1), or nullptr for an empty line.
     */
    [[nodiscard]] HydroCouple::Spatial::IPoint *endPoint() const override;

    /*!
     * \brief Whether the line ends where it starts (ICurve::isClosed()).
     * \returns LineString::isClosed() of the stored line.
     */
    [[nodiscard]] bool isClosed() const override { return m_line.isClosed(); }

    /*!
     * \brief Whether the line is a ring (ICurve::isRing()).
     *
     * Simplicity is not checked (see isSimple()), so a closed line that
     * crosses itself still reports true.
     *
     * \returns True when the line is closed and has more than three points.
     */
    [[nodiscard]] bool isRing() const override
    {
        return m_line.isClosed() && m_line.pointCount() > 3;
    }

private:
    /*!
     * \brief Builds one PointAdapter per vertex of m_line, replacing any
     *        built before.
     */
    void rebuildPoints();

    LineString m_line;                                 //!< The wrapped line, owned.
    std::vector<std::unique_ptr<PointAdapter>> m_points; //!< One adapter per vertex, owned.
};

/*!
 * \brief IPolygon over an SDK Polygon value.
 *
 * Holds a copy of the polygon and builds, at construction, a LineStringAdapter
 * for the exterior ring (id "ID:exterior", ID being this adapter's id) and one per hole
 * ("ID:interiorI" for hole I), all with the same reference system. A polygon on its
 * own belongs to no mesh: edge() and polyhedralSurface() return nullptr
 * until a surface calls setSurface(), as PolyhedralSurfaceAdapter does for
 * its patches.
 */
class HYDROCOUPLESDK_EXPORT PolygonAdapter final
    : public GeometryAdapter,
      public virtual HydroCouple::Spatial::IPolygon
{
public:
    /*!
     * \brief Construct over a copy of \p polygon.
     * \param[in] polygon The polygon; copied with its holes.
     * \param[in] id      Identifier reported by id(); "polygon" by default.
     * \param[in] index   Position within the owning collection.
     * \param[in] srs     Spatial reference (borrowed; may be nullptr), also
     *                    given to the ring adapters.
     */
    PolygonAdapter(
        const Polygon &polygon, std::string id = "polygon",
        int64_t index = 0,
        HydroCouple::Spatial::ISpatialReferenceSystem *srs = nullptr);

    //! Destroys the adapter and its ring adapters.
    ~PolygonAdapter() override = default;

    // Owns its ring adapters, so it is not copyable (see LineStringAdapter).
    //! Not copyable: the adapter owns its ring adapters.
    PolygonAdapter(const PolygonAdapter &) = delete;
    /*!
     * \brief Not assignable: the adapter owns its ring adapters.
     * \returns Not callable.
     */
    PolygonAdapter &operator=(const PolygonAdapter &) = delete;

    /*!
     * \brief The wrapped polygon value.
     * \returns The stored copy; valid for the life of the adapter.
     */
    [[nodiscard]] const Polygon &polygon() const noexcept { return m_polygon; }

    /*!
     * \brief The topological dimension (IGeometry::dimension()).
     * \returns 2.
     */
    [[nodiscard]] int dimension() const override { return 2; }

    /*!
     * \brief The geometry type (IGeometry::geometryType()).
     * \returns GeometryType::Polygon.
     */
    [[nodiscard]] GeometryType geometryType() const override
    {
        return GeometryType::Polygon;
    }

    /*!
     * \brief Well-known binary of the polygon (IGeometry::getWKB()).
     * \returns The little-endian marker 1, the type code 3, the ring count
     *          (1 + holes) as uint32, then each ring as a point count and x, y
     *          pairs, exterior ring first.
     */
    [[nodiscard]] std::vector<unsigned char> getWKB() const override;

    /*!
     * \brief Whether the polygon is empty (IGeometry::isEmpty()).
     * \returns True when the exterior ring has no points.
     */
    [[nodiscard]] bool isEmpty() const override
    {
        return m_polygon.exteriorRing().isEmpty();
    }

    // ── ISurface / IPolygon ───────────────────────────────────────────────

    /*!
     * \brief The exterior ring (IPolygon::exteriorRing()).
     * \returns A non-owning pointer to the ring adapter, valid for the life
     *          of this adapter; never nullptr (an empty polygon has an empty
     *          ring).
     */
    [[nodiscard]] HydroCouple::Spatial::ILineString *exteriorRing()
        const override;

    /*!
     * \brief The number of holes (IPolygon::interiorRingCount()).
     * \returns Polygon::numInteriorRings() of the stored polygon.
     */
    [[nodiscard]] int64_t interiorRingCount() const override
    {
        return m_polygon.numInteriorRings();
    }

    /*!
     * \brief A hole's ring (IPolygon::interiorRing()).
     * \param[in] index Zero-based hole index.
     * \returns A non-owning pointer to the hole's ring adapter, valid for the
     *          life of this adapter; nullptr when \p index is out of range.
     */
    [[nodiscard]] HydroCouple::Spatial::ILineString *interiorRing(
        int64_t index) const override;

    /*!
     * \brief The plan area (ISurface::area()).
     * \returns Polygon::area() of the stored polygon, with that function's
     *          hole-orientation behavior.
     */
    [[nodiscard]] double area() const override { return m_polygon.area(); }

    //! Newly constructed, caller-owned (interface ownership convention).
    /*!
     * \returns A new PointAdapter (id "ID:centroid") at Polygon::centroid()
     *          of the stored polygon, which is currently half the true
     *          centroid (see Polygon::centroid()); never nullptr.
     */
    [[nodiscard]] std::unique_ptr<HydroCouple::Spatial::IPoint> centroid() const override;

    //! Newly constructed, caller-owned (interface ownership convention).
    /*!
     * \returns centroid(). It is not guaranteed to lie on the surface: that
     *          needs the geometry engine the SDK does not link.
     */
    [[nodiscard]] std::unique_ptr<HydroCouple::Spatial::IPoint> pointOnSurface() const override;

    /*!
     * \brief The boundary as a multi-curve (ISurface::boundaryMultiCurve());
     *        not computed.
     * \returns nullptr; read the rings with exteriorRing() and interiorRing().
     */
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
     *
     * \returns The borrowed edge given to setSurface(); nullptr otherwise.
     *          PolyhedralSurfaceAdapter does not pass one, so its patches
     *          report nullptr.
     */
    [[nodiscard]] HydroCouple::Spatial::IEdge *edge() const override
    {
        return m_edge;
    }

    /*!
     * \brief The surface this polygon is a patch of
     *        (IPolygon::polyhedralSurface()).
     * \returns The borrowed surface given to setSurface(); nullptr for a
     *          free-standing polygon.
     */
    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    polyhedralSurface() const override
    {
        return m_surface;
    }

    /*!
     * \brief Records the surface this polygon is a patch of.
     * \param[in] surface The owning surface (borrowed; may be nullptr to
     *                    detach).
     * \param[in] edge    An edge adjacent to the patch (borrowed; may be
     *                    nullptr).
     */
    void setSurface(HydroCouple::Spatial::IPolyhedralSurface *surface,
                    HydroCouple::Spatial::IEdge *edge = nullptr) noexcept
    {
        m_surface = surface;
        m_edge = edge;
    }

private:
    HydroCouple::Spatial::IPolyhedralSurface *m_surface = nullptr; //!< Borrowed owning surface; may be nullptr.
    HydroCouple::Spatial::IEdge *m_edge = nullptr;                 //!< Borrowed adjacent edge; may be nullptr.
    Polygon m_polygon;                                             //!< The wrapped polygon, owned.
    std::unique_ptr<LineStringAdapter> m_exterior;                 //!< Adapter over the exterior ring.
    std::vector<std::unique_ptr<LineStringAdapter>> m_interiors;   //!< One adapter per hole.
    std::unique_ptr<PointAdapter> m_centroid;                      //!< Built at construction; centroid() does not use it.
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_SPATIAL_GEOMETRYADAPTERS_H
