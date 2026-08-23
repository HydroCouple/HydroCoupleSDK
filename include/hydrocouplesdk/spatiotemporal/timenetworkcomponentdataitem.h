// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timenetworkcomponentdataitem.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  TimeNetworkComponentDataItem — network-entity values varying in
 *         time (plan §7a).
 *
 * Implements the standard's ITimeNetworkComponentDataItem: time is
 * dimension 0, the entity dimension selected by networkDataObjectType()
 * (edge or node) is dimension 1 — shape() = { timeCount, entityCount }.
 * The INetwork association is borrowed from the owning component; the
 * item owns values and dimensions.
 *
 * \threadsafety Not thread-safe on its own.
 */

#ifndef HYDROCOUPLESDK_TIMENETWORKCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_TIMENETWORKCOMPONENTDATAITEM_H

#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouplesdk/temporal/timeseriescomponentdataitem.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplespatiotemporal.h"

#include <string>
#include <vector>

namespace HydroCouple::SDK::SpatioTemporal {

/*!
 * \brief Typed time-network data item ({ time, entity } storage).
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class TimeNetworkComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::ITimeNetworkComponentDataItem
{
    using TimeStore = HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>;

public:
    /*!
     * \brief Construct over a borrowed network.
     * \param[in] id             Item identifier.
     * \param[in] network        The network (borrowed; may be nullptr when
     *                           the component keeps topology elsewhere).
     * \param[in] objectType     Entity kind the values attach to
     *                           (Node or Edge).
     * \param[in] dataType       Nature of each value (Scalar, Vector, …).
     * \param[in] entityCount    Entities along dimension 1.
     * \param[in] valueDef       Value definition (borrowed).
     * \param[in] mc             Owning component (borrowed).
     */
    TimeNetworkComponentDataItem(
        std::string_view id, HydroCouple::Spatial::INetwork *network,
        HydroCouple::Spatial::NetworkDataObjectType objectType,
        HydroCouple::Spatial::SpatialDataType dataType, int entityCount,
        ValueDefinition *valueDef = nullptr,
        HydroCouple::IModelComponent *mc = nullptr)
        : AbstractComponentDataItem(
              id, {&m_timeDimension, &m_entityDimension}, valueDef, mc),
          TimeStore(/*expectedTimeSteps=*/100, entityCount),
          m_network(network),
          m_objectType(objectType),
          m_dataType(dataType),
          m_timeDimension("time", "Time dimension"),
          m_entityDimension(
              objectType == HydroCouple::Spatial::NetworkDataObjectType::Edge
                  ? "edges"
                  : "nodes",
              "Network entity dimension")
    {
    }

    ~TimeNetworkComponentDataItem() override = default;

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

    // ── INetworkComponentDataItem ─────────────────────────────────────────

    [[nodiscard]] HydroCouple::Spatial::INetwork *network() const override
    {
        return m_network;
    }

    [[nodiscard]] HydroCouple::Spatial::NetworkDataObjectType
    networkDataObjectType() const override
    {
        return m_objectType;
    }

    [[nodiscard]] HydroCouple::Spatial::SpatialDataType
    networkDataType() const override
    {
        return m_dataType;
    }

    [[nodiscard]] HydroCouple::IDimension *edgeDimension() const override
    {
        return m_objectType ==
                       HydroCouple::Spatial::NetworkDataObjectType::Edge
                   ? const_cast<Dimension *>(&m_entityDimension)
                   : nullptr;
    }

    [[nodiscard]] HydroCouple::IDimension *vertexDimension() const override
    {
        return m_objectType ==
                       HydroCouple::Spatial::NetworkDataObjectType::Node
                   ? const_cast<Dimension *>(&m_entityDimension)
                   : nullptr;
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
    HydroCouple::Spatial::INetwork               *m_network;
    HydroCouple::Spatial::NetworkDataObjectType   m_objectType;
    HydroCouple::Spatial::SpatialDataType         m_dataType;
    Dimension m_timeDimension;
    Dimension m_entityDimension;
};

using TimeNetworkComponentDataItemInt    = TimeNetworkComponentDataItem<int>;
using TimeNetworkComponentDataItemDouble = TimeNetworkComponentDataItem<double>;

extern template class HYDROCOUPLESDK_EXPORT TimeNetworkComponentDataItem<int>;
extern template class HYDROCOUPLESDK_EXPORT TimeNetworkComponentDataItem<double>;

} // namespace HydroCouple::SDK::SpatioTemporal

#endif // HYDROCOUPLESDK_TIMENETWORKCOMPONENTDATAITEM_H
