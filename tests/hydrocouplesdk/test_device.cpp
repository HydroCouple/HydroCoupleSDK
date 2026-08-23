// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Backend conformance suite for the GPU abstraction layer (plan §6): the
// same parameterized tests run over every available DeviceBackend —
// HostBackend always, KokkosBackend under USE_KOKKOS — plus
// DeviceBufferRegistry lifetimes and the space-aware transfer negotiation
// (cheapest legal path decided once, staging only when strides force it).

#include <gtest/gtest.h>

#include "hydrocouplesdk/device/devicebackend.h"
#include "hydrocouplesdk/device/devicebufferregistry.h"
#include "hydrocouplesdk/device/spacetransfer.h"

#ifdef USE_KOKKOS
#include "hydrocouplesdk/device/kokkosbackend.h"
#endif

#include <array>
#include <functional>
#include <memory>
#include <numeric>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK::Device;

namespace {

struct BackendCase
{
    std::string                                    label;
    std::function<std::unique_ptr<DeviceBackend>()> make;
};

std::vector<BackendCase> backendCases()
{
    std::vector<BackendCase> cases;
    cases.push_back({"host", [] {
                         return std::make_unique<HostBackend>();
                     }});
#ifdef USE_KOKKOS
    cases.push_back({"kokkos", [] {
                         return std::make_unique<KokkosBackend>();
                     }});
#endif
    return cases;
}

#ifdef USE_KOKKOS
/*!
 * Keeps one KokkosBackend alive across the whole run so Kokkos is
 * initialized once and finalized once (Kokkos cannot re-initialize).
 */
class KokkosLifetime : public ::testing::Environment
{
public:
    void SetUp() override
    {
        m_anchor = std::make_unique<KokkosBackend>();
    }
    void TearDown() override { m_anchor.reset(); }

private:
    std::unique_ptr<KokkosBackend> m_anchor;
};

const auto *const kKokkosLifetime =
    ::testing::AddGlobalTestEnvironment(new KokkosLifetime);
#endif

BufferDescriptor hostBuffer(void *data, DataKind kind,
                            std::span<const int64_t> shape,
                            const int64_t *stridesBytes = nullptr)
{
    BufferDescriptor descriptor;
    descriptor.data         = data;
    descriptor.kind         = kind;
    descriptor.rank         = static_cast<int32_t>(shape.size());
    descriptor.shape        = shape.data();
    descriptor.stridesBytes = stridesBytes;
    descriptor.space        = MemorySpace::Host;
    return descriptor;
}

class DeviceBackendConformance
    : public ::testing::TestWithParam<BackendCase>
{
protected:
    void SetUp() override { m_backend = GetParam().make(); }

    std::unique_ptr<DeviceBackend> m_backend;
};

} // namespace

INSTANTIATE_TEST_SUITE_P(Backends, DeviceBackendConformance,
                         ::testing::ValuesIn(backendCases()),
                         [](const auto &info) { return info.param.label; });

// ── DeviceBackend conformance ─────────────────────────────────────────────

TEST_P(DeviceBackendConformance, IdentifiesItself)
{
    EXPECT_FALSE(m_backend->name().empty());
    EXPECT_TRUE(m_backend->supportsSpace(MemorySpace::Host));
    EXPECT_FALSE(m_backend->supportsSpace(MemorySpace::Host, /*deviceId=*/99));
}

TEST_P(DeviceBackendConformance, AllocationRoundTripsEverySupportedSpace)
{
    for (MemorySpace space :
         {MemorySpace::Host, MemorySpace::HostPinned, MemorySpace::Unified,
          MemorySpace::Device})
    {
        if (!m_backend->supportsSpace(space))
            continue;

        std::string message;
        void *allocation = m_backend->allocate(space, 0, 64, &message);
        ASSERT_NE(allocation, nullptr) << message;

        std::array<double, 8> pattern{};
        std::iota(pattern.begin(), pattern.end(), 1.0);
        ASSERT_TRUE(m_backend->copy(allocation, space, 0, pattern.data(),
                                    MemorySpace::Host, 0, 64, &message))
            << message;

        std::array<double, 8> readBack{};
        ASSERT_TRUE(m_backend->copy(readBack.data(), MemorySpace::Host, 0,
                                    allocation, space, 0, 64, &message))
            << message;
        EXPECT_EQ(readBack, pattern);

        ASSERT_TRUE(m_backend->synchronize(0, &message)) << message;
        m_backend->deallocate(allocation, space, 0);
    }
}

