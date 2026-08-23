// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timeregulargridcomponentdataitems.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  TimeRegularGrid2DComponentDataItem and
 *         TimeRegularGrid3DComponentDataItem — grid-cell values varying in
 *         time (plan §7a).
 *
 * Implement the standard's ITimeRegularGrid2D/3DComponentDataItem with the
 * canonical dimension ordering: 2-D items expose
 * shape() = { time, yCells, xCells }; 3-D items
 * shape() = { time, zCells, yCells, xCells }. The time-series storage
 * mixin's flat [time × cells] block is row-major in exactly that order, so
 * the items serve rank-3/rank-4 hyperslabs directly over it — "one time
 * step, any cell window" is a strided view with no copies beyond the
 * hyperslab engine's. The IRegularGrid association is borrowed from the
 * owning component.
 *
 * \threadsafety Not thread-safe on their own.
 */

#ifndef HYDROCOUPLESDK_TIMEREGULARGRIDCOMPONENTDATAITEMS_H
#define HYDROCOUPLESDK_TIMEREGULARGRIDCOMPONENTDATAITEMS_H

#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouplesdk/temporal/timeseriescomponentdataitem.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplespatiotemporal.h"

#include <array>
#include <string>
#include <vector>

namespace HydroCouple::SDK::SpatioTemporal {

/*!
 * \brief Typed time-grid data item over a 2-D regular grid
 *        ({ time, y, x } storage).
 * \tparam T Element type (int, double).
 */
template <typename T>
class TimeRegularGrid2DComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::
          ITimeRegularGrid2DComponentDataItem
{
    using TimeStore = HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>;

public:
    /*!
     * \brief Construct over a borrowed grid.
     * \param[in] id         Item identifier.
     * \param[in] grid       The grid (borrowed; may be nullptr).
     * \param[in] objectType Mesh entity the values attach to.
     * \param[in] yCells     Cells along y (dimension 1).
     * \param[in] xCells     Cells along x (dimension 2).
     * \param[in] valueDef   Value definition (borrowed).
     * \param[in] mc         Owning component (borrowed).
     */
    TimeRegularGrid2DComponentDataItem(
        std::string_view id, HydroCouple::Spatial::IRegularGrid2D *grid,
        HydroCouple::Spatial::MeshDataObjectType objectType, int yCells,
        int xCells, ValueDefinition *valueDef = nullptr,
        HydroCouple::IModelComponent *mc = nullptr)
        : AbstractComponentDataItem(
              id, {&m_timeDimension, &m_yCellDimension, &m_xCellDimension},
              valueDef, mc),
          TimeStore(/*expectedTimeSteps=*/100, yCells * xCells),
          m_grid(grid),
          m_objectType(objectType),
          m_yCells(yCells),
          m_xCells(xCells),
          m_timeDimension("time", "Time dimension"),
          m_yCellDimension("y_cells", "Y cell dimension"),
          m_xCellDimension("x_cells", "X cell dimension")
    {
    }

    ~TimeRegularGrid2DComponentDataItem() override = default;

    // ── ITimeSeriesComponentDataItem ──────────────────────────────────────

    [[nodiscard]] const HydroCouple::Temporal::IDateTime *
    time(int64_t timeIndex) const override
    {
        return TimeStore::time(timeIndex);
    }

    [[nodiscard]] int64_t timeCount() const override
    {
        return TimeStore::timeCount();
    }

    [[nodiscard]] std::span<const double> times() const override
    {
        return TimeStore::times();
    }

    [[nodiscard]] HydroCouple::Temporal::ITimeSpan *timeSpan() const override
    {
        return TimeStore::timeSpan();
    }

    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override
    {
        return const_cast<Dimension *>(&m_timeDimension);
    }

    // ── IRegularGrid2DComponentDataItem ───────────────────────────────────

    [[nodiscard]] HydroCouple::Spatial::IRegularGrid2D *grid() const override
    {
        return m_grid;
    }

    [[nodiscard]] HydroCouple::Spatial::MeshDataObjectType
    meshDataObjectType() const override
    {
        return m_objectType;
    }

    [[nodiscard]] HydroCouple::IDimension *xCellDimension() const override
    {
        return const_cast<Dimension *>(&m_xCellDimension);
    }

    [[nodiscard]] HydroCouple::IDimension *yCellDimension() const override
    {
        return const_cast<Dimension *>(&m_yCellDimension);
    }

    /*!
     * \brief Values attach to whole cells here; no per-cell edge dimension.
     */
    [[nodiscard]] HydroCouple::IDimension *cellEdgeDimension() const override
    {
        return nullptr;
    }

    /*!
     * \brief Values attach to whole cells here; no per-cell vertex dimension.
     */
    [[nodiscard]] HydroCouple::IDimension *cellVertexDimension() const override
    {
        return nullptr;
    }

    // ── v2 typed hyperslab data plane (rank 3: time, y, x) ────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return {TimeStore::timeCount(), static_cast<int64_t>(m_yCells),
                static_cast<int64_t>(m_xCells)};
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return TimeStore::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override
    {
        const std::array<int64_t, 3> fullShape{
            TimeStore::timeCount(), static_cast<int64_t>(m_yCells),
            static_cast<int64_t>(m_xCells)};
        return HydroCouple::SDK::detail::readHyperslab<T>(
            TimeStore::rawData().data(), fullShape, destination, start, count,
            message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        const std::array<int64_t, 3> fullShape{
            TimeStore::timeCount(), static_cast<int64_t>(m_yCells),
            static_cast<int64_t>(m_xCells)};
        if (!HydroCouple::SDK::detail::writeHyperslab<T>(
                TimeStore::rawData().data(), fullShape, source, start, count,
                message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    [[nodiscard]] int yCells() const noexcept { return m_yCells; }
    [[nodiscard]] int xCells() const noexcept { return m_xCells; }

private:
    HydroCouple::Spatial::IRegularGrid2D     *m_grid;
    HydroCouple::Spatial::MeshDataObjectType  m_objectType;
    int m_yCells;
    int m_xCells;
    Dimension m_timeDimension;
    Dimension m_yCellDimension;
    Dimension m_xCellDimension;
};

/*!
 * \brief Typed time-grid data item over a 3-D regular grid
 *        ({ time, z, y, x } storage).
 * \tparam T Element type (int, double).
 */
template <typename T>
class TimeRegularGrid3DComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::
          ITimeRegularGrid3DComponentDataItem
{
    using TimeStore = HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>;

public:
    /*!
     * \brief Construct over a borrowed grid.
     * \param[in] id         Item identifier.
     * \param[in] grid       The grid (borrowed; may be nullptr).
     * \param[in] objectType Mesh entity the values attach to.
     * \param[in] zCells     Cells along z (dimension 1).
     * \param[in] yCells     Cells along y (dimension 2).
     * \param[in] xCells     Cells along x (dimension 3).
     * \param[in] valueDef   Value definition (borrowed).
     * \param[in] mc         Owning component (borrowed).
     */
    TimeRegularGrid3DComponentDataItem(
        std::string_view id, HydroCouple::Spatial::IRegularGrid3D *grid,
        HydroCouple::Spatial::MeshDataObjectType objectType, int zCells,
        int yCells, int xCells, ValueDefinition *valueDef = nullptr,
        HydroCouple::IModelComponent *mc = nullptr)
        : AbstractComponentDataItem(
              id,
              {&m_timeDimension, &m_zCellDimension, &m_yCellDimension,
               &m_xCellDimension},
              valueDef, mc),
          TimeStore(/*expectedTimeSteps=*/100, zCells * yCells * xCells),
          m_grid(grid),
          m_objectType(objectType),
          m_zCells(zCells),
          m_yCells(yCells),
          m_xCells(xCells),
          m_timeDimension("time", "Time dimension"),
          m_zCellDimension("z_cells", "Z cell dimension"),
          m_yCellDimension("y_cells", "Y cell dimension"),
          m_xCellDimension("x_cells", "X cell dimension")
    {
    }

    ~TimeRegularGrid3DComponentDataItem() override = default;

    // ── ITimeSeriesComponentDataItem ──────────────────────────────────────

    [[nodiscard]] const HydroCouple::Temporal::IDateTime *
    time(int64_t timeIndex) const override
    {
        return TimeStore::time(timeIndex);
    }

    [[nodiscard]] int64_t timeCount() const override
    {
        return TimeStore::timeCount();
    }

    [[nodiscard]] std::span<const double> times() const override
    {
        return TimeStore::times();
    }

    [[nodiscard]] HydroCouple::Temporal::ITimeSpan *timeSpan() const override
    {
        return TimeStore::timeSpan();
    }

    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override
    {
        return const_cast<Dimension *>(&m_timeDimension);
    }

    // ── IRegularGrid3DComponentDataItem ───────────────────────────────────

    [[nodiscard]] HydroCouple::Spatial::IRegularGrid3D *grid() const override
    {
        return m_grid;
    }

    [[nodiscard]] HydroCouple::Spatial::MeshDataObjectType
    meshDataObjectType() const override
    {
        return m_objectType;
    }

    [[nodiscard]] HydroCouple::IDimension *xCellDimension() const override
    {
        return const_cast<Dimension *>(&m_xCellDimension);
    }

    [[nodiscard]] HydroCouple::IDimension *yCellDimension() const override
    {
        return const_cast<Dimension *>(&m_yCellDimension);
    }

    [[nodiscard]] HydroCouple::IDimension *zCellDimension() const override
    {
        return const_cast<Dimension *>(&m_zCellDimension);
    }

    /*!
     * \brief Values attach to whole cells here; no per-cell face dimension.
     */
    [[nodiscard]] HydroCouple::IDimension *cellFaceDimension() const override
    {
        return nullptr;
    }

    /*!
     * \brief Values attach to whole cells here; no per-cell vertex dimension.
     */
    [[nodiscard]] HydroCouple::IDimension *cellVertexDimension() const override
    {
        return nullptr;
    }

    // ── v2 typed hyperslab data plane (rank 4: time, z, y, x) ─────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return {TimeStore::timeCount(), static_cast<int64_t>(m_zCells),
                static_cast<int64_t>(m_yCells),
                static_cast<int64_t>(m_xCells)};
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return TimeStore::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override
    {
        const std::array<int64_t, 4> fullShape{
            TimeStore::timeCount(), static_cast<int64_t>(m_zCells),
            static_cast<int64_t>(m_yCells), static_cast<int64_t>(m_xCells)};
        return HydroCouple::SDK::detail::readHyperslab<T>(
            TimeStore::rawData().data(), fullShape, destination, start, count,
            message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        const std::array<int64_t, 4> fullShape{
            TimeStore::timeCount(), static_cast<int64_t>(m_zCells),
            static_cast<int64_t>(m_yCells), static_cast<int64_t>(m_xCells)};
        if (!HydroCouple::SDK::detail::writeHyperslab<T>(
                TimeStore::rawData().data(), fullShape, source, start, count,
                message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

private:
    HydroCouple::Spatial::IRegularGrid3D     *m_grid;
    HydroCouple::Spatial::MeshDataObjectType  m_objectType;
    int m_zCells;
    int m_yCells;
    int m_xCells;
    Dimension m_timeDimension;
    Dimension m_zCellDimension;
    Dimension m_yCellDimension;
    Dimension m_xCellDimension;
};

using TimeRegularGrid2DComponentDataItemDouble =
    TimeRegularGrid2DComponentDataItem<double>;
using TimeRegularGrid3DComponentDataItemDouble =
    TimeRegularGrid3DComponentDataItem<double>;

extern template class HYDROCOUPLESDK_EXPORT
    TimeRegularGrid2DComponentDataItem<int>;
extern template class HYDROCOUPLESDK_EXPORT
    TimeRegularGrid2DComponentDataItem<double>;
extern template class HYDROCOUPLESDK_EXPORT
    TimeRegularGrid3DComponentDataItem<int>;
extern template class HYDROCOUPLESDK_EXPORT
    TimeRegularGrid3DComponentDataItem<double>;

} // namespace HydroCouple::SDK::SpatioTemporal

#endif // HYDROCOUPLESDK_TIMEREGULARGRIDCOMPONENTDATAITEMS_H
