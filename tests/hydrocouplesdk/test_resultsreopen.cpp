// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// The acceptance test for reopening completed runs: run a composition,
// record a manifest, then reopen it through ResultsModelComponent and
// assert the values served through the v2 data plane are byte-identical
// to what the live run produced — with the reopened side touching no
// model code at all.

#include <gtest/gtest.h>

#include "hydrocouplesdk/component/resultsmodelcomponent.h"
#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/io/csvwriter.h"
#include "hydrocouplesdk/io/iothread.h"
#include "hydrocouplesdk/io/runmanifest.h"
#include "hydrocouplesdk/io/resultreader.h"
#include "hydrocouplesdk/io/snapshot.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#ifdef HYDROCOUPLESDK_HAVE_HDF5
#include "hydrocouplesdk/io/hdf5ugridwriter.h"
#endif
#ifdef HYDROCOUPLESDK_HAVE_NETCDF
#include "hydrocouplesdk/io/netcdfugridwriter.h"
#endif
#ifdef HYDROCOUPLESDK_HAVE_GEOPACKAGE
#include "hydrocouplesdk/io/geopackagewriter.h"
#endif

#include <algorithm>
#include <array>
#include <filesystem>
#include <memory>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::IO;

namespace {

constexpr int kSteps   = 6;
constexpr int kReaches = 3;

std::filesystem::path reopenDir()
{
    const std::filesystem::path dir =
        std::filesystem::path(TEST_ARTIFACTS_DIR) / "reopen";
    std::filesystem::create_directories(dir);
    return dir;
}

/*!
 * A component whose output takes deliberately awkward values — negative,
 * fractional, and large — so an equivalence claim means something.
 */
class ProducerComponent : public AbstractModelComponent
{
public:
    explicit ProducerComponent(std::string_view id)
        : AbstractModelComponent(id, "Producer")
    {
    }

    std::vector<std::string> validate() override { return {}; }
    void prepare() override { m_step = 0; }

    void update(const std::vector<IOutput *> & = {}) override
    {
        for (int reach = 0; reach < kReaches; ++reach)
            (*flow)[reach] = value(m_step, reach);
        ++m_step;
        setStatus(m_step >= kSteps ? ComponentStatus::Done
                                   : ComponentStatus::Updated);
    }

    void finish() override { setStatus(ComponentStatus::Finished); }

    /*!
     * \brief The value written at (step, reach) — the oracle both sides check.
     */
    static double value(int step, int reach)
    {
        return (step - 2.5) * 1e3 + reach * 0.125 - 1.0 / 3.0;
    }

    [[nodiscard]] int step() const noexcept { return m_step; }

