// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   kokkosbackend.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  KokkosBackend — the first accelerator DeviceBackend (plan §6.2),
 *         built on Kokkos behind the SDK's own abstraction.
 *
 * Kokkos is an implementation detail, never a public dependency: this
 * header names no Kokkos type (the implementation file holds them all),
 * and the source is compiled only under USE_KOKKOS (vcpkg \c gpu family
 * features). Whatever execution space Kokkos was configured with — Serial,
 * OpenMP, CUDA, HIP, SYCL — its default memory space serves
 * MemorySpace::Device allocations, so the same conformance suite proves
 * the seam on a laptop (serial host memory as the "device") and on an
 * accelerator without any test change.
 *
 * The backend initializes Kokkos when nothing else has (and finalizes it
 * only if it initialized it); executables that manage Kokkos themselves
 * (ScopeGuard, test environments) are respected.
 *
 * \threadsafety See DeviceBackend.
 */

#ifndef HYDROCOUPLESDK_DEVICE_KOKKOSBACKEND_H
#define HYDROCOUPLESDK_DEVICE_KOKKOSBACKEND_H

#include "hydrocouplesdk/device/devicebackend.h"

namespace HydroCouple::SDK::Device {

/*!
 * \brief Kokkos implementation of the DeviceBackend seam.
 */
class HYDROCOUPLESDK_EXPORT KokkosBackend final : public DeviceBackend
{
public:
    KokkosBackend();
    ~KokkosBackend() override;

    KokkosBackend(const KokkosBackend &)            = delete;
    KokkosBackend &operator=(const KokkosBackend &) = delete;

    /*!
     * \brief "kokkos-" + the configured default execution space name.
     */
    [[nodiscard]] std::string name() const override;

    [[nodiscard]] int32_t deviceCount() const override;

    /*!
     * \brief Device (ordinal 0) through Kokkos' default memory space, plus
     *        the host spaces (served host-side for staging copies).
     */
    [[nodiscard]] bool supportsSpace(MemorySpace space,
                                     int32_t deviceId = 0) const override;

    [[nodiscard]] void *allocate(MemorySpace space, int32_t deviceId,
                                 size_t bytes,
                                 std::string *message = nullptr) override;

    void deallocate(void *data, MemorySpace space,
                    int32_t deviceId) noexcept override;

    /*!
     * \brief Byte copies over Kokkos deep_copy between unmanaged views,
     *        covering host↔host, host↔device, and device↔device.
     */
    [[nodiscard]] bool copy(void *destination, MemorySpace destinationSpace,
                            int32_t destinationDevice, const void *source,
                            MemorySpace sourceSpace, int32_t sourceDevice,
                            size_t bytes,
                            std::string *message = nullptr) override;

    /*!
     * \brief Kokkos fence.
     */
    [[nodiscard]] bool synchronize(int32_t deviceId,
                                   std::string *message = nullptr) override;

private:
    bool m_ownsKokkos = false; //!< True when this backend initialized Kokkos.
};

} // namespace HydroCouple::SDK::Device

#endif // HYDROCOUPLESDK_DEVICE_KOKKOSBACKEND_H
