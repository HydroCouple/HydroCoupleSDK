// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   geometryadapters.cpp
 * \author Caleb Buahin
 * \brief  Implementation of the IGeometry adapters.
 */

#include "hydrocouplesdk/spatial/geometryadapters.h"

#include <cstdint>
#include <cstring>
#include <sstream>

namespace HydroCouple::SDK::Spatial {

namespace {

//! WKB byte-order marker for little-endian, which is what this writes.
constexpr unsigned char kLittleEndian = 1;

//! WKB geometry type codes (OGC simple features, 2-D).
constexpr uint32_t kWkbPoint = 1;
constexpr uint32_t kWkbLineString = 2;
constexpr uint32_t kWkbPolygon = 3;

void appendBytes(std::vector<unsigned char> &bytes, const void *data,
                 size_t size)
{
    const auto *first = static_cast<const unsigned char *>(data);
    bytes.insert(bytes.end(), first, first + size);
}

void appendUInt32(std::vector<unsigned char> &bytes, uint32_t value)
{
    appendBytes(bytes, &value, sizeof(value));
}

void appendDouble(std::vector<unsigned char> &bytes, double value)
{
    appendBytes(bytes, &value, sizeof(value));
}

void appendHeader(std::vector<unsigned char> &bytes, uint32_t type)
{
    bytes.push_back(kLittleEndian);
    appendUInt32(bytes, type);
}

//! Writes a ring or line body: a point count followed by the coordinates.
void appendLineBody(std::vector<unsigned char> &bytes, const LineString &line)
{
    appendUInt32(bytes, static_cast<uint32_t>(line.pointCount()));

    for (int i = 0; i < line.pointCount(); ++i)
    {
        const Point point = line.pointAt(i);
        appendDouble(bytes, point.x);
        appendDouble(bytes, point.y);
    }
}

void appendCoordinateText(std::ostringstream &text, const LineString &line)
{
    text << '(';

    for (int i = 0; i < line.pointCount(); ++i)
    {
        const Point point = line.pointAt(i);

        if (i > 0)
        {
            text << ", ";
        }

        text << point.x << ' ' << point.y;
    }

    text << ')';
}

} // namespace

// ── GeometryAdapter ───────────────────────────────────────────────────────

GeometryAdapter::GeometryAdapter(
    std::string id, unsigned int index,
    HydroCouple::Spatial::ISpatialReferenceSystem *srs)
    : m_id(std::move(id)), m_index(index), m_srs(srs)
{
}

HydroCouple::Spatial::IEnvelope *GeometryAdapter::envelope() const
{
    return &m_envelope;
}

std::string GeometryAdapter::getWKT() const
{
    std::ostringstream text;

    switch (geometryType())
    {
        case GeometryType::Point:
        {
            const auto *self = dynamic_cast<const PointAdapter *>(this);
            text << "POINT (" << self->x() << ' ' << self->y() << ')';
            break;
        }

        case GeometryType::LineString:
        {
            const auto *self = dynamic_cast<const LineStringAdapter *>(this);
            text << "LINESTRING ";
            appendCoordinateText(text, self->lineString());
            break;
        }

        case GeometryType::Polygon:
        {
            const auto *self = dynamic_cast<const PolygonAdapter *>(this);
            text << "POLYGON (";
            appendCoordinateText(text, self->polygon().exteriorRing());

            for (int i = 0; i < self->polygon().numInteriorRings(); ++i)
            {
                text << ", ";
                appendCoordinateText(text, self->polygon().interiorRing(i));
            }

            text << ')';
            break;
        }

        default:
            text << "GEOMETRYCOLLECTION EMPTY";
            break;
    }

    return text.str();
}

// ── PointAdapter ──────────────────────────────────────────────────────────

PointAdapter::PointAdapter(const Point &point, std::string id,
                           unsigned int index,
                           HydroCouple::Spatial::ISpatialReferenceSystem *srs)
    : GeometryAdapter(std::move(id), index, srs), m_point(point)
{
    Envelope envelope;
    envelope.minX = envelope.maxX = point.x;
    envelope.minY = envelope.maxY = point.y;
    envelope.minZ = envelope.maxZ = point.z;

    setEnvelope(envelope);
}

std::vector<unsigned char> PointAdapter::getWKB() const
{
    std::vector<unsigned char> bytes;
    bytes.reserve(21);

    appendHeader(bytes, kWkbPoint);
    appendDouble(bytes, m_point.x);
    appendDouble(bytes, m_point.y);

    return bytes;
}

// ── LineStringAdapter ─────────────────────────────────────────────────────

LineStringAdapter::LineStringAdapter(
    const LineString &line, std::string id, unsigned int index,
    HydroCouple::Spatial::ISpatialReferenceSystem *srs)
    : GeometryAdapter(std::move(id), index, srs), m_line(line)
{
    setEnvelope(m_line.envelope());
    rebuildPoints();
}

void LineStringAdapter::rebuildPoints()
{
    // Materialised once, because the interface hands out IPoint pointers and
    // a temporary built per call would dangle the moment it returned.
    m_points.clear();
    m_points.reserve(static_cast<size_t>(m_line.pointCount()));

    for (int i = 0; i < m_line.pointCount(); ++i)
    {
        m_points.push_back(std::make_unique<PointAdapter>(
            m_line.pointAt(i), m_id + ":" + std::to_string(i),
            static_cast<unsigned int>(i), m_srs));
    }
}

std::vector<unsigned char> LineStringAdapter::getWKB() const
{
    std::vector<unsigned char> bytes;
    bytes.reserve(9 + static_cast<size_t>(m_line.pointCount()) * 16);

    appendHeader(bytes, kWkbLineString);
    appendLineBody(bytes, m_line);

    return bytes;
}

HydroCouple::Spatial::IPoint *LineStringAdapter::point(int index) const
{
    return index >= 0 && index < static_cast<int>(m_points.size())
               ? m_points[static_cast<size_t>(index)].get()
               : nullptr;
}

HydroCouple::Spatial::IPoint *LineStringAdapter::startPoint() const
{
    return m_points.empty() ? nullptr : m_points.front().get();
}

HydroCouple::Spatial::IPoint *LineStringAdapter::endPoint() const
{
    return m_points.empty() ? nullptr : m_points.back().get();
}

// ── PolygonAdapter ────────────────────────────────────────────────────────

PolygonAdapter::PolygonAdapter(
    const Polygon &polygon, std::string id, unsigned int index,
    HydroCouple::Spatial::ISpatialReferenceSystem *srs)
    : GeometryAdapter(std::move(id), index, srs), m_polygon(polygon)
{
    setEnvelope(m_polygon.envelope());

    m_exterior = std::make_unique<LineStringAdapter>(
        m_polygon.exteriorRing(), m_id + ":exterior", 0, m_srs);

    m_interiors.reserve(
        static_cast<size_t>(m_polygon.numInteriorRings()));

    for (int i = 0; i < m_polygon.numInteriorRings(); ++i)
    {
        m_interiors.push_back(std::make_unique<LineStringAdapter>(
            m_polygon.interiorRing(i),
            m_id + ":interior" + std::to_string(i),
            static_cast<unsigned int>(i), m_srs));
    }

    m_centroid = std::make_unique<PointAdapter>(m_polygon.centroid(),
                                                m_id + ":centroid", 0, m_srs);
}

std::vector<unsigned char> PolygonAdapter::getWKB() const
{
    std::vector<unsigned char> bytes;

    appendHeader(bytes, kWkbPolygon);
    appendUInt32(bytes,
                 static_cast<uint32_t>(1 + m_polygon.numInteriorRings()));

    // Exterior ring first, then the holes, as WKB requires.
    appendLineBody(bytes, m_polygon.exteriorRing());

    for (int i = 0; i < m_polygon.numInteriorRings(); ++i)
    {
        appendLineBody(bytes, m_polygon.interiorRing(i));
    }

    return bytes;
}

HydroCouple::Spatial::ILineString *PolygonAdapter::exteriorRing() const
{
    return m_exterior.get();
}

HydroCouple::Spatial::ILineString *PolygonAdapter::interiorRing(int index) const
{
    return index >= 0 && index < static_cast<int>(m_interiors.size())
               ? m_interiors[static_cast<size_t>(index)].get()
               : nullptr;
}

HydroCouple::Spatial::IPoint *PolygonAdapter::centroid() const
{
    return m_centroid.get();
}

HydroCouple::Spatial::IPoint *PolygonAdapter::pointOnSurface() const
{
    // The centroid of a convex polygon lies inside it; for a concave one it
    // may not, and computing a guaranteed interior point needs the geometry
    // engine the SDK does not link. Documented rather than approximated.
    return m_centroid.get();
}

} // namespace HydroCouple::SDK::Spatial