    Output1DDouble *flow = nullptr;

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override {}
    void createOutputs() override
    {
        m_quantity.reset(Quantity::unitLess("Flow"));
        flow = new Output1DDouble("flow", &m_dimension, kReaches,
                                  m_quantity.get(), this);
        addOutput(flow);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension                 m_dimension{"reach", "Reach"};
    std::unique_ptr<Quantity> m_quantity;
    int                       m_step = 0;
};

/*!
 * Runs the producer, writing CSV (and HDF5 when built), and returns the
 * manifest path.
 */
std::filesystem::path recordRun(const std::filesystem::path &directory)
{
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    ProducerComponent producer("producer");
    producer.initialize();
    producer.prepare();

    RunRecorder recorder("reopen-test-run");
    IOThread    io(16);

    auto csv = std::make_shared<CSVWriter>(directory / "results.csv");
    io.addWriter(csv);
    recorder.addWriter(csv);

    // Every format this build can write is recorded, so every reader the
    // build ships is exercised by the tests below.
#if defined(HYDROCOUPLESDK_HAVE_HDF5) || defined(HYDROCOUPLESDK_HAVE_NETCDF)
    MeshDefinition mesh;
    mesh.meshName = "reaches";
    mesh.nodeX    = {0.0, 1.0, 2.0};
    mesh.nodeY    = {0.0, 0.0, 0.0};
#endif

#ifdef HYDROCOUPLESDK_HAVE_HDF5
    auto hdf5 = std::make_shared<HDF5UGRIDWriter>(directory / "results.h5",
                                                  mesh);
    // UGRID writers need their fields declared before initialize().
    hdf5->addField("flow", MeshLocation::Node, "m3/s", "routed_flow");
    io.addWriter(hdf5);
    recorder.addWriter(hdf5);
#endif

#ifdef HYDROCOUPLESDK_HAVE_NETCDF
    auto netcdf = std::make_shared<NetCDFUGRIDWriter>(
        directory / "results.nc", mesh);
    netcdf->addField("flow", MeshLocation::Node, "m3/s", "routed_flow");
    io.addWriter(netcdf);
    recorder.addWriter(netcdf);
#endif

#ifdef HYDROCOUPLESDK_HAVE_GEOPACKAGE
    auto geopackage = std::make_shared<GeoPackageWriter>(
        directory / "results.gpkg", "reopen-test");
    geopackage->addField("flow", "m3/s", "routed flow");
    io.addWriter(geopackage);
    recorder.addWriter(geopackage);
#endif

    recorder.addComponent(&producer);
    EXPECT_TRUE(io.start());

    for (int step = 0; step < kSteps; ++step)
    {
        producer.update();
        Snapshot    snapshot(1000.0 + step);
        std::string message;
        EXPECT_TRUE(snapshot.capture(*producer.flow, message)) << message;
        EXPECT_TRUE(io.enqueue(std::move(snapshot)));
    }
    io.finish();
    EXPECT_EQ(io.errorCode(), 0);
    producer.finish();

    const auto manifestPath = directory / "run.json";
    std::string message;
    EXPECT_TRUE(recorder.finishToFile(manifestPath, message)) << message;
    return manifestPath;
}

BufferDescriptor hostBuffer(void *data, DataKind kind,
                            std::span<const int64_t> shape)
{
    BufferDescriptor descriptor;
    descriptor.data  = data;
    descriptor.kind  = kind;
    descriptor.rank  = static_cast<int32_t>(shape.size());
    descriptor.shape = shape.data();
    descriptor.space = MemorySpace::Host;
    return descriptor;
}

} // namespace

// ── The headline claim ────────────────────────────────────────────────────

TEST(ResultsReopenTest, ReopenedValuesAreByteIdenticalToTheLiveRun)
{
    const auto manifestPath = recordRun(reopenDir() / "identity");

    // Everything below this line uses only the manifest — no model
    // component is constructed, and none is linked for the reopened side.
    std::string message;
    auto reopened =
        ResultsModelComponent::open(manifestPath, "producer", message);
    ASSERT_NE(reopened, nullptr) << message;

    reopened->initialize();
    ASSERT_EQ(reopened->status(),
              IModelComponent::ComponentStatus::Finished)
        << "a recorded run is finished by construction";

    // results() is the standard's channel for outputs that were never
    // coupled — exactly what an analysis UI browses. An item recorded by
    // several writers is still one logical variable, exposed once.
    const std::vector<IComponentDataItem *> items = reopened->results();
    ASSERT_EQ(items.size(), 1u);
    IComponentDataItem *flow = items[0];
    EXPECT_EQ(flow->id(), "flow");
    EXPECT_EQ(flow->dataKind(), DataKind::Float64);
    EXPECT_EQ(flow->shape(), (std::vector<int64_t>{kSteps, kReaches}));

    // Whole variable, byte-identical to what the producer computed.
    std::vector<double> all(kSteps * kReaches, 0.0);
    const std::array<int64_t, 2> start{0, 0};
    const std::array<int64_t, 2> count{kSteps, kReaches};
    const std::array<int64_t, 1> flat{kSteps * kReaches};
    ASSERT_TRUE(flow->getValuesInto(
        hostBuffer(all.data(), DataKind::Float64, flat), start, count,
        &message)) << message;

    for (int step = 0; step < kSteps; ++step)
    {
        for (int reach = 0; reach < kReaches; ++reach)
        {
            const double expected = ProducerComponent::value(step, reach);
            const double actual   = all[step * kReaches + reach];
            EXPECT_EQ(std::memcmp(&expected, &actual, sizeof(double)), 0)
                << "step " << step << " reach " << reach << ": expected "
                << expected << " got " << actual;
        }
    }
}

TEST(ResultsReopenTest, ServesArbitraryHyperslabs)
{
    const auto manifestPath = recordRun(reopenDir() / "slabs");
    std::string message;
    auto reopened =
        ResultsModelComponent::open(manifestPath, "producer", message);
    ASSERT_NE(reopened, nullptr) << message;
    reopened->initialize();
    ASSERT_EQ(reopened->status(),
              IModelComponent::ComponentStatus::Finished);

    IComponentDataItem *flow = reopened->results()[0];

    // One time level, all reaches — the "current step" query a UI makes.
    std::vector<double> level(kReaches, 0.0);
    const std::array<int64_t, 2> levelStart{3, 0};
    const std::array<int64_t, 2> levelCount{1, kReaches};
    const std::array<int64_t, 1> levelShape{kReaches};
    ASSERT_TRUE(flow->getValuesInto(
        hostBuffer(level.data(), DataKind::Float64, levelShape), levelStart,
        levelCount, &message)) << message;
    for (int reach = 0; reach < kReaches; ++reach)
        EXPECT_DOUBLE_EQ(level[reach], ProducerComponent::value(3, reach));

    // One reach across time — the time-series query a plot makes.
    std::vector<double> series(kSteps, 0.0);
    const std::array<int64_t, 2> seriesStart{0, 2};
    const std::array<int64_t, 2> seriesCount{kSteps, 1};
    const std::array<int64_t, 1> seriesShape{kSteps};
    ASSERT_TRUE(flow->getValuesInto(
        hostBuffer(series.data(), DataKind::Float64, seriesShape),
        seriesStart, seriesCount, &message)) << message;
    for (int step = 0; step < kSteps; ++step)
        EXPECT_DOUBLE_EQ(series[step], ProducerComponent::value(step, 2));

    // An interior window.
    std::vector<double> window(2 * 2, 0.0);
    const std::array<int64_t, 2> windowStart{2, 1};
    const std::array<int64_t, 2> windowCount{2, 2};
    const std::array<int64_t, 1> windowShape{4};
    ASSERT_TRUE(flow->getValuesInto(
        hostBuffer(window.data(), DataKind::Float64, windowShape),
        windowStart, windowCount, &message)) << message;
    EXPECT_DOUBLE_EQ(window[0], ProducerComponent::value(2, 1));
    EXPECT_DOUBLE_EQ(window[3], ProducerComponent::value(3, 2));
}

#ifdef HYDROCOUPLESDK_HAVE_HDF5
TEST(ResultsReopenTest, PrefersTheMostCapableRecordedFormat)
{
    // The run recorded "flow" as both CSV and HDF5. One logical variable,
    // served by the format with random access and native typing.
    const auto manifestPath = recordRun(reopenDir() / "preference");
    std::string message;
    auto reopened =
        ResultsModelComponent::open(manifestPath, "producer", message);
    ASSERT_NE(reopened, nullptr) << message;
    reopened->initialize();

    ASSERT_EQ(reopened->results().size(), 1u);
    const ResultEntry *entry = reopened->entryFor("flow");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->format, "hdf5_ugrid");
    EXPECT_EQ(entry->units, "m3/s");
    EXPECT_EQ(entry->mesh, "reaches");
}
#endif

