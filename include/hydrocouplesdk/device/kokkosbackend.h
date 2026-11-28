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
 *
 * Available only when the SDK is built with USE_KOKKOS=ON (the class is
 * declared in every build, but its definitions are compiled only then).
 * MemorySpace::Device maps to the default execution space's memory space;
 * Host, HostPinned and Unified map to Kokkos::HostSpace, so Unified is not
 * managed memory under this backend. Kokkos exceptions are caught and
 * returned as messages prefixed "KokkosBackend: ".
 */
class HYDROCOUPLESDK_EXPORT KokkosBackend final : public DeviceBackend
{
public:
    //! Calls Kokkos::initialize() when Kokkos is not yet initialized, and
    //! then owns that initialization. Kokkos cannot be initialized twice in
    //! one process, so keep one owning backend alive for the run.
    KokkosBackend();
    //! Calls Kokkos::finalize() when this backend initialized Kokkos.
    ~KokkosBackend() override;

    KokkosBackend(const KokkosBackend &)            = delete;
    KokkosBackend &operator=(const KokkosBackend &) = delete;

    /*!
     * \brief "kokkos-" + the configured default execution space name.
     * \return "kokkos-" followed by Kokkos::DefaultExecutionSpace::name().
     */
    [[nodiscard]] std::string name() const override;

    /*!
     * \brief Device ordinals: the default execution space's one device.
     * \return 1.
     */
    [[nodiscard]] int32_t deviceCount() const override;

    /*!
     * \brief Device (ordinal 0) through Kokkos' default memory space, plus
     *        the host spaces (served host-side for staging copies).
     * \param[in] space    Memory space to ask about.
     * \param[in] deviceId Device ordinal; only 0 is supported.
     * \return True for every space with \p deviceId 0.
     */
    [[nodiscard]] bool supportsSpace(MemorySpace space,
                                     int32_t deviceId = 0) const override;

    /*!
     * \brief Allocates with Kokkos::kokkos_malloc in the mapped space.
     * \param[in]  space    Any space (see supportsSpace()).
     * \param[in]  deviceId 0.
     * \param[in]  bytes    Size in bytes (> 0).
     * \param[out] message  Optional: "KokkosBackend: unsupported
     *                      space/device", "KokkosBackend: zero-byte
     *                      allocation", a Kokkos exception text, or
     *                      "KokkosBackend: allocation of N bytes failed".
     * \return The allocation, or nullptr.
     */
    [[nodiscard]] void *allocate(MemorySpace space, int32_t deviceId,
                                 size_t bytes,
                                 std::string *message = nullptr) override;

    /*!
     * \brief Frees with Kokkos::kokkos_free in the mapped space; null is a
     *        no-op and exceptions are swallowed.
     * \param[in] data     The allocation (or nullptr).
     * \param[in] space    The space it was allocated in.
     * \param[in] deviceId Ignored.
     */
    void deallocate(void *data, MemorySpace space,
                    int32_t deviceId) noexcept override;

    /*!
     * \brief Byte copies over Kokkos deep_copy between unmanaged views,
     *        covering host↔host, host↔device, and device↔device.
     *
     * Fences after the copy, so it is complete on return.
     *
     * \param[out] destination       Destination address (not null).
     * \param[in]  destinationSpace  Space of \p destination.
     * \param[in]  destinationDevice 0.
     * \param[in]  source            Source address (not null).
     * \param[in]  sourceSpace       Space of \p source.
     * \param[in]  sourceDevice      0.
     * \param[in]  bytes             Bytes to copy.
     * \param[out] message           Optional: "KokkosBackend: copy involves
     *                               an unsupported space", "KokkosBackend:
     *                               null copy endpoint", or a Kokkos
     *                               exception text.
     * \return True when copied.
     */
    [[nodiscard]] bool copy(void *destination, MemorySpace destinationSpace,
                            int32_t destinationDevice, const void *source,
                            MemorySpace sourceSpace, int32_t sourceDevice,
                            size_t bytes,
                            std::string *message = nullptr) override;

    /*!
     * \brief Kokkos fence.
     * \param[in]  deviceId Ignored (one device).
     * \param[out] message  Optional; cleared.
     * \return Always true.
     */
    [[nodiscard]] bool synchronize(int32_t deviceId,
                                   std::string *message = nullptr) override;

private:
    bool m_ownsKokkos = false; //!< True when this backend initialized Kokkos.
};

} // namespace HydroCouple::SDK::Device

#endif // HYDROCOUPLESDK_DEVICE_KOKKOSBACKEND_H
