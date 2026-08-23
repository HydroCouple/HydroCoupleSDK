// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for Phase 7a: id-based exchange items (IIdBasedComponentDataItem
// over the typed data plane), the time-and-identifier item, and the
// spatiotemporal data items — temporal dimension 0 over the typed spatial
// stores, with the standard's canonical dimension orderings.

#include <gtest/gtest.h>

#include "hydrocouplesdk/data/idbasedexchangeitems.h"
#include "hydrocouplesdk/temporal/timeidbasedcomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timegeometrycomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timenetworkcomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timepolyhedralsurfacecomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timeregulargridcomponentdataitems.h"
#include "hydrocouplesdk/temporal/timedata.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <array>
#include <memory>
#include <numeric>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Temporal;
using namespace HydroCouple::SDK::SpatioTemporal;

namespace {

BufferDescriptor hostBuffer(void *data, DataKind kind,
                            std::span<const int64_t> shape)
{
    BufferDescriptor descriptor;
    descriptor.data  = data;
    descriptor.kind  = kind;
    descriptor.rank  = static_cast<int32_t>(shape.size());
    descriptor.shape = shape.data();
    descriptor.space = MemorySpace::Host;
    return descriptor;
}

} // namespace

// ── Id-based exchange items ───────────────────────────────────────────────

TEST(IdBasedItemsTest, OutputCarriesIdentifiersAndValues)
{
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Q")};
    IdBasedOutputDouble output("flows", {"reach1", "reach2", "reach3"},
                               quantity.get(), nullptr);

    EXPECT_EQ(output.identifiers(),
              (std::vector<std::string>{"reach1", "reach2", "reach3"}));
    EXPECT_EQ(output.shape(), (std::vector<int64_t>{3}));
    EXPECT_EQ(output.dataKind(), DataKind::Float64);
    ASSERT_NE(output.identifierDimension(), nullptr);
    EXPECT_EQ(output.identifierDimension()->id(), "identifiers");

    output.valueAt("reach2") = 42.0;
    EXPECT_DOUBLE_EQ(output[1], 42.0);

    std::array<double, 3> readBack{};
    const std::array<int64_t, 1> shape{3}, start{0};
    std::string message;
    ASSERT_TRUE(output.getValuesInto(
        hostBuffer(readBack.data(), DataKind::Float64, shape), start, shape,
        &message)) << message;
    EXPECT_DOUBLE_EQ(readBack[1], 42.0);
}

TEST(IdBasedItemsTest, InputConsumesMatchingProvider)
{
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Q")};
    IdBasedOutputDouble output("upstream", {"a", "b"}, quantity.get(),
                               nullptr);
    IdBasedInputDouble  input("downstream", {"a", "b"}, quantity.get(),
                              nullptr);

    ASSERT_TRUE(input.setProvider(&output));
    output.addConsumer(&input);

    output[0] = 1.5;
    output[1] = 2.5;
    input.retrieveValuesFromProvider();
    EXPECT_DOUBLE_EQ(input.valueAt("a"), 1.5);
    EXPECT_DOUBLE_EQ(input.valueAt("b"), 2.5);
}

TEST(IdBasedItemsTest, InputRejectsIncompatibleProviders)
{
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Q")};
    IdBasedInputDouble input("in", {"a", "b", "c"}, quantity.get(), nullptr);

    IdBasedOutputInt wrongKind("ints", {"a", "b", "c"}, quantity.get(),
                               nullptr);
    std::string message;
    EXPECT_FALSE(input.canConsume(&wrongKind, message));
    EXPECT_NE(message.find("DataKind"), std::string::npos);

    IdBasedOutputDouble tooFew("short", {"a"}, quantity.get(), nullptr);
    EXPECT_FALSE(input.canConsume(&tooFew, message));
    EXPECT_FALSE(input.setProvider(&tooFew));
}

// ── Time + identifier item ────────────────────────────────────────────────

