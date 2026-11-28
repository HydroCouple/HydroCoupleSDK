// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/device/kokkosbackend.h"

#include <Kokkos_Core.hpp>

namespace HydroCouple::SDK::Device {

namespace {

//! The memory space MemorySpace::Device allocations come from.
using DeviceMemorySpace =
    Kokkos::DefaultExecutionSpace::memory_space;

//! An unmanaged byte view over caller memory in \p Space (deep_copy's
//! operand type).
template <typename Space>
using ByteView =
    Kokkos::View<std::byte *, Space, Kokkos::MemoryTraits<Kokkos::Unmanaged>>;

//! Whether \p space is served from Kokkos::HostSpace (Host, HostPinned,
//! Unified).
bool isHostSide(MemorySpace space)
{
    return space == MemorySpace::Host || space == MemorySpace::HostPinned ||
           space == MemorySpace::Unified;
}

} // namespace

KokkosBackend::KokkosBackend()
{
    if (!Kokkos::is_initialized())
    {
        Kokkos::initialize();
        m_ownsKokkos = true;
    }
}

KokkosBackend::~KokkosBackend()
{
    if (m_ownsKokkos && Kokkos::is_initialized() && !Kokkos::is_finalized())
        Kokkos::finalize();
}

std::string KokkosBackend::name() const
{
    return std::string("kokkos-") + Kokkos::DefaultExecutionSpace::name();
}

int32_t KokkosBackend::deviceCount() const
{
    return 1; // Kokkos' default execution space addresses one device.
}

bool KokkosBackend::supportsSpace(MemorySpace space, int32_t deviceId) const
{
    if (deviceId != 0)
        return false;
    return space == MemorySpace::Device || isHostSide(space);
}

void *KokkosBackend::allocate(MemorySpace space, int32_t deviceId,
                              size_t bytes, std::string *message)
{
    if (!supportsSpace(space, deviceId))
    {
        if (message)
            *message = "KokkosBackend: unsupported space/device";
        return nullptr;
    }
    if (bytes == 0)
    {
        if (message)
            *message = "KokkosBackend: zero-byte allocation";
        return nullptr;
    }
    void *data = nullptr;
    try
    {
        if (space == MemorySpace::Device)
            data = Kokkos::kokkos_malloc<DeviceMemorySpace>(bytes);
        else
            data = Kokkos::kokkos_malloc<Kokkos::HostSpace>(bytes);
    }
    catch (const std::exception &exception)
    {
        if (message)
            *message = std::string("KokkosBackend: ") + exception.what();
        return nullptr;
    }
    if (!data && message)
        *message = "KokkosBackend: allocation of " + std::to_string(bytes) +
                   " bytes failed";
    if (data && message)
        message->clear();
    return data;
}

void KokkosBackend::deallocate(void *data, MemorySpace space,
                               int32_t) noexcept
{
    if (!data)
        return;
    try
    {
        if (space == MemorySpace::Device)
            Kokkos::kokkos_free<DeviceMemorySpace>(data);
        else
            Kokkos::kokkos_free<Kokkos::HostSpace>(data);
    }
    catch (...)
    {
        // noexcept contract: a free failure at teardown is unreportable.
    }
}

bool KokkosBackend::copy(void *destination, MemorySpace destinationSpace,
                         int32_t destinationDevice, const void *source,
                         MemorySpace sourceSpace, int32_t sourceDevice,
                         size_t bytes, std::string *message)
{
    if (!supportsSpace(destinationSpace, destinationDevice) ||
        !supportsSpace(sourceSpace, sourceDevice))
    {
        if (message)
            *message = "KokkosBackend: copy involves an unsupported space";
        return false;
    }
    if (!destination || !source)
    {
        if (message)
            *message = "KokkosBackend: null copy endpoint";
        return false;
    }

    auto *sourceBytes =
        const_cast<std::byte *>(static_cast<const std::byte *>(source));
    auto *destinationBytes = static_cast<std::byte *>(destination);
    const bool sourceOnHost      = isHostSide(sourceSpace);
    const bool destinationOnHost = isHostSide(destinationSpace);

    try
    {
        if (sourceOnHost && destinationOnHost)
        {
            Kokkos::deep_copy(
                ByteView<Kokkos::HostSpace>(destinationBytes, bytes),
                ByteView<Kokkos::HostSpace>(sourceBytes, bytes));
        }
        else if (sourceOnHost)
        {
            Kokkos::deep_copy(
                ByteView<DeviceMemorySpace>(destinationBytes, bytes),
                ByteView<Kokkos::HostSpace>(sourceBytes, bytes));
        }
        else if (destinationOnHost)
        {
            Kokkos::deep_copy(
                ByteView<Kokkos::HostSpace>(destinationBytes, bytes),
                ByteView<DeviceMemorySpace>(sourceBytes, bytes));
        }
        else
        {
            Kokkos::deep_copy(
                ByteView<DeviceMemorySpace>(destinationBytes, bytes),
                ByteView<DeviceMemorySpace>(sourceBytes, bytes));
        }
        Kokkos::fence();
    }
    catch (const std::exception &exception)
    {
        if (message)
            *message = std::string("KokkosBackend: ") + exception.what();
        return false;
    }
    if (message)
        message->clear();
    return true;
}

bool KokkosBackend::synchronize(int32_t, std::string *message)
{
    Kokkos::fence();
    if (message)
        message->clear();
    return true;
}

} // namespace HydroCouple::SDK::Device
