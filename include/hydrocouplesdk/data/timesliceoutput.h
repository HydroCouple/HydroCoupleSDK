// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timesliceoutput.h
 * \author Caleb Buahin
 * \version 2.1.0
 * \brief  TimeSliceOutput — presents one time level of a time-major item
 *         as a lower-rank output.
 *
 * A recorded run stores history: a variable shaped { time, entities… }.
 * A live component's input wants the present: { entities… }. This adapter
 * bridges the two, exposing time level \c timeIndex of a source item as an
 * output one rank lower, so a stored run can drive a new composition as a
 * boundary condition without every consumer learning about time.
 *
 * The slice is selected explicitly — setTimeIndex(), or advance() to walk
 * forward, or setTime() to seek the nearest recorded level. Nothing is
 * inferred from a global clock, because the SDK has none: the host (or a
 * workflow) decides which level is "now".
 *
 * The source item is borrowed and must outlive the adapter; values are
 * pulled from it on demand, so a recorded source stays lazily read.
 *
 * \threadsafety Not thread-safe.
 */

#ifndef HYDROCOUPLESDK_DATA_TIMESLICEOUTPUT_H
#define HYDROCOUPLESDK_DATA_TIMESLICEOUTPUT_H

#include "hydrocouplesdk/data/abstractoutput.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <memory>

#include <string>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief One time level of a time-major item, as a lower-rank output.
 */
class HYDROCOUPLESDK_EXPORT TimeSliceOutput final : public AbstractOutput
{
public:
    /*!
     * \brief Wraps \p source, exposing its leading (time) dimension one
     *        level at a time.
     *
     * \param[in] id        Identifier of the adapter (defaults to the
     *                      source's id so a consumer sees a familiar name).
     * \param[in] source    Time-major item, rank ≥ 2 (borrowed).
     * \param[in] component Owning component (borrowed; may be nullptr).
     */
    TimeSliceOutput(std::string_view id,
                    HydroCouple::IComponentDataItem *source,
                    HydroCouple::IModelComponent *component = nullptr);

    ~TimeSliceOutput() override;

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    TimeSliceOutput(const TimeSliceOutput &)            = delete;
    TimeSliceOutput &operator=(const TimeSliceOutput &) = delete;

    /*!
     * \brief The time level currently exposed.
     */
    [[nodiscard]] int64_t timeIndex() const noexcept;

    /*!
     * \brief Selects a time level; clamped to the recorded range.
     */
    void setTimeIndex(int64_t index);

    /*!
     * \brief Advances one level, stopping at the last.
     * \return False when already at the last level.
     */
    bool advance();

    /*!
     * \brief Number of time levels in the source.
     */
    [[nodiscard]] int64_t timeCount() const noexcept;

    /*!
     * \brief Selects the level nearest \p time, when the source exposes a
     *        time axis (ITimeSeriesComponentDataItem).
     * \return False when the source carries no time coordinates.
     */
    bool setTime(double time);

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The source's shape with the leading time dimension dropped.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override;

    [[nodiscard]] DataKind dataKind() const override;

    /*!
     * \brief Reads the selection from the current time level of the source.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override;

    /*!
     * \brief Refused: an adapter over a source is not a store.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── IOutput ───────────────────────────────────────────────────────────

    /*!
     * \brief Nothing to compute: the source holds the values.
     */
    void updateValues() override;

private:
    /*!
     * \brief Dimensions and value definition built before the base class
     *        is constructed (which needs them) and owned after.
     */
    struct Prepared
    {
        std::vector<std::unique_ptr<Dimension>> ownedDimensions;
        std::unique_ptr<ValueDefinition>        valueDefinition;
        std::vector<Dimension *>                dimensions;
    };

    [[nodiscard]] static Prepared prepare(
        std::string_view id, HydroCouple::IComponentDataItem *source);

    TimeSliceOutput(std::string_view id,
                    HydroCouple::IComponentDataItem *source,
                    HydroCouple::IModelComponent *component,
                    Prepared prepared);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_DATA_TIMESLICEOUTPUT_H
