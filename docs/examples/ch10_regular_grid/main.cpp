// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 10: regular grids. A Cartesian RegularGrid2D, the small
 *         IRegularGrid2D a component writes over it (the SDK has none), and
 *         cell values in time in TimeRegularGrid2DComponentDataItem and
 *         TimeRegularGrid3DComponentDataItem.
 *
 * Exits nonzero when any value is not the one the guide states.
 */

#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/spatial/regulargrid2d.h"
#include "hydrocouplesdk/spatial/regulargrid3d.h"
#include "hydrocouplesdk/spatiotemporal/timeregulargridcomponentdataitems.h"
#include "hydrocouplesdk/temporal/timedata.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

using namespace HydroCouple::SDK::Spatial;
namespace HCS = HydroCouple::Spatial;
using HydroCouple::SDK::SpatioTemporal::TimeRegularGrid2DComponentDataItemDouble;
using HydroCouple::SDK::SpatioTemporal::TimeRegularGrid3DComponentDataItemDouble;
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

//! [grid_view]
//! IRegularGrid2D over an SDK RegularGrid2D, every cell active. The grid is
//! borrowed; the view's spans point into its node arrays.
class GridView final : public HydroCouple::SDK::Identity,
                       public virtual HCS::IRegularGrid2D
{
public:
    GridView(const RegularGrid2D &grid, HCS::ISpatialReferenceSystem *srs)
        : Identity("grid", "Model grid"), m_grid(grid), m_srs(srs),
          m_active(static_cast<size_t>(grid.numCells()), 1)
    {
    }

    HCS::ISpatialReferenceSystem *spatialReferenceSystem() const override { return m_srs; }

    HCS::RegularGridType gridType() const override
    {
        switch (m_grid.gridType())
        {
            case RegularGrid2D::Cartesian:   return HCS::RegularGridType::Cartesian;
            case RegularGrid2D::Rectilinear: return HCS::RegularGridType::Rectilinear;
            case RegularGrid2D::Curvilinear: return HCS::RegularGridType::Curvilinear;
        }
        return HCS::RegularGridType::Curvilinear;
    }

    int64_t numXNodes() const override { return m_grid.numXNodes(); }
    int64_t numYNodes() const override { return m_grid.numYNodes(); }

    double xNodeLocation(int64_t i, int64_t j) const override
    {
        return m_grid.nodeX(static_cast<int>(i), static_cast<int>(j));
    }
    double yNodeLocation(int64_t i, int64_t j) const override
    {
        return m_grid.nodeY(static_cast<int>(i), static_cast<int>(j));
    }

    // Row-major [yNode][xNode], which is RegularGrid2D's own layout.
    std::span<const double> nodeXs() const override { return m_grid.nodeXData(); }
    std::span<const double> nodeYs() const override { return m_grid.nodeYData(); }

    bool isActive(int64_t, int64_t) const override { return true; }
    std::span<const uint8_t> activeCells() const override { return m_active; }

private:
    const RegularGrid2D &m_grid;
    HCS::ISpatialReferenceSystem *m_srs;
    std::vector<uint8_t> m_active;
};
//! [grid_view]

} // namespace

