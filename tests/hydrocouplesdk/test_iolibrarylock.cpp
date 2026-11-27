// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for ioLibraryMutex(): every SDK entry point that calls HDF5 or
// NetCDF holds the process-wide lock, and several writers and readers on
// different threads in one process produce files that read back exactly.
//
// The first group is the gate. Each case holds the lock on the test thread,
// starts one entry point on another thread, and requires that the call waits
// for the lock and then completes. An entry point that skips the lock
// finishes while the lock is still held, and the case fails. The stress case
// is the evidence that the guarantee holds under real concurrency.

#include <gtest/gtest.h>
#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/io/iolibrarylock.h"
#include "hydrocouplesdk/io/iothread.h"
#include "hydrocouplesdk/io/resultreader.h"
#include "hydrocouplesdk/io/ugridreader.h"
#ifdef HYDROCOUPLESDK_HAVE_NETCDF
#include "hydrocouplesdk/io/netcdfugridwriter.h"
#endif
#ifdef HYDROCOUPLESDK_HAVE_HDF5
#include "hydrocouplesdk/io/hdf5ugridwriter.h"
#endif

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::IO;
using namespace std::chrono_literals;

#ifndef TEST_ARTIFACTS_DIR
#define TEST_ARTIFACTS_DIR "."
#endif

namespace {

//! A file under tests/artifacts named after the running case.
std::filesystem::path artifactPath(const std::string &name)
{
    const std::filesystem::path dir{TEST_ARTIFACTS_DIR};
    std::filesystem::create_directories(dir);
    const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
    return dir / (std::string(info->test_suite_name()) + "." + info->name() +
                  "." + name);
}

MeshDefinition twoTriangleMesh()
{
    MeshDefinition mesh;
    mesh.meshName        = "mesh2d";
    mesh.nodeX           = {0.0, 1.0, 1.0, 0.0};
    mesh.nodeY           = {0.0, 0.0, 1.0, 1.0};
    mesh.faceNodeOffsets = {0, 3, 6};
    mesh.faceNodes       = {0, 1, 3, 1, 2, 3};
    mesh.edgeNodes       = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {1, 3}};
    return mesh;
}

FieldSlice doubleField(const std::string &id, std::vector<double> values)
{
    FieldSlice field;
    field.itemId = id;
    field.kind   = DataKind::Float64;
    field.shape  = {static_cast<int64_t>(values.size())};
    field.data.resize(values.size() * sizeof(double));
    std::memcpy(field.data.data(), values.data(), field.data.size());
    return field;
}

/*!
 * Runs \p call on another thread while this thread holds the lock.
 *
 * Succeeds when the call was still waiting after the hold, and finished once
 * the lock was released: what "this entry point takes the lock" looks like
 * from outside. 200 ms is far longer than any of these calls takes on a
 * two-triangle mesh, so a call that ignores the lock has long finished.
 */
::testing::AssertionResult waitsForTheLock(const std::function<void()> &call)
{
    std::unique_lock<std::recursive_mutex> held(ioLibraryMutex());
    std::atomic<bool> finished{false};
    std::thread worker([&] {
        call();
        finished = true;
    });
    std::this_thread::sleep_for(200ms);
    const bool waited = !finished.load();
    held.unlock();
    worker.join();

    if (!waited)
        return ::testing::AssertionFailure()
               << "the call finished while the IO library lock was held";
    if (!finished.load())
        return ::testing::AssertionFailure() << "the call never finished";
    return ::testing::AssertionSuccess();
}

/*!
 * Every result-reader entry point that reaches the library, on a file
 * already written: open, readTimes, read, and the destructor that closes.
 */
