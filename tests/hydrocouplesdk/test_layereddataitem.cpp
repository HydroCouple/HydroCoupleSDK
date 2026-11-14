// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for LayeredMeshComponentDataItem — mesh-entity values resolved into
// vertical layers, over the typed data plane.
//
// The three things worth pinning are the dimension ordering (a partner reading
// through the polyhedral-surface base must find the entity at dimension 0), the
// hyperslab round-trip, and that the vertical coordinate handed out stays live:
// a layered item whose elevations went stale would answer confidently and
// wrongly for the rest of a run.

#include <gtest/gtest.h>

#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/spatial/layeredmeshcomponentdataitem.h"

#include <memory>
#include <span>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Spatial;

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

constexpr int kCells  = 4;
constexpr int kLayers = 3;

//! A sigma coordinate over a flat bed, thin enough to be obviously correct.
//! `setStage` is what lets a gate move the free surface underneath the item.
class TestSigmaCoordinate final : public HydroCouple::Spatial::IVerticalCoordinate
{
public:
    TestSigmaCoordinate(int layers, double bed, double stage)
        : m_layers(layers), m_bed(bed), m_stage(stage)
    {
    }

    void setStage(double stage) { m_stage = stage; ++m_epoch; }

    [[nodiscard]] HydroCouple::Spatial::VerticalCoordinateKind kind() const override
    {
        return HydroCouple::Spatial::VerticalCoordinateKind::Sigma;
    }

    [[nodiscard]] int64_t layerCount() const override { return m_layers; }
    [[nodiscard]] int64_t columnCount() const override { return m_columns; }
    [[nodiscard]] bool isTimeVarying() const override { return true; }
    [[nodiscard]] uint64_t geometryEpoch() const override { return m_epoch; }

    [[nodiscard]] double interfaceElevation(int64_t, int64_t interfaceIndex) const override
    {
        const double sigma =
            static_cast<double>(interfaceIndex) / static_cast<double>(m_layers);
        return m_stage + sigma * (m_bed - m_stage);
    }

    // Bulk [column][interface] profile, rebuilt whenever the stage moves.
    [[nodiscard]] std::span<const double> interfaceElevations() const override
    {
        if (m_profile.size() != static_cast<size_t>(m_columns * (m_layers + 1)))
            m_profile.resize(static_cast<size_t>(m_columns * (m_layers + 1)));
        for (int64_t c = 0; c < m_columns; ++c)
            for (int64_t k = 0; k <= m_layers; ++k)
                m_profile[static_cast<size_t>(c * (m_layers + 1) + k)] = interfaceElevation(c, k);
        return m_profile;
    }

private:
    int64_t  m_layers;
    int64_t  m_columns = 1;
    double   m_bed;
    double   m_stage;
    uint64_t m_epoch = 0;
    mutable std::vector<double> m_profile;
};

} // namespace

TEST(LayeredMeshItemTest, EntityIsDimensionZeroAndLayerIsDimensionOne)
{
    TestSigmaCoordinate vertical(kLayers, -10.0, 0.0);
    LayeredMeshComponentDataItem<double> item(
        "temperature", /*surface=*/nullptr, &vertical,
        HydroCouple::Spatial::MeshLocation::Face,
        HydroCouple::Spatial::SpatialDataType::Scalar, kCells, kLayers);

    // The ordering is fixed by IPolyhedralSurfaceComponentDataItem, not chosen
    // here: a partner that only understands surfaces has to find the entity at
    // dimension 0 and be able to ignore what follows.
    EXPECT_EQ(item.shape(), (std::vector<int64_t>{kCells, kLayers}));
    EXPECT_EQ(item.dataKind(), DataKind::Float64);

    ASSERT_NE(item.entityDimension(), nullptr);
    EXPECT_EQ(item.entityDimension()->id(), "faces");
    EXPECT_EQ(item.location(), HydroCouple::Spatial::MeshLocation::Face);

    ASSERT_NE(item.layerDimension(), nullptr);
    EXPECT_EQ(item.layerDimension()->id(), "layers");

    // The layer dimension must not masquerade as the entity dimension.
    EXPECT_NE(item.layerDimension(), item.entityDimension());
}

