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
 *
 * \details It is an output (AbstractOutput), not an adapted output: it has no
 * factory and no arguments, and any item can be its source, not only an
 * IOutput. Its dimensions are copies of the source's dimensions after the
 * first, named by their ids (one "element" dimension when the source has no
 * others), and it owns them. Its value definition is a new unitless
 * Quantity named `<id>_slice`; the source's unit is not carried over, so a
 * consumer that checks units sees "unitless".
 *
 * The time index starts at 0. Reads are forwarded to the source with the
 * time index prepended, so they fail exactly when the source's read fails
 * (for example when the source has no time levels yet).
 */
class HYDROCOUPLESDK_EXPORT TimeSliceOutput final : public AbstractOutput
{
public:
    /*!
     * \brief Wraps \p source, exposing its leading (time) dimension one
     *        level at a time.
     *
     * \details There is no default for \p id: pass the source's id to give a
     *          consumer a familiar name, or a new one.
     *
     * \param[in] id        Identifier of the adapter (defaults to the
     *                      source's id so a consumer sees a familiar name).
     * \param[in] source    Time-major item, rank ≥ 2 (borrowed).
     * \param[in] component Owning component (borrowed; may be nullptr).
     *
     * \details \p source must not be nullptr: shape(), dataKind() and reads
     *          dereference it. A rank-1 source is accepted but yields a
     *          rank-0 shape with one "element" dimension.
     */
    TimeSliceOutput(std::string_view id,
                    HydroCouple::IComponentDataItem *source,
                    HydroCouple::IModelComponent *component = nullptr);

    //! Destroys the slice and the dimensions and value definition it owns;
    //! the source is not touched.
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
     * \return Zero-based index into the source's dimension 0; 0 initially.
     */
    [[nodiscard]] int64_t timeIndex() const noexcept;

    /*!
     * \brief Selects a time level; clamped to the recorded range.
     * \details Clamped to [0, timeCount() - 1], or to 0 when the source has
     *          no levels. No value-changed event is fired.
     * \param[in] index Requested zero-based time level.
     */
    void setTimeIndex(int64_t index);

    /*!
     * \brief Advances one level, stopping at the last.
     * \return False when already at the last level.
     */
    bool advance();

    /*!
     * \brief Number of time levels in the source.
     * \return The source's current extent in dimension 0, or 0 for a
     *         rank-0 source. A growing source reports its latest extent.
     */
    [[nodiscard]] int64_t timeCount() const noexcept;

    /*!
     * \brief Selects the level nearest \p time, when the source exposes a
     *        time axis (ITimeSeriesComponentDataItem).
     * \details Searches the source's times() for the smallest absolute
     *          difference; on a tie the earlier level wins. There is no
     *          tolerance and no range check: a time far outside the record
     *          selects the first or last level.
     * \param[in] time Instant to seek, as a Julian day (the units of the
     *                 source's times()).
     * \return False when the source carries no time coordinates.
     */
    bool setTime(double time);

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The source's shape with the leading time dimension dropped.
     * \return The source's current shape without its first entry.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override;

    /*!
     * \brief The source's element type.
     * \return source->dataKind().
     */
    [[nodiscard]] DataKind dataKind() const override;

    /*!
     * \brief Reads the selection from the current time level of the source.
     * \details Calls the source's getValuesInto() with start
     *          {timeIndex(), start...} and count {1, count...}. The
     *          destination's element count must therefore equal the product
     *          of \p count; its rank is free.
     * \param[in]  destination Caller's buffer.
     * \param[in]  start       First index in each slice dimension.
     * \param[in]  count       Extent in each slice dimension.
     * \param[out] message     The source's failure message; may be nullptr.
     * \return The source's result.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override;

    /*!
     * \brief Refused: an adapter over a source is not a store.
     * \param[in]  source  Ignored.
     * \param[in]  start   Ignored.
     * \param[in]  count   Ignored.
     * \param[out] message Receives `item "<id>" is a time slice of another
     *                     item and is read-only`; may be nullptr.
     * \return false.
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
        std::vector<std::unique_ptr<Dimension>> ownedDimensions; //!< The slice's dimensions (owned).
        std::unique_ptr<ValueDefinition>        valueDefinition; //!< The unitless `<id>_slice` quantity.
        std::vector<Dimension *>                dimensions;      //!< Non-owning view of ownedDimensions for the base.
    };

    /*!
     * \brief Builds the slice's dimensions and value definition.
     * \param[in] id     Slice identifier, used to name the value definition.
     * \param[in] source Source item; nullptr yields one "element" dimension.
     * \return The prepared objects, moved into the slice by the constructor.
     */
    [[nodiscard]] static Prepared prepare(
        std::string_view id, HydroCouple::IComponentDataItem *source);

    /*!
     * \brief Delegated constructor that takes ownership of \p prepared.
     * \param[in] id        Slice identifier.
     * \param[in] source    Source item (borrowed).
     * \param[in] component Owning component (borrowed; may be nullptr).
     * \param[in] prepared  Output of prepare(), moved into the slice.
     */
    TimeSliceOutput(std::string_view id,
                    HydroCouple::IComponentDataItem *source,
                    HydroCouple::IModelComponent *component,
                    Prepared prepared);

    struct Impl;
    std::unique_ptr<Impl> m_impl; //!< Source, time index and owned metadata.
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_DATA_TIMESLICEOUTPUT_H
