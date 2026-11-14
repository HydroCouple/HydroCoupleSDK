// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the run manifest: ResultEntry round trips, writer catalogs
// describing what was actually written, and RunRecorder assembling a
// manifest whose catalog is the index a reader can serve from.

#include <gtest/gtest.h>

#include "hydrocouplesdk/io/runmanifest.h"
#include "hydrocouplesdk/io/csvwriter.h"
#include "hydrocouplesdk/io/iothread.h"
#include "hydrocouplesdk/io/snapshot.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <filesystem>
#include <fstream>
#include <memory>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::IO;

namespace {

std::filesystem::path artifactDir()
{
    const std::filesystem::path dir =
        std::filesystem::path(TEST_ARTIFACTS_DIR) / "manifest";
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

// ── ResultEntry ───────────────────────────────────────────────────────────

TEST(ResultEntryTest, RoundTripsThroughJson)
{
    ResultEntry entry;
    entry.componentId = "channel";
    entry.itemId      = "flow";
    entry.artifact    = "results.h5";
    entry.format      = "hdf5_ugrid";
    entry.variable    = "flow";
    entry.kind        = DataKind::Float64;
    entry.shape       = {24, 3};
    entry.dimensions  = {"time", "face"};
    entry.units       = "m3/s";
    entry.description = "Routed flow";
    entry.mesh        = "reaches";
    entry.location    = MeshLocation::Face;
    entry.time        = ResultTimeAxis{24, 1.0, 24.0, "julian_day"};

    ResultEntry parsed;
    std::string message;
    ASSERT_TRUE(ResultEntry::fromJson(entry.toJson(), parsed, message))
        << message;

    EXPECT_EQ(parsed.componentId, "channel");
    EXPECT_EQ(parsed.itemId, "flow");
    EXPECT_EQ(parsed.kind, DataKind::Float64);
    EXPECT_EQ(parsed.shape, (std::vector<int64_t>{24, 3}));
    EXPECT_EQ(parsed.dimensions, (std::vector<std::string>{"time", "face"}));
    EXPECT_EQ(parsed.units, "m3/s");
    EXPECT_EQ(parsed.mesh, "reaches");
    ASSERT_TRUE(parsed.location.has_value());
    EXPECT_EQ(*parsed.location, MeshLocation::Face);
    ASSERT_TRUE(parsed.time.has_value());
    EXPECT_EQ(parsed.time->count, 24);
    EXPECT_DOUBLE_EQ(parsed.time->end, 24.0);
    EXPECT_EQ(parsed.elementCount(), 72);
    EXPECT_EQ(parsed.variableName(), "flow");
    EXPECT_EQ(parsed.toJson(), entry.toJson());
}

TEST(ResultEntryTest, DiagnosesMalformedEntries)
{
    ResultEntry entry;
    std::string message;

    EXPECT_FALSE(ResultEntry::fromJson(
        nlohmann::json::parse(R"({"artifact": "a", "format": "csv",
                                  "kind": "Float64"})"), entry, message));
    EXPECT_NE(message.find("item"), std::string::npos);

    EXPECT_FALSE(ResultEntry::fromJson(
        nlohmann::json::parse(R"({"item": "x", "artifact": "a",
                                  "format": "csv", "kind": "Complex128",
                                  "shape": [1]})"), entry, message));
    EXPECT_NE(message.find("Complex128"), std::string::npos);

    EXPECT_FALSE(ResultEntry::fromJson(
        nlohmann::json::parse(R"({"item": "x", "artifact": "a",
                                  "format": "csv", "kind": "Float64",
                                  "shape": [2, 3],
                                  "dimensions": ["time"]})"), entry, message));
    EXPECT_NE(message.find("parallel to shape"), std::string::npos);
}

// ── Writer catalog ────────────────────────────────────────────────────────

TEST(WriterCatalogTest, CsvWriterDescribesWhatItWrote)
{
    Dimension dim{"i", "Index"};
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Q")};
    Output1DDouble flow("flow", &dim, 3, quantity.get(), nullptr);
    Output1DInt    count("count", &dim, 2, quantity.get(), nullptr);

    const auto path = artifactDir() / "catalog.csv";
    CSVWriter  writer(path);
    ASSERT_EQ(writer.initialize(), 0);

    // The catalog is empty until something has been written.
    EXPECT_TRUE(writer.catalog().empty());

    for (int step = 0; step < 4; ++step)
    {
        flow[0] = step; flow[1] = step + 0.5; flow[2] = step + 1.0;
        count[0] = step; count[1] = step * 2;

        Snapshot snapshot(100.0 + step);
        std::string message;
        ASSERT_TRUE(snapshot.capture(flow, message)) << message;
        ASSERT_TRUE(snapshot.capture(count, message)) << message;
        ASSERT_EQ(writer.write(snapshot), 0);
    }
    ASSERT_EQ(writer.finalize(), 0);

    const std::vector<ResultEntry> catalog = writer.catalog();
    ASSERT_EQ(catalog.size(), 2u);

    const ResultEntry &flowEntry = catalog[0];
    EXPECT_EQ(flowEntry.itemId, "flow");
    EXPECT_EQ(flowEntry.format, "csv");
    EXPECT_EQ(flowEntry.artifact, "catalog.csv");
    EXPECT_EQ(flowEntry.kind, DataKind::Float64);
    EXPECT_EQ(flowEntry.shape, (std::vector<int64_t>{4, 3}));
    EXPECT_EQ(flowEntry.dimensions,
              (std::vector<std::string>{"time", "element"}));
    ASSERT_TRUE(flowEntry.time.has_value());
    EXPECT_EQ(flowEntry.time->count, 4);
    EXPECT_DOUBLE_EQ(flowEntry.time->start, 100.0);
    EXPECT_DOUBLE_EQ(flowEntry.time->end, 103.0);

    EXPECT_EQ(catalog[1].itemId, "count");
    EXPECT_EQ(catalog[1].kind, DataKind::Int32);
    EXPECT_EQ(catalog[1].shape, (std::vector<int64_t>{4, 2}));
}

// ── RunManifest ───────────────────────────────────────────────────────────

TEST(RunManifestTest, WritesReadsAndResolvesArtifacts)
{
    RunManifest manifest;
    manifest.id         = "run-1";
    manifest.caption    = "Test run";
    manifest.status     = RunStatus::Completed;
    manifest.sdkVersion = "2.1.0";
    manifest.compositionPath = "composition.json";

    ComponentOutcome outcome;
    outcome.id          = "channel";
    outcome.finalStatus = "Finished";
    manifest.components.push_back(outcome);

    ResultEntry entry;
    entry.componentId = "channel";
    entry.itemId      = "flow";
    entry.artifact    = "results.csv";
    entry.format      = "csv";
    entry.kind        = DataKind::Float64;
    entry.shape       = {4, 3};
    manifest.results.push_back(entry);

    const auto path = artifactDir() / "run.json";
    std::string message;
    ASSERT_TRUE(manifest.write(path, message)) << message;

    RunManifest reopened;
    ASSERT_TRUE(RunManifest::read(path, reopened, message)) << message;

    EXPECT_EQ(reopened.id, "run-1");
    EXPECT_EQ(reopened.status, RunStatus::Completed);
    EXPECT_EQ(reopened.sdkVersion, "2.1.0");
    ASSERT_EQ(reopened.components.size(), 1u);
    EXPECT_EQ(reopened.components[0].finalStatus, "Finished");

    // Artifact paths resolve against the manifest's own directory.
    ASSERT_EQ(reopened.results.size(), 1u);
    EXPECT_EQ(reopened.artifactPath(reopened.results[0]),
              artifactDir() / "results.csv");

    // Lookup helpers.
    ASSERT_NE(reopened.entry("channel", "flow"), nullptr);
    EXPECT_EQ(reopened.entry("channel", "missing"), nullptr);
    EXPECT_EQ(reopened.entriesFor("channel").size(), 1u);
    EXPECT_TRUE(reopened.entriesFor("ghost").empty());
}

TEST(RunManifestTest, RejectsUnsupportedVersionAndMalformedDocuments)
{
    RunManifest manifest;
    std::string message;

    EXPECT_FALSE(RunManifest::fromJson(
        nlohmann::json::parse(R"({"schema_version": "9.0", "run": {},
                                  "results": []})"), manifest, message));
    EXPECT_NE(message.find("9.0"), std::string::npos);

    EXPECT_FALSE(RunManifest::fromJson(nlohmann::json::parse(R"({})"),
                                       manifest, message));
    EXPECT_NE(message.find("run"), std::string::npos);

    EXPECT_FALSE(RunManifest::fromJson(
        nlohmann::json::parse(R"({"run": {"id": "r"}})"), manifest, message));
    EXPECT_NE(message.find("results"), std::string::npos);

    EXPECT_FALSE(RunManifest::read(artifactDir() / "does-not-exist.json",
                                   manifest, message));
    EXPECT_NE(message.find("Cannot open"), std::string::npos);
}

// ── RunRecorder ───────────────────────────────────────────────────────────

TEST(RunRecorderTest, AssemblesManifestFromWritersAndComponents)
{
    Dimension dim{"i", "Index"};
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Q")};
    Output1DDouble flow("flow", &dim, 2, quantity.get(), nullptr);

    const auto directory = artifactDir();
    auto writer = std::make_shared<CSVWriter>(directory / "recorded.csv");

    RunRecorder recorder("run-42");
    recorder.setCaption("Recorded run");
    recorder.setComposition(directory / "composition.json");
    recorder.addWriter(writer);

    // Drive the writer through the IO thread, exactly as a run does.
    IOThread io(8);
    io.addWriter(writer);
    ASSERT_TRUE(io.start());
    for (int step = 0; step < 3; ++step)
    {
        flow[0] = step; flow[1] = step + 0.25;
        Snapshot snapshot(200.0 + step);
        std::string message;
        ASSERT_TRUE(snapshot.capture(flow, message)) << message;
        ASSERT_TRUE(io.enqueue(std::move(snapshot)));
    }
    io.finish(); // finalize() runs here — catalogs are valid afterwards.
    ASSERT_EQ(io.errorCode(), 0);

    const RunManifest manifest = recorder.finish(RunStatus::Completed);

    EXPECT_EQ(manifest.id, "run-42");
    EXPECT_EQ(manifest.caption, "Recorded run");
    EXPECT_EQ(manifest.status, RunStatus::Completed);
    EXPECT_FALSE(manifest.started.empty());
    EXPECT_FALSE(manifest.finished.empty());
    EXPECT_FALSE(manifest.sdkVersion.empty());
    EXPECT_NE(manifest.compositionPath.find("composition.json"),
              std::string::npos);

    ASSERT_EQ(manifest.results.size(), 1u);
    EXPECT_EQ(manifest.results[0].itemId, "flow");
    EXPECT_EQ(manifest.results[0].shape, (std::vector<int64_t>{3, 2}));
    EXPECT_EQ(manifest.results[0].format, "csv");
}

TEST(RunRecorderTest, GeneratesRunIdAndWritesToFile)
{
    RunRecorder recorder;
    const auto  path = artifactDir() / "generated.json";
    std::string message;
    ASSERT_TRUE(recorder.finishToFile(path, message, RunStatus::Stopped))
        << message;

    RunManifest manifest;
    ASSERT_TRUE(RunManifest::read(path, manifest, message)) << message;
    EXPECT_FALSE(manifest.id.empty()); // UUID generated
    EXPECT_EQ(manifest.status, RunStatus::Stopped);
    EXPECT_TRUE(manifest.results.empty());
}

TEST(RunStatusTest, NamesRoundTrip)
{
    for (RunStatus status :
         {RunStatus::Completed, RunStatus::Failed, RunStatus::Stopped})
    {
        RunStatus parsed{};
        ASSERT_TRUE(runStatusFromName(runStatusName(status), parsed));
        EXPECT_EQ(parsed, status);
    }
    RunStatus unused{};
    EXPECT_FALSE(runStatusFromName("aborted", unused));
}