TEST_P(DeviceBackendConformance, RejectsBadRequests)
{
    std::string message;
    EXPECT_EQ(m_backend->allocate(MemorySpace::Host, 0, 0, &message),
              nullptr);
    EXPECT_FALSE(message.empty());
    EXPECT_EQ(m_backend->allocate(MemorySpace::Device, 42, 16, &message),
              nullptr);
    m_backend->deallocate(nullptr, MemorySpace::Host, 0); // null is a no-op
}

// ── DeviceBufferRegistry ──────────────────────────────────────────────────

TEST_P(DeviceBackendConformance, RegistryOwnsMirrors)
{
    DeviceBufferRegistry registry(m_backend.get());
    const MemorySpace space = m_backend->supportsSpace(MemorySpace::Device)
                                  ? MemorySpace::Device
                                  : MemorySpace::Host;

    std::string message;
    const auto *mirror = registry.acquire("flow", DataKind::Float64, {3, 4},
                                          space, 0, &message);
    ASSERT_NE(mirror, nullptr) << message;
    EXPECT_EQ(mirror->bytes, 96);
    EXPECT_EQ(registry.count(), 1u);

    // Same signature: same mirror. Different signature: refused.
    EXPECT_EQ(registry.acquire("flow", DataKind::Float64, {3, 4}, space, 0,
                               &message),
              mirror);
    EXPECT_EQ(registry.acquire("flow", DataKind::Int32, {3, 4}, space, 0,
                               &message),
              nullptr);
    EXPECT_NE(message.find("different signature"), std::string::npos);

    // Upload / download through the backend.
    std::vector<double> pattern(12);
    std::iota(pattern.begin(), pattern.end(), 0.5);
    ASSERT_TRUE(registry.upload("flow", pattern.data(), 96, &message))
        << message;
    std::vector<double> readBack(12, -1.0);
    ASSERT_TRUE(registry.download("flow", readBack.data(), 96, &message))
        << message;
    EXPECT_EQ(readBack, pattern);

    // Size mismatches and unknown keys are diagnosed.
    EXPECT_FALSE(registry.upload("flow", pattern.data(), 8, &message));
    EXPECT_FALSE(registry.download("missing", readBack.data(), 96, &message));

    // Descriptor view carries the typed signature.
    BufferDescriptor view;
    ASSERT_TRUE(registry.descriptor("flow", view, &message)) << message;
    EXPECT_EQ(view.kind, DataKind::Float64);
    EXPECT_EQ(view.rank, 2);
    EXPECT_EQ(view.shape[1], 4);
    EXPECT_EQ(view.space, space);

    EXPECT_TRUE(registry.release("flow"));
    EXPECT_FALSE(registry.release("flow"));
    EXPECT_EQ(registry.count(), 0u);
}

// ── Space-aware transfer ──────────────────────────────────────────────────

TEST_P(DeviceBackendConformance, NegotiationPicksTheCheapestLegalPath)
{
    std::array<double, 12> a{}, b{};
    const std::array<int64_t, 1> shape{12};
    const auto hostA = hostBuffer(a.data(), DataKind::Float64, shape);
    const auto hostB = hostBuffer(b.data(), DataKind::Float64, shape);

    std::string message;
    EXPECT_EQ(negotiateCopyPath(hostA, hostB, *m_backend, message),
              CopyPath::HostHyperslab);

    // Kind and count mismatches are negotiation errors.
    auto wrongKind = hostB;
    wrongKind.kind = DataKind::Int32;
    EXPECT_EQ(negotiateCopyPath(hostA, wrongKind, *m_backend, message),
              CopyPath::Unsupported);
    const std::array<int64_t, 1> shorter{6};
    EXPECT_EQ(negotiateCopyPath(
                  hostA, hostBuffer(b.data(), DataKind::Float64, shorter),
                  *m_backend, message),
              CopyPath::Unsupported);

    if (!m_backend->supportsSpace(MemorySpace::Device))
        return; // device paths proven on backends that have one

    DeviceBufferRegistry registry(m_backend.get());
    const auto *mirror = registry.acquire("d", DataKind::Float64, {12},
                                          MemorySpace::Device, 0, &message);
    ASSERT_NE(mirror, nullptr) << message;
    BufferDescriptor device;
    ASSERT_TRUE(registry.descriptor("d", device, &message)) << message;

    EXPECT_EQ(negotiateCopyPath(hostA, device, *m_backend, message),
              CopyPath::BackendCopy);

    // Strided host side forces staging; strided device side is refused.
    const int64_t stride = 2 * static_cast<int64_t>(sizeof(double));
    std::array<double, 24> spread{};
    const auto stridedHost =
        hostBuffer(spread.data(), DataKind::Float64, shape, &stride);
    EXPECT_EQ(negotiateCopyPath(stridedHost, device, *m_backend, message),
              CopyPath::StageThroughHost);

    auto stridedDevice         = device;
    stridedDevice.stridesBytes = &stride;
    EXPECT_EQ(negotiateCopyPath(hostA, stridedDevice, *m_backend, message),
              CopyPath::Unsupported);
    EXPECT_NE(message.find("strided device"), std::string::npos);
}

