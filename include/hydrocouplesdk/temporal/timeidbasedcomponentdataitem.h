// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timeidbasedcomponentdataitem.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  TimeIdBasedComponentDataItem — identifier-keyed values varying
 *         in time (plan §7a).
 *
 * Implements the standard's ITimeIdBasedComponentDataItem: time is
 * dimension 0 and the identifier dimension is dimension 1 —
 * shape() = { timeCount, identifierCount } over the time-series storage
 * mixin, with identifiers fixed at construction.
 *
 * \threadsafety Not thread-safe on its own.
 */

#ifndef HYDROCOUPLESDK_TIMEIDBASEDCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_TIMEIDBASEDCOMPONENTDATAITEM_H

#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouplesdk/temporal/timeseriescomponentdataitem.h"
#include "hydrocouplesdk/core/dimension.h"

#include <string>
#include <vector>

namespace HydroCouple::SDK::Temporal {

/*!
 * \brief Typed time-and-identifier data item ({ time, id } storage).
 *
 * \details A record of one value per identifier per time step: a station
 * series, a per-reach history. It is a plain data item, neither input nor
 * output, for a record that other code reads through the data plane (a
 * state item, a store behind a TimeSliceOutput). The element count per time
 * step is the number of identifiers, fixed
 * at construction; append rows with addTimeStep() (one value per
 * identifier). It owns its two dimensions: "time" (role Time) and
 * "identifiers" (role Entity). The temporal declarations default to
 * Unknown; set them with setTimeKind(), setTimeInterpolation() and
 * setTimeExtrapolation().
 *
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class TimeIdBasedComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::Temporal::ITimeIdBasedComponentDataItem
{
    //! The storage mixin, named for the delegating overrides below.
    using TimeStore = TimeSeriesComponentDataItem<T>;

public:
    /*!
     * \brief Construct with fixed identifiers (dimension 1 order).
     * \param id          Item identifier.
     * \param identifiers Entity identifiers; their count is the element size.
     * \param valueDef    Value definition (borrowed; may be nullptr).
     * \param mc          Owning component (borrowed; may be nullptr).
     */
    TimeIdBasedComponentDataItem(std::string_view id,
                                 std::vector<std::string> identifiers,
                                 ValueDefinition *valueDef = nullptr,
                                 HydroCouple::IModelComponent *mc = nullptr)
        : AbstractComponentDataItem(
              id, {&m_timeDimension, &m_identifierDimension}, valueDef, mc),
          TimeStore(/*expectedTimeSteps=*/100,
                    static_cast<int>(identifiers.size())),
          m_identifiers(std::move(identifiers)),
          m_timeDimension("time", "Time dimension"),
          m_identifierDimension("identifiers", "Identifier dimension")
    {
    }

    //! Destroys the item and its dimensions.
    ~TimeIdBasedComponentDataItem() override = default;

    // ── ITimeSeriesComponentDataItem ──────────────────────────────────────

    //! The instant of a time step.
    //! \param[in] timeIndex Zero-based time step.
    //! \return The recorded IDateTime (non-owning), or nullptr out of range.
    [[nodiscard]] const HydroCouple::Temporal::IDateTime *
    time(int64_t timeIndex) const override
    {
        return TimeStore::time(timeIndex);
    }

    //! Number of recorded time steps.
    //! \return shape()[0].
    [[nodiscard]] int64_t timeCount() const override
    {
        return TimeStore::timeCount();
    }

    //! All recorded instants as Julian days.
    //! \return A span valid until the next time step is added.
    [[nodiscard]] std::span<const double> times() const override
    {
        return TimeStore::times();
    }

    //! The first-to-last span of the recorded instants.
    //! \return A pointer owned by the item and replaced by every addTimeStep().
    [[nodiscard]] HydroCouple::Temporal::ITimeSpan *timeSpan() const override
    {
        return TimeStore::timeSpan();
    }

    //! The time axis (dimension 0), owned by the item.
    //! \return The member dimension "time".
    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override
    {
        return const_cast<Dimension *>(&m_timeDimension);
    }

    //! What each value's time coordinate refers to.
    //! \return The declared kind; Unknown by default.
    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeKind();
    }

    //! Declared interval length in days.
    //! \return 0 unless declared.
    [[nodiscard]] double intervalLength() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::intervalLength();
    }

    //! Declared interpolation policy.
    //! \return Unknown unless setTimeInterpolation() was called.
    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeInterpolation();
    }

    //! Declared extrapolation policy.
    //! \return Unknown unless setTimeExtrapolation() was called.
    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const override
    {
        return HydroCouple::SDK::Temporal::TimeSeriesComponentDataItem<T>::timeExtrapolation();
    }

    // ── ITimeIdBasedComponentDataItem ─────────────────────────────────────

    //! The entity identifiers, in dimension-1 order.
    //! \return A copy of the list given at construction.
    [[nodiscard]] std::vector<std::string> identifiers() const override
    {
        return m_identifiers;
    }

    //! The identifier axis (dimension 1), owned by the item.
    //! \return The member dimension "identifiers".
    [[nodiscard]] HydroCouple::IDimension *identifierDimension() const override
    {
        return const_cast<Dimension *>(&m_identifierDimension);
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
    std::vector<std::string> m_identifiers;   //!< Identifiers, fixed at construction.
    Dimension m_timeDimension;                //!< Dimension 0: "time", role Time.
    Dimension m_identifierDimension;          //!< Dimension 1: "identifiers", role Entity.
};

using TimeIdBasedComponentDataItemInt    = TimeIdBasedComponentDataItem<int>;    //!< Time-and-id item of \c int.
using TimeIdBasedComponentDataItemDouble = TimeIdBasedComponentDataItem<double>; //!< Time-and-id item of \c double.
//! Time-and-id item of \c std::string.
using TimeIdBasedComponentDataItemString =
    TimeIdBasedComponentDataItem<std::string>;

extern template class HYDROCOUPLESDK_EXPORT TimeIdBasedComponentDataItem<int>;
extern template class HYDROCOUPLESDK_EXPORT
    TimeIdBasedComponentDataItem<double>;
extern template class HYDROCOUPLESDK_EXPORT
    TimeIdBasedComponentDataItem<std::string>;

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMEIDBASEDCOMPONENTDATAITEM_H
