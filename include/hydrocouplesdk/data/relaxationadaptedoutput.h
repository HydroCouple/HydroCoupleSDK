// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   relaxationadaptedoutput.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Adapted output applying under-relaxation between refreshes.
 */

#ifndef HYDROCOUPLESDK_RELAXATIONADAPTEDOUTPUT_H
#define HYDROCOUPLESDK_RELAXATIONADAPTEDOUTPUT_H

#include "hydrocouplesdk/data/abstractadaptedoutput.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <memory>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Under-relaxation of a rank-1 Double adaptee across refreshes:
 *        y_k[i] = (1 - λ) * y_{k-1}[i] + λ * x_k[i].
 *
 * Damps oscillation in cyclic exchange groups (a time-stepped workflow
 * sweeping a feedback loop more than once per step), and in a stepped
 * composition acts as a first-order lag between two components. Argument
 * \c relaxation_factor is λ, default 1 — a passthrough — and is read at
 * every refresh, so a change applies from the next one. initialize() clears
 * the history and refreshes once; with no history, that refresh passes the
 * adaptee through (as does any refresh after the adaptee changes shape).
 *
 * Deliberately stateful: two refreshes at the same adaptee state produce
 * different values when λ < 1. That is the transformation, not a defect —
 * hosts that refresh once per sweep get exactly the blend they asked for.
 * The state is the adapter's own previous values, so states() and
 * differentiableStates() both list the adapter itself.
 *
 * Differentiable (IDifferentiableAdaptedOutput, G2.4). At the most recent
 * refresh, with x the adaptee's values, y_prev the values before it and
 * t = ȳ_out + ȳ_state (the cotangents of the output and of the state it
 * carries to the next refresh): x̄ = λ t, ȳ_prev = (1 − λ) t and
 * λ̄ = Σ t (x − y_prev). A passthrough refresh has x̄ = t and nothing else.
 *
 * Checkpointable (ICheckpointableAdaptedOutput, G2.4). The token is the
 * state — the values, bit for bit, and so whether there is a history — so it
 * can be restored by another process and releasing it frees nothing. A
 * token whose values do not fit the adaptee's shape is refused.
 *
 * \details In formulas, with \f$\lambda\f$ the relaxation factor, refresh
 * \f$k\f$ computes \f[ y^{(k)}_i = (1-\lambda)\,y^{(k-1)}_i + \lambda\,x^{(k)}_i, \f]
 * and with \f$t = \bar{y}_{\mathrm{out}} + \bar{y}_{\mathrm{state}}\f$ the
 * adjoint of a blending refresh is
 * \f[ \bar{x} = \lambda\,t, \qquad \bar{y}^{(k-1)} = (1-\lambda)\,t, \qquad
 *     \bar{\lambda} = \sum_i t_i\,\bigl(x^{(k)}_i - y^{(k-1)}_i\bigr). \f]
 *
 * Argument (an Argument1DDouble of length 1):
 * | Id | Default | Meaning |
 * | --- | --- | --- |
 * | `relaxation_factor` | 1 | \f$\lambda\f$, unitless. 1 passes the adaptee through, 0 freezes the first values, values between damp. Not range-checked. |
 *
 * Every refresh blends once, so the result depends on how many refreshes the
 * host makes per exchange. AbstractAdaptedOutput::updateValues(query) on a
 * parent adapter refreshes this one twice (two blends).
 */
