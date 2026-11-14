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
 */
class HYDROCOUPLESDK_EXPORT RelaxationAdaptedOutput
    : public AbstractAdaptedOutput,
      public virtual HydroCouple::IDifferentiableAdaptedOutput,
      public virtual HydroCouple::ICheckpointableAdaptedOutput
{
public:
    /*!
     * \param id       Instance identifier.
     * \param adaptee  Rank-1 Double output being adapted (borrowed).
     * \param factory  Creating factory. Registration with the adaptee is the
     *                 factory's postcondition, not this constructor's.
     */
    RelaxationAdaptedOutput(std::string_view id,
                            HydroCouple::IOutput *adaptee,
                            HydroCouple::IAdaptedOutputFactory *factory);

    ~RelaxationAdaptedOutput() override = default;

    // ── IAdaptedOutput ────────────────────────────────────────────────────

    //! Clears the history and computes first values (a passthrough).
    void initialize() override;

    //! Blends the adaptee's current values into the running state, with
    //! the relaxation factor's current value.
    void refresh() override;

    //! Delegates to refresh().
    void updateValues() override;

    //! The adapter itself: its previous values are its state.
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> states() const override;

    // ── v2 typed hyperslab data plane ─────────────────────────────────────

    [[nodiscard]] std::vector<int64_t> shape() const override;
    [[nodiscard]] DataKind dataKind() const override;

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override;

    //! Always fails — adapted outputs are read-only.
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── IDifferentiableAdaptedOutput ──────────────────────────────────────

    //! The relaxation factor.
    [[nodiscard]] std::vector<HydroCouple::IArgument *>
    differentiableArguments() const override;
    //! The adapter itself (StateBefore / StateAfter).
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *>
    differentiableStates() const override;
    //! Seeds: this (Output, StateAfter). Results: adaptee (Input), this
    //! (StateBefore), the relaxation factor (Argument).
    [[nodiscard]] bool vjp(HydroCouple::DifferentialSet seeds,
                           HydroCouple::DifferentialSet results,
                           std::string *message = nullptr) override;
    //! Seeds: adaptee (Input), this (StateBefore), the relaxation factor
    //! (Argument). Results: this (Output and/or StateAfter).
    [[nodiscard]] bool jvp(HydroCouple::DifferentialSet seeds,
                           HydroCouple::DifferentialSet results,
                           std::string *message = nullptr) override;

    // ── ICheckpointableAdaptedOutput ──────────────────────────────────────

    bool saveState(std::string &token, std::string &message) override;
    bool restoreState(const std::string &token, std::string &message) override;
    //! The token is the state: nothing to free.
    bool releaseState(const std::string &token, std::string &message) override;

private:
    bool pullAdaptee(std::vector<double> &into) const;

    Dimension m_argDimension{"scalar"};
    std::unique_ptr<Quantity> m_factorQuantity{
        Quantity::unitLess("Relaxation factor")};

    Argument1DDouble m_relaxationFactor;

    std::vector<double> m_values;

    // The linearization point of vjp()/jvp(): the most recent refresh.
    bool m_haveRefresh = false;      //!< false before the first, and after a restore
    bool m_blended = false;          //!< false when it passed the adaptee through
    double m_lambda = 1.0;           //!< λ that refresh used
    std::vector<double> m_source;    //!< x: the adaptee's values it read
    std::vector<double> m_previous;  //!< y_prev: the values it blended from
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_RELAXATIONADAPTEDOUTPUT_H