void expectReaderTakesTheLock(const std::filesystem::path &path,
                              const ResultEntry &entry)
{
    std::string message;
    std::unique_ptr<IResultReader> reader =
        createResultReader(entry.format, message);
    ASSERT_NE(reader, nullptr) << message;

    bool opened = false;
    EXPECT_TRUE(waitsForTheLock([&] { opened = reader->open(path, message); }))
        << "open()";
    ASSERT_TRUE(opened) << message;

    std::vector<double> times;
    bool timed = false;
    EXPECT_TRUE(waitsForTheLock(
        [&] { timed = reader->readTimes(entry, times, message); }))
        << "readTimes()";
    EXPECT_TRUE(timed) << message;

    std::vector<double> values(static_cast<size_t>(entry.elementCount()));
    const std::array<int64_t, 2> start{0, 0};
    const std::array<int64_t, 1> flat{entry.elementCount()};
    bool read = false;
    EXPECT_TRUE(waitsForTheLock([&] {
        read = reader->read(entry, start, entry.shape,
                            detail::hostDescriptor(values.data(),
                                                   DataKind::Float64, flat),
                            message);
    }))
        << "read()";
    EXPECT_TRUE(read) << message;

    EXPECT_TRUE(waitsForTheLock([&] { reader.reset(); })) << "destructor";
}

/*!
 * Drives \p writer through initialize, one write and finalize, requiring
 * each to wait for the lock. Returns the catalog for the read-back cases.
 */
template <typename Writer>
std::vector<ResultEntry> expectWriterTakesTheLock(Writer &writer)
{
    writer.addField("depth", MeshLocation::Node, "m");

    int code = -1;
    EXPECT_TRUE(waitsForTheLock([&] { code = writer.initialize(); }))
        << "initialize()";
    EXPECT_EQ(code, 0) << writer.errorMessage();

    Snapshot snapshot(2451545.0);
    snapshot.addField(doubleField("depth", {1.0, 2.0, 3.0, 4.0}));
    code = -1;
    EXPECT_TRUE(waitsForTheLock([&] { code = writer.write(snapshot); }))
        << "write()";
    EXPECT_EQ(code, 0) << writer.errorMessage();

    code = -1;
    EXPECT_TRUE(waitsForTheLock([&] { code = writer.finalize(); }))
        << "finalize()";
    EXPECT_EQ(code, 0) << writer.errorMessage();

    return writer.catalog();
}

} // namespace

// ── The lock is shared ────────────────────────────────────────────────────

TEST(IOLibraryLock, IsOneMutexForTheWholeProcess)
{
    EXPECT_EQ(&ioLibraryMutex(), &ioLibraryMutex());

    std::recursive_mutex *fromAnotherThread = nullptr;
    std::thread([&] { fromAnotherThread = &ioLibraryMutex(); }).join();
    EXPECT_EQ(fromAnotherThread, &ioLibraryMutex());
}

// ── Every entry point waits for it ────────────────────────────────────────

#ifdef HYDROCOUPLESDK_HAVE_NETCDF

TEST(IOLibraryLock, NetCDFWriterHoldsItForEachCall)
{
    const auto path = artifactPath("mesh.nc");
    std::filesystem::remove(path);
    NetCDFUGRIDWriter writer(path, twoTriangleMesh());
    const auto entries = expectWriterTakesTheLock(writer);
    ASSERT_EQ(entries.size(), 1u);
}

TEST(IOLibraryLock, NetCDFResultReaderHoldsItForEachCall)
{
    const auto path = artifactPath("mesh.nc");
    std::filesystem::remove(path);
    NetCDFUGRIDWriter writer(path, twoTriangleMesh());
    const auto entries = expectWriterTakesTheLock(writer);
    ASSERT_EQ(entries.size(), 1u);
    expectReaderTakesTheLock(path, entries.front());
}

