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
 *
 * The registry fills it and frees \c data; callers read it through the
 * const pointer acquire() and find() return, which stays valid until the
 * key is released or the registry is destroyed.
 */
struct DeviceAllocation
{
    void        *data     = nullptr;           //!< Backend allocation.
    MemorySpace  space    = MemorySpace::Host; //!< Space it lives in.
    int32_t      deviceId = 0;                 //!< Device ordinal.
    DataKind     kind     = DataKind::Unknown; //!< Element type.
    std::vector<int64_t> shape;                //!< Logical extents.
    int64_t      bytes    = 0;                 //!< Logical size: element count × element size (a zero-element mirror still holds a 1-byte allocation).
};

/*!
 * \brief Owns the device mirrors of a component's data items.
 *
 * Typical use: acquire() a mirror per item at prepare(), upload() host
 * values before a kernel and download() after it, hand descriptor() views
 * to copyBuffer() or to a device-capable data plane, release() or destroy
 * the registry at finish().
 */
class HYDROCOUPLESDK_EXPORT DeviceBufferRegistry final
{
public:
    /*!
     * \brief Construct over a backend (borrowed; must outlive the registry).
     * \param[in] backend Backend every allocation and copy goes through (not
     *                    null).
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
     * \return The borrowed backend given to the constructor.
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
     *                      error — release() it first. Messages:
     *                      "DeviceBufferRegistry: 'KEY' has no fixed-width
     *                      element size", "... 'KEY' has a negative extent",
     *                      "... 'KEY' already registered with a different
     *                      signature (release() it first)", "...: backend
     *                      'NAME' does not support the requested
     *                      space/device", or the backend's allocate() message.
     *                      Cleared on success.
     * \return The mirror (owned by the registry; valid until release() or
     *         destruction), or nullptr. Its contents are uninitialized.
     */
    [[nodiscard]] const DeviceAllocation *acquire(
        const std::string &key, DataKind kind,
        const std::vector<int64_t> &shape, MemorySpace space,
        int32_t deviceId = 0, std::string *message = nullptr);

    /*!
     * \brief The mirror for \p key, or nullptr.
     * \param[in] key Data-item identifier.
     * \return The registered mirror, or nullptr when the key is unknown.
     */
    [[nodiscard]] const DeviceAllocation *find(const std::string &key) const;

    /*!
     * \brief Frees and forgets the mirror for \p key.
     *
     * Pointers and descriptors obtained for the key become invalid.
     *
     * \param[in] key Data-item identifier.
     * \return False when the key is unknown.
     */
    bool release(const std::string &key);

    /*!
     * \brief Number of live allocations.
     * \return Registered keys.
     */
    [[nodiscard]] size_t count() const;

    /*!
     * \brief Copies \p bytes from host memory into the mirror.
     *
     * The copy runs outside the registry's lock; a concurrent release() of
     * the same key is the caller's to prevent.
     *
     * \param[in]  key      Data-item identifier.
     * \param[in]  hostData Contiguous host source of \p bytes bytes.
     * \param[in]  bytes    Must equal the mirror's DeviceAllocation::bytes.
     * \param[out] message  Optional: "DeviceBufferRegistry: unknown key
     *                      'KEY'", "DeviceBufferRegistry: upload of N bytes
     *                      into 'KEY' (M bytes)", or the backend's copy()
     *                      message.
     * \return True when copied.
     */
    [[nodiscard]] bool upload(const std::string &key, const void *hostData,
                              int64_t bytes, std::string *message = nullptr);

    /*!
     * \brief Copies \p bytes from the mirror into host memory.
     * \param[in]  key      Data-item identifier.
     * \param[out] hostData Contiguous host destination of \p bytes bytes.
     * \param[in]  bytes    Must equal the mirror's DeviceAllocation::bytes.
     * \param[out] message  Optional: unknown key, "DeviceBufferRegistry:
     *                      download of N bytes from 'KEY' (M bytes)", or the
     *                      backend's copy() message.
     * \return True when copied.
     */
    [[nodiscard]] bool download(const std::string &key, void *hostData,
                                int64_t bytes,
                                std::string *message = nullptr) const;

    /*!
     * \brief Typed descriptor over the mirror (contiguous; shape memory
     *        owned by the registry — valid until release()).
     *
     * Sets \c data, \c kind, \c rank, \c shape, \c stridesBytes (null),
     * \c space and \c deviceId of \p out. \c backend, \c itemSizeBytes
     * and \c queue are left as \p out held them, so start from a
     * default-constructed descriptor and set \c backend yourself when a
     * consumer needs the runtime named.
     *
     * \param[in]  key     Data-item identifier.
     * \param[out] out     Descriptor to fill.
     * \param[out] message Optional: "DeviceBufferRegistry: unknown key
     *                     'KEY'"; cleared on success.
     * \return False with \p message set when the key is unknown.
     */
    [[nodiscard]] bool descriptor(const std::string &key,
                                  BufferDescriptor &out,
                                  std::string *message = nullptr) const;

private:
    DeviceBackend *m_backend; //!< Borrowed backend.

    mutable std::mutex                                m_mutex;       //!< Guards \c m_allocations.
    std::unordered_map<std::string, DeviceAllocation> m_allocations; //!< Mirrors by key.
};

} // namespace HydroCouple::SDK::Device

#endif // HYDROCOUPLESDK_DEVICE_DEVICEBUFFERREGISTRY_H
