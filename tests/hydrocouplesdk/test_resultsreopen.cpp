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
#include "hydrocouplespatiotemporal.h"
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
#include <sqlite3.h>
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

    std::vector<std::string> validate() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Validating);
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Valid);
        return {};
    }
    void prepare() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Preparing);
        m_step = 0;
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Updated);
    }

    void update(const std::vector<IOutput *> & = {}) override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Updating);
        for (int reach = 0; reach < kReaches; ++reach)
            (*flow)[reach] = value(m_step, reach);
        ++m_step;
        setStatus(m_step >= kSteps ? ComponentStatus::Done
                                   : ComponentStatus::Updated);
    }

    void finish() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Finishing);
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Finished);
    }

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
/*!
 * The mesh every artifact in the fixture is written against.
 *
 * Named here rather than built inside recordRun so a test can compare what a
 * reader answered with against what was written, instead of against a second
 * read of the same file — which would agree with itself however wrong it was.
 */
MeshDefinition fixtureMesh()
{
    MeshDefinition mesh;
    mesh.meshName = "reaches";
    // Three nodes, because the producer records one value per node and the
    // UGRID writers check that the field and the mesh agree.
    mesh.nodeX = {0.0, 1.0, 1.0};
    mesh.nodeY = {0.0, 0.0, 1.0};

    // One face over them, so connectivity is something a reader can get
    // wrong. A node-only mesh would make every face assertion below compare
    // zero against zero.
    mesh.faceNodeOffsets = {0, 3};
    mesh.faceNodes       = {0, 1, 2};

    // And its three edges. A value can be recorded on an edge as easily as on
    // a node, and edges are the half of the connectivity that survives no
    // conversion unless something carries it deliberately.
    mesh.edgeNodes = {{0, 1}, {1, 2}, {2, 0}};
    return mesh;
}

