// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/device/devicebufferregistry.h"
#include "hydrocouplehelpers.h"

namespace HydroCouple::SDK::Device {

namespace {

//! Product of \p shape's extents (1 for an empty shape), or -1 when an
//! extent is negative.
int64_t elementCount(const std::vector<int64_t> &shape)
{
    int64_t count = 1;
    for (int64_t extent : shape)
    {
        if (extent < 0)
            return -1;
        count *= extent;
    }
    return count;
}

} // namespace

DeviceBufferRegistry::DeviceBufferRegistry(DeviceBackend *backend)
    : m_backend(backend)
{
}

DeviceBufferRegistry::~DeviceBufferRegistry()
{
    for (auto &[key, allocation] : m_allocations)
        m_backend->deallocate(allocation.data, allocation.space,
                              allocation.deviceId);
}

DeviceBackend *DeviceBufferRegistry::backend() const noexcept
{
    return m_backend;
}

const DeviceAllocation *DeviceBufferRegistry::acquire(
    const std::string &key, DataKind kind, const std::vector<int64_t> &shape,
    MemorySpace space, int32_t deviceId, std::string *message)
{
    const int64_t elementSize = Helpers::dataKindSize(kind);
    if (elementSize <= 0)
    {
        if (message)
            *message = "DeviceBufferRegistry: '" + key +
                       "' has no fixed-width element size";
        return nullptr;
    }
    const int64_t elements = elementCount(shape);
    if (elements < 0)
    {
        if (message)
            *message = "DeviceBufferRegistry: '" + key +
                       "' has a negative extent";
        return nullptr;
    }
    const int64_t bytes = elements * elementSize;

    std::scoped_lock lock(m_mutex);
    auto found = m_allocations.find(key);
    if (found != m_allocations.end())
    {
        const DeviceAllocation &existing = found->second;
        if (existing.kind == kind && existing.shape == shape &&
            existing.space == space && existing.deviceId == deviceId)
        {
            if (message)
                message->clear();
            return &existing;
        }
        if (message)
            *message = "DeviceBufferRegistry: '" + key +
                       "' already registered with a different signature "
                       "(release() it first)";
        return nullptr;
    }

    if (!m_backend->supportsSpace(space, deviceId))
    {
        if (message)
            *message = "DeviceBufferRegistry: backend '" +
                       m_backend->name() +
                       "' does not support the requested space/device";
        return nullptr;
    }

    void *data =
        m_backend->allocate(space, deviceId,
                            static_cast<size_t>(bytes > 0 ? bytes : 1),
                            message);
    if (!data)
        return nullptr;

    DeviceAllocation allocation;
    allocation.data     = data;
    allocation.space    = space;
    allocation.deviceId = deviceId;
    allocation.kind     = kind;
    allocation.shape    = shape;
    allocation.bytes    = bytes;

    auto [inserted, ok] = m_allocations.emplace(key, std::move(allocation));
    if (message)
        message->clear();
    return &inserted->second;
}

const DeviceAllocation *
DeviceBufferRegistry::find(const std::string &key) const
{
    std::scoped_lock lock(m_mutex);
    auto found = m_allocations.find(key);
    return found == m_allocations.end() ? nullptr : &found->second;
}

bool DeviceBufferRegistry::release(const std::string &key)
{
    std::scoped_lock lock(m_mutex);
    auto found = m_allocations.find(key);
    if (found == m_allocations.end())
        return false;
    m_backend->deallocate(found->second.data, found->second.space,
                          found->second.deviceId);
    m_allocations.erase(found);
    return true;
}

size_t DeviceBufferRegistry::count() const
{
    std::scoped_lock lock(m_mutex);
    return m_allocations.size();
}

bool DeviceBufferRegistry::upload(const std::string &key,
                                  const void *hostData, int64_t bytes,
                                  std::string *message)
{
    const DeviceAllocation *allocation = find(key);
    if (!allocation)
    {
        if (message)
            *message = "DeviceBufferRegistry: unknown key '" + key + "'";
        return false;
    }
    if (bytes != allocation->bytes)
    {
        if (message)
            *message = "DeviceBufferRegistry: upload of " +
                       std::to_string(bytes) + " bytes into '" + key +
                       "' (" + std::to_string(allocation->bytes) + " bytes)";
        return false;
    }
    return m_backend->copy(allocation->data, allocation->space,
                           allocation->deviceId, hostData, MemorySpace::Host,
                           0, static_cast<size_t>(bytes), message);
}

bool DeviceBufferRegistry::download(const std::string &key, void *hostData,
                                    int64_t bytes,
                                    std::string *message) const
{
    const DeviceAllocation *allocation = find(key);
    if (!allocation)
    {
        if (message)
            *message = "DeviceBufferRegistry: unknown key '" + key + "'";
        return false;
    }
    if (bytes != allocation->bytes)
    {
        if (message)
            *message = "DeviceBufferRegistry: download of " +
                       std::to_string(bytes) + " bytes from '" + key +
                       "' (" + std::to_string(allocation->bytes) + " bytes)";
        return false;
    }
    return m_backend->copy(hostData, MemorySpace::Host, 0, allocation->data,
                           allocation->space, allocation->deviceId,
                           static_cast<size_t>(bytes), message);
}

bool DeviceBufferRegistry::descriptor(const std::string &key,
                                      BufferDescriptor &out,
                                      std::string *message) const
{
    std::scoped_lock lock(m_mutex);
    auto found = m_allocations.find(key);
    if (found == m_allocations.end())
    {
        if (message)
            *message = "DeviceBufferRegistry: unknown key '" + key + "'";
        return false;
    }
    const DeviceAllocation &allocation = found->second;
    out.data         = allocation.data;
    out.kind         = allocation.kind;
    out.rank         = static_cast<int32_t>(allocation.shape.size());
    out.shape        = allocation.shape.data();
    out.stridesBytes = nullptr;
    out.space        = allocation.space;
    out.deviceId     = allocation.deviceId;
    if (message)
        message->clear();
    return true;
}

} // namespace HydroCouple::SDK::Device
