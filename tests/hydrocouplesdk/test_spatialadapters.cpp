// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the spatial adapters: the SDK's value types served through the
// standard's IGeometry family. WKB is checked against what a geometry reader
// must find in it — byte order, type code, counts, coordinates — because a
// header with the wrong endianness parses as a different geometry entirely
// and no round-trip through our own code would notice.

#include <gtest/gtest.h>

#include "hydrocouplesdk/spatial/geometryadapters.h"
#include "hydrocouplesdk/spatial/meshadapters.h"

#include <cstdint>
#include <cstring>

using namespace HydroCouple::SDK::Spatial;
namespace HCS = HydroCouple::Spatial;

namespace {

//! Reads a little-endian uint32 out of a WKB buffer.
uint32_t readUInt32(const std::vector<unsigned char> &bytes, size_t offset)
{
    uint32_t value = 0;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));

    return value;
}

//! Reads a little-endian double out of a WKB buffer.
double readDouble(const std::vector<unsigned char> &bytes, size_t offset)
{
    double value = 0.0;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));

    return value;
}

LineString makeRing()
{
    LineString ring;
    ring.addPoint(Point(0.0, 0.0));
    ring.addPoint(Point(4.0, 0.0));
    ring.addPoint(Point(4.0, 3.0));
    ring.addPoint(Point(0.0, 3.0));
    ring.addPoint(Point(0.0, 0.0));

    return ring;
}

} // namespace

TEST(SpatialAdaptersTest, PointServesItsCoordinatesAndEnvelope)
{
    const PointAdapter adapter(Point(3.0, -7.0, 2.0, 9.0), "p", 4);

    EXPECT_EQ(adapter.geometryType(), HCS::IGeometry::GeometryType::Point);
    EXPECT_EQ(adapter.dimension(), 0);
    EXPECT_EQ(adapter.id(), "p");
    EXPECT_EQ(adapter.index(), 4u);

    EXPECT_DOUBLE_EQ(adapter.x(), 3.0);
    EXPECT_DOUBLE_EQ(adapter.y(), -7.0);
    EXPECT_DOUBLE_EQ(adapter.z(), 2.0);
    EXPECT_DOUBLE_EQ(adapter.m(), 9.0);

    // A point's envelope is the point, which is a zero-area rectangle — not
    // an empty one.
    ASSERT_NE(adapter.envelope(), nullptr);
    EXPECT_DOUBLE_EQ(adapter.envelope()->minX(), 3.0);
    EXPECT_DOUBLE_EQ(adapter.envelope()->maxX(), 3.0);
    EXPECT_DOUBLE_EQ(adapter.envelope()->minY(), -7.0);
}

TEST(SpatialAdaptersTest, PointWkbIsWhatAReaderExpects)
{
    const PointAdapter adapter(Point(1.5, -2.5));
    const std::vector<unsigned char> wkb = adapter.getWKB();

    // 1 byte order + 4 type + 2 doubles.
    ASSERT_EQ(wkb.size(), 21u);

    EXPECT_EQ(wkb[0], 1) << "byte order is not little-endian";
    EXPECT_EQ(readUInt32(wkb, 1), 1u) << "type code is not wkbPoint";
    EXPECT_DOUBLE_EQ(readDouble(wkb, 5), 1.5);
    EXPECT_DOUBLE_EQ(readDouble(wkb, 13), -2.5);
}

TEST(SpatialAdaptersTest, LineStringServesItsPointsAsInterfaces)
{
    LineString line;
    line.addPoint(Point(0.0, 0.0));
    line.addPoint(Point(3.0, 4.0));

    const LineStringAdapter adapter(line, "l", 0);

    EXPECT_EQ(adapter.geometryType(),
              HCS::IGeometry::GeometryType::LineString);
    EXPECT_EQ(adapter.pointCount(), 2);
    EXPECT_DOUBLE_EQ(adapter.length(), 5.0);
    EXPECT_FALSE(adapter.isClosed());

    // The interface hands out pointers, so the adapter must own the points it
    // returns — a temporary built per call would dangle immediately.
    ASSERT_NE(adapter.point(1), nullptr);
    EXPECT_DOUBLE_EQ(adapter.point(1)->x(), 3.0);
    EXPECT_EQ(adapter.startPoint(), adapter.point(0));
    EXPECT_EQ(adapter.endPoint(), adapter.point(1));

    EXPECT_EQ(adapter.point(-1), nullptr);
    EXPECT_EQ(adapter.point(2), nullptr);
}

