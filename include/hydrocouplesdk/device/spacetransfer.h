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
 * returns the cheapest legal path — the host hyperslab engine when both
 * sides are host-resident (full strided support), a single backend copy
 * when a device is involved and both sides are contiguous, or staging
 * through a host buffer when a strided host view meets a device buffer.
 * copyBuffer() executes whichever path was negotiated, validating kind
 * and element count up front. Device-side descriptors must be contiguous
 * — strided device views would force element-wise traffic and are
 * rejected as a negotiation error rather than silently degraded.
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
    HostHyperslab,    //!< Both host: the strided hyperslab engine copies.
    BackendCopy,      //!< Contiguous endpoints: one DeviceBackend::copy().
    StageThroughHost, //!< Strided host side + device side: gather/scatter
                      //!< through a temporary host block.
    Unsupported       //!< No legal path (see the negotiation message).
};

/*!
 * \brief True when \p descriptor is C-contiguous (null strides, or strides
 *        equal to the packed row-major byte steps).
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool isContiguous(
    const BufferDescriptor &descriptor);

/*!
 * \brief Picks the cheapest legal path for copying \p source into
 *        \p destination through \p backend. Call once at prepare().
 * \param[out] message Explains an Unsupported result.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT CopyPath negotiateCopyPath(
    const BufferDescriptor &source, const BufferDescriptor &destination,
    const DeviceBackend &backend, std::string &message);

/*!
 * \brief Copies every element of \p source into \p destination along the
 *        negotiated (or freshly negotiated) path.
 *
 * Validates matching kind and element count. Shapes may differ in rank as
 * long as the element counts agree (a {2, N} block may fill a {2*N} view).
 *
 * \param[in]  backend     Backend executing device-side copies.
 * \param[in]  source      Source descriptor (any supported space).
 * \param[in]  destination Destination descriptor (any supported space).
 * \param[out] message     Failure description when returning false.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool copyBuffer(
    DeviceBackend &backend, const BufferDescriptor &source,
    const BufferDescriptor &destination, std::string &message);

} // namespace HydroCouple::SDK::Device

#endif // HYDROCOUPLESDK_DEVICE_SPACETRANSFER_H