std::filesystem::path recordRun(const std::filesystem::path &directory)
{
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    ProducerComponent producer("producer");
    producer.initialize();
    EXPECT_TRUE(producer.validate().empty());
    producer.prepare();

    RunRecorder recorder("reopen-test-run");
    IOThread    io(16);

    auto csv = std::make_shared<CSVWriter>(directory / "results.csv");
    io.addWriter(csv);
    recorder.addWriter(csv);

    // Every format this build can write is recorded, so every reader the
    // build ships is exercised by the tests below.
    const MeshDefinition mesh = fixtureMesh();

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
    // The same mesh the UGRID writers get: a GeoPackage stores the nodes as
    // point features, which is the geometry its reader can answer with.
    geopackage->setMesh(mesh);
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

    // Dimensions are labeled from the catalog so a UI can name axes.
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

// ── Slice 6a: the readers answer for geometry ────────────────────────────
//
// The catalog names what a variable's values are attached to -- mesh and
// location -- so that a reader would not have to infer it. Until something
// reads that back, the field is a comment: a run can be plotted and cannot
// be mapped. Each format is checked against the mesh that was *written*,
// not against a second read of the same file.

TEST(ResultsReopenTest, EveryRecordedFormatReadsBackItsGeometry)
{
    const auto manifestPath = recordRun(reopenDir() / "geometry");
    RunManifest manifest;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, manifest, message)) << message;
    ASSERT_FALSE(manifest.results.empty());

    const MeshDefinition written = fixtureMesh();
    int                  answered = 0;

    for (const ResultEntry &entry : manifest.results)
    {
        SCOPED_TRACE("format: " + entry.format);

        std::string reason;
        auto reader = createResultReader(entry.format, reason);
        ASSERT_NE(reader, nullptr) << reason;
        ASSERT_TRUE(reader->open(manifestPath.parent_path() / entry.artifact,
                                 reason))
            << reason;

        MeshDefinition read;
        const bool     got = reader->readMesh(entry, read, reason);

        if (entry.format == "csv")
        {
            // A CSV of values is a complete recording of values. It reports
            // why there is no geometry rather than failing as though
            // something had gone wrong.
            EXPECT_FALSE(got) << "a CSV artifact produced geometry";
            EXPECT_NE(reason.find("csv"), std::string::npos)
                << "the reason does not name the format: " << reason;
            continue;
        }

        ASSERT_TRUE(got) << entry.format << ": " << reason;
        ++answered;

        ASSERT_EQ(read.nodeCount(), written.nodeCount())
            << entry.format << " read back a different number of nodes";

        for (int64_t node = 0; node < written.nodeCount(); ++node)
        {
            EXPECT_DOUBLE_EQ(read.nodeX[static_cast<size_t>(node)],
                             written.nodeX[static_cast<size_t>(node)])
                << entry.format << " node " << node;
            EXPECT_DOUBLE_EQ(read.nodeY[static_cast<size_t>(node)],
                             written.nodeY[static_cast<size_t>(node)])
                << entry.format << " node " << node;
        }

        if (entry.format == "geopackage")
        {
            // The GeoPackage writer stores node points and no connectivity,
            // so its reader answers with a node-only mesh rather than
            // inventing faces. That is the format's limit, recorded here so
            // it reads as a decision rather than as a gap.
            EXPECT_EQ(read.faceCount(), 0)
                << "the GeoPackage reader invented connectivity";
        }
        else
        {
            ASSERT_EQ(read.faceCount(), written.faceCount())
                << entry.format << " lost the mesh's faces";

            EXPECT_EQ(read.faceNodeOffsets, written.faceNodeOffsets)
                << entry.format << " changed which nodes belong to which face";
            EXPECT_EQ(read.faceNodes, written.faceNodes)
                << entry.format
                << " read the connectivity back with different indices"
                   " -- a start_index or a fill-value that was not undone";
        }

        if (entry.format == "geopackage")
        {
            EXPECT_EQ(read.edgeCount(), 0)
                << "the GeoPackage reader invented edges";
        }
        else
        {
            EXPECT_EQ(read.edgeCount(), written.edgeCount())
                << entry.format << " lost the mesh's edges";
            EXPECT_EQ(read.edgeNodes, written.edgeNodes)
                << entry.format << " read the edge endpoints back changed";
        }

        std::string valid;
        EXPECT_TRUE(read.validate(valid)) << entry.format << ": " << valid;
    }

    EXPECT_GT(answered, 0)
        << "no format in this build answered with geometry at all";
}

TEST(ResultsReopenTest, AnEntryNamingNoMeshIsNotGivenOne)
{
    const auto manifestPath = recordRun(reopenDir() / "geometry-unnamed");
    RunManifest manifest;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, manifest, message)) << message;

    for (const ResultEntry &entry : manifest.results)
    {
        if (entry.format == "csv" || entry.format == "geopackage")
            continue;

        // A UGRID file may hold several meshes -- a 1-D network and a 2-D
        // floodplain both. Falling back to "the first one" for an entry that
        // names none would hand back geometry the values have nothing to do
        // with, which is worse than answering that there is none.
        ResultEntry unnamed = entry;
        unnamed.mesh.clear();

        std::string reason;
        auto reader = createResultReader(entry.format, reason);
        ASSERT_NE(reader, nullptr) << reason;
        ASSERT_TRUE(reader->open(manifestPath.parent_path() / entry.artifact,
                                 reason))
            << reason;

        MeshDefinition read;
        EXPECT_FALSE(reader->readMesh(unnamed, read, reason))
            << entry.format << " produced a mesh for an entry naming none";
        EXPECT_EQ(read.nodeCount(), 0);
    }
}

#ifdef HYDROCOUPLESDK_HAVE_GEOPACKAGE

