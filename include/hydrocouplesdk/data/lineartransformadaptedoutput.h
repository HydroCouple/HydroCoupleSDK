// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   lineartransformadaptedoutput.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Adapted output applying y = multiplier * x + offset element-wise.
 */

#ifndef HYDROCOUPLESDK_LINEARTRANSFORMADAPTEDOUTPUT_H
#define HYDROCOUPLESDK_LINEARTRANSFORMADAPTEDOUTPUT_H

#include "hydrocouplesdk/data/abstractadaptedoutput.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <memory>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Element-wise linear transformation of a rank-1 Double adaptee:
 *        y[i] = multiplier * x[i] + offset.
 *
 * Unit conversion is the motivating use (m³/s → cfs, °C → °F). Arguments
 * \c multiplier (default 1) and \c offset (default 0) are registered in the
 * constructor, so a chain builder that applies step arguments before
 * initialize() lands on them; initialize() latches the coefficients and
 * computes the first values.
 */
class HYDROCOUPLESDK_EXPORT LinearTransformAdaptedOutput
    : public AbstractAdaptedOutput
{
public:
    /*!
     * \param id       Instance identifier.
     * \param adaptee  Rank-1 Double output being adapted (borrowed).
     * \param factory  Creating factory. Registration with the adaptee is the
     *                 factory's postcondition, not this constructor's.
     */
    LinearTransformAdaptedOutput(std::string_view id,
                                 HydroCouple::IOutput *adaptee,
                                 HydroCouple::IAdaptedOutputFactory *factory);

    ~LinearTransformAdaptedOutput() override = default;

    // ── IAdaptedOutput ────────────────────────────────────────────────────

    //! Latches multiplier/offset from the arguments and computes first values.
    void initialize() override;

    //! Re-reads the adaptee and re-applies the transformation.
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
    //! Pulls the adaptee's rank-1 slab; false (untouched values) on failure.
    bool pullAdaptee(std::vector<double> &into) const;

    Dimension m_argDimension{"scalar"};
    std::unique_ptr<Quantity> m_multiplierQuantity{
        Quantity::unitLess("Multiplier")};
    std::unique_ptr<Quantity> m_offsetQuantity{Quantity::unitLess("Offset")};

    Argument1DDouble m_multiplier;
    Argument1DDouble m_offset;

    double m_m = 1.0;
    double m_b = 0.0;
    std::vector<double> m_values;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_LINEARTRANSFORMADAPTEDOUTPUT_H