class HYDROCOUPLESDK_EXPORT RelaxationAdaptedOutput
    : public AbstractAdaptedOutput,
      public virtual HydroCouple::IDifferentiableAdaptedOutput,
      public virtual HydroCouple::ICheckpointableAdaptedOutput
{
public:
    /*!
     * \brief Builds the adapter with relaxation factor 1 and no history.
     * \param id       Instance identifier.
     * \param adaptee  Rank-1 Double output being adapted (borrowed).
     * \param factory  Creating factory. Registration with the adaptee is the
     *                 factory's postcondition, not this constructor's.
     */
    RelaxationAdaptedOutput(std::string_view id,
                            HydroCouple::IOutput *adaptee,
                            HydroCouple::IAdaptedOutputFactory *factory);

    //! Destroys the adapter and deregisters it from its adaptee.
    ~RelaxationAdaptedOutput() override = default;

    // ── IAdaptedOutput ────────────────────────────────────────────────────

    //! Clears the history and computes first values (a passthrough).
    void initialize() override;

    //! Blends the adaptee's current values into the running state, with
    //! the relaxation factor's current value.
    //! \details When the adaptee is gone, not rank 1, or refuses the read,
    //! the values stay and nothing is emitted (and there is then no refresh
    //! to differentiate). When the stored values' length differs from the
    //! adaptee's (the first refresh, or a shape change), the adaptee passes
    //! through. Ends by firing value-changed and refreshing the children.
    void refresh() override;

    //! Delegates to refresh().
    void updateValues() override;

    //! The adapter itself: its previous values are its state.
    //! \return A one-element vector holding this adapter.
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> states() const override;

    // ── v2 typed hyperslab data plane ─────────────────────────────────────

    /*!
     * \brief The adaptee's current shape.
     * \return The adaptee's shape(), or an empty vector when detached.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override;
    /*!
     * \brief The element type of the relaxed values.
     * \return Always DataKind::Float64.
     */
    [[nodiscard]] DataKind dataKind() const override;

    /*!
     * \brief Copies a hyperslab of the relaxed values.
     * \details Fails with `RelaxationAdaptedOutput "<id>" has no values;
     *          initialize() has not run` while the adapter holds no values;
     *          otherwise follows AbstractComponentDataItem::getValuesInto().
     * \param[in]  destination Host Float64 buffer receiving the values.
     * \param[in]  start       First index (one entry).
     * \param[in]  count       Number of values (one entry).
     * \param[out] message     Failure description; may be nullptr.
     * \return true when the values were copied.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override;

    //! Always fails — adapted outputs are read-only.
    //! \param[in]  source  Ignored.
    //! \param[in]  start   Ignored.
    //! \param[in]  count   Ignored.
    //! \param[out] message Receives "RelaxationAdaptedOutput is read-only".
    //! \return false.
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── IDifferentiableAdaptedOutput ──────────────────────────────────────

    //! The relaxation factor.
    //! \return The relaxation_factor argument (owned by the adapter).
    [[nodiscard]] std::vector<HydroCouple::IArgument *>
    differentiableArguments() const override;
    //! The adapter itself (StateBefore / StateAfter).
    //! \return The same list as states().
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *>
    differentiableStates() const override;
    //! Seeds: this (Output, StateAfter). Results: adaptee (Input), this
    //! (StateBefore), the relaxation factor (Argument).
    //! \details Linearized at the most recent refresh; refused before one and
    //! after restoreState() ("has no refresh to differentiate"). The Output
    //! and StateAfter seeds add. For a passthrough refresh the adaptee's
    //! cotangent is the seed and the others are zero.
    //! \param[in] seeds   Cotangents of this adapter (Output and/or StateAfter).
    //! \param[in] results Buffers to overwrite (whole-item Float64).
    //! \param[out] message Failure description; may be nullptr.
    //! \return true on success.
    [[nodiscard]] bool vjp(HydroCouple::DifferentialSet seeds,
                           HydroCouple::DifferentialSet results,
                           std::string *message = nullptr) override;
    //! Seeds: adaptee (Input), this (StateBefore), the relaxation factor
    //! (Argument). Results: this (Output and/or StateAfter).
    //! \details Both result roles receive the same tangent. Same refusals as vjp().
    //! \param[in] seeds   Tangents; absent ones are zero.
    //! \param[in] results Buffers to overwrite (whole-item Float64).
    //! \param[out] message Failure description; may be nullptr.
    //! \return true on success.
    [[nodiscard]] bool jvp(HydroCouple::DifferentialSet seeds,
                           HydroCouple::DifferentialSet results,
                           std::string *message = nullptr) override;

    // ── ICheckpointableAdaptedOutput ──────────────────────────────────────

    /*!
     * \brief Saves the current values as a self-contained token.
     * \details The token is the text "relaxation-v1", the value count and
     *          each value's IEEE-754 bit pattern as 16 hexadecimal digits,
     *          separated by spaces. It is exact for every double, independent
     *          of locale, and can be restored by another process. An adapter
     *          with no history saves a count of 0.
     * \param[out] token   Receives the token.
     * \param[out] message Not changed.
     * \return Always true.
     */
    bool saveState(std::string &token, std::string &message) override;
    /*!
     * \brief Restores values saved by saveState().
     * \details Refused, with the message prefixed by
     *          `RelaxationAdaptedOutput "<id>": `, when the token does not
     *          start with the tag and a count ("not a relaxation token"),
     *          when a non-zero count differs from the adaptee's current
     *          length ("the token holds N values but the adaptee has a
     *          different shape"), when a value is missing or not hexadecimal
     *          ("truncated or malformed token"), or when text follows the
     *          values ("trailing data in token"). A refused token changes
     *          nothing. On success the values are replaced, nothing is
     *          emitted, no child is refreshed, and the next vjp() or jvp() is
     *          refused until a refresh.
     * \param[in]  token   A token from saveState().
     * \param[out] message Cleared on success; the reason on failure.
     * \return true when the state was restored.
     */
    bool restoreState(const std::string &token, std::string &message) override;
    //! The token is the state: nothing to free.
    //! \param[in]  token   Ignored.
    //! \param[out] message Cleared.
    //! \return Always true.
    bool releaseState(const std::string &token, std::string &message) override;

private:
    //! Pulls the adaptee's rank-1 slab as Float64.
    //! \param[out] into Resized to the adaptee's length and filled.
    //! \return false when the adaptee is gone, not rank 1, or refuses the read.
    bool pullAdaptee(std::vector<double> &into) const;

    Dimension m_argDimension{"scalar"}; //!< Length axis of the factor argument.
    //! Unitless value definition of the factor argument.
    std::unique_ptr<Quantity> m_factorQuantity{
        Quantity::unitLess("Relaxation factor")};

    Argument1DDouble m_relaxationFactor; //!< Argument "relaxation_factor", read at every refresh.

    std::vector<double> m_values;        //!< Relaxed values: the output and the carried state.

    // The linearization point of vjp()/jvp(): the most recent refresh.
    bool m_haveRefresh = false;      //!< false before the first, and after a restore
    bool m_blended = false;          //!< false when it passed the adaptee through
    double m_lambda = 1.0;           //!< λ that refresh used
    std::vector<double> m_source;    //!< x: the adaptee's values it read
    std::vector<double> m_previous;  //!< y_prev: the values it blended from
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_RELAXATIONADAPTEDOUTPUT_H
