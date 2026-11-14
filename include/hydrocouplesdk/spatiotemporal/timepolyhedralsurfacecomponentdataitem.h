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
 * location() (patch/cell, edge, or vertex) is dimension 1 —
 * shape() = { timeCount, entityCount }. The IPolyhedralSurface/ITIN
 * association is borrowed from the owning component. The three entity
 * dimension accessors return the dimension the values are attached to and
 * nullptr for the others, mirroring the storage truthfully.
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
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class TimeSeriesPolyhedralSurfaceComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::
          ITimeSeriesPolyhedralSurfaceComponentDataItem
{
    using TimeStore = HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>;
    using MeshLocation = HydroCouple::Spatial::MeshLocation;

public:
    /*!
     * \brief Construct over a borrowed surface.
     * \param[in] id          Item identifier.
     * \param[in] surface     The polyhedral surface (borrowed; may be
     *                        nullptr when topology lives elsewhere).
     * \param[in] objectType  Mesh entity the values attach to
     *                        (Cell, Edge, or Vertex).
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

    ~TimeSeriesPolyhedralSurfaceComponentDataItem() override = default;

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

    // ── IPolyhedralSurfaceComponentDataItem ───────────────────────────────

    [[nodiscard]] MeshLocation location() const override
    {
        return m_objectType;
    }

    [[nodiscard]] HydroCouple::Spatial::SpatialDataType
    meshDataType() const override
    {
        return m_dataType;
    }

    [[nodiscard]] HydroCouple::Spatial::IPolyhedralSurface *
    polyhedralSurface() const override
    {
        return m_surface;
    }

    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return const_cast<Dimension *>(&m_entityDimension);
    }

    [[nodiscard]] HydroCouple::Spatial::VectorBasis vectorBasis() const override
    {
        return m_vectorBasis;
    }

    //! Declares the frame of vector/tensor values (Unknown for scalars).
    void setVectorBasis(HydroCouple::Spatial::VectorBasis basis)
    {
        m_vectorBasis = basis;
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
    [[nodiscard]] static const char *
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

    HydroCouple::Spatial::IPolyhedralSurface *m_surface;
    MeshLocation                        m_objectType;
    HydroCouple::Spatial::SpatialDataType     m_dataType;
    HydroCouple::Spatial::VectorBasis         m_vectorBasis = HydroCouple::Spatial::VectorBasis::Unknown;
    Dimension m_timeDimension;
    Dimension m_entityDimension;
};

/*!
 * \brief TIN specialization: the surface is an ITIN.
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

    ~TimeSeriesTINComponentDataItem() override = default;

    [[nodiscard]] HydroCouple::Spatial::ITIN *TIN() const override
    {
        return m_tin;
    }

private:
    HydroCouple::Spatial::ITIN *m_tin;
};

using TimeSeriesPolyhedralSurfaceComponentDataItemDouble =
    TimeSeriesPolyhedralSurfaceComponentDataItem<double>;
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