TEST_P(DeviceBackendConformance, CopyBufferMovesValuesAcrossEveryPath)
{
    std::string message;

    // Host strided → host reshaped (odometer path).
    std::array<double, 8> spread{1, -9, 2, -9, 3, -9, 4, -9};
    const int64_t stride = 2 * static_cast<int64_t>(sizeof(double));
    const std::array<int64_t, 1> flat4{4};
    const std::array<int64_t, 2> grid22{2, 2};
    std::array<double, 4> reshaped{};
    ASSERT_TRUE(copyBuffer(
        *m_backend,
        hostBuffer(spread.data(), DataKind::Float64, flat4, &stride),
        hostBuffer(reshaped.data(), DataKind::Float64, grid22), message))
        << message;
    EXPECT_EQ(reshaped, (std::array<double, 4>{1, 2, 3, 4}));

    if (!m_backend->supportsSpace(MemorySpace::Device))
        return;

    // Device-resident producer: host → device → host without a user copy.
    DeviceBufferRegistry registry(m_backend.get());
    ASSERT_NE(registry.acquire("field", DataKind::Float64, {4},
                               MemorySpace::Device, 0, &message),
              nullptr) << message;
    BufferDescriptor device;
    ASSERT_TRUE(registry.descriptor("field", device, &message)) << message;

    ASSERT_TRUE(copyBuffer(
        *m_backend,
        hostBuffer(spread.data(), DataKind::Float64, flat4, &stride), device,
        message)) << message; // staged: strided host → device

    std::array<double, 4> received{};
    ASSERT_TRUE(copyBuffer(
        *m_backend, device,
        hostBuffer(received.data(), DataKind::Float64, flat4), message))
        << message; // backend copy: device → contiguous host
    EXPECT_EQ(received, (std::array<double, 4>{1, 2, 3, 4}));

    // Device → strided host (staged scatter).
    std::array<double, 8> scattered{};
    ASSERT_TRUE(copyBuffer(
        *m_backend, device,
        hostBuffer(scattered.data(), DataKind::Float64, flat4, &stride),
        message)) << message;
    EXPECT_DOUBLE_EQ(scattered[0], 1.0);
    EXPECT_DOUBLE_EQ(scattered[2], 2.0);
    EXPECT_DOUBLE_EQ(scattered[6], 4.0);
}

// ── isContiguous ──────────────────────────────────────────────────────────

TEST(SpaceTransferTest, ContiguityDetection)
{
    std::array<double, 6> data{};
    const std::array<int64_t, 2> shape{2, 3};
    EXPECT_TRUE(isContiguous(
        hostBuffer(data.data(), DataKind::Float64, shape)));

    const std::array<int64_t, 2> packed{3 * sizeof(double), sizeof(double)};
    EXPECT_TRUE(isContiguous(
        hostBuffer(data.data(), DataKind::Float64, shape, packed.data())));

    const std::array<int64_t, 2> padded{4 * sizeof(double), sizeof(double)};
    EXPECT_FALSE(isContiguous(
        hostBuffer(data.data(), DataKind::Float64, shape, padded.data())));
}
