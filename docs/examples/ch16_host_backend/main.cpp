// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 16: the host backend, a registry of mirrors, and the
 *         space-aware copy negotiation, checked value by value.
 */

#include "hydrocouplesdk/device/devicebackend.h"
#include "hydrocouplesdk/device/devicebufferregistry.h"
#include "hydrocouplesdk/device/spacetransfer.h"

#include <array>
#include <cstdio>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK::Device;

namespace {

const char *pathName(CopyPath path)
{
    switch (path)
    {
    case CopyPath::HostHyperslab:    return "HostHyperslab";
    case CopyPath::BackendCopy:      return "BackendCopy";
    case CopyPath::StageThroughHost: return "StageThroughHost";
    case CopyPath::Unsupported:      return "Unsupported";
    }
    return "?";
}

BufferDescriptor hostView(double *data, const int64_t *shape, int32_t rank,
                          const int64_t *stridesBytes = nullptr)
{
    BufferDescriptor descriptor;
    descriptor.data         = data;
    descriptor.kind         = DataKind::Float64;
    descriptor.rank         = rank;
    descriptor.shape        = shape;
    descriptor.stridesBytes = stridesBytes;
    descriptor.space        = MemorySpace::Host;
    return descriptor;
}

int fail(const std::string &what)
{
    std::fprintf(stderr, "FAILED: %s\n", what.c_str());
    return 1;
}

} // namespace

int main()
{
    //! [backend]
    HostBackend backend;
    std::printf("backend \"%s\", %d device ordinals\n", backend.name().c_str(),
                backend.deviceCount());
    const std::array<std::pair<MemorySpace, const char *>, 4> spaces{{
        {MemorySpace::Host, "Host"},
        {MemorySpace::HostPinned, "HostPinned"},
        {MemorySpace::Unified, "Unified"},
        {MemorySpace::Device, "Device"},
    }};
    for (const auto &[space, name] : spaces)
        std::printf("  supportsSpace(%s) = %s\n", name,
                    backend.supportsSpace(space) ? "true" : "false");
    if (backend.supportsSpace(MemorySpace::Device))
        return fail("the host backend has no device");
    //! [backend]

    //! [registry]
    // One registry per component: mirrors keyed by item id.
    DeviceBufferRegistry registry(&backend);
    std::string message;
    const DeviceAllocation *depth = registry.acquire(
        "depth", DataKind::Float64, {2, 3}, MemorySpace::Unified, 0, &message);
    if (!depth)
        return fail(message);
    std::printf("mirror 'depth': %lld bytes, rank %zu\n",
                static_cast<long long>(depth->bytes), depth->shape.size());

    const std::array<double, 6> values{0.1, 0.2, 0.3, 1.1, 1.2, 1.3}; // m
    if (!registry.upload("depth", values.data(), sizeof(values), &message))
        return fail(message);

    // The same key with another signature is refused; so is a device mirror
    // on a backend without devices.
    const bool sameAgain = registry.acquire("depth", DataKind::Float64, {2, 3},
                                            MemorySpace::Unified, 0, &message) == depth;
    const bool otherShape = registry.acquire("depth", DataKind::Float64, {6},
                                             MemorySpace::Unified, 0, &message) != nullptr;
    std::printf("acquire {6}: %s\n", message.c_str());
    const bool onDevice = registry.acquire("velocity", DataKind::Float64, {6},
                                           MemorySpace::Device, 0, &message) != nullptr;
    std::printf("acquire Device: %s\n", message.c_str());
    if (!sameAgain || otherShape || onDevice)
        return fail("registry signature rules");
    //! [registry]

    //! [negotiate]
    // A descriptor over the mirror, and three host views of other layouts.
    BufferDescriptor mirror;
    if (!registry.descriptor("depth", mirror, &message))
        return fail(message);

    std::array<double, 6> flat{};
    const int64_t six = 6;
    const BufferDescriptor flatView = hostView(flat.data(), &six, 1);

    // Every other element of a 12-element array: a strided view of 6.
    std::array<double, 12> spread{};
    const int64_t stride = 2 * sizeof(double);
    const BufferDescriptor stridedView = hostView(spread.data(), &six, 1, &stride);

    BufferDescriptor deviceView = flatView;
    deviceView.space = MemorySpace::Device;

    std::printf("mirror -> flat:    %s\n",
                pathName(negotiateCopyPath(mirror, flatView, backend, message)));
    std::printf("mirror -> strided: %s\n",
                pathName(negotiateCopyPath(mirror, stridedView, backend, message)));
    const CopyPath refused = negotiateCopyPath(flatView, deviceView, backend, message);
    std::printf("flat -> Device:    %s (%s)\n", pathName(refused), message.c_str());
    if (refused != CopyPath::Unsupported)
        return fail("a device side on the host backend should be refused");
    //! [negotiate]

    //! [copy]
    // A {2, 3} block fills a {6} view (element counts agree), and a strided
    // view receives every element into every other slot.
    if (!copyBuffer(backend, mirror, flatView, message) ||
        !copyBuffer(backend, mirror, stridedView, message))
        return fail(message);
    std::printf("flat:   ");
    for (double v : flat)
        std::printf(" %.1f", v);
    std::printf("\nspread: ");
    for (double v : spread)
        std::printf(" %.1f", v);
    std::printf("\n");
    for (size_t i = 0; i < values.size(); ++i)
        if (flat[i] != values[i] || spread[2 * i] != values[i] || spread[2 * i + 1] != 0.0)
            return fail("copied values");

    std::array<double, 6> readBack{};
    if (!registry.download("depth", readBack.data(), sizeof(readBack), &message) ||
        readBack != values)
        return fail("download: " + message);
    //! [copy]

    //! [release]
    std::printf("live mirrors: %zu\n", registry.count());
    if (!registry.release("depth") || registry.release("depth") || registry.count() != 0)
        return fail("release");
    std::printf("after release: %zu\n", registry.count());
    //! [release]
    return 0;
}
