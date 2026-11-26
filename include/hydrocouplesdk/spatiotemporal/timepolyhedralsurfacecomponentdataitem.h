// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timepolyhedralsurfacecomponentdataitem.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  TimeSeriesPolyhedralSurfaceComponentDataItem and the TIN
 *         specialization — mesh-entity values varying in time (plan §7a).
 *
 * Implements the standard's ITimeSeriesPolyhedralSurfaceComponentDataItem:
 * time is dimension 0, the mesh entity dimension selected by
 * location() (Node, Edge, Face or Volume) is dimension 1 —
 * shape() = { timeCount, entityCount }. The IPolyhedralSurface/ITIN
 * association is borrowed from the owning component. The single
 * entityDimension() accessor returns the dimension the values are attached
 * to, named after the location ("nodes", "edges", "faces", "volumes").
 *
 * PolyhedralSurfaceAdapter (meshadapters.h) is the SDK's IPolyhedralSurface
 * implementation to borrow; the SDK has no ITIN implementation, so a TIN
 * item needs one from the component (or nullptr).
 *
 * \threadsafety Not thread-safe on their own.
 */

#ifndef HYDROCOUPLESDK_TIMEPOLYHEDRALSURFACECOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_TIMEPOLYHEDRALSURFACECOMPONENTDATAITEM_H

#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouplesdk/temporal/timeseriescomponentdataitem.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplespatiotemporal.h"

#include <string>
#include <vector>

namespace HydroCouple::SDK::SpatioTemporal {

/*!
 * \brief Typed time-mesh data item ({ time, mesh-entity } storage).
 *
 * The item starts with no time steps; the owning component appends them
 * with addTimeStep() (from TimeSeriesComponentDataItem), entityCount values
 * each, time-major: value (t, e) is at t·entityCount + e, so "one time step,
 * every face" is contiguous. Entity indexes are those of the surface's
 * meshView(). The item is always rank 2: no Component dimension is added for
 * Vector, Tensor or MultiScalar data. The entity count is not checked
 * against the surface.
 *
 * Like the other spatiotemporal items it derives from
 * AbstractComponentDataItem, not AbstractOutput, so it cannot be added with
 * addOutput(). The int and double instantiations are exported.
 *
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class TimeSeriesPolyhedralSurfaceComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::
          ITimeSeriesPolyhedralSurfaceComponentDataItem
{
    using TimeStore = HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>; //!< The time-series value store.
    using MeshLocation = HydroCouple::Spatial::MeshLocation; //!< The standard's entity enumeration.

public:
    /*!
     * \brief Construct over a borrowed surface.
     * \param[in] id          Item identifier.
     * \param[in] surface     The polyhedral surface (borrowed; may be
     *                        nullptr when topology lives elsewhere).
     * \param[in] objectType  Mesh entity the values attach to
     *                        (Node, Edge, Face or Volume).
     * \param[in] dataType    Nature of each value (Scalar, Vector, …).
     * \param[in] entityCount Entities along dimension 1.
     * \param[in] valueDef    Value definition (borrowed).
     * \param[in] mc          Owning component (borrowed).
     */
    TimeSeriesPolyhedralSurfaceComponentDataItem(
        std::string_view id,
        HydroCouple::Spatial::IPolyhedralSurface *surface,
        MeshLocation objectType,
        HydroCouple::Spatial::SpatialDataType dataType, int entityCount,
        ValueDefinition *valueDef = nullptr,
        HydroCouple::IModelComponent *mc = nullptr)
        : AbstractComponentDataItem(
              id, {&m_timeDimension, &m_entityDimension}, valueDef, mc),
          TimeStore(/*expectedTimeSteps=*/100, entityCount),
          m_surface(surface),
          m_objectType(objectType),
          m_dataType(dataType),
          m_timeDimension("time", "Time dimension"),
          m_entityDimension(entityDimensionId(objectType),
                            "Mesh entity dimension")
    {
    }

    /*!
     * Destroys the item and its values; the surface is not owned.
     */
    ~TimeSeriesPolyhedralSurfaceComponentDataItem() override = default;

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

    // ── IPolyhedralSurfaceComponentDataItem ───────────────────────────────

    /*!
     * \brief Which mesh entity the values sit on (IPolyhedralSurfaceComponentDataItem::location()).
     * \returns The location given at construction.
     */
    [[nodiscard]] MeshLocation location() const override
    {
        return m_objectType;
    }

    [[nodiscard]] HydroCouple::Spatial::SpatialDataType
    /*!
     * \brief The structure of each value (IPolyhedralSurfaceComponentDataItem::meshDataType()).
     * \returns The data type given at construction.
     */
    meshDataType() const override
    {
        return m_dataType;
    }

    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    /*!
     * \brief The surface (IPolyhedralSurfaceComponentDataItem::polyhedralSurface()).
     * \returns The borrowed surface (the TIN, for the TIN specialization);
     *          nullptr when none was given. The connection validator reads the
     *          item's reference system from it.
     */
    polyhedralSurface() const override
    {
        return m_surface;
    }

