// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the shared hyperslab copy engine (bufferio.h): selection
// validation, contiguous fast path, strided fallback, and string buffers.

#include <gtest/gtest.h>

#include "hydrocouplesdk/data/bufferio.h"

#include <array>
#include <numeric>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;

namespace {

BufferDescriptor makeHost(void *data, DataKind kind,
                          std::span<const int64_t> shape,
                          const int64_t *stridesBytes = nullptr)
{
    BufferDescriptor d = detail::hostDescriptor(data, kind, shape);
    d.stridesBytes = stridesBytes;
    return d;
}

} // namespace

// ── validateSelection ─────────────────────────────────────────────────────

TEST(BufferIO, ValidateRejectsKindMismatch)
{
    std::array<double, 4> buf{};
    const std::array<int64_t, 1> shape{4};
    const auto d = makeHost(buf.data(), DataKind::Float32, shape);
    const std::array<int64_t, 1> start{0}, count{4};
    EXPECT_EQ(detail::validateSelection(shape, d, DataKind::Float64, start, count),
              "kind mismatch");
}

TEST(BufferIO, ValidateRejectsDeviceMemory)
{
    std::array<double, 4> buf{};
    const std::array<int64_t, 1> shape{4};
    auto d = makeHost(buf.data(), DataKind::Float64, shape);
    d.space = MemorySpace::Device;
    const std::array<int64_t, 1> start{0}, count{4};
    EXPECT_EQ(detail::validateSelection(shape, d, DataKind::Float64, start, count),
              "unsupported memory space");
}

TEST(BufferIO, ValidateRejectsRankMismatch)
{
    std::array<double, 4> buf{};
    const std::array<int64_t, 1> shape{4};
    const auto d = makeHost(buf.data(), DataKind::Float64, shape);
    const std::array<int64_t, 2> start{0, 0}, count{2, 2};
    EXPECT_EQ(detail::validateSelection(shape, d, DataKind::Float64, start, count),
              "selection rank mismatch");
}

TEST(BufferIO, ValidateRejectsOutOfBounds)
{
    std::array<double, 4> buf{};
    const std::array<int64_t, 1> shape{4};
    const auto d = makeHost(buf.data(), DataKind::Float64, shape);
    const std::array<int64_t, 1> start{2}, count{3};
    EXPECT_EQ(detail::validateSelection(shape, d, DataKind::Float64, start, count),
              "selection out of bounds");
}

TEST(BufferIO, ValidateRejectsElementCountMismatch)
{
    std::array<double, 4> buf{};
    const std::array<int64_t, 1> bufShape{4};
    const auto d = makeHost(buf.data(), DataKind::Float64, bufShape);
    const std::array<int64_t, 1> shape{8}, start{0}, count{3};
    EXPECT_EQ(detail::validateSelection(shape, d, DataKind::Float64, start, count),
              "buffer element count mismatch");
}

// ── 1-D round trips ───────────────────────────────────────────────────────

TEST(BufferIO, Read1DContiguousSlice)
{
    std::vector<double> storage(10);
    std::iota(storage.begin(), storage.end(), 0.0);
    const std::array<int64_t, 1> shape{10};

    std::array<double, 4> out{};
    const std::array<int64_t, 1> outShape{4}, start{3}, count{4};
    const auto d = makeHost(out.data(), DataKind::Float64, outShape);

    std::string msg;
    ASSERT_TRUE(detail::readHyperslab<double>(storage.data(), shape, d, start,
                                              count, &msg)) << msg;
    EXPECT_EQ(out, (std::array<double, 4>{3, 4, 5, 6}));
}

TEST(BufferIO, Write1DContiguousSlice)
{
    std::vector<int> storage(6, 0);
    const std::array<int64_t, 1> shape{6};

    std::array<int, 3> in{7, 8, 9};
    const std::array<int64_t, 1> inShape{3}, start{2}, count{3};
    const auto d = makeHost(in.data(), DataKind::Int32, inShape);

    ASSERT_TRUE(detail::writeHyperslab<int>(storage.data(), shape, d, start,
                                            count, nullptr));
    EXPECT_EQ(storage, (std::vector<int>{0, 0, 7, 8, 9, 0}));
}

TEST(BufferIO, Read1DStridedDestination)
{
    std::vector<double> storage{1, 2, 3, 4};
    const std::array<int64_t, 1> shape{4};

    // Destination writes every other element of an 8-double scratch area.
    std::array<double, 8> scratch{};
    const std::array<int64_t, 1> outShape{4};
    const int64_t stride = 2 * static_cast<int64_t>(sizeof(double));
    const auto d = makeHost(scratch.data(), DataKind::Float64, outShape, &stride);

    const std::array<int64_t, 1> start{0}, count{4};
    ASSERT_TRUE(detail::readHyperslab<double>(storage.data(), shape, d, start,
                                              count, nullptr));
    EXPECT_EQ(scratch, (std::array<double, 8>{1, 0, 2, 0, 3, 0, 4, 0}));
}

// ── 2-D round trips ───────────────────────────────────────────────────────

TEST(BufferIO, Read2DInteriorBlock)
{
    // 4x5 storage, values = row*10 + col.
    std::vector<double> storage;
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 5; ++c)
            storage.push_back(r * 10.0 + c);
    const std::array<int64_t, 2> shape{4, 5};

    std::array<double, 6> out{};
    const std::array<int64_t, 2> outShape{2, 3}, start{1, 2}, count{2, 3};
    const auto d = makeHost(out.data(), DataKind::Float64, outShape);

    ASSERT_TRUE(detail::readHyperslab<double>(storage.data(), shape, d, start,
                                              count, nullptr));
    EXPECT_EQ(out, (std::array<double, 6>{12, 13, 14, 22, 23, 24}));
}

TEST(BufferIO, Write2DInteriorBlock)
{
    std::vector<int> storage(12, 0); // 3x4
    const std::array<int64_t, 2> shape{3, 4};

    std::array<int, 4> in{1, 2, 3, 4};
    const std::array<int64_t, 2> inShape{2, 2}, start{1, 1}, count{2, 2};
    const auto d = makeHost(in.data(), DataKind::Int32, inShape);

    ASSERT_TRUE(detail::writeHyperslab<int>(storage.data(), shape, d, start,
                                            count, nullptr));
    EXPECT_EQ(storage, (std::vector<int>{0, 0, 0, 0,
                                         0, 1, 2, 0,
                                         0, 3, 4, 0}));
}

TEST(BufferIO, EmptySelectionSucceeds)
{
    std::vector<double> storage(4, 1.0);
    const std::array<int64_t, 1> shape{4};
    const std::array<int64_t, 1> outShape{0}, start{2}, count{0};
    double dummy = 0.0;
    const auto d = makeHost(&dummy, DataKind::Float64, outShape);
    EXPECT_TRUE(detail::readHyperslab<double>(storage.data(), shape, d, start,
                                              count, nullptr));
}

// ── String buffers ────────────────────────────────────────────────────────

TEST(BufferIO, StringRoundTrip)
{
    std::vector<std::string> storage{"a", "b", "c", "d"};
    const std::array<int64_t, 1> shape{4};

    std::array<std::string, 2> out;
    const std::array<int64_t, 1> outShape{2}, start{1}, count{2};
    const auto d = makeHost(out.data(), DataKind::String, outShape);

    ASSERT_TRUE(detail::readHyperslab<std::string>(storage.data(), shape, d,
                                                   start, count, nullptr));
    EXPECT_EQ(out[0], "b");
    EXPECT_EQ(out[1], "c");
}
