// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 16: a device mirror through the Kokkos backend (built only
 *         with USE_KOKKOS=ON): upload from a strided host view, read back
 *         through a contiguous one, and check the values.
 */

#include "hydrocouplesdk/device/devicebufferregistry.h"
#include "hydrocouplesdk/device/kokkosbackend.h"
#include "hydrocouplesdk/device/spacetransfer.h"

#include <array>
#include <cstdio>
#include <string>

using namespace HydroCouple;
using namespace HydroCouple::SDK::Device;

int main()
{
    //! [kokkos]
    // Initializes Kokkos (and finalizes it on destruction) unless the
    // program already did; keep this one backend alive for the whole run.
    KokkosBackend backend;
    std::printf("backend \"%s\", Device supported: %s\n", backend.name().c_str(),
                backend.supportsSpace(MemorySpace::Device) ? "true" : "false");

    std::string message;
    {
        DeviceBufferRegistry registry(&backend);
        if (!registry.acquire("flow", DataKind::Float64, {4}, MemorySpace::Device, 0,
                              &message))
        {
            std::fprintf(stderr, "FAILED: %s\n", message.c_str());
            return 1;
        }
        BufferDescriptor device;
        (void)registry.descriptor("flow", device, &message);

        // Host values in every other slot: a strided view of four elements.
        std::array<double, 8> spread{1.5, -1, 2.5, -1, 3.5, -1, 4.5, -1};
        const int64_t four = 4;
        const int64_t stride = 2 * sizeof(double);
        BufferDescriptor host;
        host.data         = spread.data();
        host.kind         = DataKind::Float64;
        host.rank         = 1;
        host.shape        = &four;
        host.stridesBytes = &stride;
        host.space        = MemorySpace::Host;

        std::array<double, 4> back{};
        BufferDescriptor contiguous = host;
        contiguous.data         = back.data();
        contiguous.stridesBytes = nullptr;

        const bool staged = negotiateCopyPath(host, device, backend, message) ==
                            CopyPath::StageThroughHost;
        const bool direct = negotiateCopyPath(device, contiguous, backend, message) ==
                            CopyPath::BackendCopy;
        if (!staged || !direct || !copyBuffer(backend, host, device, message) ||
            !copyBuffer(backend, device, contiguous, message))
        {
            std::fprintf(stderr, "FAILED: %s\n", message.c_str());
            return 1;
        }
        std::printf("read back: %.1f %.1f %.1f %.1f\n", back[0], back[1], back[2],
                    back[3]);
        if (back != std::array<double, 4>{1.5, 2.5, 3.5, 4.5})
        {
            std::fprintf(stderr, "FAILED: values\n");
            return 1;
        }
    } // the registry frees the mirror here, before the backend finalizes Kokkos
    //! [kokkos]
    return 0;
}