TEST(ResultsReopenTest, AGeoPackagePointWithAnEnvelopeIsStillRead)
{
    const auto manifestPath = recordRun(reopenDir() / "geometry-envelope");
    RunManifest manifest;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, manifest, message)) << message;

    const auto entry = std::find_if(
        manifest.results.begin(), manifest.results.end(),
        [](const ResultEntry &candidate)
        { return candidate.format == "geopackage"; });
    ASSERT_NE(entry, manifest.results.end());

    const auto artifact = manifestPath.parent_path() / entry->artifact;

    // The SDK's writer stores no envelope, but a GeoPackage is a standard and
    // an artifact that has been through another tool may carry one. The
    // header declares its size precisely so a reader can step over it; this
    // writes such a row and checks that the reader does.
    constexpr double kX = 41.5;
    constexpr double kY = -7.25;

    std::vector<uint8_t> blob(8 + 32 + 21, 0);
    blob[0] = 'G';
    blob[1] = 'P';
    blob[2] = 0;
    blob[3] = 0x03; // little-endian, envelope = xy (four doubles)

    const int32_t srsId = -1;
    std::memcpy(blob.data() + 4, &srsId, 4);

    const double envelope[4] = {kX, kX, kY, kY};
    std::memcpy(blob.data() + 8, envelope, sizeof(envelope));

    uint8_t *wkb = blob.data() + 8 + 32;
    wkb[0] = 0x01;
    const uint32_t point = 1;
    std::memcpy(wkb + 1, &point, 4);
    std::memcpy(wkb + 5, &kX, 8);
    std::memcpy(wkb + 13, &kY, 8);

    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open_v2(artifact.string().c_str(), &db,
                              SQLITE_OPEN_READWRITE, nullptr),
              SQLITE_OK);

    sqlite3_stmt *update = nullptr;
    ASSERT_EQ(sqlite3_prepare_v2(
                  db, "UPDATE mesh_nodes SET geom=? WHERE node_index=0", -1,
                  &update, nullptr),
              SQLITE_OK);
    sqlite3_bind_blob(update, 1, blob.data(), static_cast<int>(blob.size()),
                      SQLITE_TRANSIENT);
    ASSERT_EQ(sqlite3_step(update), SQLITE_DONE);
    sqlite3_finalize(update);
    sqlite3_close(db);

    std::string reason;
    auto        reader = createResultReader("geopackage", reason);
    ASSERT_NE(reader, nullptr) << reason;
    ASSERT_TRUE(reader->open(artifact, reason)) << reason;

    MeshDefinition read;
    ASSERT_TRUE(reader->readMesh(*entry, read, reason)) << reason;
    ASSERT_GT(read.nodeCount(), 0);

    // A reader that assumed no envelope would take the first eight bytes of
    // it as the coordinate and answer with a point nowhere near this one.
    EXPECT_DOUBLE_EQ(read.nodeX.front(), kX);
    EXPECT_DOUBLE_EQ(read.nodeY.front(), kY);
}

#endif // HYDROCOUPLESDK_HAVE_GEOPACKAGE

#ifdef HYDROCOUPLESDK_HAVE_GEOPACKAGE

TEST(ResultsReopenTest, AGeoPackageWithNoNodesSaysSoRatherThanAnsweringEmpty)
{
    const auto manifestPath = recordRun(reopenDir() / "geometry-nonodes");
    RunManifest manifest;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, manifest, message)) << message;

    const auto entry = std::find_if(
        manifest.results.begin(), manifest.results.end(),
        [](const ResultEntry &candidate)
        { return candidate.format == "geopackage"; });
    ASSERT_NE(entry, manifest.results.end());

    const auto artifact = manifestPath.parent_path() / entry->artifact;

    // A GeoPackage written without a mesh has the table and no rows, which is
    // what a run recorded for its values alone looks like.
    sqlite3 *db = nullptr;
    ASSERT_EQ(sqlite3_open_v2(artifact.string().c_str(), &db,
                              SQLITE_OPEN_READWRITE, nullptr),
              SQLITE_OK);
    ASSERT_EQ(sqlite3_exec(db, "DELETE FROM mesh_nodes", nullptr, nullptr,
                           nullptr),
              SQLITE_OK);
    sqlite3_close(db);

    std::string reason;
    auto        reader = createResultReader("geopackage", reason);
    ASSERT_NE(reader, nullptr) << reason;
    ASSERT_TRUE(reader->open(artifact, reason)) << reason;

    MeshDefinition read;

    // An empty mesh returned as a success is the worst of both: a consumer
    // draws nothing and is told nothing was wrong.
    EXPECT_FALSE(reader->readMesh(*entry, read, reason))
        << "an empty mesh_nodes table was reported as a mesh";
    EXPECT_FALSE(reason.empty()) << "no reason was given";
    EXPECT_EQ(read.nodeCount(), 0);
}

