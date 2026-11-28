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
 * initialize() lands on them. The coefficients are read at every refresh
 * (initialize() makes the first), so a change applies from the next one --
 * which is what lets a calibration loop set them between runs without
 * initializing the adapter again (and refreshing everything below it).
 *
 * Differentiable (IDifferentiableAdaptedOutput, Phase G2.1): with x the
 * adaptee's values at the most recent refresh() and ȳ the cotangent of this
 * output, the adjoint is x̄ = m·ȳ, m̄ = Σ ȳ·x, b̄ = Σ ȳ — so a gradient
 * crosses a unit conversion, and the conversion's own coefficients can be
 * calibrated. The coefficients are those of the most recent refresh.
 *
 * \details In formulas, with \f$m\f$ the multiplier and \f$b\f$ the offset,
 * each refresh computes \f[ y_i = m\,x_i + b, \f] and the adjoint is
 * \f[ \bar{x}_i = m\,\bar{y}_i, \qquad \bar{m} = \sum_i \bar{y}_i x_i,
 *     \qquad \bar{b} = \sum_i \bar{y}_i. \f]
 *
 * Arguments (each an Argument1DDouble of length 1, set as a JSON array or an
 * object with "values", for example `[0.0353147]` or `{"values": [2.0]}`):
 * | Id | Default | Meaning |
 * | --- | --- | --- |
 * | `multiplier` | 1 | \f$m\f$, unitless |
 * | `offset` | 0 | \f$b\f$, in the adaptee's units after scaling |
 *
 * Only element 0 of each argument is read; an argument resized to length 0
 * makes the next refresh throw std::out_of_range.
 *
 * The adapter reports no dimensions and no value definition (both null or
 * empty): the units of its values are the adaptee's scaled by the
 * multiplier, and nothing records them. shape() is the adaptee's current
 * shape. Its values are empty, and reads fail, until the first successful
 * refresh.
 */
class HYDROCOUPLESDK_EXPORT LinearTransformAdaptedOutput
    : public AbstractAdaptedOutput,
      public virtual HydroCouple::IDifferentiableAdaptedOutput
{
public:
    /*!
     * \brief Builds the adapter with multiplier 1 and offset 0.
     * \param id       Instance identifier.
     * \param adaptee  Rank-1 Double output being adapted (borrowed).
     * \param factory  Creating factory. Registration with the adaptee is the
     *                 factory's postcondition, not this constructor's.
     */
    LinearTransformAdaptedOutput(std::string_view id,
                                 HydroCouple::IOutput *adaptee,
                                 HydroCouple::IAdaptedOutputFactory *factory);

    //! Destroys the adapter and deregisters it from its adaptee.
    ~LinearTransformAdaptedOutput() override = default;

    // ── IAdaptedOutput ────────────────────────────────────────────────────

    //! Computes the first values.
    //! \details The same as refresh(); it can be called again at any time.
    void initialize() override;

    //! Re-reads the adaptee and the coefficients and re-applies the
    //! transformation.
    //! \details Reads the adaptee's whole rank-1 slab as Float64. When the
    //! adaptee is gone, is not rank 1, or refuses the read, the previous
    //! values stay, nothing is emitted, and the adapter has no refresh to
    //! differentiate. Otherwise it stores the new values, fires
    //! value-changed for the whole slab, and refreshes its own children.
    void refresh() override;

    //! Delegates to refresh().
    void updateValues() override;

    // ── v2 typed hyperslab data plane ─────────────────────────────────────

    /*!
     * \brief The adaptee's current shape.
     * \return The adaptee's shape(), or an empty vector when detached. It can
     *         differ from the stored values' length until the next refresh.
     */
    [[nodiscard]] std::vector<int64_t> shape() const override;
    /*!
     * \brief The element type of the transformed values.
     * \return Always DataKind::Float64.
     */
    [[nodiscard]] DataKind dataKind() const override;

    /*!
     * \brief Copies a hyperslab of the transformed values from the most
     *        recent refresh.
     * \details Fails with `LinearTransformAdaptedOutput "<id>" has no values;
     *          initialize() has not run` until a refresh succeeds; otherwise
     *          follows AbstractComponentDataItem::getValuesInto() over the
     *          stored rank-1 values.
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
    //! \param[out] message Receives "LinearTransformAdaptedOutput is read-only".
    //! \return false.
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override;

    // ── IDifferentiableAdaptedOutput ──────────────────────────────────────
    //! The multiplier and the offset.
    //! \return The two argument objects, multiplier first (owned by the adapter).
    [[nodiscard]] std::vector<HydroCouple::IArgument *>
    differentiableArguments() const override;
    //! None: the transformation has no memory.
    //! \return An empty vector.
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *>
    differentiableStates() const override;
    //! Seed: this (Output). Results: adaptee (Input), multiplier/offset (Argument).
    //! \details Linearized at the most recent successful refresh; refused
    //! before one ("has no refresh to differentiate"). Absent seeds are zero;
    //! a seed or result naming any other item or role is refused. Seed and
    //! result buffers are whole-item Float64 buffers (length 1 for the
    //! arguments).
    //! \param[in] seeds   Cotangent of this adapter (role Output).
    //! \param[in] results Buffers to overwrite.
    //! \param[out] message Failure description; may be nullptr.
    //! \return true on success.
    [[nodiscard]] bool vjp(HydroCouple::DifferentialSet seeds,
                           HydroCouple::DifferentialSet results,
                           std::string *message = nullptr) override;
    //! Seeds: adaptee (Input), multiplier/offset (Argument). Result: this (Output).
    //! \details Computes \f$\dot{y}_i = m\,\dot{x}_i + \dot{m}\,x_i + \dot{b}\f$
    //! at the most recent refresh; the same refusals as vjp().
    //! \param[in] seeds   Tangents of the adaptee and the arguments.
    //! \param[in] results Buffer for this adapter's tangent (role Output).
    //! \param[out] message Failure description; may be nullptr.
    //! \return true on success.
    [[nodiscard]] bool jvp(HydroCouple::DifferentialSet seeds,
                           HydroCouple::DifferentialSet results,
                           std::string *message = nullptr) override;

private:
    //! Pulls the adaptee's rank-1 slab; false (untouched values) on failure.
    //! \param[out] into Resized to the adaptee's length and filled.
    //! \return true when the read succeeded.
    bool pullAdaptee(std::vector<double> &into) const;

    Dimension m_argDimension{"scalar"}; //!< Length axis of the two arguments.
    //! Unitless value definition of the multiplier argument.
    std::unique_ptr<Quantity> m_multiplierQuantity{
        Quantity::unitLess("Multiplier")};
    //! Unitless value definition of the offset argument.
    std::unique_ptr<Quantity> m_offsetQuantity{Quantity::unitLess("Offset")};

    Argument1DDouble m_multiplier; //!< Argument "multiplier", read at every refresh.
    Argument1DDouble m_offset;     //!< Argument "offset", read at every refresh.

    double m_m = 1.0;              //!< Multiplier used by the most recent refresh.
    double m_b = 0.0;              //!< Offset used by the most recent refresh.
    std::vector<double> m_values;  //!< Transformed values served by getValuesInto().
    //! The adaptee's values at the most recent refresh(): the
    //! linearization point of vjp()/jvp().
    std::vector<double> m_source;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_LINEARTRANSFORMADAPTEDOUTPUT_H
