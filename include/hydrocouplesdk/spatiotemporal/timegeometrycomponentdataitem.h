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
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class TimeGeometryComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem
{
    using TimeStore = HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>;

public:
    /*!
     * \brief Construct over a borrowed geometry list.
     * \param[in] id           Item identifier.
     * \param[in] geometryType Geometry type of every entry.
     * \param[in] geometries   Geometry per entity (borrowed; the owning
     *                         component keeps them alive).
     * \param[in] valueDef     Value definition (borrowed; may be nullptr).
     * \param[in] mc           Owning component (borrowed; may be nullptr).
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

    ~TimeGeometryComponentDataItem() override = default;

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

    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeKind();
    }

    [[nodiscard]] double intervalLength() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::intervalLength();
    }

    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeInterpolation();
    }

    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeExtrapolation();
    }

    // ── IGeometryComponentDataItem ────────────────────────────────────────

    [[nodiscard]] HydroCouple::Spatial::IGeometry::GeometryType
    geometryType() const override
    {
        return m_geometryType;
    }

    [[nodiscard]] int64_t geometryCount() const override
    {
        return static_cast<int64_t>(m_geometries.size());
    }

    [[nodiscard]] HydroCouple::Spatial::IGeometry *
    geometry(int64_t geometryIndex) const override
    {
        if (geometryIndex < 0 ||
            geometryIndex >= static_cast<int64_t>(m_geometries.size()))
            return nullptr;
        return m_geometries[static_cast<size_t>(geometryIndex)];
    }

    [[nodiscard]] HydroCouple::IDimension *geometryDimension() const override
    {
        return const_cast<Dimension *>(&m_geometryDimension);
    }

    [[nodiscard]] HydroCouple::Spatial::IEnvelope *envelope() const override
    {
        return const_cast<Spatial::EnvelopeAdapter *>(&m_envelope);
    }

    /*!
     * \brief Sets the bounding envelope the owning component computed.
     */
    void setEnvelope(const Spatial::Envelope &envelope)
    {
        m_envelope.setEnvelope(envelope);
    }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return TimeStore::storageShape();
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
        return TimeStore::getSlab(destination, start, count, message);
    }

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
    HydroCouple::Spatial::IGeometry::GeometryType  m_geometryType;
    std::vector<HydroCouple::Spatial::IGeometry *> m_geometries;
    Dimension                m_timeDimension;
    Dimension                m_geometryDimension;
    Spatial::EnvelopeAdapter m_envelope;
};

using TimeGeometryComponentDataItemInt    = TimeGeometryComponentDataItem<int>;
using TimeGeometryComponentDataItemDouble = TimeGeometryComponentDataItem<double>;

extern template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<int>;
extern template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<double>;

} // namespace HydroCouple::SDK::SpatioTemporal

#endif // HYDROCOUPLESDK_TIMEGEOMETRYCOMPONENTDATAITEM_H