TEST(SpatialAdaptersTest, LineStringWkbCarriesItsPointCount)
{
    LineString line;
    line.addPoint(Point(0.0, 1.0));
    line.addPoint(Point(2.0, 3.0));
    line.addPoint(Point(4.0, 5.0));

    const LineStringAdapter adapter(line);
    const std::vector<unsigned char> wkb = adapter.getWKB();

    ASSERT_EQ(wkb.size(), 9u + 3u * 16u);

    EXPECT_EQ(wkb[0], 1);
    EXPECT_EQ(readUInt32(wkb, 1), 2u) << "type code is not wkbLineString";
    EXPECT_EQ(readUInt32(wkb, 5), 3u) << "the point count is wrong";
    EXPECT_DOUBLE_EQ(readDouble(wkb, 9), 0.0);
    EXPECT_DOUBLE_EQ(readDouble(wkb, 17), 1.0);
    EXPECT_DOUBLE_EQ(readDouble(wkb, 9 + 32), 4.0);
}

TEST(SpatialAdaptersTest, PolygonServesRingsAndArea)
{
    Polygon polygon{makeRing()};

    LineString hole;
    hole.addPoint(Point(1.0, 1.0));
    hole.addPoint(Point(2.0, 1.0));
    hole.addPoint(Point(2.0, 2.0));
    hole.addPoint(Point(1.0, 1.0));
    polygon.addInteriorRing(hole);

    const PolygonAdapter adapter(polygon, "poly", 2);

    EXPECT_EQ(adapter.geometryType(), HCS::IGeometry::GeometryType::Polygon);
    EXPECT_EQ(adapter.dimension(), 2);

    ASSERT_NE(adapter.exteriorRing(), nullptr);
    EXPECT_EQ(adapter.exteriorRing()->pointCount(), 5);
    EXPECT_TRUE(adapter.exteriorRing()->isClosed());

    ASSERT_EQ(adapter.interiorRingCount(), 1);
    ASSERT_NE(adapter.interiorRing(0), nullptr);
    EXPECT_EQ(adapter.interiorRing(0)->pointCount(), 4);
    EXPECT_EQ(adapter.interiorRing(1), nullptr);

    EXPECT_GT(adapter.area(), 0.0);
    ASSERT_NE(adapter.centroid(), nullptr);
}

TEST(SpatialAdaptersTest, PolygonWkbListsTheExteriorRingFirst)
{
    Polygon polygon{makeRing()};

    LineString hole;
    hole.addPoint(Point(1.0, 1.0));
    hole.addPoint(Point(2.0, 1.0));
    hole.addPoint(Point(2.0, 2.0));
    hole.addPoint(Point(1.0, 1.0));
    polygon.addInteriorRing(hole);

    const PolygonAdapter adapter(polygon);
    const std::vector<unsigned char> wkb = adapter.getWKB();

    EXPECT_EQ(wkb[0], 1);
    EXPECT_EQ(readUInt32(wkb, 1), 3u) << "type code is not wkbPolygon";
    EXPECT_EQ(readUInt32(wkb, 5), 2u) << "the ring count is wrong";

    // WKB requires the exterior ring first; a reader takes ring 0 as the
    // outline and the rest as holes, so the order is not cosmetic.
    EXPECT_EQ(readUInt32(wkb, 9), 5u) << "ring 0 is not the exterior";
    EXPECT_DOUBLE_EQ(readDouble(wkb, 13), 0.0);
    EXPECT_DOUBLE_EQ(readDouble(wkb, 21), 0.0);
}

TEST(SpatialAdaptersTest, WktNamesTheGeometryType)
{
    const PointAdapter point(Point(1.0, 2.0));
    EXPECT_EQ(point.getWKT().rfind("POINT", 0), 0u);

    LineString line;
    line.addPoint(Point(0.0, 0.0));
    line.addPoint(Point(1.0, 1.0));
    EXPECT_EQ(LineStringAdapter(line).getWKT().rfind("LINESTRING", 0), 0u);

    EXPECT_EQ(PolygonAdapter(Polygon{makeRing()}).getWKT().rfind("POLYGON", 0),
              0u);
}