TEST(ResultsReopenTest, CarriesMetadataAndTimeAxis)
{
    const auto manifestPath = recordRun(reopenDir() / "metadata");
    std::string message;
    auto reopened =
        ResultsModelComponent::open(manifestPath, "producer", message);
    ASSERT_NE(reopened, nullptr) << message;
    reopened->initialize();

    EXPECT_EQ(reopened->manifest().id, "reopen-test-run");
    EXPECT_EQ(reopened->sourceComponentId(), "producer");

    const ResultEntry *entry = reopened->entryFor("flow");
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->kind, DataKind::Float64);
    EXPECT_EQ(entry->shape, (std::vector<int64_t>{kSteps, kReaches}));
    EXPECT_EQ(reopened->entryFor("missing"), nullptr);
    EXPECT_TRUE(entry->time.has_value());

    // The time axis survives the round trip.
    std::vector<double> times;
    ASSERT_TRUE(reopened->times("flow", times, message)) << message;
    ASSERT_EQ(times.size(), static_cast<size_t>(kSteps));
    EXPECT_DOUBLE_EQ(times.front(), 1000.0);
    EXPECT_DOUBLE_EQ(times.back(), 1000.0 + kSteps - 1);

    // Dimensions are labelled from the catalog so a UI can name axes.
    IComponentDataItem *flow = reopened->results()[0];
    ASSERT_EQ(flow->dimensions().size(), 2u);
    EXPECT_EQ(flow->dimensions()[0]->id(), "time");
}

