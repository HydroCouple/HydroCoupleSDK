// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   devicebackend.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  DeviceBackend — the SDK's vendor-neutral accelerator seam
 *         (plan §6), plus the always-present HostBackend.
 *
 * The GPU abstraction follows the same seam discipline as the transport
 * layer: BufferDescriptor already carries a MemorySpace tag and a device
 * ordinal, and everything the SDK does with device memory — allocate,
 * free, copy between space pairs, synchronize — goes through this small
 * abstract interface. No vendor type (CUDA, HIP, SYCL, Kokkos) ever
 * appears in a public SDK header; accelerator backends implement this
 * interface behind their own build gates (KokkosBackend under USE_KOKKOS
 * is the first), and HostBackend is always present so code paths that
 * negotiate spaces need no special casing on machines without devices.
 *
 * \threadsafety Backend implementations must tolerate concurrent calls
 * from different threads for different allocations; operations on the
 * same allocation are the caller's to order.
 */

#ifndef HYDROCOUPLESDK_DEVICE_DEVICEBACKEND_H
#define HYDROCOUPLESDK_DEVICE_DEVICEBACKEND_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"

#include <cstddef>
#include <string>

namespace HydroCouple::SDK::Device {

/*!
 * \brief Abstract accelerator backend: allocation, space-pair copies, and
 *        synchronization for one vendor runtime.
 */
class HYDROCOUPLESDK_EXPORT DeviceBackend
{
public:
    virtual ~DeviceBackend() = default;

    /*!
     * \brief Human-readable backend name (for example "host", "kokkos-serial").
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /*!
     * \brief Number of device ordinals this backend can address
     *        (0 for a host-only backend).
     */
    [[nodiscard]] virtual int32_t deviceCount() const = 0;

    /*!
     * \brief True when the backend can allocate in \p space on \p deviceId.
     */
    [[nodiscard]] virtual bool supportsSpace(MemorySpace space,
                                             int32_t deviceId = 0) const = 0;

    /*!
     * \brief Allocates \p bytes in \p space on \p deviceId.
     * \return The allocation, or nullptr with \p message set.
     */
    [[nodiscard]] virtual void *allocate(MemorySpace space, int32_t deviceId,
                                         size_t bytes,
                                         std::string *message = nullptr) = 0;

    /*!
     * \brief Frees an allocation obtained from allocate(). Null is a no-op.
     */
    virtual void deallocate(void *data, MemorySpace space,
                            int32_t deviceId) noexcept = 0;

    /*!
     * \brief Copies \p bytes between any supported space pair
     *        (host↔host, host↔device, device↔device).
     * \return False with \p message set when the pair is unsupported or
     *         the copy failed.
     */
    [[nodiscard]] virtual bool copy(void *destination,
                                    MemorySpace destinationSpace,
                                    int32_t destinationDevice,
                                    const void *source,
                                    MemorySpace sourceSpace,
                                    int32_t sourceDevice, size_t bytes,
                                    std::string *message = nullptr) = 0;

    /*!
     * \brief Blocks until all outstanding operations on \p deviceId are
     *        complete (no-op for synchronous backends).
     */
    [[nodiscard]] virtual bool synchronize(int32_t deviceId,
                                           std::string *message = nullptr) = 0;
};

/*!
 * \brief The always-present host backend.
 *
 * Supports Host, HostPinned, and Unified (all served from ordinary host
 * memory — pinning is a device-runtime concept, so without one the
 * distinction is a tag, not a behavior), rejects Device, and copies with
 * memcpy. Guarantees space-negotiation code has a backend on every
 * machine.
 */
class HYDROCOUPLESDK_EXPORT HostBackend final : public DeviceBackend
{
public:
    HostBackend()           = default;
    ~HostBackend() override = default;

    [[nodiscard]] std::string name() const override;
    [[nodiscard]] int32_t     deviceCount() const override;
    [[nodiscard]] bool supportsSpace(MemorySpace space,
                                     int32_t deviceId = 0) const override;
    [[nodiscard]] void *allocate(MemorySpace space, int32_t deviceId,
                                 size_t bytes,
                                 std::string *message = nullptr) override;
    void deallocate(void *data, MemorySpace space,
                    int32_t deviceId) noexcept override;
    [[nodiscard]] bool copy(void *destination, MemorySpace destinationSpace,
                            int32_t destinationDevice, const void *source,
                            MemorySpace sourceSpace, int32_t sourceDevice,
                            size_t bytes,
                            std::string *message = nullptr) override;
    [[nodiscard]] bool synchronize(int32_t deviceId,
                                   std::string *message = nullptr) override;
};

} // namespace HydroCouple::SDK::Device

#endif // HYDROCOUPLESDK_DEVICE_DEVICEBACKEND_H
