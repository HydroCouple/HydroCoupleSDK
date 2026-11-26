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
 * This class is the SDK's own interface, not part of the HydroCouple
 * standard. It is unrelated to the standard's HydroCouple::DeviceBackend
 * enumeration (the runtime tag a BufferDescriptor carries): the SDK's
 * device code reads a descriptor's \c space and \c deviceId, never its
 * \c backend or \c queue, and every copy is synchronous.
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
 *
 * Implement it to bring a new runtime to the SDK; DeviceBufferRegistry and
 * copyBuffer() use nothing else. A backend reports failure through a false
 * or null return and an optional message, never by throwing.
 */
class HYDROCOUPLESDK_EXPORT DeviceBackend
{
public:
    //! Destroys the backend; allocations it made must already be freed.
    virtual ~DeviceBackend() = default;

    /*!
     * \brief Human-readable backend name (for example "host", or "kokkos-"
     *        followed by the Kokkos execution space name).
     * \return The name; used in DeviceBufferRegistry and copyBuffer() messages.
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /*!
     * \brief Number of device ordinals this backend can address
     *        (0 for a host-only backend).
     * \return The count of valid \c deviceId values for MemorySpace::Device.
     */
    [[nodiscard]] virtual int32_t deviceCount() const = 0;

    /*!
     * \brief True when the backend can allocate in \p space on \p deviceId.
     * \param[in] space    Memory space to ask about.
     * \param[in] deviceId Device ordinal (0 for host spaces).
     * \return Whether allocate() and copy() accept the pair.
     */
    [[nodiscard]] virtual bool supportsSpace(MemorySpace space,
                                             int32_t deviceId = 0) const = 0;

    /*!
     * \brief Allocates \p bytes in \p space on \p deviceId.
     * \param[in]  space    Memory space; must satisfy supportsSpace().
     * \param[in]  deviceId Device ordinal.
     * \param[in]  bytes    Size in bytes (> 0).
     * \param[out] message  Optional; the failure reason, cleared on success.
     * \return The allocation, or nullptr with \p message set. The caller
     *         owns it and frees it with deallocate() on the same backend,
     *         space and device.
     */
    [[nodiscard]] virtual void *allocate(MemorySpace space, int32_t deviceId,
                                         size_t bytes,
                                         std::string *message = nullptr) = 0;

    /*!
     * \brief Frees an allocation obtained from allocate(). Null is a no-op.
     * \param[in] data     The allocation (or nullptr).
     * \param[in] space    The space it was allocated in.
     * \param[in] deviceId The device it was allocated on.
     */
    virtual void deallocate(void *data, MemorySpace space,
                            int32_t deviceId) noexcept = 0;

    /*!
     * \brief Copies \p bytes between any supported space pair
     *        (host↔host, host↔device, device↔device).
     *
     * The copy is complete when the call returns.
     *
     * \param[out] destination       Destination address (not null).
     * \param[in]  destinationSpace  Space of \p destination.
     * \param[in]  destinationDevice Device of \p destination.
     * \param[in]  source            Source address (not null).
     * \param[in]  sourceSpace       Space of \p source.
     * \param[in]  sourceDevice      Device of \p source.
     * \param[in]  bytes             Bytes to copy; both ranges must hold at
     *                               least this many.
     * \param[out] message           Optional; the failure reason, cleared on
     *                               success.
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
     * \param[in]  deviceId Device ordinal.
     * \param[out] message  Optional; the failure reason, cleared on success.
     * \return True when the device is idle.
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
    //! Creates the backend; it holds no state.
    HostBackend()           = default;
    //! Destroys the backend.
    ~HostBackend() override = default;

    /*!
     * \brief The backend's name.
     * \return "host".
     */
    [[nodiscard]] std::string name() const override;
    /*!
     * \brief Device ordinals: none.
     * \return 0.
     */
    [[nodiscard]] int32_t     deviceCount() const override;
    /*!
     * \brief True for the host spaces on ordinal 0.
     * \param[in] space    Memory space to ask about.
     * \param[in] deviceId Device ordinal; only 0 is supported.
     * \return True for Host, HostPinned and Unified with \p deviceId 0;
     *         false for Device or any other ordinal.
     */
    [[nodiscard]] bool supportsSpace(MemorySpace space,
                                     int32_t deviceId = 0) const override;
    /*!
     * \brief Allocates ordinary host memory with std::malloc.
     * \param[in]  space    Host, HostPinned or Unified (all served alike).
     * \param[in]  deviceId 0.
     * \param[in]  bytes    Size in bytes (> 0).
     * \param[out] message  Optional: "HostBackend: unsupported space/device",
     *                      "HostBackend: zero-byte allocation" or
     *                      "HostBackend: allocation of N bytes failed";
     *                      cleared on success.
     * \return The allocation (free it with deallocate()), or nullptr.
     */
    [[nodiscard]] void *allocate(MemorySpace space, int32_t deviceId,
                                 size_t bytes,
                                 std::string *message = nullptr) override;
    /*!
     * \brief Frees with std::free; null is a no-op.
     * \param[in] data     The allocation (or nullptr).
     * \param[in] space    Ignored.
     * \param[in] deviceId Ignored.
     */
    void deallocate(void *data, MemorySpace space,
                    int32_t deviceId) noexcept override;
    /*!
     * \brief Copies with std::memcpy between host-servable spaces.
     *
     * The ranges must not overlap.
     *
     * \param[out] destination       Destination address (not null).
     * \param[in]  destinationSpace  Host, HostPinned or Unified.
     * \param[in]  destinationDevice 0.
     * \param[in]  source            Source address (not null).
     * \param[in]  sourceSpace       Host, HostPinned or Unified.
     * \param[in]  sourceDevice      0.
     * \param[in]  bytes             Bytes to copy.
     * \param[out] message           Optional: "HostBackend: copy involves an
     *                               unsupported space" or "HostBackend: null
     *                               copy endpoint"; cleared on success.
     * \return True when copied.
     */
    [[nodiscard]] bool copy(void *destination, MemorySpace destinationSpace,
                            int32_t destinationDevice, const void *source,
                            MemorySpace sourceSpace, int32_t sourceDevice,
                            size_t bytes,
                            std::string *message = nullptr) override;
    /*!
     * \brief Nothing to wait for: every host copy is synchronous.
     * \param[in]  deviceId Ignored.
     * \param[out] message  Optional; cleared.
     * \return Always true.
     */
    [[nodiscard]] bool synchronize(int32_t deviceId,
                                   std::string *message = nullptr) override;
};

} // namespace HydroCouple::SDK::Device

#endif // HYDROCOUPLESDK_DEVICE_DEVICEBACKEND_H
