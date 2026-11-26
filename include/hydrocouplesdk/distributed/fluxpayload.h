// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   fluxpayload.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  FluxDerivativePayload — the SDK's flux+derivative exchange
 *         payload convention (plan §5.4, the SWMM coupling lesson).
 *
 * Exchanging bare fluxes between loosely coupled components is a known
 * source of interface instability: the receiver applies a flux computed
 * against the sender's *old* state. The SWMM engine's lesson is to exchange
 * the flux together with its derivative with respect to the coupling state
 * (e.g., dQ/dh), so the receiver can evaluate a linearized boundary
 * \f$ q(h) \approx q_0 + \frac{\partial q}{\partial h}\,(h - h_0) \f$
 * inside its own iteration — first-order implicit coupling at the cost of
 * one extra array.
 *
 * The convention deliberately introduces no new wire format: a
 * flux+derivative payload is a rank-2 Float64 block of shape {2, N} —
 * row 0 the fluxes, row 1 the derivatives — so it travels through
 * ValuePacker and every ITransport backend (InProc, MPI) exactly like any
 * other typed payload, with kind and shape validated on unpack. This class
 * owns such a block, exposes typed row views, evaluates the linearized
 * boundary, and validates the shape contract on descriptors received from
 * a peer.
 *
 * \threadsafety Not thread-safe; one payload belongs to one exchange side.
 */

#ifndef HYDROCOUPLESDK_DISTRIBUTED_FLUXPAYLOAD_H
#define HYDROCOUPLESDK_DISTRIBUTED_FLUXPAYLOAD_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"

#include <span>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Distributed {

/*!
 * \brief Owned flux+derivative pair block with the {2, N} shape convention.
 *
 * Units are the caller's: the flux row carries whatever the exchanged flux
 * is (m³/s, say) and the derivative row its derivative with respect to the
 * coupling state (m³/s per meter of head, that is m²/s, for dQ/dh). The block is copyable;
 * a copy owns its own values.
 */
class HYDROCOUPLESDK_EXPORT FluxDerivativePayload final
{
public:
    /*!
     * \brief Construct with \p count entities (all values zero).
     * \param[in] count Entities N; a negative value is treated as 0.
     */
    explicit FluxDerivativePayload(int64_t count);

    /*!
     * \brief Number of entities (N).
     * \return N as fixed at construction (never negative).
     */
    [[nodiscard]] int64_t count() const noexcept;

    /*!
     * \brief Mutable flux row (row 0).
     * \return A view of N values, valid while the payload lives.
     */
    [[nodiscard]] std::span<double> flux() noexcept;
    /*!
     * \brief Read-only flux row (row 0).
     * \return A view of N values, valid while the payload lives.
     */
    [[nodiscard]] std::span<const double> flux() const noexcept;

    /*!
     * \brief Mutable derivative row (row 1): ∂flux/∂state per entity.
     * \return A view of N values, valid while the payload lives.
     */
    [[nodiscard]] std::span<double> derivative() noexcept;
    /*!
     * \brief Read-only derivative row (row 1).
     * \return A view of N values, valid while the payload lives.
     */
    [[nodiscard]] std::span<const double> derivative() const noexcept;

    /*!
     * \brief Descriptor over the whole block — Float64, rank 2, shape
     *        {2, N}, host, contiguous — ready for any ITransport or data
     *        item setValuesFrom()/getValuesInto().
     *
     * The descriptor points into this object (values and shape), so it is
     * valid only while the payload lives. It may be used as a receive
     * destination although the function is const.
     *
     * \return The descriptor.
     */
    [[nodiscard]] BufferDescriptor descriptor() const noexcept;

    /*!
     * \brief Evaluates the linearized boundary flux for entity \p index:
     *        flux[index] + derivative[index] * (state - referenceState).
     *
     * That is \f$ q(h) \approx q_0 + \frac{\partial q}{\partial h}\,(h - h_0) \f$
     * with \f$ q_0 \f$ the flux row, \f$ \partial q / \partial h \f$ the
     * derivative row, \f$ h \f$ = \p state and \f$ h_0 \f$ = \p referenceState.
     *
     * \param[in] index          Entity, 0 ≤ index < count().
     * \param[in] state          Current coupling state (same units as the
     *                           state the derivative was taken against).
     * \param[in] referenceState State at which the flux was computed.
     * \return The linearized flux.
     * \exception std::out_of_range \p index is outside [0, count()).
     */
    [[nodiscard]] double linearizedFlux(int64_t index, double state,
                                        double referenceState) const;

    /*!
     * \brief Checks a peer descriptor against the convention: Float64,
     *        rank 2, shape {2, N} with N == count() (any N when \p count
     *        is negative).
     * \param[in]  descriptor The received descriptor to check.
     * \param[in]  count      Required N, or -1 to accept any.
     * \param[out] message    Failure description when returning false:
     *                        "flux+derivative payload must be Float64",
     *                        "flux+derivative payload must have rank 2
     *                        (shape {2, N})", "... row dimension must be 2
     *                        (flux row then derivative row), got R", or
     *                        "flux+derivative payload carries M entities;
     *                        expected N". Cleared on success.
     * \return True when the descriptor follows the convention. Strides,
     *         memory space and data pointer are not checked.
     */
    [[nodiscard]] static bool conformsToConvention(
        const BufferDescriptor &descriptor, int64_t count,
        std::string &message);

private:
    int64_t              m_count;   //!< Entities N (≥ 0).
    std::vector<double>  m_values;  //!< Row-major {2, N}: flux then derivative.
    std::vector<int64_t> m_shape;   //!< {2, N} backing the descriptor.
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_FLUXPAYLOAD_H