#endif // HYDROCOUPLESDK_HAVE_GEOPACKAGE

// ── Slice 6b: the item carries the geometry ──────────────────────────────
//
// Reading the mesh is only half of it. Until the item itself answers as a
// spatial one, a consumer has to know which artifact a run was recorded in
// and open it a second time -- which is exactly the duplication the manifest
// exists to prevent.

TEST(ResultsReopenTest, AReopenedItemIsSpatialAndCarriesItsMesh)
{
    namespace HCS  = HydroCouple::Spatial;
    namespace HCST = HydroCouple::SpatioTemporal;

    const auto manifestPath = recordRun(reopenDir() / "spatial-item");
    RunManifest manifest;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, manifest, message)) << message;

    auto component = ResultsModelComponent::open(manifestPath, "producer",
                                                 message);
    ASSERT_NE(component, nullptr) << message;
    component->initialize();
    ASSERT_NE(component->status(),
              HydroCouple::IModelComponent::ComponentStatus::Failed);

    const auto items = component->results();
    ASSERT_FALSE(items.empty());

    const MeshDefinition written = fixtureMesh();

    auto *surfaceItem =
        dynamic_cast<HCS::IPolyhedralSurfaceComponentDataItem *>(items.front());
    ASSERT_NE(surfaceItem, nullptr)
        << "the most capable recorded format produced an item with no "
           "geometry";

    // Both halves through one item: a consumer should not have to hold a
    // spatial item and a time series and hope they describe the same thing.
    EXPECT_NE(dynamic_cast<HCST::ITimeSeriesPolyhedralSurfaceComponentDataItem *>(
                  items.front()),
              nullptr);

    const HCS::IPolyhedralSurface *surface = surfaceItem->polyhedralSurface();
    ASSERT_NE(surface, nullptr);

    EXPECT_EQ(surface->vertexCount(), written.nodeCount());
    EXPECT_EQ(surface->patchCount(), written.faceCount());

    // The values are recorded per node, so the item says so, and its single
    // entity dimension is that node axis.
    EXPECT_EQ(surfaceItem->location(),
              HCS::MeshLocation::Node);
    EXPECT_NE(surfaceItem->entityDimension(), nullptr);

    // The entity dimension is the one the values run along, which is the
    // second: time is the first, as the interface documents.
    ASSERT_EQ(items.front()->dimensions().size(), 2u);
    EXPECT_EQ(surfaceItem->entityDimension(),
              items.front()->dimensions().back());

    for (int64_t vertex = 0; vertex < surface->vertexCount(); ++vertex)
    {
        const HCS::IVertex *point = surface->vertex(vertex);
        ASSERT_NE(point, nullptr) << "vertex " << vertex;

        // Vertex order is what ties value i to node i. Right points in the
        // wrong order pass every count and draw the wrong map.
        EXPECT_DOUBLE_EQ(point->x(), written.nodeX[static_cast<size_t>(vertex)])
            << "vertex " << vertex;
        EXPECT_DOUBLE_EQ(point->y(), written.nodeY[static_cast<size_t>(vertex)])
            << "vertex " << vertex;
    }

    component->finish();
}