TEST(IOLibraryLock, UgridReaderHoldsItForEachCall)
{
    const auto path = artifactPath("mesh.nc");
    std::filesystem::remove(path);
    NetCDFUGRIDWriter writer(path, twoTriangleMesh());
    expectWriterTakesTheLock(writer);

    std::string message;
    std::vector<std::string> names;
    EXPECT_TRUE(waitsForTheLock([&] { names = ugridMeshNames(path, message); }))
        << "ugridMeshNames()";
    EXPECT_EQ(names, std::vector<std::string>{"mesh2d"}) << message;

    MeshDefinition mesh;
    bool read = false;
    EXPECT_TRUE(waitsForTheLock(
        [&] { read = readUGRIDMesh(path, "mesh2d", mesh, message); }))
        << "readUGRIDMesh()";
    EXPECT_TRUE(read) << message;
    EXPECT_EQ(mesh.nodeX.size(), 4u);

    // A plain 2-D mesh has no vertical coordinate, so the answer is false;
    // what is under test is that the file was opened under the lock.
    VerticalCoordinate vertical;
    EXPECT_TRUE(waitsForTheLock([&] {
        (void)readVerticalCoordinate(path, "mesh2d", 0, vertical, message);
    }))
        << "readVerticalCoordinate()";
}

#endif // HYDROCOUPLESDK_HAVE_NETCDF

#ifdef HYDROCOUPLESDK_HAVE_HDF5

TEST(IOLibraryLock, HDF5WriterHoldsItForEachCall)
{
    const auto path = artifactPath("mesh.h5");
    std::filesystem::remove(path);
    HDF5UGRIDWriter writer(path, twoTriangleMesh());
    const auto entries = expectWriterTakesTheLock(writer);
    ASSERT_EQ(entries.size(), 1u);
}

TEST(IOLibraryLock, HDF5ResultReaderHoldsItForEachCall)
{
    const auto path = artifactPath("mesh.h5");
    std::filesystem::remove(path);
    HDF5UGRIDWriter writer(path, twoTriangleMesh());
    const auto entries = expectWriterTakesTheLock(writer);
    ASSERT_EQ(entries.size(), 1u);
    expectReaderTakesTheLock(path, entries.front());
}

#endif // HYDROCOUPLESDK_HAVE_HDF5

// ── Under real concurrency ────────────────────────────────────────────────

#if defined(HYDROCOUPLESDK_HAVE_NETCDF) || defined(HYDROCOUPLESDK_HAVE_HDF5)

/*!
 * Four components record at once, each through its own IOThread and its own
 * file, while two more threads keep reading a finished mesh: the shape of a
 * host drawing results while a coupled run records. Every file must then
 * read back value for value.
 */