TEST(TimeIdBasedItemTest, TimeIsDimensionZero)
{
    TimeIdBasedComponentDataItemDouble item("stage", {"gauge1", "gauge2"});

    item.addTimeStep(std::make_shared<TimeData>("t0", 100.0), {1.0, 10.0});
    item.addTimeStep(std::make_shared<TimeData>("t1", 101.0), {2.0, 20.0});

    EXPECT_EQ(item.shape(), (std::vector<int64_t>{2, 2}));
    EXPECT_EQ(item.timeCount(), 2);
    EXPECT_EQ(item.identifiers(),
              (std::vector<std::string>{"gauge1", "gauge2"}));
    EXPECT_DOUBLE_EQ(item.times()[1], 101.0);
    EXPECT_DOUBLE_EQ(item.timeSpan()->julianDay(), 100.0);

    // "Current time step, all identifiers" is one contiguous slab.
    std::array<double, 2> row{};
    const std::array<int64_t, 2> start{1, 0}, count{1, 2};
    const std::array<int64_t, 1> outShape{2};
    std::string message;
    ASSERT_TRUE(item.getValuesInto(
        hostBuffer(row.data(), DataKind::Float64, outShape), start, count,
        &message)) << message;
    EXPECT_DOUBLE_EQ(row[0], 2.0);
    EXPECT_DOUBLE_EQ(row[1], 20.0);
}

// ── Time-geometry item ────────────────────────────────────────────────────

TEST(TimeGeometryItemTest, GeometryDimensionAndEnvelope)
{
    // Geometry pointers are borrowed from the owning component; the item's
    // job is counts, dimensions, envelope, and the data plane.
    std::vector<HydroCouple::Spatial::IGeometry *> geometries{nullptr,
                                                              nullptr,
                                                              nullptr};
    TimeGeometryComponentDataItemDouble item(
        "depths", HydroCouple::Spatial::IGeometry::GeometryType::Point,
        geometries);

    EXPECT_EQ(item.geometryCount(), 3);
    EXPECT_EQ(item.geometryType(),
              HydroCouple::Spatial::IGeometry::GeometryType::Point);
    EXPECT_EQ(item.geometry(5), nullptr);
    ASSERT_NE(item.geometryDimension(), nullptr);
    ASSERT_NE(item.timeDimension(), nullptr);

    item.setEnvelope(HydroCouple::SDK::Spatial::Envelope(0.0, -1.0, 10.0, 5.0));
    ASSERT_NE(item.envelope(), nullptr);
    EXPECT_DOUBLE_EQ(item.envelope()->maxX(), 10.0);
    EXPECT_DOUBLE_EQ(item.envelope()->minY(), -1.0);

    item.addTimeStep(std::make_shared<TimeData>("t0", 200.0),
                     {1.0, 2.0, 3.0});
    EXPECT_EQ(item.shape(), (std::vector<int64_t>{1, 3}));
    EXPECT_DOUBLE_EQ(item.getValueAt(0, 2), 3.0);
}

// ── Time-network item ─────────────────────────────────────────────────────

TEST(TimeNetworkItemTest, EntityDimensionFollowsObjectType)
{
    TimeNetworkComponentDataItemDouble edges(
        "flows", /*network=*/nullptr,
        HydroCouple::Spatial::NetworkDataObjectType::Edge,
        HydroCouple::Spatial::SpatialDataType::Scalar, /*entityCount=*/4);

    EXPECT_EQ(edges.networkDataObjectType(),
              HydroCouple::Spatial::NetworkDataObjectType::Edge);
    EXPECT_NE(edges.edgeDimension(), nullptr);
    EXPECT_EQ(edges.vertexDimension(), nullptr);
    EXPECT_EQ(edges.network(), nullptr);

    edges.addTimeStep(std::make_shared<TimeData>("t0", 300.0),
                      {1.0, 2.0, 3.0, 4.0});
    edges.addTimeStep(std::make_shared<TimeData>("t1", 301.0),
                      {5.0, 6.0, 7.0, 8.0});
    EXPECT_EQ(edges.shape(), (std::vector<int64_t>{2, 4}));
    EXPECT_DOUBLE_EQ(edges.getValueAt(1, 3), 8.0);

    TimeNetworkComponentDataItemDouble nodes(
        "heads", nullptr, HydroCouple::Spatial::NetworkDataObjectType::Node,
        HydroCouple::Spatial::SpatialDataType::Scalar, 2);
    EXPECT_EQ(nodes.edgeDimension(), nullptr);
    EXPECT_NE(nodes.vertexDimension(), nullptr);
}

// ── Time-mesh items ───────────────────────────────────────────────────────