TEST(ResultsReopenTest, AnItemWithNoRecordedMeshIsNotSpatial)
{
    namespace HCS = HydroCouple::Spatial;

    const auto directory = reopenDir() / "spatial-none";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    // A run recorded to CSV alone: values, times, and no geometry anywhere.
    ProducerComponent producer("producer");
    producer.initialize();
    ASSERT_TRUE(producer.validate().empty());
    producer.prepare();

    RunRecorder recorder("csv-only-run");
    IOThread    io(16);
    auto        csv = std::make_shared<CSVWriter>(directory / "results.csv");
    io.addWriter(csv);
    recorder.addWriter(csv);
    recorder.addComponent(&producer);
    ASSERT_TRUE(io.start());

    for (int step = 0; step < kSteps; ++step)
    {
        producer.update();
        Snapshot    snapshot(1000.0 + step);
        std::string capture;
        ASSERT_TRUE(snapshot.capture(*producer.flow, capture)) << capture;
        ASSERT_TRUE(io.enqueue(std::move(snapshot)));
    }

    io.finish();
    ASSERT_EQ(io.errorCode(), 0);
    producer.finish();

    const auto  manifestPath = directory / "run.json";
    std::string message;
    ASSERT_TRUE(recorder.finishToFile(manifestPath, message)) << message;

    auto component = ResultsModelComponent::open(manifestPath, "producer",
                                                 message);
    ASSERT_NE(component, nullptr) << message;
    component->initialize();
    ASSERT_NE(component->status(),
              HydroCouple::IModelComponent::ComponentStatus::Failed);

    const auto items = component->results();
    ASSERT_FALSE(items.empty());

    // Not spatial, rather than spatial with nothing behind it. A consumer
    // asks the question once and gets an answer it can act on.
    EXPECT_EQ(dynamic_cast<HCS::IPolyhedralSurfaceComponentDataItem *>(
                  items.front()),
              nullptr)
        << "an item with no recorded mesh claimed to carry geometry";

    // And it is still a perfectly good time series of values.
    auto *series =
        dynamic_cast<HydroCouple::Temporal::ITimeSeriesComponentDataItem *>(
            items.front());
    ASSERT_NE(series, nullptr);
    EXPECT_EQ(series->timeCount(), kSteps);

    component->finish();
}

TEST(ResultsReopenTest, AReopenedPatchIsAClosedRingOfItsFacesNodes)
{
    namespace HCS = HydroCouple::Spatial;

    const auto manifestPath = recordRun(reopenDir() / "spatial-patches");
    RunManifest manifest;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, manifest, message)) << message;

    auto component = ResultsModelComponent::open(manifestPath, "producer",
                                                 message);
    ASSERT_NE(component, nullptr) << message;
    component->initialize();

    const auto items = component->results();
    ASSERT_FALSE(items.empty());

    auto *surfaceItem =
        dynamic_cast<HCS::IPolyhedralSurfaceComponentDataItem *>(items.front());
    ASSERT_NE(surfaceItem, nullptr);

    const HCS::IPolyhedralSurface *surface = surfaceItem->polyhedralSurface();
    ASSERT_NE(surface, nullptr);

    const MeshDefinition written = fixtureMesh();
    ASSERT_GT(written.faceCount(), 0);
    ASSERT_EQ(surface->patchCount(), written.faceCount());

    for (int64_t face = 0; face < written.faceCount(); ++face)
    {
        const HCS::IPolygon *patch = surface->patch(face);
        ASSERT_NE(patch, nullptr) << "face " << face;

        const HCS::ILineString *ring = patch->exteriorRing();
        ASSERT_NE(ring, nullptr) << "face " << face;

        const int64_t begin = written.faceNodeOffsets[static_cast<size_t>(face)];
        const int64_t end =
            written.faceNodeOffsets[static_cast<size_t>(face) + 1];
        const int corners = static_cast<int>(end - begin);

        // UGRID lists a face's nodes once. A ring repeats the first at the
        // end, and one that does not is a polyline drawn as an open shape --
        // which still counts as one patch and still looks like a mesh until
        // you notice every cell is missing a side.
        ASSERT_EQ(ring->pointCount(), corners + 1) << "face " << face;

        for (int corner = 0; corner < corners; ++corner)
        {
            const HCS::IPoint *point = ring->point(corner);
            ASSERT_NE(point, nullptr) << "face " << face << " corner "
                                      << corner;

            const size_t node = static_cast<size_t>(
                written.faceNodes[static_cast<size_t>(begin + corner)]);

            EXPECT_DOUBLE_EQ(point->x(), written.nodeX[node])
                << "face " << face << " corner " << corner;
            EXPECT_DOUBLE_EQ(point->y(), written.nodeY[node])
                << "face " << face << " corner " << corner;
        }

        const HCS::IPoint *first = ring->point(0);
        const HCS::IPoint *last  = ring->point(corners);
        ASSERT_NE(first, nullptr);
        ASSERT_NE(last, nullptr);
        EXPECT_DOUBLE_EQ(last->x(), first->x()) << "face " << face;
        EXPECT_DOUBLE_EQ(last->y(), first->y()) << "face " << face;
    }

    component->finish();
}

