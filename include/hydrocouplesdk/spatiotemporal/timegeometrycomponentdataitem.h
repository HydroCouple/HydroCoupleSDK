// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timegeometrycomponentdataitem.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  TimeGeometryComponentDataItem — geometry-attached values varying
 *         in time (plan §7a).
 *
 * Implements the standard's ITimeGeometryComponentDataItem by composing
 * the time-series storage mixin (time is dimension 0) with a borrowed
 * geometry list (dimension 1): shape() = { timeCount, geometryCount } and
 * "current time step, all geometries" is a contiguous slab. Geometry
 * objects are supplied by the owning component (which implements the
 * standard's IGeometry on its own geometry types); the item owns only the
 * values, the dimensions, and the envelope adapter.
 *
 * The geometry adapters in geometryadapters.h (PointAdapter,
 * LineStringAdapter, PolygonAdapter) are the SDK's ready-made IGeometry
 * implementations to put in that list.
 *
 * \threadsafety Not thread-safe on its own.
 */

#ifndef HYDROCOUPLESDK_TIMEGEOMETRYCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_TIMEGEOMETRYCOMPONENTDATAITEM_H

#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouplesdk/temporal/timeseriescomponentdataitem.h"
#include "hydrocouplesdk/spatial/envelopeadapter.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplespatiotemporal.h"

#include <string>
#include <vector>

namespace HydroCouple::SDK::SpatioTemporal {

/*!
 * \brief Typed time-geometry data item ({ time, geometry } storage).
 *
 * The item starts with no time steps; the owning component appends them
 * with addTimeStep() (from TimeSeriesComponentDataItem), one value per
 * geometry each, and may then rewrite values through setValuesFrom(),
 * setValueAt() or rawData(). Values are time-major: value (t, g) is at
 * t·geometryCount() + g. The geometry count is fixed at construction. The
 * temporal semantics (timeKind(), timeInterpolation(), timeExtrapolation())
 * are Unknown until the component sets them with setTimeKind() and the
 * other setters of the storage mixin; the connection validator warns about
 * undeclared ones.
 *
 * The item derives from AbstractComponentDataItem, not from AbstractOutput,
 * so it describes values a component holds (an internal store or an
 * argument) but cannot be added with addOutput(); an exchangeable
 * time-geometry output composes AbstractOutput with the same storage, as
 * LayeredMeshOutput does for layered meshes. The int and double
 * instantiations are exported from the library.
 *
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class TimeGeometryComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem
{
    using TimeStore = HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>; //!< The time-series value store.

public:
    /*!
     * \brief Construct over a borrowed geometry list.
     * \param[in] id           Item identifier.
     * \param[in] geometryType Geometry type of every entry.
     * \param[in] geometries   Geometry per entity (borrowed; the owning
     *                         component keeps them alive).
     * \param[in] valueDef     Value definition (borrowed; may be nullptr).
     * \param[in] mc           Owning component (borrowed; may be nullptr).
     *
     * The list is copied (the pointers, not the geometries) and fixes the
     * element count; entries may be nullptr. \p geometryType is reported as
     * given and not checked against the entries. The envelope starts empty;
     * set it with setEnvelope().
     */
    TimeGeometryComponentDataItem(
        std::string_view id,
        HydroCouple::Spatial::IGeometry::GeometryType geometryType,
        std::vector<HydroCouple::Spatial::IGeometry *> geometries,
        ValueDefinition *valueDef = nullptr,
        HydroCouple::IModelComponent *mc = nullptr)
        : AbstractComponentDataItem(
              id, {&m_timeDimension, &m_geometryDimension}, valueDef, mc),
          TimeStore(/*expectedTimeSteps=*/100,
                    static_cast<int>(geometries.size())),
          m_geometryType(geometryType),
          m_geometries(std::move(geometries)),
          m_timeDimension("time", "Time dimension"),
          m_geometryDimension("geometries", "Geometry dimension")
    {
    }

    //! Destroys the item and its values; the geometries are not owned.
    ~TimeGeometryComponentDataItem() override = default;

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
     * \returns A span from the first to the last step (duration 0 for none
     *          or one). The object is replaced on every addTimeStep() and
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

    // ── IGeometryComponentDataItem ────────────────────────────────────────

    /*!
     * \brief The declared geometry type (IGeometryComponentDataItem::geometryType()).
     * \returns The type given at construction.
     */
    [[nodiscard]] HydroCouple::Spatial::IGeometry::GeometryType
    geometryType() const override
    {
        return m_geometryType;
    }

    /*!
     * \brief The number of geometries (IGeometryComponentDataItem::geometryCount()).
     * \returns The length of the list given at construction.
     */
    [[nodiscard]] int64_t geometryCount() const override
    {
        return static_cast<int64_t>(m_geometries.size());
    }

    /*!
     * \brief A geometry by index (IGeometryComponentDataItem::geometry()).
     * \param[in] geometryIndex Zero-based geometry index.
     * \returns The borrowed geometry (nullptr if that entry was nullptr);
     *          nullptr when \p geometryIndex is out of range. The connection
     *          validator reads the item's reference system from geometry 0.
     */
    [[nodiscard]] HydroCouple::Spatial::IGeometry *
    geometry(int64_t geometryIndex) const override
    {
        if (geometryIndex < 0 ||
            geometryIndex >= static_cast<int64_t>(m_geometries.size()))
            return nullptr;
        return m_geometries[static_cast<size_t>(geometryIndex)];
    }

    /*!
     * \brief The geometry axis (IGeometryComponentDataItem::geometryDimension()).
     * \returns Dimension 1 ("geometries", role Entity), never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *geometryDimension() const override
    {
        return const_cast<Dimension *>(&m_geometryDimension);
    }

    /*!
     * \brief The bounds of all geometries (IGeometryComponentDataItem::envelope()).
     * \returns A pointer to the item's own envelope adapter, never nullptr,
     *          valid for the life of the item; empty (isValid() false on its
     *          Envelope) until setEnvelope() is called. Nothing computes it
     *          from the geometries.
     */
    [[nodiscard]] HydroCouple::Spatial::IEnvelope *envelope() const override
    {
        return const_cast<Spatial::EnvelopeAdapter *>(&m_envelope);
    }

    /*!
     * \brief Sets the bounding envelope the owning component computed.
     * \param[in] envelope The bounds envelope() reports from now on; copied.
     */
    void setEnvelope(const Spatial::Envelope &envelope)
    {
        m_envelope.setEnvelope(envelope);
    }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The item's shape (IComponentDataItem::shape()).
     * \returns { timeCount(), geometryCount() }.
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
     * \param[in]  start       { first time step, first geometry }.
     * \param[in]  count       { time steps, geometries } to copy.
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
     * addTimeStep()); on success emits the value-changed signal for the
     * block.
     *
     * \param[in]  source  Buffer of dataKind() holding the block.
     * \param[in]  start   { first time step, first geometry }.
     * \param[in]  count   { time steps, geometries } to write.
     * \param[out] message Receives the reason on failure; may be nullptr.
     * \returns False, with nothing written or signaled, when the selection
     *          or the buffer does not fit.
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
    HydroCouple::Spatial::IGeometry::GeometryType  m_geometryType; //!< The declared geometry type.
    std::vector<HydroCouple::Spatial::IGeometry *> m_geometries;   //!< Borrowed geometries, one per element.
    Dimension                m_timeDimension;                      //!< Dimension 0, time.
    Dimension                m_geometryDimension;                  //!< Dimension 1, the geometries.
    Spatial::EnvelopeAdapter m_envelope;                           //!< The envelope set by setEnvelope().
};

//! Integer-valued time-geometry item, exported from the library.
using TimeGeometryComponentDataItemInt    = TimeGeometryComponentDataItem<int>;
//! Double-valued time-geometry item, exported from the library.
using TimeGeometryComponentDataItemDouble = TimeGeometryComponentDataItem<double>;

extern template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<int>;
extern template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<double>;

} // namespace HydroCouple::SDK::SpatioTemporal

#endif // HYDROCOUPLESDK_TIMEGEOMETRYCOMPONENTDATAITEM_H
