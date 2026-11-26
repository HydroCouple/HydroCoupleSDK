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
 * The input records history: every retrieve adds a row, whether or not the
 * provider has a new time step, so retrieving twice at one provider time
 * stores that time twice. The time dimension is the first entry of the
 * dimension list; give it the id "time" so that its role is Time.
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
     * \details Declares TimeInterpolation::None and TimeExtrapolation::Refuse.
     * \param id             Input identifier, unique within the component.
     * \param dimensions     Time axis first, then the element axis if any
     *                       (borrowed; the list is copied).
     * \param valueDef       Value definition (borrowed; may be nullptr).
     * \param modelComponent Owning component (borrowed; may be nullptr).
     * \param elementSize    Values per time step.
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

    //! The instant of a time step.
    //! \param[in] timeIndex Zero-based time step.
    //! \return The recorded IDateTime (non-owning), or nullptr out of range.
    [[nodiscard]] const IDateTime *time(int64_t timeIndex) const override
    {
        return TimeSeriesComponentDataItem<T>::time(timeIndex);
    }

    //! Number of recorded time steps.
    //! \return shape()[0].
    [[nodiscard]] int64_t timeCount() const override
    {
        return TimeSeriesComponentDataItem<T>::timeCount();
    }

    //! All recorded instants as Julian days.
    //! \return A span valid until the next time step is added.
    [[nodiscard]] std::span<const double> times() const override
    {
        return TimeSeriesComponentDataItem<T>::times();
    }

    //! The first-to-last span of the recorded instants.
    //! \return A pointer owned by the item and replaced by every addTimeStep().
    [[nodiscard]] ITimeSpan *timeSpan() const override
    {
        return TimeSeriesComponentDataItem<T>::timeSpan();
    }

    //! The time axis: the first dimension given at construction.
    //! \return That dimension (borrowed), or nullptr when the list was empty.
    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override
    {
        return m_timeDim;
    }

    //! What each value's time coordinate refers to.
    //! \return The declared kind; Unknown unless setTimeKind() was called.
    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override
    {
        return TimeSeriesComponentDataItem<T>::timeKind();
    }

    //! Declared interval length in days.
    //! \return 0 unless setTimeKind() declared one.
    [[nodiscard]] double intervalLength() const override
    {
        return TimeSeriesComponentDataItem<T>::intervalLength();
    }

    //! Declared interpolation policy (None from the constructor).
    //! \return The policy.
    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override
    {
        return TimeSeriesComponentDataItem<T>::timeInterpolation();
    }

    //! Declared extrapolation policy (Refuse from the constructor).
    //! \return The policy.
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
     * \details The provider must implement ITimeSeriesComponentDataItem and
     *          have shape { times, elementSize() }. Temporal declarations are
     *          not compared here.
     * \param[in]  provider Candidate output.
     * \param[out] message  "Provider is null", "Provider is not a
     *                      time-series data item", "Provider data kind
     *                      mismatch" or "Provider element count mismatch".
     * \return true when the provider fits.
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
     * \details Reads row timeCount() - 1 of the provider, appends it with the
     *          provider's Julian day (a new TimeData named "Time") and fires
     *          value-changed for the new row. Returns without a change when
     *          unconnected, when the provider does not fit or holds no time
     *          step, or when the read fails.
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
    HydroCouple::SDK::Dimension *m_timeDim; //!< Time axis (borrowed; may be nullptr).
};

using TimeSeriesInputInt    = TimeSeriesInput<int>;         //!< Time-series input of \c int.
using TimeSeriesInputDouble = TimeSeriesInput<double>;      //!< Time-series input of \c double.
using TimeSeriesInputString = TimeSeriesInput<std::string>; //!< Time-series input of \c std::string.

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
 * It holds every recorded time step and serves them by index; it declares
 * TimeInterpolation::None and TimeExtrapolation::Refuse because it never
 * computes a value between or beyond its instants. A consumer that needs a
 * value at its own time wires a Temporal::TemporalInterpolationAdaptedOutput
 * (offering "temporal_interpolation", single-column Float64 series only).
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
     * \details Declares TimeInterpolation::None and TimeExtrapolation::Refuse.
     * \param id             Output identifier, unique within the component.
     * \param dimensions     Time axis first, then the element axis if any
     *                       (borrowed; the list is copied).
     * \param valueDef       Value definition (borrowed; may be nullptr).
     * \param modelComponent Producing component (borrowed; may be nullptr).
     * \param elementSize    Values per time step.
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

    //! The instant of a time step.
    //! \param[in] timeIndex Zero-based time step.
    //! \return The recorded IDateTime (non-owning), or nullptr out of range.
    [[nodiscard]] const IDateTime *time(int64_t timeIndex) const override
    {
        return TimeSeriesComponentDataItem<T>::time(timeIndex);
    }

    //! Number of recorded time steps.
    //! \return shape()[0].
    [[nodiscard]] int64_t timeCount() const override
    {
        return TimeSeriesComponentDataItem<T>::timeCount();
    }

    //! All recorded instants as Julian days.
    //! \return A span valid until the next time step is added.
    [[nodiscard]] std::span<const double> times() const override
    {
        return TimeSeriesComponentDataItem<T>::times();
    }

    //! The first-to-last span of the recorded instants.
    //! \return A pointer owned by the item and replaced by every addTimeStep().
    [[nodiscard]] ITimeSpan *timeSpan() const override
    {
        return TimeSeriesComponentDataItem<T>::timeSpan();
    }

    //! The time axis: the first dimension given at construction.
    //! \return That dimension (borrowed), or nullptr when the list was empty.
    [[nodiscard]] HydroCouple::IDimension *timeDimension() const override
    {
        return m_timeDim;
    }

    //! What each value's time coordinate refers to.
    //! \return The declared kind; Unknown unless setTimeKind() was called.
    [[nodiscard]] HydroCouple::Temporal::TimeKind timeKind() const override
    {
        return TimeSeriesComponentDataItem<T>::timeKind();
    }

    //! Declared interval length in days.
    //! \return 0 unless setTimeKind() declared one.
    [[nodiscard]] double intervalLength() const override
    {
        return TimeSeriesComponentDataItem<T>::intervalLength();
    }

    //! Declared interpolation policy (None from the constructor).
    //! \return The policy.
    [[nodiscard]] HydroCouple::Temporal::TimeInterpolation timeInterpolation() const override
    {
        return TimeSeriesComponentDataItem<T>::timeInterpolation();
    }

    //! Declared extrapolation policy (Refuse from the constructor).
    //! \return The policy.
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

    //! No-op: the component appends time steps itself.
    void updateValues() override {}

private:
    HydroCouple::SDK::Dimension *m_timeDim; //!< Time axis (borrowed; may be nullptr).
};

using TimeSeriesOutputInt    = TimeSeriesOutput<int>;         //!< Time-series output of \c int.
using TimeSeriesOutputDouble = TimeSeriesOutput<double>;      //!< Time-series output of \c double.
using TimeSeriesOutputString = TimeSeriesOutput<std::string>; //!< Time-series output of \c std::string.

extern template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<int>;
extern template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<double>;
extern template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<std::string>;

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMESERIESEXCHANGEITEMS_H
