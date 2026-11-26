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
 * The SDK has no IRegularGrid2D or IRegularGrid3D implementation (its
 * RegularGrid2D and RegularGrid3D are value types), so the grid passed here
 * is one the component implements, or nullptr. Whatever location() is
 * declared, these items hold one value per cell: cellEdgeDimension(),
 * cellFaceDimension() and cellVertexDimension() return nullptr.
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
 *
 * The item starts with no time steps; the owning component appends them
 * with addTimeStep(), yCells·xCells values each in row order (cell (y, x) at
 * y·xCells + x). Value (t, y, x) is then at (t·yCells + y)·xCells + x. The
 * cell counts are not checked against the grid. The y and x cell dimensions
 * have roles Row and Column. The int and double instantiations are
 * exported; the item derives from AbstractComponentDataItem and cannot be
 * added with addOutput().
 *
 * \tparam T Element type (int, double).
 */
template <typename T>
class TimeRegularGrid2DComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::
          ITimeRegularGrid2DComponentDataItem
{
    using TimeStore = HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>; //!< The time-series value store.

public:
    /*!
     * \brief Construct over a borrowed grid.
     * \param[in] id         Item identifier.
     * \param[in] grid       The grid (borrowed; may be nullptr).
     * \param[in] objectType Mesh entity the values attach to; reported by
     *                       location() only (Face for cells, by the
     *                       standard's convention).
     * \param[in] yCells     Cells along y (dimension 1).
     * \param[in] xCells     Cells along x (dimension 2).
     * \param[in] valueDef   Value definition (borrowed).
     * \param[in] mc         Owning component (borrowed).
     */
    TimeRegularGrid2DComponentDataItem(
        std::string_view id, HydroCouple::Spatial::IRegularGrid2D *grid,
        HydroCouple::Spatial::MeshLocation objectType, int yCells,
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

    /*!
     * Destroys the item and its values; the grid is not owned.
     */
    ~TimeRegularGrid2DComponentDataItem() override = default;

    // ── ITimeSeriesComponentDataItem ──────────────────────────────────────

    /*!
     * \brief The time of a step (ITimeSeriesComponentDataItem::time()).
     * \param[in] timeIndex Zero-based time-step index.
     * \returns A non-owning pointer to the step's time, valid until
     *          clearTimeSteps(); nullptr when \p timeIndex is out of range.
     */
    [[nodiscard]] const HydroCouple::Temporal::IDateTime *
    time(int64_t timeIndex) const override
    {
        return TimeStore::time(timeIndex);
    }

    /*!
     * \brief The number of time steps (ITimeSeriesComponentDataItem::timeCount()).
     * \returns The number of addTimeStep() calls since construction or the
     *          last clearTimeSteps().
     */
    [[nodiscard]] int64_t timeCount() const override
    {
        return TimeStore::timeCount();
    }

    /*!
     * \brief All step times as Julian days (ITimeSeriesComponentDataItem::times()).
     * \returns timeCount() values, valid until the next addTimeStep() or
     *          clearTimeSteps().
     */
    [[nodiscard]] std::span<const double> times() const override
    {
        return TimeStore::times();
    }

    /*!
     * \brief The span of the held steps (ITimeSeriesComponentDataItem::timeSpan()).
     * \returns A span from the first to the last step (duration 0 for none or
     *          one). The object is replaced on every addTimeStep() and
     *          clearTimeSteps(), so the pointer is valid only until then.
     */
    [[nodiscard]] HydroCouple::Temporal::ITimeSpan *timeSpan() const override
    {
        return TimeStore::timeSpan();
    }

    /*!
     * \brief The time axis (ITimeSeriesComponentDataItem::timeDimension()).
     * \returns Dimension 0 ("time", role Time), never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override
    {
        return const_cast<Dimension *>(&m_timeDimension);
    }

    /*!
     * \brief What a time coordinate refers to (ITimeSeriesComponentDataItem::timeKind()).
     * \returns The kind set with setTimeKind(); TimeKind::Unknown by default.
     */
    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeKind();
    }

    /*!
     * \brief The averaging interval (ITimeSeriesComponentDataItem::intervalLength()).
     * \returns The length in days set with setTimeKind(); 0 by default.
     */
    [[nodiscard]] double intervalLength() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::intervalLength();
    }

    /*!
     * \brief Behavior between held times (ITimeSeriesComponentDataItem::timeInterpolation()).
     * \returns The policy set with setTimeInterpolation(); Unknown by default.
     */
    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeInterpolation();
    }

    /*!
     * \brief Behavior outside the held times (ITimeSeriesComponentDataItem::timeExtrapolation()).
     * \returns The policy set with setTimeExtrapolation(); Unknown by default.
     */
    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeExtrapolation();
    }

    // ── IRegularGrid2DComponentDataItem ───────────────────────────────────

    /*!
     * \brief The grid (IRegularGrid2DComponentDataItem::grid()).
     * \returns The borrowed grid; nullptr when none was given. The connection
     *          validator reads the item's reference system from it.
     */
    [[nodiscard]] HydroCouple::Spatial::IRegularGrid2D *grid() const override
    {
        return m_grid;
    }

    [[nodiscard]] HydroCouple::Spatial::MeshLocation
    /*!
     * \brief The declared location (IRegularGrid2DComponentDataItem::location()).
     * \returns The location given at construction; the storage is per cell
     *          whatever it is.
     */
    location() const override
    {
        return m_objectType;
    }

    /*!
     * \brief The x cell axis (IRegularGrid2DComponentDataItem::xCellDimension()).
     * \returns Dimension 2 ("x_cells", role Column), never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *xCellDimension() const override
    {
        return const_cast<Dimension *>(&m_xCellDimension);
    }

    /*!
     * \brief The y cell axis (IRegularGrid2DComponentDataItem::yCellDimension()).
     * \returns Dimension 1 ("y_cells", role Row), never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *yCellDimension() const override
    {
        return const_cast<Dimension *>(&m_yCellDimension);
    }

    /*!
     * \brief Values attach to whole cells here; no per-cell edge dimension.
     * \returns nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *cellEdgeDimension() const override
    {
        return nullptr;
    }

    /*!
     * \brief Values attach to whole cells here; no per-cell vertex dimension.
     * \returns nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *cellVertexDimension() const override
    {
        return nullptr;
    }

    // ── v2 typed hyperslab data plane (rank 3: time, y, x) ────────────────

    /*!
     * \brief The item's shape (IComponentDataItem::shape()).
     * \returns { timeCount(), yCells, xCells }.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return {TimeStore::timeCount(), static_cast<int64_t>(m_yCells),
                static_cast<int64_t>(m_xCells)};
    }

    /*!
     * \brief The element kind (IComponentDataItem::dataKind()).
     * \returns The DataKind of T.
     */
    [[nodiscard]] DataKind dataKind() const override
    {
        return TimeStore::storageKind();
    }

    /*!
     * \brief Copies a block of values out (IComponentDataItem::getValuesInto()).
     * \param[in]  destination Buffer of dataKind() receiving the block.
     * \param[in]  start       { first time step, first y cell, first x cell }.
     * \param[in]  count       { time steps, y cells, x cells } to copy.
     * \param[out] message     Receives the reason on failure; may be nullptr.
     * \returns False when the selection or the buffer does not fit.
     */
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

    /*!
     * \brief Overwrites a block of existing values (IComponentDataItem::setValuesFrom()).
     *
     * Writes only into time steps that already exist (add them with
     * addTimeStep()); on success emits the value-changed signal for the block.
     *
     * \param[in]  source  Buffer of dataKind() holding the block.
     * \param[in]  start   { first time step, first y cell, first x cell }.
     * \param[in]  count   { time steps, y cells, x cells } to write.
     * \param[out] message Receives the reason on failure; may be nullptr.
     * \returns False, with nothing written or signaled, when the selection or
     *          the buffer does not fit.
     */
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

    /*!
     * \brief The number of cells along y.
     * \returns yCells as given at construction.
     */
    [[nodiscard]] int yCells() const noexcept { return m_yCells; }
    /*!
     * \brief The number of cells along x.
     * \returns xCells as given at construction.
     */
    [[nodiscard]] int xCells() const noexcept { return m_xCells; }

