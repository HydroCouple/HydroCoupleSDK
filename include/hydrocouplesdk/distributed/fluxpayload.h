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
 */
class HYDROCOUPLESDK_EXPORT FluxDerivativePayload final
{
public:
    /*!
     * \brief Construct with \p count entities (all values zero).
     */
    explicit FluxDerivativePayload(int64_t count);

    /*!
     * \brief Number of entities (N).
     */
    [[nodiscard]] int64_t count() const noexcept;

    /*!
     * \brief Mutable flux row (row 0).
     */
    [[nodiscard]] std::span<double> flux() noexcept;
    [[nodiscard]] std::span<const double> flux() const noexcept;

    /*!
     * \brief Mutable derivative row (row 1): ∂flux/∂state per entity.
     */
    [[nodiscard]] std::span<double> derivative() noexcept;
    [[nodiscard]] std::span<const double> derivative() const noexcept;

    /*!
     * \brief Descriptor over the whole block — Float64, rank 2, shape
     *        {2, N}, host, contiguous — ready for any ITransport or data
     *        item setValuesFrom()/getValuesInto().
     */
    [[nodiscard]] BufferDescriptor descriptor() const noexcept;

    /*!
     * \brief Evaluates the linearized boundary flux for entity \p index:
     *        flux[index] + derivative[index] * (state - referenceState).
     */
    [[nodiscard]] double linearizedFlux(int64_t index, double state,
                                        double referenceState) const;

    /*!
     * \brief Checks a peer descriptor against the convention: Float64,
     *        rank 2, shape {2, N} with N == count() (any N when \p count
     *        is negative).
     * \param[in]  descriptor The received descriptor to check.
     * \param[in]  count      Required N, or -1 to accept any.
     * \param[out] message    Failure description when returning false.
     */
    [[nodiscard]] static bool conformsToConvention(
        const BufferDescriptor &descriptor, int64_t count,
        std::string &message);

private:
    int64_t              m_count;
    std::vector<double>  m_values;  //!< Row-major {2, N}: flux then derivative.
    std::vector<int64_t> m_shape;   //!< {2, N} backing the descriptor.
};

} // namespace HydroCouple::SDK::Distributed

#endif // HYDROCOUPLESDK_DISTRIBUTED_FLUXPAYLOAD_H