TEST(ResultsReopenTest, AReopenedSurfaceReportsItsConnectivityInBulk)
{
    namespace HCS = HydroCouple::Spatial;

    const auto manifestPath = recordRun(reopenDir() / "spatial-meshview");
    RunManifest manifest;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, manifest, message)) << message;

    auto component = ResultsModelComponent::open(manifestPath, "producer",
                                                 message);
    ASSERT_NE(component, nullptr) << message;
    component->initialize();

    const auto items = component->results();
    ASSERT_FALSE(items.empty());

    auto *surfaceItem =
        dynamic_cast<HCS::IPolyhedralSurfaceComponentDataItem *>(items.front());
    ASSERT_NE(surfaceItem, nullptr);

    const HCS::IPolyhedralSurface *surface = surfaceItem->polyhedralSurface();
    ASSERT_NE(surface, nullptr);

    // The standard calls this "the accessor partitioners, interpolating
    // adapters, IO writers, and device staging must use", and promises it is
    // never null. A view that reports no faces and no edges for a surface
    // that has both is not a lesser answer -- it is a wrong one, and it is
    // wrong in the shape of an empty mesh rather than a missing feature.
    const HCS::IMeshView *view = surface->meshView();
    ASSERT_NE(view, nullptr);

    const MeshDefinition written = fixtureMesh();

    EXPECT_EQ(view->nodeCount(), written.nodeCount());
    EXPECT_EQ(view->faceCount(), written.faceCount());
    EXPECT_EQ(view->edgeCount(), written.edgeCount())
        << "the surface's bulk view lost the edges it was built from";

    ASSERT_EQ(view->faceNodes().size(), written.faceNodes.size());

    for (size_t slot = 0; slot < written.faceNodes.size(); ++slot)
        EXPECT_EQ(view->faceNodes()[slot], written.faceNodes[slot])
            << "face node " << slot;

    // Edges are pairs in the definition and one flat span in the interface,
    // so this is where the two vocabularies could quietly disagree.
    ASSERT_EQ(view->edgeNodes().size(), written.edgeNodes.size() * 2);

    for (size_t edge = 0; edge < written.edgeNodes.size(); ++edge)
    {
        EXPECT_EQ(view->edgeNodes()[edge * 2], written.edgeNodes[edge][0])
            << "edge " << edge << " origin";
        EXPECT_EQ(view->edgeNodes()[edge * 2 + 1], written.edgeNodes[edge][1])
            << "edge " << edge << " destination";
    }

    component->finish();
}