private:
    HydroCouple::Spatial::IRegularGrid2D     *m_grid; //!< Borrowed grid; may be nullptr.
    HydroCouple::Spatial::MeshLocation  m_objectType; //!< The declared location.
    int m_yCells; //!< Cells along y.
    int m_xCells; //!< Cells along x.
    Dimension m_timeDimension; //!< Dimension 0, time.
    Dimension m_yCellDimension; //!< The y cell dimension.
    Dimension m_xCellDimension; //!< The x cell dimension.
};

/*!
 * \brief Typed time-grid data item over a 3-D regular grid
 *        ({ time, z, y, x } storage).
 *
 * As TimeRegularGrid2DComponentDataItem with a z axis (role Depth) in front:
 * each time step holds zCells·yCells·xCells values, cell (z, y, x) at
 * (z·yCells + y)·xCells + x. The standard names Volume as the location of
 * cell values on a 3-D grid. Unlike the 2-D item this class has no cell-count
 * accessors; read them from shape().
 *
 * \tparam T Element type (int, double).
 */
template <typename T>
class TimeRegularGrid3DComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::
          ITimeRegularGrid3DComponentDataItem
{
    using TimeStore = HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>; //!< The time-series value store.

public:
    /*!
     * \brief Construct over a borrowed grid.
     * \param[in] id         Item identifier.
     * \param[in] grid       The grid (borrowed; may be nullptr).
     * \param[in] objectType Mesh entity the values attach to; reported by
     *                       location() only (Volume for cells, by the
     *                       standard's convention).
     * \param[in] zCells     Cells along z (dimension 1).
     * \param[in] yCells     Cells along y (dimension 2).
     * \param[in] xCells     Cells along x (dimension 3).
     * \param[in] valueDef   Value definition (borrowed).
     * \param[in] mc         Owning component (borrowed).
     */
    TimeRegularGrid3DComponentDataItem(
        std::string_view id, HydroCouple::Spatial::IRegularGrid3D *grid,
        HydroCouple::Spatial::MeshLocation objectType, int zCells,
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

    /*!
     * Destroys the item and its values; the grid is not owned.
     */
    ~TimeRegularGrid3DComponentDataItem() override = default;

    // ── ITimeSeriesComponentDataItem ──────────────────────────────────────

    /*!
     * \brief The time of a step (ITimeSeriesComponentDataItem::time()).
     * \param[in] timeIndex Zero-based time-step index.
     * \returns A non-owning pointer to the step's time, valid until
     *          clearTimeSteps(); nullptr when \p timeIndex is out of range.
     */
    [[nodiscard]] const HydroCouple::Temporal::IDateTime *
    time(int64_t timeIndex) const override
    {
        return TimeStore::time(timeIndex);
    }

    /*!
     * \brief The number of time steps (ITimeSeriesComponentDataItem::timeCount()).
     * \returns The number of addTimeStep() calls since construction or the
     *          last clearTimeSteps().
     */
    [[nodiscard]] int64_t timeCount() const override
    {
        return TimeStore::timeCount();
    }

    /*!
     * \brief All step times as Julian days (ITimeSeriesComponentDataItem::times()).
     * \returns timeCount() values, valid until the next addTimeStep() or
     *          clearTimeSteps().
     */
    [[nodiscard]] std::span<const double> times() const override
    {
        return TimeStore::times();
    }

    /*!
     * \brief The span of the held steps (ITimeSeriesComponentDataItem::timeSpan()).
     * \returns A span from the first to the last step (duration 0 for none or
     *          one). The object is replaced on every addTimeStep() and
     *          clearTimeSteps(), so the pointer is valid only until then.
     */
    [[nodiscard]] HydroCouple::Temporal::ITimeSpan *timeSpan() const override
    {
        return TimeStore::timeSpan();
    }

    /*!
     * \brief The time axis (ITimeSeriesComponentDataItem::timeDimension()).
     * \returns Dimension 0 ("time", role Time), never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override
    {
        return const_cast<Dimension *>(&m_timeDimension);
    }

    /*!
     * \brief What a time coordinate refers to (ITimeSeriesComponentDataItem::timeKind()).
     * \returns The kind set with setTimeKind(); TimeKind::Unknown by default.
     */
    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeKind();
    }

    /*!
     * \brief The averaging interval (ITimeSeriesComponentDataItem::intervalLength()).
     * \returns The length in days set with setTimeKind(); 0 by default.
     */
    [[nodiscard]] double intervalLength() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::intervalLength();
    }

    /*!
     * \brief Behavior between held times (ITimeSeriesComponentDataItem::timeInterpolation()).
     * \returns The policy set with setTimeInterpolation(); Unknown by default.
     */
    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeInterpolation();
    }

    /*!
     * \brief Behavior outside the held times (ITimeSeriesComponentDataItem::timeExtrapolation()).
     * \returns The policy set with setTimeExtrapolation(); Unknown by default.
     */
    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeExtrapolation();
    }

    // ── IRegularGrid3DComponentDataItem ───────────────────────────────────

    /*!
     * \brief The grid (IRegularGrid3DComponentDataItem::grid()).
     * \returns The borrowed grid; nullptr when none was given.
     */
    [[nodiscard]] HydroCouple::Spatial::IRegularGrid3D *grid() const override
    {
        return m_grid;
    }

    [[nodiscard]] HydroCouple::Spatial::MeshLocation
    /*!
     * \brief The declared location (IRegularGrid3DComponentDataItem::location()).
     * \returns The location given at construction; the storage is per cell
     *          whatever it is.
     */
    location() const override
    {
        return m_objectType;
    }

    /*!
     * \brief The x cell axis (IRegularGrid3DComponentDataItem::xCellDimension()).
     * \returns Dimension 3 ("x_cells", role Column), never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *xCellDimension() const override
    {
        return const_cast<Dimension *>(&m_xCellDimension);
    }

    /*!
     * \brief The y cell axis (IRegularGrid3DComponentDataItem::yCellDimension()).
     * \returns Dimension 2 ("y_cells", role Row), never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *yCellDimension() const override
    {
        return const_cast<Dimension *>(&m_yCellDimension);
    }

    /*!
     * \brief The z cell axis (IRegularGrid3DComponentDataItem::zCellDimension()).
     * \returns Dimension 1 ("z_cells", role Depth), never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *zCellDimension() const override
    {
        return const_cast<Dimension *>(&m_zCellDimension);
    }

    /*!
     * \brief Values attach to whole cells here; no per-cell face dimension.
     * \returns nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *cellFaceDimension() const override
    {
        return nullptr;
    }

    /*!
     * \brief Values attach to whole cells here; no per-cell vertex dimension.
     * \returns nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *cellVertexDimension() const override
    {
        return nullptr;
    }

    // ── v2 typed hyperslab data plane (rank 4: time, z, y, x) ─────────────

    /*!
     * \brief The item's shape (IComponentDataItem::shape()).
     * \returns { timeCount(), zCells, yCells, xCells }.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return {TimeStore::timeCount(), static_cast<int64_t>(m_zCells),
                static_cast<int64_t>(m_yCells),
                static_cast<int64_t>(m_xCells)};
    }

    /*!
     * \brief The element kind (IComponentDataItem::dataKind()).
     * \returns The DataKind of T.
     */
    [[nodiscard]] DataKind dataKind() const override
    {
        return TimeStore::storageKind();
    }

    /*!
     * \brief Copies a block of values out (IComponentDataItem::getValuesInto()).
     * \param[in]  destination Buffer of dataKind() receiving the block.
     * \param[in]  start       { first time step, first z cell, first y cell, first x cell }.
     * \param[in]  count       { time steps, z cells, y cells, x cells } to copy.
     * \param[out] message     Receives the reason on failure; may be nullptr.
     * \returns False when the selection or the buffer does not fit.
     */
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

    /*!
     * \brief Overwrites a block of existing values (IComponentDataItem::setValuesFrom()).
     *
     * Writes only into time steps that already exist (add them with
     * addTimeStep()); on success emits the value-changed signal for the block.
     *
     * \param[in]  source  Buffer of dataKind() holding the block.
     * \param[in]  start   { first time step, first z cell, first y cell, first x cell }.
     * \param[in]  count   { time steps, z cells, y cells, x cells } to write.
     * \param[out] message Receives the reason on failure; may be nullptr.
     * \returns False, with nothing written or signaled, when the selection or
     *          the buffer does not fit.
     */
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
    HydroCouple::Spatial::IRegularGrid3D     *m_grid; //!< Borrowed grid; may be nullptr.
    HydroCouple::Spatial::MeshLocation  m_objectType; //!< The declared location.
    int m_zCells; //!< Cells along z.
    int m_yCells; //!< Cells along y.
    int m_xCells; //!< Cells along x.
    Dimension m_timeDimension; //!< Dimension 0, time.
    Dimension m_zCellDimension; //!< The z cell dimension.
    Dimension m_yCellDimension; //!< The y cell dimension.
    Dimension m_xCellDimension; //!< The x cell dimension.
};

//! Double-valued time-series 2-D grid item, exported from the library.
using TimeRegularGrid2DComponentDataItemDouble =
    TimeRegularGrid2DComponentDataItem<double>;
//! Double-valued time-series 3-D grid item, exported from the library.
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