TEST(LayeredMeshItemTest, ValuesRoundTripThroughTheHyperslabPlane)
{
    TestSigmaCoordinate vertical(kLayers, -10.0, 0.0);
    LayeredMeshComponentDataItem<double> item(
        "temperature", nullptr, &vertical,
        HydroCouple::Spatial::MeshLocation::Face,
        HydroCouple::Spatial::SpatialDataType::Scalar, kCells, kLayers);

    // cell*10 + layer, so a transposed read is unmistakable rather than merely
    // wrong — 21 in the wrong slot could not be mistaken for 12.
    std::vector<double> written;
    for (int c = 0; c < kCells; ++c)
        for (int k = 0; k < kLayers; ++k)
            written.push_back(c * 10.0 + k);

    const std::array<int64_t, 2> shape{kCells, kLayers};
    const std::array<int64_t, 2> start{0, 0};
    const std::array<int64_t, 2> count{kCells, kLayers};

    std::string message;
    ASSERT_TRUE(item.setValuesFrom(
        hostBuffer(written.data(), DataKind::Float64, shape), start, count, &message))
        << message;

    std::vector<double> read(written.size(), 0.0);
    ASSERT_TRUE(item.getValuesInto(
        hostBuffer(read.data(), DataKind::Float64, shape), start, count, &message))
        << message;

    EXPECT_EQ(read, written);
}

TEST(LayeredMeshItemTest, APartialSlabTouchesOnlyItsOwnCells)
{
    TestSigmaCoordinate vertical(kLayers, -10.0, 0.0);
    LayeredMeshComponentDataItem<double> item(
        "temperature", nullptr, &vertical,
        HydroCouple::Spatial::MeshLocation::Face,
        HydroCouple::Spatial::SpatialDataType::Scalar, kCells, kLayers);
    item.setDefaultValue(-999.0);
    item.resetDataArray();

    // Write only cells 1 and 2. If the entity and layer strides were swapped,
    // this would spill into cells the caller never named.
    std::vector<double>          patch{1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    const std::array<int64_t, 2> patchShape{2, kLayers};
    const std::array<int64_t, 2> start{1, 0};
    const std::array<int64_t, 2> count{2, kLayers};

    std::string message;
    ASSERT_TRUE(item.setValuesFrom(
        hostBuffer(patch.data(), DataKind::Float64, patchShape), start, count,
        &message))
        << message;

    for (int k = 0; k < kLayers; ++k)
    {
        EXPECT_EQ(item(0, k), -999.0) << "cell 0 layer " << k;
        EXPECT_EQ(item(3, k), -999.0) << "cell 3 layer " << k;
    }
    EXPECT_EQ(item(1, 0), 1.0);
    EXPECT_EQ(item(2, 2), 6.0);
}

TEST(LayeredMeshItemTest, TheVerticalCoordinateItHandsOutIsLive)
{
    TestSigmaCoordinate vertical(kLayers, -10.0, 0.0);
    LayeredMeshComponentDataItem<double> item(
        "temperature", nullptr, &vertical,
        HydroCouple::Spatial::MeshLocation::Face,
        HydroCouple::Spatial::SpatialDataType::Scalar, kCells, kLayers);

    ASSERT_NE(item.verticalCoordinate(), nullptr);
    EXPECT_EQ(item.verticalCoordinate()->layerCount(), kLayers);
    EXPECT_TRUE(item.verticalCoordinate()->isTimeVarying());

    const double before = item.verticalCoordinate()->interfaceElevation(0, 1);

    // Raise the free surface under the item, as a running model would.
    vertical.setStage(2.0);
    const double after = item.verticalCoordinate()->interfaceElevation(0, 1);

    // The point of borrowing rather than snapshotting. An item that had copied
    // its elevations at construction would report the old mesh forever, and
    // nothing about the values it returned would look wrong.
    EXPECT_GT(after, before);
    EXPECT_DOUBLE_EQ(item.verticalCoordinate()->interfaceElevation(0, 0), 2.0);
    EXPECT_DOUBLE_EQ(item.verticalCoordinate()->interfaceElevation(0, kLayers), -10.0);
}

TEST(LayeredMeshItemTest, ReadableThroughThePolyhedralSurfaceBase)
{
    TestSigmaCoordinate vertical(kLayers, -10.0, 0.0);
    LayeredMeshComponentDataItem<double> item(
        "temperature", nullptr, &vertical,
        HydroCouple::Spatial::MeshLocation::Face,
        HydroCouple::Spatial::SpatialDataType::Scalar, kCells, kLayers);

    // A partner that only understands surfaces must still get the topology and
    // the values; that is the whole reason this extends the surface item rather
    // than replacing it.
    auto *surfaceView =
        static_cast<HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem *>(&item);
    EXPECT_EQ(surfaceView->shape(), (std::vector<int64_t>{kCells, kLayers}));
    ASSERT_NE(surfaceView->entityDimension(), nullptr);

    // And a partner that cares only about layering can find it without knowing
    // whether it was handed a mesh or a network.
    auto *layering = dynamic_cast<HydroCouple::Spatial::ILayering *>(surfaceView);
    ASSERT_NE(layering, nullptr);
    EXPECT_EQ(layering->verticalCoordinate(), &vertical);
}