TEST(ResultsReopenTest, RecordedItemsAreReadOnly)
{
    const auto manifestPath = recordRun(reopenDir() / "readonly");
    std::string message;
    auto reopened =
        ResultsModelComponent::open(manifestPath, "producer", message);
    ASSERT_NE(reopened, nullptr) << message;
    reopened->initialize();

    IComponentDataItem *flow = reopened->results()[0];
    std::array<double, kReaches> values{1.0, 2.0, 3.0};
    const std::array<int64_t, 2> start{0, 0};
    const std::array<int64_t, 2> count{1, kReaches};
    const std::array<int64_t, 1> shape{kReaches};

    EXPECT_FALSE(flow->setValuesFrom(
        hostBuffer(values.data(), DataKind::Float64, shape), start, count,
        &message));
    EXPECT_NE(message.find("read-only"), std::string::npos);
}

// ── Failure modes are diagnosed, never silent ─────────────────────────────

TEST(ResultsReopenTest, MissingArtifactFailsWithFatalDiagnostic)
{
    const auto directory    = reopenDir() / "missing";
    const auto manifestPath = recordRun(directory);

    // Remove every artifact the catalog references, whichever format the
    // component would have preferred.
    RunManifest recorded;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, recorded, message)) << message;
    for (const ResultEntry &entry : recorded.results)
        std::filesystem::remove(recorded.artifactPath(entry));

    auto reopened =
        ResultsModelComponent::open(manifestPath, "producer", message);
    ASSERT_NE(reopened, nullptr) << message;

    reopened->initialize();
    EXPECT_EQ(reopened->status(), IModelComponent::ComponentStatus::Failed);

    const std::vector<ErrorEntry> errors = reopened->errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors.back().severity, ErrorEntry::Severity::Fatal);
    EXPECT_NE(errors.back().message.find("results."), std::string::npos)
        << "the diagnostic must name the file that could not be opened";
}

TEST(ResultsReopenTest, UnknownComponentIsDiagnosed)
{
    const auto manifestPath = recordRun(reopenDir() / "unknown");
    std::string message;
    auto reopened =
        ResultsModelComponent::open(manifestPath, "ghost", message);
    ASSERT_NE(reopened, nullptr) << message;

    reopened->initialize();
    EXPECT_EQ(reopened->status(), IModelComponent::ComponentStatus::Failed);
    ASSERT_FALSE(reopened->errors().empty());
    EXPECT_NE(reopened->errors().back().message.find("ghost"),
              std::string::npos);
}

TEST(ResultReaderTest, RegistryReportsWhatThisBuildCanRead)
{
    std::string message;
    EXPECT_TRUE(resultFormatSupported("csv"));
    EXPECT_EQ(createResultReader("parquet", message), nullptr);
    EXPECT_NE(message.find("unknown result format"), std::string::npos);

#ifndef HYDROCOUPLESDK_HAVE_HDF5
    // A format the SDK knows but this build cannot serve says so plainly.
    EXPECT_EQ(createResultReader("hdf5_ugrid", message), nullptr);
    EXPECT_NE(message.find("rebuild"), std::string::npos);
#else
    EXPECT_TRUE(resultFormatSupported("hdf5_ugrid"));
#endif
}

// ── Every shipped reader is exercised against its own writer ──────────────

namespace {

/*!
 * Reads a whole recorded variable through the reader its format selects.
 */
void readWholeVariable(const RunManifest &manifest, const ResultEntry &entry,
                       std::vector<double> &values)
{
    std::string message;
    auto reader = createResultReader(entry.format, message);
    ASSERT_NE(reader, nullptr) << entry.format << ": " << message;
    ASSERT_TRUE(reader->open(manifest.artifactPath(entry), message))
        << entry.format << ": " << message;

    values.assign(static_cast<size_t>(entry.elementCount()), 0.0);
    const std::array<int64_t, 2> start{0, 0};
    const std::vector<int64_t>   count = entry.shape;
    const std::array<int64_t, 1> flat{entry.elementCount()};
    ASSERT_TRUE(reader->read(entry, start, count,
                             hostBuffer(values.data(), DataKind::Float64,
                                        flat),
                             message))
        << entry.format << ": " << message;
}

} // namespace

