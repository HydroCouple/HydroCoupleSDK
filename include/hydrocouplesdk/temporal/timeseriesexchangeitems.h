// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timeseriesexchangeitems.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Time-series exchange items (Input/Output) for temporal data flow.
 *
 * TimeSeriesInput<T> / TimeSeriesOutput<T> combine AbstractInput /
 * AbstractOutput with TimeSeriesComponentDataItem<T> and implement both
 * ITimeSeriesComponentDataItem and the v2 typed hyperslab data plane
 * (time is dimension 0 of the item's shape).
 */

#ifndef HYDROCOUPLESDK_TIMESERIESEXCHANGEITEMS_H
#define HYDROCOUPLESDK_TIMESERIESEXCHANGEITEMS_H

#include "hydrocouplesdk/data/abstractinput.h"
#include "hydrocouplesdk/data/abstractoutput.h"
#include "hydrocouplesdk/core/dimension.h"
#include "timeseriescomponentdataitem.h"

#include <string>
#include <vector>

#include "hydrocoupletemporal.h"

namespace HydroCouple::SDK::Temporal {

// Interface types from the standard's Temporal namespace.
using namespace HydroCouple::Temporal;

// ── TimeSeriesInput<T> ────────────────────────────────────────────────────

/*!
 * \brief Time-series input exchange item.
 *
 * Receives time-varying data from provider components. The default
 * retrieveValuesFromProvider() appends the provider's most recent time
 * step to local storage when the provider is itself a time-series item of
 * matching DataKind and element count.
 *
 * \tparam T Scalar value type (int, double, std::string, …).
 */
template <typename T>
class TimeSeriesInput
    : public HydroCouple::SDK::AbstractInput,
      public TimeSeriesComponentDataItem<T>,
      public virtual ITimeSeriesComponentDataItem
{
public:
    /*!
     * \brief Construct with identifier, dimensions (time axis first),
     *        value definition, and owning component.
     */
    TimeSeriesInput(std::string_view id,
                    const std::vector<HydroCouple::SDK::Dimension *> &dimensions,
                    HydroCouple::SDK::ValueDefinition *valueDef,
                    HydroCouple::IModelComponent *modelComponent,
                    int elementSize = 1)
        : AbstractInput(id, dimensions, valueDef, modelComponent),
          TimeSeriesComponentDataItem<T>(100, elementSize),
          m_timeDim(dimensions.empty() ? nullptr : dimensions[0])
    {
        // retrieveValuesFromProvider() takes the provider's latest held
        // instant as it is, so this input needs no value fabricated between
        // or beyond the provider's instants. Subclasses that query at their
        // own times should declare what they accept instead.
        this->setTimeInterpolation(HydroCouple::Temporal::TimeInterpolation::None);
        this->setTimeExtrapolation(HydroCouple::Temporal::TimeExtrapolation::Refuse);
    }

    ~TimeSeriesInput() override = default;

    // ── ITimeSeriesComponentDataItem ─────────────────────────────────────

    [[nodiscard]] const IDateTime *time(int64_t timeIndex) const override
    {
        return TimeSeriesComponentDataItem<T>::time(timeIndex);
    }

    [[nodiscard]] int64_t timeCount() const override
    {
        return TimeSeriesComponentDataItem<T>::timeCount();
    }

    [[nodiscard]] std::span<const double> times() const override
    {
        return TimeSeriesComponentDataItem<T>::times();
    }

    [[nodiscard]] ITimeSpan *timeSpan() const override
    {
        return TimeSeriesComponentDataItem<T>::timeSpan();
    }

    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override
    {
        return m_timeDim;
    }

    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override
    {
        return TimeSeriesComponentDataItem<T>::timeKind();
    }

    [[nodiscard]] double intervalLength() const override
    {
        return TimeSeriesComponentDataItem<T>::intervalLength();
    }

    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override
    {
        return TimeSeriesComponentDataItem<T>::timeInterpolation();
    }

    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const override
    {
        return TimeSeriesComponentDataItem<T>::timeExtrapolation();
    }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return TimeSeriesComponentDataItem<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return TimeSeriesComponentDataItem<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return TimeSeriesComponentDataItem<T>::getSlab(destination, start,
                                                       count, message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!TimeSeriesComponentDataItem<T>::setSlab(source, start, count,
                                                     message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── IInput ────────────────────────────────────────────────────────────

    /*!
     * \brief Accepts any time-series provider of the same DataKind and
     *        element count.
     */
    [[nodiscard]] bool canConsume(HydroCouple::IOutput *provider,
                                  std::string &message) const override
    {
        if (!provider)
        {
            message = "Provider is null";
            return false;
        }
        const auto *series =
            dynamic_cast<const ITimeSeriesComponentDataItem *>(provider);
        if (!series)
        {
            message = "Provider is not a time-series data item";
            return false;
        }
        if (provider->dataKind() != TimeSeriesComponentDataItem<T>::storageKind())
        {
            message = "Provider data kind mismatch";
            return false;
        }
        const auto srcShape = provider->shape();
        if (srcShape.size() != 2 ||
            srcShape[1] != this->elementSize())
        {
            message = "Provider element count mismatch";
            return false;
        }
        return true;
    }

    /*!
     * \brief Appends the provider's most recent time step to local storage.
     */
    void retrieveValuesFromProvider() override
    {
        HydroCouple::IOutput *p = provider();
        const auto *series = dynamic_cast<const ITimeSeriesComponentDataItem *>(p);
        if (!p || !series ||
            p->dataKind() != TimeSeriesComponentDataItem<T>::storageKind())
            return;
        const auto srcShape = p->shape();
        if (srcShape.size() != 2 || srcShape[1] != this->elementSize() ||
            srcShape[0] == 0)
            return;

        const int64_t lastTime = srcShape[0] - 1;
        const int64_t n        = srcShape[1];
        std::vector<T> row(static_cast<size_t>(n));
        const std::array<int64_t, 2> count{1, n};
        const BufferDescriptor dest = HydroCouple::SDK::detail::hostDescriptor(
            row.data(), TimeSeriesComponentDataItem<T>::storageKind(),
            std::span<const int64_t>(count.data(), 2));
        const std::array<int64_t, 2> start{lastTime, 0};
        std::string message;
        if (!p->getValuesInto(dest, std::span<const int64_t>(start.data(), 2),
                              std::span<const int64_t>(count.data(), 2),
                              &message))
            return;

        const double jd = series->times()[static_cast<size_t>(lastTime)];
        this->addTimeStep(std::make_shared<TimeData>("Time", jd), row);
        emitValueChanged({this->timeCount() - 1, 0}, {1, n});
    }

    /*!
     * \brief Hook for subclasses to apply retrieved values to model state.
     */
    void applyData() override {}

private:
    HydroCouple::SDK::Dimension *m_timeDim;
};

using TimeSeriesInputInt    = TimeSeriesInput<int>;
using TimeSeriesInputDouble = TimeSeriesInput<double>;
using TimeSeriesInputString = TimeSeriesInput<std::string>;

extern template class HYDROCOUPLESDK_EXPORT TimeSeriesInput<int>;
extern template class HYDROCOUPLESDK_EXPORT TimeSeriesInput<double>;
extern template class HYDROCOUPLESDK_EXPORT TimeSeriesInput<std::string>;


// ── TimeSeriesOutput<T> ───────────────────────────────────────────────────

/*!
 * \brief Time-series output exchange item.
 *
 * Provides time-varying data to consumer components. Model components
 * append time steps via addTimeStep() (or override updateValues()).
 *
 * \tparam T Scalar value type (int, double, std::string, …).
 */
template <typename T>
class TimeSeriesOutput
    : public HydroCouple::SDK::AbstractOutput,
      public TimeSeriesComponentDataItem<T>,
      public virtual ITimeSeriesComponentDataItem
{
public:
    /*!
     * \brief Construct with identifier, dimensions (time axis first),
     *        value definition, and owning component.
     */
    TimeSeriesOutput(std::string_view id,
                     const std::vector<HydroCouple::SDK::Dimension *> &dimensions,
                     HydroCouple::SDK::ValueDefinition *valueDef,
                     HydroCouple::IModelComponent *modelComponent,
                     int elementSize = 1)
        : AbstractOutput(id, dimensions, valueDef, modelComponent),
          TimeSeriesComponentDataItem<T>(100, elementSize),
          m_timeDim(dimensions.empty() ? nullptr : dimensions[0])
    {
        // The data plane serves held instants by index and nothing else: no
        // value is made up between two instants or past the last one. A
        // consumer that needs that wires a TemporalInterpolationAdaptedOutput.
        this->setTimeInterpolation(HydroCouple::Temporal::TimeInterpolation::None);
        this->setTimeExtrapolation(HydroCouple::Temporal::TimeExtrapolation::Refuse);
    }

    ~TimeSeriesOutput() override = default;

    // ── ITimeSeriesComponentDataItem ─────────────────────────────────────

    [[nodiscard]] const IDateTime *time(int64_t timeIndex) const override
    {
        return TimeSeriesComponentDataItem<T>::time(timeIndex);
    }

    [[nodiscard]] int64_t timeCount() const override
    {
        return TimeSeriesComponentDataItem<T>::timeCount();
    }

    [[nodiscard]] std::span<const double> times() const override
    {
        return TimeSeriesComponentDataItem<T>::times();
    }

    [[nodiscard]] ITimeSpan *timeSpan() const override
    {
        return TimeSeriesComponentDataItem<T>::timeSpan();
    }

    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override
    {
        return m_timeDim;
    }

    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override
    {
        return TimeSeriesComponentDataItem<T>::timeKind();
    }

    [[nodiscard]] double intervalLength() const override
    {
        return TimeSeriesComponentDataItem<T>::intervalLength();
    }

    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override
    {
        return TimeSeriesComponentDataItem<T>::timeInterpolation();
    }

    [[nodiscard]] HydroCouple::Temporal::TimeExtrapolation timeExtrapolation() const override
    {
        return TimeSeriesComponentDataItem<T>::timeExtrapolation();
    }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return TimeSeriesComponentDataItem<T>::storageShape();
    }

    [[nodiscard]] DataKind dataKind() const override
    {
        return TimeSeriesComponentDataItem<T>::storageKind();
    }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override
    {
        return TimeSeriesComponentDataItem<T>::getSlab(destination, start,
                                                       count, message);
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override
    {
        if (!TimeSeriesComponentDataItem<T>::setSlab(source, start, count,
                                                     message))
            return false;
        emitValueChanged({start.begin(), start.end()},
                         {count.begin(), count.end()});
        return true;
    }

    // ── IOutput ───────────────────────────────────────────────────────────

    void updateValues() override {}

private:
    HydroCouple::SDK::Dimension *m_timeDim;
};

using TimeSeriesOutputInt    = TimeSeriesOutput<int>;
using TimeSeriesOutputDouble = TimeSeriesOutput<double>;
using TimeSeriesOutputString = TimeSeriesOutput<std::string>;

extern template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<int>;
extern template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<double>;
extern template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<std::string>;

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESEXCHANGEITEMS_H