TEST(TimeMeshItemTest, EntityDimensionMirrorsAttachment)
{
    TimeSeriesTINComponentDataItemDouble cells(
        "wse", /*tin=*/nullptr, HydroCouple::Spatial::MeshDataObjectType::Cell,
        HydroCouple::Spatial::SpatialDataType::Scalar, /*entityCount=*/5);

    EXPECT_EQ(cells.TIN(), nullptr);
    EXPECT_EQ(cells.polyhedralSurface(), nullptr);
    EXPECT_NE(cells.patchDimension(), nullptr);
    EXPECT_EQ(cells.edgeDimension(), nullptr);
    EXPECT_EQ(cells.vertexDimension(), nullptr);

    cells.addTimeStep(std::make_shared<TimeData>("t0", 400.0),
                      {1, 2, 3, 4, 5});
    EXPECT_EQ(cells.shape(), (std::vector<int64_t>{1, 5}));

    TimeSeriesPolyhedralSurfaceComponentDataItem<double> vertices(
        "elev", nullptr, HydroCouple::Spatial::MeshDataObjectType::Vertex,
        HydroCouple::Spatial::SpatialDataType::Scalar, 3);
    EXPECT_EQ(vertices.patchDimension(), nullptr);
    EXPECT_NE(vertices.vertexDimension(), nullptr);
}

// ── Time-grid items ───────────────────────────────────────────────────────

TEST(TimeGridItemTest, Rank3HyperslabOver2DGrid)
{
    TimeRegularGrid2DComponentDataItemDouble item(
        "depth", /*grid=*/nullptr,
        HydroCouple::Spatial::MeshDataObjectType::Cell, /*yCells=*/2,
        /*xCells=*/3);

    // Two time steps of a 2×3 field: values t*100 + y*10 + x.
    for (int t = 0; t < 2; ++t)
    {
        std::vector<double> field;
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 3; ++x)
                field.push_back(t * 100.0 + y * 10.0 + x);
        item.addTimeStep(
            std::make_shared<TimeData>("t" + std::to_string(t), 500.0 + t),
            field);
    }

    EXPECT_EQ(item.shape(), (std::vector<int64_t>{2, 2, 3}));
    ASSERT_NE(item.yCellDimension(), nullptr);
    ASSERT_NE(item.xCellDimension(), nullptr);
    EXPECT_EQ(item.cellEdgeDimension(), nullptr);

    // Window: time 1, row 1, columns 1..2.
    std::array<double, 2> window{};
    const std::array<int64_t, 3> start{1, 1, 1}, count{1, 1, 2};
    const std::array<int64_t, 1> outShape{2};
    std::string message;
    ASSERT_TRUE(item.getValuesInto(
        hostBuffer(window.data(), DataKind::Float64, outShape), start, count,
        &message)) << message;
    EXPECT_DOUBLE_EQ(window[0], 111.0);
    EXPECT_DOUBLE_EQ(window[1], 112.0);

    // Write the same window back scaled and confirm through direct access.
    window = {-1.0, -2.0};
    ASSERT_TRUE(item.setValuesFrom(
        hostBuffer(window.data(), DataKind::Float64, outShape), start, count,
        &message)) << message;
    EXPECT_DOUBLE_EQ(item.getValueAt(1, 1 * 3 + 1), -1.0);
    EXPECT_DOUBLE_EQ(item.getValueAt(1, 1 * 3 + 2), -2.0);
}

TEST(TimeGridItemTest, Rank4HyperslabOver3DGrid)
{
    TimeRegularGrid3DComponentDataItemDouble item(
        "conc", /*grid=*/nullptr,
        HydroCouple::Spatial::MeshDataObjectType::Cell, /*zCells=*/2,
        /*yCells=*/2, /*xCells=*/2);

    std::vector<double> field(8);
    std::iota(field.begin(), field.end(), 0.0); // z-major {z, y, x}
    item.addTimeStep(std::make_shared<TimeData>("t0", 600.0), field);

    EXPECT_EQ(item.shape(), (std::vector<int64_t>{1, 2, 2, 2}));
    ASSERT_NE(item.zCellDimension(), nullptr);
    EXPECT_EQ(item.cellFaceDimension(), nullptr);

    // Column through z at (y=1, x=0): elements 2 and 6.
    std::array<double, 2> column{};
    const std::array<int64_t, 4> start{0, 0, 1, 0}, count{1, 2, 1, 1};
    const std::array<int64_t, 1> outShape{2};
    std::string message;
    ASSERT_TRUE(item.getValuesInto(
        hostBuffer(column.data(), DataKind::Float64, outShape), start, count,
        &message)) << message;
    EXPECT_DOUBLE_EQ(column[0], 2.0);
    EXPECT_DOUBLE_EQ(column[1], 6.0);
}
