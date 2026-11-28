// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 10: geometry values, the adapters that serve them through
 *         the HydroCouple geometry interfaces, and a geometry data item whose
 *         values vary in time.
 *
 * Builds two stream gauges (points), a reach (a line with measures) and a
 * catchment (a polygon) as SDK values, wraps them in PointAdapter,
 * LineStringAdapter and PolygonAdapter with a declared reference system,
 * reads them back through the interfaces, and records gauge stages in a
 * TimeGeometryComponentDataItem. Exits nonzero when any value is not the
 * one the guide states.
 */

#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/spatial/geometryadapters.h"
#include "hydrocouplesdk/spatiotemporal/timegeometrycomponentdataitem.h"
#include "hydrocouplesdk/temporal/timedata.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple::SDK::Spatial;
namespace HCS = HydroCouple::Spatial;
using HydroCouple::SDK::SpatioTemporal::TimeGeometryComponentDataItemDouble;
using HydroCouple::SDK::Temporal::TimeData;

namespace {

int failures = 0;

void check(bool ok, const char *what)
{
    if (!ok)
    {
        std::printf("FAILED: %s\n", what);
        ++failures;
    }
}

bool near(double a, double b) { return std::abs(a - b) < 1e-9; }

//! [reference]
//! A projected system with a declared vertical datum: UTM zone 17N (EPSG
//! 26917) for x and y, NAVD88 height (EPSG 5703) for z, both in meters.
class Utm17Navd88 final : public HCS::ISpatialReferenceSystem
{
public:
    int authSRID() const override { return 26917; }
    const std::string &authName() const override { return m_epsg; }
    const std::string &srText() const override { return m_empty; }
    HydroCouple::IUnit::DistanceUnits distanceUnits() const override
    {
        return HydroCouple::IUnit::DistanceUnits::Meters;
    }
    const std::string &verticalAuthName() const override { return m_epsg; }
    int verticalAuthSRID() const override { return 5703; }
    const std::string &verticalSrText() const override { return m_empty; }
    HydroCouple::IUnit::DistanceUnits verticalDistanceUnits() const override
    {
        return HydroCouple::IUnit::DistanceUnits::Meters;
    }

private:
    std::string m_epsg = "EPSG";
    std::string m_empty;
};
//! [reference]

} // namespace