TEST(IOLibraryLock, ConcurrentWritersAndReadersReadBackExactly)
{
    constexpr int kWriters = 4;
    constexpr int kSteps   = 40;
    constexpr int kNodes   = 4;

    auto value = [](int writer, int step, int node) {
        return writer * 1000.0 + step * 10.0 + node;
    };

    struct Recording
    {
        std::filesystem::path path;
        std::shared_ptr<IOutputWriter> writer;
    };
    std::vector<Recording> recordings;
    for (int w = 0; w < kWriters; ++w)
    {
        Recording r;
#if defined(HYDROCOUPLESDK_HAVE_NETCDF) && defined(HYDROCOUPLESDK_HAVE_HDF5)
        const bool netcdf = w % 2 == 0;
#elif defined(HYDROCOUPLESDK_HAVE_NETCDF)
        const bool netcdf = true;
#else
        const bool netcdf = false;
#endif
        r.path = artifactPath("writer" + std::to_string(w) +
                              (netcdf ? ".nc" : ".h5"));
        std::filesystem::remove(r.path);
#ifdef HYDROCOUPLESDK_HAVE_NETCDF
        if (netcdf)
        {
            auto writer =
                std::make_shared<NetCDFUGRIDWriter>(r.path, twoTriangleMesh());
            writer->addField("depth", MeshLocation::Node, "m");
            r.writer = writer;
        }
#endif
#ifdef HYDROCOUPLESDK_HAVE_HDF5
        if (!netcdf)
        {
            auto writer =
                std::make_shared<HDF5UGRIDWriter>(r.path, twoTriangleMesh());
            writer->addField("depth", MeshLocation::Node, "m");
            r.writer = writer;
        }
#endif
        recordings.push_back(std::move(r));
    }

#ifdef HYDROCOUPLESDK_HAVE_NETCDF
    // A finished file for the readers, written before anything else starts.
    const auto shared = artifactPath("shared.nc");
    std::filesystem::remove(shared);
    {
        NetCDFUGRIDWriter writer(shared, twoTriangleMesh());
        ASSERT_EQ(writer.initialize(), 0) << writer.errorMessage();
        ASSERT_EQ(writer.finalize(), 0) << writer.errorMessage();
    }
#endif

    std::atomic<bool> writing{true};
    std::atomic<int>  meshReads{0};
    std::atomic<int>  meshFailures{0};
    std::vector<std::thread> readers;
#ifdef HYDROCOUPLESDK_HAVE_NETCDF
    for (int r = 0; r < 2; ++r)
    {
        readers.emplace_back([&] {
            while (writing.load())
            {
                MeshDefinition mesh;
                std::string message;
                if (readUGRIDMesh(shared, "mesh2d", mesh, message) &&
                    mesh.nodeX.size() == kNodes)
                    ++meshReads;
                else
                    ++meshFailures;
            }
        });
    }
#endif

    std::vector<std::thread> producers;
    std::vector<int> errors(kWriters, -1);
    for (int w = 0; w < kWriters; ++w)
    {
        producers.emplace_back([&, w] {
            IOThread io(4);
            io.addWriter(recordings[w].writer);
            if (!io.start())
            {
                errors[w] = io.errorCode();
                return;
            }
            for (int step = 0; step < kSteps; ++step)
            {
                std::vector<double> values(kNodes);
                for (int node = 0; node < kNodes; ++node)
                    values[node] = value(w, step, node);
                Snapshot snapshot(2451545.0 + step);
                snapshot.addField(doubleField("depth", values));
                (void)io.enqueue(std::move(snapshot));
            }
            io.finish();
            errors[w] = io.errorCode();
        });
    }
    for (std::thread &t : producers)
        t.join();
    writing = false;
    for (std::thread &t : readers)
        t.join();

    for (int w = 0; w < kWriters; ++w)
        EXPECT_EQ(errors[w], 0) << "writer " << w;
#ifdef HYDROCOUPLESDK_HAVE_NETCDF
    EXPECT_EQ(meshFailures.load(), 0);
    EXPECT_GT(meshReads.load(), 0);
#endif

    for (int w = 0; w < kWriters; ++w)
    {
        const auto entries = recordings[w].writer->catalog();
        ASSERT_EQ(entries.size(), 1u);
        const ResultEntry &entry = entries.front();
        ASSERT_EQ(entry.shape, (std::vector<int64_t>{kSteps, kNodes}));

        std::string message;
        auto reader = createResultReader(entry.format, message);
        ASSERT_NE(reader, nullptr) << message;
        ASSERT_TRUE(reader->open(recordings[w].path, message)) << message;

        std::vector<double> values(static_cast<size_t>(entry.elementCount()));
        const std::array<int64_t, 2> start{0, 0};
        const std::array<int64_t, 1> flat{entry.elementCount()};
        ASSERT_TRUE(reader->read(entry, start, entry.shape,
                                 detail::hostDescriptor(values.data(),
                                                        DataKind::Float64,
                                                        flat),
                                 message))
            << message;
        for (int step = 0; step < kSteps; ++step)
            for (int node = 0; node < kNodes; ++node)
                ASSERT_EQ(values[static_cast<size_t>(step * kNodes + node)],
                          value(w, step, node))
                    << "writer " << w << " step " << step << " node " << node;
    }
}

#endif