int main()
{
    //! [grid]
    // 4 x 3 nodes, 100 m apart, so 3 x 2 cells of 100 m x 100 m.
    const std::shared_ptr<RegularGrid2D> grid =
        RegularGrid2D::cartesian(500000.0, 4500000.0, 100.0, 100.0, 4, 3);
    double cx = 0.0, cy = 0.0;
    grid->cellCenter(2, 1, cx, cy);
    //! [grid]

    std::printf("grid: %d x %d nodes, %d x %d cells, cell (2,1) center (%.1f, %.1f), "
                "area %.1f m2, node (3,2) at (%.1f, %.1f)\n",
                grid->numXNodes(), grid->numYNodes(), grid->numXCells(), grid->numYCells(),
                cx, cy, grid->cellArea(2, 1), grid->nodeX(3, 2), grid->nodeY(3, 2));
    check(grid->numCells() == 6, "cell count");
    check(near(cx, 500250.0) && near(cy, 4500150.0), "cell center");
    check(near(grid->cellArea(2, 1), 10000.0), "cell area");
    check(grid->cellArea(3, 0) == 0.0, "a cell index out of range has area 0");
    check(RegularGrid2D::rectilinear({0.0, 1.0}, {0.0}, 3, 2) == nullptr,
          "rectilinear refuses arrays that do not match the counts");

    //! [grid_item]
    GridView view(*grid, nullptr);

    // Water depth per cell (m), two time steps; values y-major, y*3 + x.
    TimeRegularGrid2DComponentDataItemDouble depth(
        "depth", &view, HCS::MeshLocation::Face, grid->numYCells(), grid->numXCells());
    depth.addTimeStep(std::make_shared<TimeData>("t0", 2026, 6, 1, 0),
                      {0.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    depth.addTimeStep(std::make_shared<TimeData>("t1", 2026, 6, 1, 1),
                      {0.1, 0.2, 0.3, 0.4, 0.5, 0.6});

    // One time step, one grid row: the window { t = 1, y = 1, x = 0..2 }.
    std::array<double, 3> row{};
    const std::array<int64_t, 3> start{1, 1, 0}, count{1, 1, 3};
    const std::array<int64_t, 1> bufferShape{3};
    std::string message;
    const bool read = depth.getValuesInto(
        HydroCouple::SDK::detail::hostDescriptor(row.data(), HydroCouple::DataKind::Float64,
                                                 bufferShape),
        start, count, &message);
    //! [grid_item]

    const auto shape = depth.shape();
    std::printf("depth item: shape {%lld, %lld, %lld}, axes \"%s\" \"%s\" \"%s\", "
                "row y=1 at t1: %.1f %.1f %.1f, grid %s with %lld x-nodes\n",
                static_cast<long long>(shape[0]), static_cast<long long>(shape[1]),
                static_cast<long long>(shape[2]), depth.timeDimension()->id().c_str(),
                depth.yCellDimension()->id().c_str(), depth.xCellDimension()->id().c_str(),
                row[0], row[1], row[2], depth.grid()->id().c_str(),
                static_cast<long long>(depth.grid()->numXNodes()));
    check(read, message.c_str());
    check(shape == std::vector<int64_t>{2, 2, 3}, "shape is { time, y, x }");
    check(near(row[0], 0.4) && near(row[2], 0.6), "row window");
    check(depth.cellEdgeDimension() == nullptr, "values are per cell");
    check(depth.grid()->nodeXs().size() == 12, "the view serves the node arrays");

    //! [grid3d]
    // A 3-D grid of 3 x 3 x 4 nodes: 2 x 2 cells in plan, 3 layers of 0.5 m.
    const std::shared_ptr<RegularGrid3D> grid3 =
        RegularGrid3D::cartesian(0.0, 0.0, 10.0, 100.0, 100.0, -0.5, 3, 3, 4);

    TimeRegularGrid3DComponentDataItemDouble temperature(
        "temperature", nullptr, HCS::MeshLocation::Volume, 3, 2, 2);
    std::vector<double> field(12);
    std::iota(field.begin(), field.end(), 10.0); // (z*2 + y)*2 + x + 10
    temperature.addTimeStep(std::make_shared<TimeData>("t0", 2026, 6, 1, 0), field);

    // The water column under cell (y = 1, x = 0): one value per layer.
    std::array<double, 3> column{};
    const std::array<int64_t, 4> start3{0, 0, 1, 0}, count3{1, 3, 1, 1};
    const std::array<int64_t, 1> columnShape{3};
    const bool read3 = temperature.getValuesInto(
        HydroCouple::SDK::detail::hostDescriptor(column.data(), HydroCouple::DataKind::Float64,
                                                 columnShape),
        start3, count3, &message);
    //! [grid3d]

    std::printf("grid3d: %d nodes, %d cells, node (0,0,3) z %.1f; temperature shape "
                "{%lld, %lld, %lld, %lld}, column %.0f %.0f %.0f\n",
                grid3->numNodes(), grid3->numCells(), grid3->nodeZ(0, 0, 3),
                static_cast<long long>(temperature.shape()[0]),
                static_cast<long long>(temperature.shape()[1]),
                static_cast<long long>(temperature.shape()[2]),
                static_cast<long long>(temperature.shape()[3]), column[0], column[1], column[2]);
    check(read3, message.c_str());
    check(grid3->numCells() == 12 && near(grid3->nodeZ(0, 0, 3), 8.5), "3-D grid");
    check(near(column[0], 12.0) && near(column[1], 16.0) && near(column[2], 20.0),
          "column through the layers");

    std::printf("%s\n", failures == 0 ? "all checks passed" : "checks FAILED");
    return failures == 0 ? 0 : 1;
}
