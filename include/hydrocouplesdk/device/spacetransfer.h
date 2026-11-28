// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   spacetransfer.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Space-aware buffer transfer — negotiation and execution of
 *         copies between BufferDescriptors in any memory space (plan §6.3).
 *
 * Exchange connections decide *how* values move exactly once, at
 * prepare() time: negotiateCopyPath() inspects the two descriptors and
 * returns the cheapest legal path — a host copy when both sides are
 * host-accessible (one memcpy when both are contiguous, otherwise an
 * element-wise copy with full strided support), a single backend copy
 * when a device is involved and both sides are contiguous, or staging
 * through a host buffer when a strided host view meets a device buffer.
 * copyBuffer() negotiates again and executes that path, validating kind
 * and element count up front. Device-side descriptors must be contiguous
 * — strided device views would force element-wise traffic and are
 * rejected as a negotiation error rather than silently degraded.
 *
 * Host-accessible means MemorySpace::Host, HostPinned or Unified; only
 * Device is treated as device memory. Element sizes come from the kind
 * (BufferDescriptor::itemSizeBytes, \c backend and \c queue are not read),
 * so only fixed-width kinds transfer. Every copy is synchronous.
 *
 * \threadsafety Stateless free functions; concurrent calls on disjoint
 * buffers are safe.
 */

#ifndef HYDROCOUPLESDK_DEVICE_SPACETRANSFER_H
#define HYDROCOUPLESDK_DEVICE_SPACETRANSFER_H

#include "hydrocouplesdk/device/devicebackend.h"

#include <string>

namespace HydroCouple::SDK::Device {

/*!
 * \brief The copy strategy negotiated for one (source, destination) pair.
 */
enum class CopyPath : uint8_t
{
    HostHyperslab,    //!< Both host-accessible: memcpy, or an element-wise strided copy.
    BackendCopy,      //!< Contiguous endpoints: one DeviceBackend::copy().
    StageThroughHost, //!< Strided host side + device side: gather/scatter
                      //!< through a temporary host block.
    Unsupported       //!< No legal path (see the negotiation message).
};

/*!
 * \brief True when \p descriptor is C-contiguous (null strides, or strides
 *        equal to the packed row-major byte steps).
 *
 * The stride of a dimension of extent 0 or 1 is not checked.
 *
 * \param[in] descriptor Descriptor to test; its element size is the
 *                       kind's natural size.
 * \return True when the elements are packed row-major.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool isContiguous(
    const BufferDescriptor &descriptor);

/*!
 * \brief Picks the cheapest legal path for copying \p source into
 *        \p destination through \p backend. Call once at prepare().
 *
 * Checks, in order: equal kinds, a fixed-width kind, equal element
 * counts; then, when either side is device memory, that the backend
 * supports each device side and that each device side is contiguous.
 *
 * \param[in]  source      Source descriptor.
 * \param[in]  destination Destination descriptor.
 * \param[in]  backend     Backend whose supportsSpace() decides device
 *                         sides.
 * \param[out] message Explains an Unsupported result: "space transfer:
 *                     element kinds differ", "... kind has no fixed width",
 *                     "... element counts differ", "... backend 'NAME' does
 *                     not support the source space" (or destination space),
 *                     "... strided device source is not supported" (or
 *                     destination). Cleared otherwise.
 * \return HostHyperslab, BackendCopy, StageThroughHost or Unsupported.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT CopyPath negotiateCopyPath(
    const BufferDescriptor &source, const BufferDescriptor &destination,
    const DeviceBackend &backend, std::string &message);

/*!
 * \brief Copies every element of \p source into \p destination along the
 *        path negotiateCopyPath() picks for the pair.
 *
 * Validates matching kind and element count. Shapes may differ in rank as
 * long as the element counts agree (a {2, N} block may fill a {2*N} view):
 * elements are taken in row-major order of the source shape and placed in
 * row-major order of the destination shape. The function negotiates on
 * every call; there is no way to pass a path negotiated earlier. Staging
 * allocates a temporary host block of the payload's size.
 *
 * \param[in]  backend     Backend executing device-side copies.
 * \param[in]  source      Source descriptor (any supported space).
 * \param[in]  destination Destination descriptor (any supported space).
 * \param[out] message     Failure description when returning false (a
 *                         negotiation message, or the backend's copy()
 *                         message).
 * \return True when every element was copied.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool copyBuffer(
    DeviceBackend &backend, const BufferDescriptor &source,
    const BufferDescriptor &destination, std::string &message);

} // namespace HydroCouple::SDK::Device

#endif // HYDROCOUPLESDK_DEVICE_SPACETRANSFER_H
