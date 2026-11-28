// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/device/devicebackend.h"

#include <cstdlib>
#include <cstring>

namespace HydroCouple::SDK::Device {

namespace {

//! Whether \p space is served from ordinary host memory (Host, HostPinned,
//! Unified).
bool hostServable(MemorySpace space)
{
    return space == MemorySpace::Host || space == MemorySpace::HostPinned ||
           space == MemorySpace::Unified;
}

} // namespace

std::string HostBackend::name() const
{
    return "host";
}

int32_t HostBackend::deviceCount() const
{
    return 0;
}

bool HostBackend::supportsSpace(MemorySpace space, int32_t deviceId) const
{
    return deviceId == 0 && hostServable(space);
}

void *HostBackend::allocate(MemorySpace space, int32_t deviceId, size_t bytes,
                            std::string *message)
{
    if (!supportsSpace(space, deviceId))
    {
        if (message)
            *message = "HostBackend: unsupported space/device";
        return nullptr;
    }
    if (bytes == 0)
    {
        if (message)
            *message = "HostBackend: zero-byte allocation";
        return nullptr;
    }
    void *data = std::malloc(bytes);
    if (!data && message)
        *message = "HostBackend: allocation of " + std::to_string(bytes) +
                   " bytes failed";
    if (data && message)
        message->clear();
    return data;
}

void HostBackend::deallocate(void *data, MemorySpace, int32_t) noexcept
{
    std::free(data);
}

bool HostBackend::copy(void *destination, MemorySpace destinationSpace,
                       int32_t destinationDevice, const void *source,
                       MemorySpace sourceSpace, int32_t sourceDevice,
                       size_t bytes, std::string *message)
{
    if (!supportsSpace(destinationSpace, destinationDevice) ||
        !supportsSpace(sourceSpace, sourceDevice))
    {
        if (message)
            *message = "HostBackend: copy involves an unsupported space";
        return false;
    }
    if (!destination || !source)
    {
        if (message)
            *message = "HostBackend: null copy endpoint";
        return false;
    }
    std::memcpy(destination, source, bytes);
    if (message)
        message->clear();
    return true;
}

bool HostBackend::synchronize(int32_t, std::string *message)
{
    if (message)
        message->clear();
    return true;
}

} // namespace HydroCouple::SDK::Device