TEST(ResultsReopenTest, EveryRecordedFormatReadsBackTheSameValues)
{
    const auto manifestPath = recordRun(reopenDir() / "formats");
    RunManifest manifest;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, manifest, message)) << message;

    // Whatever this build wrote, it must be able to read — and every
    // format must agree with the values the producer computed.
    ASSERT_FALSE(manifest.results.empty());
    std::vector<std::string> formatsChecked;

    for (const ResultEntry &entry : manifest.results)
    {
        SCOPED_TRACE("format: " + entry.format);
        EXPECT_TRUE(resultFormatSupported(entry.format))
            << "this build wrote " << entry.format
            << " but cannot read it back";
        EXPECT_EQ(entry.shape, (std::vector<int64_t>{kSteps, kReaches}));

        std::vector<double> values;
        readWholeVariable(manifest, entry, values);
        ASSERT_EQ(values.size(), static_cast<size_t>(kSteps * kReaches));

        for (int step = 0; step < kSteps; ++step)
        {
            for (int reach = 0; reach < kReaches; ++reach)
            {
                EXPECT_DOUBLE_EQ(values[step * kReaches + reach],
                                 ProducerComponent::value(step, reach))
                    << entry.format << " step " << step << " reach " << reach;
            }
        }
        formatsChecked.push_back(entry.format);
    }

    // CSV is always present; the gated formats appear when built.
    EXPECT_NE(std::find(formatsChecked.begin(), formatsChecked.end(), "csv"),
              formatsChecked.end());
#ifdef HYDROCOUPLESDK_HAVE_HDF5
    EXPECT_NE(std::find(formatsChecked.begin(), formatsChecked.end(),
                        "hdf5_ugrid"),
              formatsChecked.end());
#endif
#ifdef HYDROCOUPLESDK_HAVE_NETCDF
    EXPECT_NE(std::find(formatsChecked.begin(), formatsChecked.end(),
                        "netcdf_ugrid"),
              formatsChecked.end());
#endif
#ifdef HYDROCOUPLESDK_HAVE_GEOPACKAGE
    EXPECT_NE(std::find(formatsChecked.begin(), formatsChecked.end(),
                        "geopackage"),
              formatsChecked.end());
#endif
}

TEST(ResultsReopenTest, EveryRecordedFormatServesHyperslabsAndTimes)
{
    const auto manifestPath = recordRun(reopenDir() / "format-slabs");
    RunManifest manifest;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, manifest, message)) << message;

    for (const ResultEntry &entry : manifest.results)
    {
        SCOPED_TRACE("format: " + entry.format);
        auto reader = createResultReader(entry.format, message);
        ASSERT_NE(reader, nullptr) << message;
        ASSERT_TRUE(reader->open(manifest.artifactPath(entry), message))
            << message;

        // A single time level — the query a UI makes constantly.
        std::vector<double> level(kReaches, 0.0);
        const std::array<int64_t, 2> start{2, 0};
        const std::array<int64_t, 2> count{1, kReaches};
        const std::array<int64_t, 1> shape{kReaches};
        ASSERT_TRUE(reader->read(entry, start, count,
                                 hostBuffer(level.data(), DataKind::Float64,
                                            shape),
                                 message)) << message;
        for (int reach = 0; reach < kReaches; ++reach)
            EXPECT_DOUBLE_EQ(level[reach],
                             ProducerComponent::value(2, reach));

        // A single reach across time — the time-series query a plot makes.
        std::vector<double> series(kSteps, 0.0);
        const std::array<int64_t, 2> seriesStart{0, 1};
        const std::array<int64_t, 2> seriesCount{kSteps, 1};
        const std::array<int64_t, 1> seriesShape{kSteps};
        ASSERT_TRUE(reader->read(entry, seriesStart, seriesCount,
                                 hostBuffer(series.data(), DataKind::Float64,
                                            seriesShape),
                                 message)) << message;
        for (int step = 0; step < kSteps; ++step)
            EXPECT_DOUBLE_EQ(series[step],
                             ProducerComponent::value(step, 1));

        // Time coordinates: from the artifact where stored, otherwise
        // reconstructed from the recorded axis. Either way, the values.
        std::vector<double> times;
        ASSERT_TRUE(reader->readTimes(entry, times, message)) << message;
        ASSERT_EQ(times.size(), static_cast<size_t>(kSteps));
        EXPECT_DOUBLE_EQ(times.front(), 1000.0);
        EXPECT_DOUBLE_EQ(times.back(), 1000.0 + kSteps - 1);

        // Out-of-range selections are refused, not silently clamped.
        const std::array<int64_t, 2> badStart{kSteps, 0};
        EXPECT_FALSE(reader->read(entry, badStart, count,
                                  hostBuffer(level.data(), DataKind::Float64,
                                             shape),
                                  message));
        EXPECT_NE(message.find("out of range"), std::string::npos);
    }
}
