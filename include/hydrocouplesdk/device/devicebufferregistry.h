// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   devicebufferregistry.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  DeviceBufferRegistry — maps data items to device allocations
 *         (plan §6.1).
 *
 * Components advertising Capability::DeviceBuffers keep device mirrors of
 * their fields; the registry owns those allocations so lifetimes are
 * managed in exactly one place: acquire() creates (or returns) the mirror
 * for a key, upload()/download() move bytes between the mirror and host
 * memory through the backend, descriptor() views the mirror as a typed
 * BufferDescriptor ready for the data plane, and the registry frees
 * everything it still holds on destruction. Keys are item identifiers, so
 * one registry serves a whole component.
 *
 * \threadsafety Registry mutations are internally locked; concurrent
 * operations on the same allocation's bytes are the caller's to order.
 */

#ifndef HYDROCOUPLESDK_DEVICE_DEVICEBUFFERREGISTRY_H
#define HYDROCOUPLESDK_DEVICE_DEVICEBUFFERREGISTRY_H

#include "hydrocouplesdk/device/devicebackend.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace HydroCouple::SDK::Device {

/*!
 * \brief One device-resident mirror owned by the registry.
 */
struct DeviceAllocation
{
    void        *data     = nullptr;           //!< Backend allocation.
    MemorySpace  space    = MemorySpace::Host; //!< Space it lives in.
    int32_t      deviceId = 0;                 //!< Device ordinal.
    DataKind     kind     = DataKind::Unknown; //!< Element type.
    std::vector<int64_t> shape;                //!< Logical extents.
    int64_t      bytes    = 0;                 //!< Allocation size.
};

/*!
 * \brief Owns the device mirrors of a component's data items.
 */
class HYDROCOUPLESDK_EXPORT DeviceBufferRegistry final
{
public:
    /*!
     * \brief Construct over a backend (borrowed; must outlive the registry).
     */
    explicit DeviceBufferRegistry(DeviceBackend *backend);

    /*!
     * \brief Frees every allocation still registered.
     */
    ~DeviceBufferRegistry();

    DeviceBufferRegistry(const DeviceBufferRegistry &)            = delete;
    DeviceBufferRegistry &operator=(const DeviceBufferRegistry &) = delete;

    /*!
     * \brief The backend allocations go through.
     */
    [[nodiscard]] DeviceBackend *backend() const noexcept;

    /*!
     * \brief Creates the mirror for \p key, or returns the existing one
     *        when kind/shape/space/device all match.
     * \param[in]  key      Data-item identifier.
     * \param[in]  kind     Element type (fixed-width kinds only).
     * \param[in]  shape    Logical extents.
     * \param[in]  space    Target space (must be backend-supported).
     * \param[in]  deviceId Device ordinal.
     * \param[out] message  Failure description on nullptr return. An
     *                      existing key with a different signature is an
     *                      error — release() it first.
     */
    [[nodiscard]] const DeviceAllocation *acquire(
        const std::string &key, DataKind kind,
        const std::vector<int64_t> &shape, MemorySpace space,
        int32_t deviceId = 0, std::string *message = nullptr);

    /*!
     * \brief The mirror for \p key, or nullptr.
     */
    [[nodiscard]] const DeviceAllocation *find(const std::string &key) const;

    /*!
     * \brief Frees and forgets the mirror for \p key.
     * \return False when the key is unknown.
     */
    bool release(const std::string &key);

    /*!
     * \brief Number of live allocations.
     */
    [[nodiscard]] size_t count() const;

    /*!
     * \brief Copies \p bytes from host memory into the mirror.
     */
    [[nodiscard]] bool upload(const std::string &key, const void *hostData,
                              int64_t bytes, std::string *message = nullptr);

    /*!
     * \brief Copies \p bytes from the mirror into host memory.
     */
    [[nodiscard]] bool download(const std::string &key, void *hostData,
                                int64_t bytes,
                                std::string *message = nullptr) const;

    /*!
     * \brief Typed descriptor over the mirror (contiguous; shape memory
     *        owned by the registry — valid until release()).
     * \return False with \p message set when the key is unknown.
     */
    [[nodiscard]] bool descriptor(const std::string &key,
                                  BufferDescriptor &out,
                                  std::string *message = nullptr) const;

private:
    DeviceBackend *m_backend;

    mutable std::mutex                                m_mutex;
    std::unordered_map<std::string, DeviceAllocation> m_allocations;
};

} // namespace HydroCouple::SDK::Device

#endif // HYDROCOUPLESDK_DEVICE_DEVICEBUFFERREGISTRY_H