int main()
{
    //! [values]
    // Two gauges, with their bed elevations as z (meters, NAVD88).
    const Point upstream(512000.0, 4512000.0, 210.0);
    const Point downstream(512300.0, 4512400.0, 204.0);

    // A reach from one gauge to the other; m carries the river station (m).
    LineString reach;
    reach.addPoint(Point(512000.0, 4512000.0, 210.0, 0.0));
    reach.addPoint(Point(512300.0, 4512400.0, 204.0, 500.0));

    // A 400 m x 300 m catchment, counterclockwise, first point repeated last.
    LineString outline;
    outline.addPoints({Point(511900.0, 4511900.0), Point(512300.0, 4511900.0),
                       Point(512300.0, 4512200.0), Point(511900.0, 4512200.0),
                       Point(511900.0, 4511900.0)});
    const Polygon catchment(outline);
    //! [values]

    std::printf("reach: %d points, length %.3f m (3-D), closed %d\n",
                reach.pointCount(), reach.length(), reach.isClosed());
    std::printf("catchment: valid %d, area %.1f m2, contains upstream gauge %d\n",
                catchment.isValid(), catchment.area(),
                catchment.containsPoint(upstream));
    check(near(reach.length(), std::sqrt(300.0 * 300.0 + 400.0 * 400.0 + 6.0 * 6.0)),
          "reach length is the 3-D segment length");
    check(near(catchment.area(), 120000.0), "catchment area is 400 m x 300 m");
    check(catchment.containsPoint(upstream) && !catchment.containsPoint(downstream),
          "the upstream gauge is inside the catchment, the downstream one is not");

    //! [adapters]
    Utm17Navd88 reference;
    PointAdapter gaugeA(upstream, "gauge_a", 0, &reference);
    PointAdapter gaugeB(downstream, "gauge_b", 1, &reference);
    const LineStringAdapter reachAdapter(reach, "reach", 0, &reference);
    const PolygonAdapter catchmentAdapter(catchment, "catchment", 0, &reference);

    // Read back through the standard's interfaces only.
    const HCS::ILineString &line = reachAdapter;
    const HCS::IPolygon &area = catchmentAdapter;
    //! [adapters]

    std::printf("gauge_a: type %d, z %.1f, WKB %zu bytes, WKT \"%s\"\n",
                static_cast<int>(gaugeA.geometryType()), gaugeA.z(),
                gaugeA.getWKB().size(), gaugeA.getWKT().c_str());
    std::printf("reach adapter: %lld points, point 1 id \"%s\", end m %.1f, WKB %zu bytes\n",
                static_cast<long long>(line.pointCount()), line.point(1)->id().c_str(),
                line.endPoint()->m(), reachAdapter.getWKB().size());
    std::printf("catchment adapter: area %.1f, exterior ring %lld points, holes %lld, "
                "envelope x %.0f..%.0f\n",
                area.area(), static_cast<long long>(area.exteriorRing()->pointCount()),
                static_cast<long long>(area.interiorRingCount()),
                area.envelope()->minX(), area.envelope()->maxX());
    std::printf("reference: %s:%d, vertical %s:%d; is3D %d; intersects %d; buffer %s\n",
                gaugeA.spatialReferenceSystem()->authName().c_str(),
                gaugeA.spatialReferenceSystem()->authSRID(),
                gaugeA.spatialReferenceSystem()->verticalAuthName().c_str(),
                gaugeA.spatialReferenceSystem()->verticalAuthSRID(), gaugeA.is3D(),
                area.intersects(gaugeA), area.buffer(10.0) ? "object" : "nullptr");

    check(gaugeA.getWKB().size() == 21, "point WKB is 21 bytes");
    check(reachAdapter.getWKB().size() == 9 + 2 * 16, "line WKB is 9 + 16 per point");
    check(line.point(1)->id() == "reach:1", "line points carry derived ids");
    check(line.startPoint() == line.point(0), "start point is the owned point 0");
    check(area.interiorRing(0) == nullptr, "no hole 0");
    check(near(area.envelope()->maxY(), 4512200.0), "polygon envelope");
    check(!gaugeA.is3D() && gaugeA.z() == 210.0, "adapters keep z but report 2-D");
    check(!area.intersects(gaugeA) && area.buffer(10.0) == nullptr,
          "predicates and constructive operations are not computed");

    //! [geometry_item]
    // Stage at the two gauges, one value per gauge per time step.
    TimeGeometryComponentDataItemDouble stage(
        "gauge_stage", HCS::IGeometry::GeometryType::Point, {&gaugeA, &gaugeB});
    stage.setEnvelope(Envelope(upstream, downstream));
    stage.setTimeKind(HydroCouple::Temporal::TimeKind::Instantaneous);
    stage.addTimeStep(std::make_shared<TimeData>("t0", 2026, 1, 1, 0), {211.20, 205.10});
    stage.addTimeStep(std::make_shared<TimeData>("t1", 2026, 1, 1, 1), {211.45, 205.30});
    stage.addTimeStep(std::make_shared<TimeData>("t2", 2026, 1, 1, 2), {211.60, 205.55});

    // "The latest time step, every gauge" is one contiguous slab.
    std::array<double, 2> latest{};
    const std::array<int64_t, 2> start{2, 0}, count{1, 2};
    const std::array<int64_t, 1> bufferShape{2};
    std::string message;
    const bool read = stage.getValuesInto(
        HydroCouple::SDK::detail::hostDescriptor(latest.data(),
                                                 HydroCouple::DataKind::Float64,
                                                 bufferShape),
        start, count, &message);
    //! [geometry_item]

    const auto shape = stage.shape();
    std::printf("gauge_stage: shape {%lld, %lld}, dimensions \"%s\", \"%s\", "
                "geometry 1 \"%s\", latest %.2f %.2f, envelope maxY %.0f\n",
                static_cast<long long>(shape[0]), static_cast<long long>(shape[1]),
                stage.timeDimension()->id().c_str(), stage.geometryDimension()->id().c_str(),
                stage.geometry(1)->id().c_str(), latest[0], latest[1],
                stage.envelope()->maxY());
    check(read, message.c_str());
    check(shape == std::vector<int64_t>{3, 2}, "shape is { time, geometry }");
    check(near(latest[0], 211.60) && near(latest[1], 205.55), "latest slab");
    check(stage.geometry(2) == nullptr, "geometry index out of range");
    check(near(stage.timeSpan()->duration(), 2.0 / 24.0), "time span is two hours");

    std::printf("%s\n", failures == 0 ? "all checks passed" : "checks FAILED");
    return failures == 0 ? 0 : 1;
}