TEST(SpatialAdaptersTest, MeshViewServesTheDefinitionsOwnArrays)
{
    HydroCouple::SDK::IO::MeshDefinition definition;
    definition.meshName = "m";
    definition.nodeX = {0.0, 1.0, 1.0, 0.0};
    definition.nodeY = {0.0, 0.0, 1.0, 1.0};
    definition.faceNodeOffsets = {0, 3, 6};
    definition.faceNodes = {0, 1, 2, 0, 2, 3};
    definition.edgeNodes = {{0, 1}, {1, 2}};

    const MeshViewAdapter view(definition);

    EXPECT_EQ(view.nodeCount(), 4);
    EXPECT_EQ(view.faceCount(), 2);
    EXPECT_EQ(view.edgeCount(), 2);

    ASSERT_EQ(view.nodeX().size(), 4u);
    EXPECT_DOUBLE_EQ(view.nodeX()[2], 1.0);
    ASSERT_EQ(view.faceNodes().size(), 6u);
    EXPECT_EQ(view.faceNodeOffsets()[2], 6);

    // MeshDefinition keeps edges as pairs while the interface hands out one
    // flat span, so this is the one place the two representations differ.
    ASSERT_EQ(view.edgeNodes().size(), 4u);
    EXPECT_EQ(view.edgeNodes()[0], 0);
    EXPECT_EQ(view.edgeNodes()[1], 1);
    EXPECT_EQ(view.edgeNodes()[2], 1);
    EXPECT_EQ(view.edgeNodes()[3], 2);
}

TEST(SpatialAdaptersTest, NetworkServesVerticesEdgesAndAMeshView)
{
    Network network;
    const int a = network.addVertex(0.0, 0.0);
    const int b = network.addVertex(10.0, 0.0);
    const int c = network.addVertex(10.0, 10.0);

    network.addEdge(a, b);
    network.addEdge(b, c);

    const NetworkAdapter adapter(network);

    EXPECT_EQ(adapter.vertexCount(), 3);
    EXPECT_EQ(adapter.edgeCount(), 2);

    ASSERT_NE(adapter.vertex(1), nullptr);
    EXPECT_DOUBLE_EQ(adapter.vertex(1)->x(), 10.0);
    EXPECT_EQ(adapter.vertex(1)->index(), 1u);

    ASSERT_NE(adapter.edge(0), nullptr);
    ASSERT_NE(adapter.edge(0)->orig(), nullptr);
    ASSERT_NE(adapter.edge(0)->dest(), nullptr);

    // The endpoints must be the graph's own vertices, not copies: a consumer
    // comparing identity to find a shared node would otherwise never match.
    EXPECT_EQ(adapter.edge(0)->orig(), adapter.vertex(0));
    EXPECT_EQ(adapter.edge(0)->dest(), adapter.vertex(1));
    EXPECT_EQ(adapter.edge(1)->orig(), adapter.vertex(1));

    EXPECT_EQ(adapter.vertex(-1), nullptr);
    EXPECT_EQ(adapter.edge(9), nullptr);

    ASSERT_NE(adapter.meshView(), nullptr);
    EXPECT_EQ(adapter.meshView()->nodeCount(), 3);
    EXPECT_EQ(adapter.meshView()->edgeCount(), 2);
    EXPECT_EQ(adapter.meshView()->faceCount(), 0)
        << "a network has no faces";
}

TEST(SpatialAdaptersTest, NetworkSkipsEdgesNamingVerticesItDoesNotHave)
{
    Network network;
    network.addVertex(0.0, 0.0);
    network.addVertex(1.0, 1.0);
    network.addEdge(0, 1);

    // A malformed graph: dereferencing this would read outside the vertex
    // array.
    const_cast<std::vector<NetworkEdge> &>(network.edges())
        .push_back(NetworkEdge(0, 99));

    const NetworkAdapter adapter(network);

    EXPECT_EQ(adapter.edgeCount(), 1) << "an out-of-range edge was adapted";
}

TEST(SpatialAdaptersTest, PolyhedralSurfaceServesItsPatches)
{
    PolyhedralSurface surface;
    surface.addPatch(std::make_shared<Polygon>(makeRing()));
    surface.addVertex(Point(0.0, 0.0));
    surface.addVertex(Point(4.0, 3.0));

    const PolyhedralSurfaceAdapter adapter(surface);

    EXPECT_EQ(adapter.geometryType(),
              HCS::IGeometry::GeometryType::PolyhedralSurface);
    EXPECT_EQ(adapter.patchCount(), 1);
    EXPECT_EQ(adapter.vertexCount(), 2);

    ASSERT_NE(adapter.patch(0), nullptr);
    EXPECT_EQ(adapter.patch(0)->exteriorRing()->pointCount(), 5);

    // A patch names the surface it belongs to, which is what the standard's
    // IPolygon::polyhedralSurface() exists for.
    EXPECT_EQ(adapter.patch(0)->polyhedralSurface(), &adapter);

    ASSERT_NE(adapter.meshView(), nullptr);
    EXPECT_EQ(adapter.meshView()->nodeCount(), 2);
}
