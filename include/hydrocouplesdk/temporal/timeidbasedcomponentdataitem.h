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
 * \tparam T Element type (int, double, std::string).
 */
template <typename T>
class TimeIdBasedComponentDataItem
    : public HydroCouple::SDK::AbstractComponentDataItem,
      public TimeSeriesComponentDataItem<T>,
      public virtual HydroCouple::Temporal::ITimeIdBasedComponentDataItem
{
    using TimeStore = TimeSeriesComponentDataItem<T>;

public:
    /*!
     * \brief Construct with fixed identifiers (dimension 1 order).
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

    ~TimeIdBasedComponentDataItem() override = default;

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

    // ── ITimeIdBasedComponentDataItem ─────────────────────────────────────

    [[nodiscard]] std::vector<std::string> identifiers() const override
    {
        return m_identifiers;
    }

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
    std::vector<std::string> m_identifiers;
    Dimension m_timeDimension;
    Dimension m_identifierDimension;
};

using TimeIdBasedComponentDataItemInt    = TimeIdBasedComponentDataItem<int>;
using TimeIdBasedComponentDataItemDouble = TimeIdBasedComponentDataItem<double>;
using TimeIdBasedComponentDataItemString =
    TimeIdBasedComponentDataItem<std::string>;

extern template class HYDROCOUPLESDK_EXPORT TimeIdBasedComponentDataItem<int>;
extern template class HYDROCOUPLESDK_EXPORT
    TimeIdBasedComponentDataItem<double>;
extern template class HYDROCOUPLESDK_EXPORT
    TimeIdBasedComponentDataItem<std::string>;

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMEIDBASEDCOMPONENTDATAITEM_H