    /*!
     * \brief The entity axis (IPolyhedralSurfaceComponentDataItem::entityDimension()).
     * \returns Dimension 1, never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return const_cast<Dimension *>(&m_entityDimension);
    }

    /*!
     * \brief The frame of vector values (IPolyhedralSurfaceComponentDataItem::vectorBasis()).
     * \returns The basis set with setVectorBasis(); VectorBasis::Unknown by default.
     */
    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        return m_vectorBasis;
    }

    //! Declares the frame of vector/tensor values (Unknown for scalars).
    /*!
     * \param[in] basis The basis vectorBasis() reports from now on.
     */
    void setVectorBasis(HydroCouple::Spatial::VectorBasis basis)
    {
        m_vectorBasis = basis;
    }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The item's shape (IComponentDataItem::shape()).
     * \returns { timeCount(), entityCount }.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return TimeStore::storageShape();
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
     * \param[in]  start       { first time step, first entity }.
     * \param[in]  count       { time steps, entities } to copy.
     * \param[out] message     Receives the reason on failure; may be nullptr.
     * \returns False when the selection or the buffer does not fit.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override
    {
        return TimeStore::getSlab(destination, start, count, message);
    }

    /*!
     * \brief Overwrites a block of existing values (IComponentDataItem::setValuesFrom()).
     *
     * Writes only into time steps that already exist (add them with
     * addTimeStep()); on success emits the value-changed signal for the block.
     *
     * \param[in]  source  Buffer of dataKind() holding the block.
     * \param[in]  start   { first time step, first entity }.
     * \param[in]  count   { time steps, entities } to write.
     * \param[out] message Receives the reason on failure; may be nullptr.
     * \returns False, with nothing written or signaled, when the selection or
     *          the buffer does not fit.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!TimeStore::setSlab(source, start, count, message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

private:
    [[nodiscard]] static const char *
    /*!
     * \brief The id of the entity dimension for a location.
     * \param[in] objectType The location.
     * \returns "nodes", "edges", "faces" or "volumes" (each role Entity);
     *          "entities" for a value outside the enumeration.
     */
    entityDimensionId(MeshLocation objectType)
    {
        switch (objectType)
        {
            case MeshLocation::Node:   return "nodes";
            case MeshLocation::Edge:   return "edges";
            case MeshLocation::Face:   return "faces";
            case MeshLocation::Volume: return "volumes";
        }
        return "entities";
    }

    HydroCouple::Spatial::IPolyhedralSurface *m_surface; //!< Borrowed surface; may be nullptr.
    MeshLocation                        m_objectType; //!< Where the values sit.
    HydroCouple::Spatial::SpatialDataType     m_dataType; //!< Structure of each value.
    HydroCouple::Spatial::VectorBasis         m_vectorBasis = HydroCouple::Spatial::VectorBasis::Unknown; //!< Frame of vector values.
    Dimension m_timeDimension; //!< Dimension 0, time.
    Dimension m_entityDimension; //!< Dimension 1, the entities.
};

/*!
 * \brief TIN specialization: the surface is an ITIN.
 *
 * Identical to its base except that it also answers
 * ITimeSeriesTINComponentDataItem::TIN(); polyhedralSurface() returns the
 * same object. The class is final.
 *
 * \tparam T Element type.
 */
template <typename T>
class TimeSeriesTINComponentDataItem final
    : public TimeSeriesPolyhedralSurfaceComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::ITimeSeriesTINComponentDataItem
{
public:
    /*!
     * \brief Construct over a borrowed TIN (also serves as the surface).
     * \param[in] id          Item identifier.
     * \param[in] tin         The TIN (borrowed; may be nullptr).
     * \param[in] objectType  Mesh entity the values attach to; Face for
     *                        values per triangle.
     * \param[in] dataType    Structure of each value.
     * \param[in] entityCount Entities along dimension 1.
     * \param[in] valueDef    Value definition (borrowed).
     * \param[in] mc          Owning component (borrowed).
     */
    TimeSeriesTINComponentDataItem(
        std::string_view id, HydroCouple::Spatial::ITIN *tin,
        HydroCouple::Spatial::MeshLocation objectType,
        HydroCouple::Spatial::SpatialDataType dataType, int entityCount,
        ValueDefinition *valueDef = nullptr,
        HydroCouple::IModelComponent *mc = nullptr)
        : TimeSeriesPolyhedralSurfaceComponentDataItem<T>(
              id, tin, objectType, dataType, entityCount, valueDef, mc),
          m_tin(tin)
    {
    }

    /*!
     * Destroys the item; the TIN is not owned.
     */
    ~TimeSeriesTINComponentDataItem() override = default;

    /*!
     * \brief The TIN (ITimeSeriesTINComponentDataItem::TIN()).
     * \returns The borrowed TIN given at construction; may be nullptr.
     */
    [[nodiscard]] HydroCouple::Spatial::ITIN *TIN() const override
    {
        return m_tin;
    }

private:
    HydroCouple::Spatial::ITIN *m_tin; //!< Borrowed TIN; may be nullptr.
};

//! Double-valued time-series surface item, exported from the library.
using TimeSeriesPolyhedralSurfaceComponentDataItemDouble =
    TimeSeriesPolyhedralSurfaceComponentDataItem<double>;
//! Double-valued time-series TIN item, exported from the library.
using TimeSeriesTINComponentDataItemDouble =
    TimeSeriesTINComponentDataItem<double>;

extern template class HYDROCOUPLESDK_EXPORT
    TimeSeriesPolyhedralSurfaceComponentDataItem<int>;
extern template class HYDROCOUPLESDK_EXPORT
    TimeSeriesPolyhedralSurfaceComponentDataItem<double>;
extern template class HYDROCOUPLESDK_EXPORT
    TimeSeriesTINComponentDataItem<int>;
extern template class HYDROCOUPLESDK_EXPORT
    TimeSeriesTINComponentDataItem<double>;

} // namespace HydroCouple::SDK::SpatioTemporal

#endif // HYDROCOUPLESDK_TIMEPOLYHEDRALSURFACECOMPONENTDATAITEM_H
