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
 * sweeping a feedback loop more than once per step). Argument
 * \c relaxation_factor is λ, default 1 — a passthrough. The first refresh
 * after initialize() has no history and passes the adaptee through.
 *
 * Deliberately stateful: two refreshes at the same adaptee state produce
 * different values when λ < 1. That is the transformation, not a defect —
 * hosts that refresh once per sweep get exactly the blend they asked for.
 */
class HYDROCOUPLESDK_EXPORT RelaxationAdaptedOutput
    : public AbstractAdaptedOutput
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

    //! Latches λ, clears history, and computes first values.
    void initialize() override;

    //! Blends the adaptee's current values into the running state.
    void refresh() override;

    //! Delegates to refresh().
    void updateValues() override;

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

private:
    bool pullAdaptee(std::vector<double> &into) const;

    Dimension m_argDimension{"scalar"};
    std::unique_ptr<Quantity> m_factorQuantity{
        Quantity::unitLess("Relaxation factor")};

    Argument1DDouble m_relaxationFactor;

    double m_lambda = 1.0;
    std::vector<double> m_values;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_RELAXATIONADAPTEDOUTPUT_H
