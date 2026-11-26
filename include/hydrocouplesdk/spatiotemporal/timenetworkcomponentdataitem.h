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
 * dimension 0, the entity dimension selected by location()
 * (edge or node) is dimension 1 — shape() = { timeCount, entityCount }.
 * The INetwork association is borrowed from the owning component; the
 * item owns values and dimensions.
 *
 * NetworkAdapter (meshadapters.h) is the SDK's INetwork implementation to
 * borrow.
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
 *
 * The item starts with no time steps; the owning component appends them
 * with addTimeStep() (from TimeSeriesComponentDataItem), entityCount values
 * each, time-major: value (t, e) is at t·entityCount + e. The item is always
 * rank 2: it does not add the trailing Component dimension the standard
 * calls for when networkDataType() is not Scalar, so vector values must be
 * packed by the caller. The entity count is not checked against the
 * network's vertex or edge count.
 *
 * Like the other spatiotemporal items it derives from
 * AbstractComponentDataItem, not AbstractOutput, so it cannot be added
 * with addOutput(). The int and double instantiations are exported from
 * the library.
 *
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class TimeNetworkComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::SpatioTemporal::ITimeNetworkComponentDataItem
{
    using TimeStore = HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>; //!< The time-series value store.

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
     *
     * The entity dimension is named "edges" when \p objectType is Edge and
     * "nodes" otherwise (role Entity either way); the time dimension is
     * "time".
     */
    TimeNetworkComponentDataItem(
        std::string_view id, HydroCouple::Spatial::INetwork *network,
        HydroCouple::Spatial::MeshLocation objectType,
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
              objectType == HydroCouple::Spatial::MeshLocation::Edge
                  ? "edges"
                  : "nodes",
              "Network entity dimension")
    {
    }

    /*!
     * Destroys the item and its values; the network is not owned.
     */
    ~TimeNetworkComponentDataItem() override = default;

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

    // ── INetworkComponentDataItem ─────────────────────────────────────────

    /*!
     * \brief The network (INetworkComponentDataItem::network()).
     * \returns The borrowed network; nullptr when none was given. The
     *          connection validator reads the reference system from its first
     *          vertex.
     */
    [[nodiscard]] HydroCouple::Spatial::INetwork *network() const override
    {
        return m_network;
    }

    [[nodiscard]] HydroCouple::Spatial::MeshLocation
    /*!
     * \brief Which network entity the values sit on (INetworkComponentDataItem::location()).
     * \returns The location given at construction (Node or Edge).
     */
    location() const override
    {
        return m_objectType;
    }

    [[nodiscard]] HydroCouple::Spatial::SpatialDataType
    /*!
     * \brief The structure of each value (INetworkComponentDataItem::networkDataType()).
     * \returns The data type given at construction.
     */
    networkDataType() const override
    {
        return m_dataType;
    }

    /*!
     * \brief The entity axis (INetworkComponentDataItem::entityDimension()).
     * \returns Dimension 1 ("edges" or "nodes"), never nullptr.
     */
    [[nodiscard]] HydroCouple::IDimension *entityDimension() const override
    {
        return const_cast<Dimension *>(&m_entityDimension);
    }

    /*!
     * \brief The frame of vector values (INetworkComponentDataItem::vectorBasis()).
     * \returns The basis set with setVectorBasis(); VectorBasis::Unknown by
     *          default (AlongEntity is the usual basis for edge values).
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
    HydroCouple::Spatial::INetwork               *m_network; //!< Borrowed network; may be nullptr.
    HydroCouple::Spatial::MeshLocation   m_objectType; //!< Where the values sit.
    HydroCouple::Spatial::SpatialDataType         m_dataType; //!< Structure of each value.
    HydroCouple::Spatial::VectorBasis m_vectorBasis = HydroCouple::Spatial::VectorBasis::Unknown; //!< Frame of vector values.
    Dimension m_timeDimension; //!< Dimension 0, time.
    Dimension m_entityDimension; //!< Dimension 1, the entities.
};

//! Integer-valued time-network item, exported from the library.
using TimeNetworkComponentDataItemInt    = TimeNetworkComponentDataItem<int>;
//! Double-valued time-network item, exported from the library.
using TimeNetworkComponentDataItemDouble = TimeNetworkComponentDataItem<double>;

extern template class HYDROCOUPLESDK_EXPORT TimeNetworkComponentDataItem<int>;
extern template class HYDROCOUPLESDK_EXPORT TimeNetworkComponentDataItem<double>;

} // namespace HydroCouple::SDK::SpatioTemporal

#endif // HYDROCOUPLESDK_TIMENETWORKCOMPONENTDATAITEM_H
