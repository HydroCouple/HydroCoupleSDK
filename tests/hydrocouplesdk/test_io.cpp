// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the HydroCouple::SDK::IO layer: Snapshot capture through the
// typed data plane, the CSV reference writer, IOThread ordering /
// back-pressure / drain / error latching, and ModelInitializer document
// validation, argument application, wiring, and round-trip serialization.

#include <gtest/gtest.h>

#include "hydrocouplesdk/io/snapshot.h"
#include "hydrocouplesdk/io/iothread.h"
#include "hydrocouplesdk/io/csvwriter.h"
#include "hydrocouplesdk/io/modelinitializer.h"

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <thread>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::IO;

#ifndef TEST_ARTIFACTS_DIR
#define TEST_ARTIFACTS_DIR "."
#endif

namespace {

/*!
 * \brief Where a test writes \p name, under a path only that test uses.
 *
 * Namespaced by the running case: ctest registers every gtest case as a test
 * of its own and `-jN` runs them as concurrent processes over this one
 * directory, so cases asking for the same name overwrite and delete each
 * other's files mid-read.
 */
std::filesystem::path artifactPath(const std::string &name)
{
    const std::filesystem::path dir{TEST_ARTIFACTS_DIR};
    std::filesystem::create_directories(dir);

    if (const auto *info =
            ::testing::UnitTest::GetInstance()->current_test_info())
    {
        return dir / (std::string(info->test_suite_name()) + "." + info->name()
                      + "." + name);
    }

    return dir / name;
}

/*!
 * Writer test double recording delivered snapshots; optionally slow (to
 * exercise back-pressure) or failing (to exercise error latching).
 */
class RecordingWriter final : public IOutputWriter
{
public:
    std::string name() const override { return "recording"; }
    int initialize() override { return initCode; }
    int write(const Snapshot &snapshot) override
    {
        if (delayMs > 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        times.push_back(snapshot.time());
        return writeCode;
    }
    int finalize() override { finalized = true; return 0; }
    std::string errorMessage() const override { return "test failure"; }

    std::vector<double> times;
    std::atomic<bool>   finalized{false};
    int                 initCode  = 0;
    int                 writeCode = 0;
    int                 delayMs   = 0;
};

/*!
 * Minimal component: one Argument1DDouble "coeffs", one Output1DDouble
 * "flow", one Input1DDouble "inflow".
 */
class IOTestComponent : public AbstractModelComponent
{
public:
    explicit IOTestComponent(std::string_view id)
        : AbstractModelComponent(id)
    {}

    std::vector<std::string> validate() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Validating);
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Valid);
        return {};
    }
    void prepare() override {}
    void update(const std::vector<HydroCouple::IOutput *> & = {}) override {}
    void finish() override {}

    Argument1DDouble *coeffs = nullptr;
    Output1DDouble   *flow   = nullptr;
    Input1DDouble    *inflow = nullptr;

protected:
    void createArguments() override
    {
        m_quantity.reset(Quantity::unitLess("IOTestQuantity"));
        coeffs = new Argument1DDouble("coeffs", &m_dim, 0, m_quantity.get(), this);
        addArgument(coeffs);
    }
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        inflow = new Input1DDouble("inflow", &m_dim, 3, m_quantity.get(), this);
        addInput(inflow);
    }
    void createOutputs() override
    {
        flow = new Output1DDouble("flow", &m_dim, 3, m_quantity.get(), this);
        addOutput(flow);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension m_dim{"i", "Index"};
    std::unique_ptr<Quantity> m_quantity;
};

} // namespace

// ── Snapshot ──────────────────────────────────────────────────────────────

TEST(SnapshotTest, CapturesNumericItem)
{
    Dimension dim{"i", "Index"};
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Q")};
    Output1DDouble item("flow", &dim, 3, quantity.get(), nullptr);
    item[0] = 1.5; item[1] = 2.5; item[2] = 3.5;

    Snapshot snapshot(100.0);
    std::string message;
    ASSERT_TRUE(snapshot.capture(item, message)) << message;

    ASSERT_EQ(snapshot.fields().size(), 1u);
    const FieldSlice &field = snapshot.fields()[0];
    EXPECT_EQ(field.itemId, "flow");
    EXPECT_EQ(field.kind, DataKind::Float64);
    EXPECT_EQ(field.shape, (std::vector<int64_t>{3}));
    EXPECT_DOUBLE_EQ(field.values<double>()[1], 2.5);
}

TEST(SnapshotTest, RejectsStringItems)
{
    Dimension dim{"i", "Index"};
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Q")};
    Output1DString item("labels", &dim, 2, quantity.get(), nullptr);

    Snapshot snapshot(0.0);
    std::string message;
    EXPECT_FALSE(snapshot.capture(item, message));
    EXPECT_FALSE(message.empty());
}

TEST(SnapshotTest, CaptureIsAnOwnedCopy)
{
    Dimension dim{"i", "Index"};
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Q")};
    Output1DDouble item("flow", &dim, 1, quantity.get(), nullptr);
    item[0] = 7.0;

    Snapshot snapshot(0.0);
    std::string message;
    ASSERT_TRUE(snapshot.capture(item, message)) << message;
    item[0] = -1.0; // mutate after capture
    EXPECT_DOUBLE_EQ(snapshot.fields()[0].values<double>()[0], 7.0);
}

// ── CSVWriter ─────────────────────────────────────────────────────────────

TEST(CSVWriterTest, WritesLongFormatRows)
{
    const auto path = artifactPath("io_csvwriter.csv");
    Dimension dim{"i", "Index"};
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Q")};
    Output1DDouble item("flow", &dim, 2, quantity.get(), nullptr);
    item[0] = 1.0; item[1] = 2.0;

    CSVWriter writer(path);
    ASSERT_EQ(writer.initialize(), 0) << writer.errorMessage();

    Snapshot snapshot(100.5);
    std::string message;
    ASSERT_TRUE(snapshot.capture(item, message)) << message;
    ASSERT_EQ(writer.write(snapshot), 0) << writer.errorMessage();
    ASSERT_EQ(writer.finalize(), 0) << writer.errorMessage();

    std::ifstream file(path);
    std::string line;
    std::getline(file, line);
    EXPECT_EQ(line, "time,item,index,value");
    std::getline(file, line);
    EXPECT_EQ(line, "100.5,flow,0,1");
    std::getline(file, line);
    EXPECT_EQ(line, "100.5,flow,1,2");
}

TEST(CSVWriterTest, ReportsUnopenablePath)
{
    CSVWriter writer(std::filesystem::path(TEST_ARTIFACTS_DIR) /
                     "no_such_dir_marker" / "" / "");
    // A directory path cannot be opened as a file.
    CSVWriter dirWriter{std::filesystem::path(TEST_ARTIFACTS_DIR)};
    EXPECT_NE(dirWriter.initialize(), 0);
    EXPECT_FALSE(dirWriter.errorMessage().empty());
}

// ── IOThread ──────────────────────────────────────────────────────────────

TEST(IOThreadTest, DeliversInOrderAndDrainsOnFinish)
{
    auto writer = std::make_shared<RecordingWriter>();
    IOThread io(8);
    io.addWriter(writer);
    ASSERT_TRUE(io.start());

    for (int i = 0; i < 20; ++i)
        ASSERT_TRUE(io.enqueue(Snapshot(static_cast<double>(i))));
    io.finish();

    ASSERT_EQ(writer->times.size(), 20u);
    for (int i = 0; i < 20; ++i)
        EXPECT_DOUBLE_EQ(writer->times[static_cast<size_t>(i)], i);
    EXPECT_TRUE(writer->finalized.load());
    EXPECT_EQ(io.errorCode(), 0);
    EXPECT_EQ(io.deliveredCount(), 20u);
}

TEST(IOThreadTest, BackPressureBlocksProducerUntilDrained)
{
    auto writer = std::make_shared<RecordingWriter>();
    writer->delayMs = 5; // slow consumer
    IOThread io(2);      // tiny queue
    io.addWriter(writer);
    ASSERT_TRUE(io.start());

    const auto begin = std::chrono::steady_clock::now();
    for (int i = 0; i < 10; ++i)
        ASSERT_TRUE(io.enqueue(Snapshot(static_cast<double>(i))));
    io.finish();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - begin);

    // 10 writes × 5 ms with capacity 2 forces the producer to wait; every
    // snapshot still arrives, in order. The lower bound carries a 50%%
    // scheduling margin so coarse timers / loaded CI machines cannot flake
    // the assertion (the semantic proof is completeness + ordering below).
    EXPECT_GE(elapsed.count(), 5 * (10 - 2) / 2);
    ASSERT_EQ(writer->times.size(), 10u);
    EXPECT_DOUBLE_EQ(writer->times.back(), 9.0);
}

TEST(IOThreadTest, LatchesFirstWriterErrorAndDisablesWriter)
{
    auto failing = std::make_shared<RecordingWriter>();
    failing->writeCode = 7;
    auto healthy = std::make_shared<RecordingWriter>();

    IOThread io(4);
    io.addWriter(failing);
    io.addWriter(healthy);
    ASSERT_TRUE(io.start());
    for (int i = 0; i < 3; ++i)
        ASSERT_TRUE(io.enqueue(Snapshot(static_cast<double>(i))));
    io.finish();

    EXPECT_EQ(io.errorCode(), 7);
    EXPECT_NE(io.errorMessage().find("test failure"), std::string::npos);
    EXPECT_EQ(failing->times.size(), 1u);  // disabled after first failure
    EXPECT_EQ(healthy->times.size(), 3u);  // unaffected
}

TEST(IOThreadTest, EnqueueAfterFinishIsRejected)
{
    IOThread io(2);
    io.addWriter(std::make_shared<RecordingWriter>());
    ASSERT_TRUE(io.start());
    io.finish();
    EXPECT_FALSE(io.enqueue(Snapshot(0.0)));
}

// ── ModelInitializer ──────────────────────────────────────────────────────

class ModelInitializerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_up   = std::make_unique<IOTestComponent>("upstream");
        m_down = std::make_unique<IOTestComponent>("downstream");
        m_resolver = [this](const std::string &id) -> IModelComponent * {
            if (id == "upstream")   return m_up.get();
            if (id == "downstream") return m_down.get();
            return nullptr;
        };
    }

    static constexpr const char *kDocument = R"({
        "components": [
            { "id": "upstream",
              "arguments": { "coeffs": { "values": [0.013, 0.02] } } },
            { "id": "downstream", "arguments": {} }
        ],
        "connections": [
            { "from": { "component": "upstream",   "output": "flow" },
              "to":   { "component": "downstream", "input":  "inflow" } }
        ]
    })";

    std::unique_ptr<IOTestComponent>    m_up, m_down;
    ModelInitializer::ComponentResolver m_resolver;
};

TEST_F(ModelInitializerTest, AppliesArgumentsInitializesAndWires)
{
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(kDocument, message))
        << message;

    ASSERT_NE(m_up->coeffs, nullptr);
    EXPECT_EQ(m_up->coeffs->length(), 2);
    EXPECT_DOUBLE_EQ((*m_up->coeffs)[0], 0.013);
    EXPECT_EQ(m_up->status(),
              IModelComponent::ComponentStatus::Initialized);
    EXPECT_EQ(m_down->inflow->provider(),
              static_cast<IOutput *>(m_up->flow));
}

TEST_F(ModelInitializerTest, RejectsUnknownComponentBeforeApplying)
{
    ModelInitializer initializer(m_resolver);
    std::string message;
    EXPECT_FALSE(initializer.initializeFromJsonText(
        R"({"components": [{"id": "ghost"}]})", message));
    EXPECT_NE(message.find("ghost"), std::string::npos);
    // Nothing was applied to the known components.
    EXPECT_EQ(m_up->status(), IModelComponent::ComponentStatus::Created);
}

TEST_F(ModelInitializerTest, RejectsUnknownArgumentWithActionableMessage)
{
    // Arguments are created during initialize(); pre-initialize so the
    // validator can see them, then feed a bogus argument id.
    m_up->initialize();
    ModelInitializer initializer(m_resolver);
    std::string message;
    EXPECT_FALSE(initializer.initializeFromJsonText(
        R"({"components": [{"id": "upstream",
                            "arguments": {"bogus": 1}}]})", message));
    EXPECT_NE(message.find("bogus"), std::string::npos);
    EXPECT_NE(message.find("upstream"), std::string::npos);
}

TEST_F(ModelInitializerTest, RejectsIncompatibleConnection)
{
    ModelInitializer initializer(m_resolver);
    std::string message;
    EXPECT_FALSE(initializer.initializeFromJsonText(R"({
        "components": [ { "id": "upstream" }, { "id": "downstream" } ],
        "connections": [
            { "from": { "component": "upstream",   "output": "flow" },
              "to":   { "component": "downstream", "input":  "no_such" } }
        ]})", message));
    EXPECT_NE(message.find("no_such"), std::string::npos);
}

TEST_F(ModelInitializerTest, RoundTripSerializesSameShape)
{
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(kDocument, message))
        << message;

    const nlohmann::json document =
        initializer.serialize({"upstream", "downstream"}, message);
    ASSERT_FALSE(document.is_null()) << message;

    // Argument values survive the round trip.
    const auto &upstream = document["components"][0];
    EXPECT_EQ(upstream["id"], "upstream");
    EXPECT_EQ(upstream["arguments"]["coeffs"]["values"],
              nlohmann::json::parse("[0.013, 0.02]"));
    // The connection is reproduced from live wiring.
    ASSERT_EQ(document["connections"].size(), 1u);
    EXPECT_EQ(document["connections"][0]["from"]["output"], "flow");
    EXPECT_EQ(document["connections"][0]["to"]["input"], "inflow");

    // And the serialized document re-initializes fresh components.
    auto up2   = std::make_unique<IOTestComponent>("upstream");
    auto down2 = std::make_unique<IOTestComponent>("downstream");
    ModelInitializer second([&](const std::string &id) -> IModelComponent * {
        if (id == "upstream")   return up2.get();
        if (id == "downstream") return down2.get();
        return nullptr;
    });
    ASSERT_TRUE(second.initialize(document, message)) << message;
    EXPECT_DOUBLE_EQ((*up2->coeffs)[1], 0.02);
}

TEST_F(ModelInitializerTest, SerializeToFileWritesReviewableDocument)
{
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(kDocument, message))
        << message;

    const auto path = artifactPath("io_composition_roundtrip.json");
    ASSERT_TRUE(initializer.serializeToFile({"upstream", "downstream"}, path,
                                            message)) << message;
    std::ifstream file(path);
    nlohmann::json document;
    file >> document;
    EXPECT_TRUE(document.contains("components"));
}

TEST(ModelInitializerYaml, YamlFileWithoutSupportFailsClearly)
{
    if (ModelInitializer::yamlSupported())
        GTEST_SKIP() << "yaml-cpp available; the failure path is moot";

    const auto path = artifactPath("io_composition.yaml");
    {
        std::ofstream file(path);
        file << "components:\n  - id: upstream\n";
    }
    ModelInitializer initializer(
        [](const std::string &) -> IModelComponent * { return nullptr; });
    std::string message;
    EXPECT_FALSE(initializer.initializeFromFile(path, message));
    EXPECT_NE(message.find("yaml"), std::string::npos);
}

// ── End to end: initializer → snapshots → IOThread → CSV ──────────────────

TEST(IOEndToEnd, ComposeCaptureWriteVerify)
{
    IOTestComponent component("model");
    ModelInitializer initializer(
        [&](const std::string &id) -> IModelComponent * {
            return id == "model" ? &component : nullptr;
        });
    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(R"({
        "components": [{ "id": "model",
                         "arguments": { "coeffs": { "values": [1.0] } } }]
    })", message)) << message;

    const auto path = artifactPath("io_end_to_end.csv");
    IOThread io(4);
    io.addWriter(std::make_shared<CSVWriter>(path));
    ASSERT_TRUE(io.start());

    for (int step = 0; step < 3; ++step)
    {
        (*component.flow)[0] = step * 10.0;
        (*component.flow)[1] = step * 10.0 + 1.0;
        (*component.flow)[2] = step * 10.0 + 2.0;
        Snapshot snapshot(100.0 + step);
        ASSERT_TRUE(snapshot.capture(*component.flow, message)) << message;
        ASSERT_TRUE(io.enqueue(std::move(snapshot)));
    }
    io.finish();
    ASSERT_EQ(io.errorCode(), 0) << io.errorMessage();

    // 1 header + 3 snapshots × 3 elements.
    std::ifstream file(path);
    int lines = 0;
    for (std::string line; std::getline(file, line);)
        ++lines;
    EXPECT_EQ(lines, 1 + 9);
}

// ── Sidecar binary references (efficiency contract, plan §4.7) ────────────

#include "hydrocouplesdk/io/sidecar.h"
#include <chrono>

class SidecarTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Per case, not one "sidecars" for all of them: SetUp() wipes this
        // directory, so in parallel one case cleared another's files out
        // from under it in the middle of a run.
        m_dir = std::filesystem::path(TEST_ARTIFACTS_DIR) / "sidecars"
                / ::testing::UnitTest::GetInstance()->current_test_info()->name();
        std::filesystem::remove_all(m_dir);
        std::filesystem::create_directories(m_dir);

        m_component = std::make_unique<IOTestComponent>("model");
        m_resolver  = [this](const std::string &id) -> IModelComponent * {
            return id == "model" ? m_component.get() : nullptr;
        };
    }

    std::filesystem::path               m_dir;
    std::unique_ptr<IOTestComponent>    m_component;
    ModelInitializer::ComponentResolver m_resolver;
};

TEST_F(SidecarTest, SmallPayloadsStayInline)
{
    ModelInitializer initializer(m_resolver);
    initializer.setSidecarDirectory(m_dir);
    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(R"({
        "components": [{ "id": "model",
                         "arguments": { "coeffs": { "values": [1.0, 2.0] } } }]
    })", message)) << message;

    const auto document = initializer.serialize({"model"}, message);
    ASSERT_FALSE(document.is_null()) << message;
    // 2 doubles = 16 bytes < 1024 → inline array, no sidecar file.
    EXPECT_TRUE(document["components"][0]["arguments"]["coeffs"]
                    .contains("values"));
    EXPECT_TRUE(std::filesystem::is_empty(m_dir));
}

TEST_F(SidecarTest, LargePayloadBecomesReferenceAndRoundTrips)
{
    // 1000 doubles = 8000 bytes > 1024 → sidecar.
    std::string message;
    m_component->initialize();
    m_component->coeffs->resizeDataArray(1000);
    for (int i = 0; i < 1000; ++i)
        (*m_component->coeffs)[i] = i * 0.5;

    ModelInitializer initializer(m_resolver);
    initializer.setSidecarDirectory(m_dir);
    const auto document = initializer.serialize({"model"}, message);
    ASSERT_FALSE(document.is_null()) << message;

    const auto &payload = document["components"][0]["arguments"]["coeffs"];
    ASSERT_TRUE(payload.contains("@ref")) << payload.dump();
    EXPECT_EQ(payload["kind"], "Float64");
    EXPECT_EQ(payload["shape"], nlohmann::json::parse("[1000]"));
    EXPECT_TRUE(std::filesystem::exists(m_dir / "model_coeffs.bin"));
    EXPECT_EQ(std::filesystem::file_size(m_dir / "model_coeffs.bin"),
              1000u * sizeof(double));

    // Re-initialize a fresh component from the document + sidecar.
    auto fresh = std::make_unique<IOTestComponent>("model");
    ModelInitializer second(
        [&](const std::string &id) -> IModelComponent * {
            return id == "model" ? fresh.get() : nullptr;
        });
    second.setSidecarDirectory(m_dir);
    ASSERT_TRUE(second.initialize(document, message)) << message;
    ASSERT_EQ(fresh->coeffs->length(), 1000);
    EXPECT_DOUBLE_EQ((*fresh->coeffs)[999], 999 * 0.5);
}

TEST_F(SidecarTest, CorruptedSidecarFailsWithActionableMessage)
{
    std::string message;
    m_component->initialize();
    m_component->coeffs->resizeDataArray(1000);
    ModelInitializer initializer(m_resolver);
    initializer.setSidecarDirectory(m_dir);
    const auto document = initializer.serialize({"model"}, message);
    ASSERT_FALSE(document.is_null()) << message;

    // Truncate the sidecar.
    std::filesystem::resize_file(m_dir / "model_coeffs.bin", 100);

    auto fresh = std::make_unique<IOTestComponent>("model");
    ModelInitializer second(
        [&](const std::string &id) -> IModelComponent * {
            return id == "model" ? fresh.get() : nullptr;
        });
    second.setSidecarDirectory(m_dir);
    EXPECT_FALSE(second.initialize(document, message));
    EXPECT_NE(message.find("100"), std::string::npos);
}

TEST_F(SidecarTest, TenMillionElementRoundTripAtBulkSpeed)
{
    // The plan's §4.7 benchmark: a 10⁷-element (80 MB) argument round-trips
    // through a sidecar reference with a flat document and bulk-copy speed.
    constexpr int64_t kCount = 10'000'000;
    std::string message;
    m_component->initialize();
    m_component->coeffs->resizeDataArray(static_cast<int>(kCount));
    auto &raw = m_component->coeffs->rawData();
    for (int64_t i = 0; i < kCount; ++i)
        raw[static_cast<size_t>(i)] = static_cast<double>(i % 1000);

    ModelInitializer initializer(m_resolver);
    initializer.setSidecarDirectory(m_dir);

    const auto begin = std::chrono::steady_clock::now();
    const auto document = initializer.serialize({"model"}, message);
    ASSERT_FALSE(document.is_null()) << message;

    auto fresh = std::make_unique<IOTestComponent>("model");
    ModelInitializer second(
        [&](const std::string &id) -> IModelComponent * {
            return id == "model" ? fresh.get() : nullptr;
        });
    second.setSidecarDirectory(m_dir);
    ASSERT_TRUE(second.initialize(document, message)) << message;
    const auto elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - begin).count();

    // Correctness.
    ASSERT_EQ(fresh->coeffs->length(), kCount);
    EXPECT_DOUBLE_EQ((*fresh->coeffs)[kCount - 1],
                     static_cast<double>((kCount - 1) % 1000));
    // The document stayed flat — a reference object, not a 10⁷-element
    // array. This is the assertion that actually proves the sidecar took
    // the bulk path: a DOM-shaped serialization of 10⁷ doubles produces a
    // ~70 MB document, so it cannot pass this bound at any speed.
    EXPECT_LT(document.dump().size(), 4096u);

    // Wall-clock is reported, not asserted tightly. Throughput on a shared
    // CI runner is set by the machine's disk and by whatever else is running
    // on it — a threshold near the observed time (~3 s here) measures the
    // runner, not the SDK, and flakes under load. The bound below is a
    // pathology guard only; the structural assertion above is what fails if
    // the sidecar path regresses.
    const double mb = 2.0 * kCount * sizeof(double) / (1024.0 * 1024.0);
    std::printf("[ sidecar ] %.0f MB round trip in %.2f s (%.0f MB/s)\n",
                mb, elapsed, mb / elapsed);
    EXPECT_LT(elapsed, 120.0);
}

// ── Argument bindings (@from) ─────────────────────────────────────────────

namespace {

/*!
 * A provider: no inputs, one produced output. A real mesh generator in
 * miniature — it computes its output during its run, not before.
 */
class ProviderComponent : public AbstractModelComponent
{
public:
    explicit ProviderComponent(std::string_view id)
        : AbstractModelComponent(id)
    {}

    std::vector<std::string> validate() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Validating);
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Valid);
        return {};
    }
    void prepare() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Preparing);
        ++prepared;
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Updated);
    }
    void update(const std::vector<HydroCouple::IOutput *> & = {}) override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Updating);
        using Status = HydroCouple::IModelComponent::ComponentStatus;
        if (failOnUpdate)
        {
            setStatus(Status::Failed, "the mesher broke a tooth");
            return;
        }
        (*produced)[0] = 10.0;
        (*produced)[1] = 20.0;
        (*produced)[2] = 30.0;
        setStatus(Status::Done);
    }
    void finish() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Finishing);
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Finished);
    }

    Output1DDouble *produced     = nullptr;
    int             prepared     = 0;
    bool            failOnUpdate = false;

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
        m_quantity.reset(Quantity::unitLess("Produced"));
        produced =
            new Output1DDouble("produced", &m_dim, 3, m_quantity.get(), this);
        addOutput(produced);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension m_dim{"i", "Index"};
    std::unique_ptr<Quantity> m_quantity;
};

} // namespace

class ArgumentBindingTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_provider = std::make_unique<ProviderComponent>("prov");
        m_consumer = std::make_unique<IOTestComponent>("consumer");
        m_second   = std::make_unique<IOTestComponent>("second");
        m_resolver = [this](const std::string &id) -> IModelComponent * {
            if (id == "prov")     return m_provider.get();
            if (id == "consumer") return m_consumer.get();
            if (id == "second")   return m_second.get();
            return nullptr;
        };
    }

    // The consumer is declared FIRST on purpose: staging, not the
    // document, orders initialization.
    static constexpr const char *kBoundDocument = R"({
        "components": [
            { "id": "consumer",
              "arguments": { "coeffs": { "@from": {
                  "component": "prov", "output": "produced" } } } },
            { "id": "prov" }
        ]
    })";

    std::unique_ptr<ProviderComponent>  m_provider;
    std::unique_ptr<IOTestComponent>    m_consumer, m_second;
    ModelInitializer::ComponentResolver m_resolver;
};

TEST_F(ArgumentBindingTest, ABoundArgumentReceivesTheProvidersProducedValue)
{
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(kBoundDocument, message))
        << message;

    using Status = HydroCouple::IModelComponent::ComponentStatus;
    EXPECT_EQ(m_provider->status(), Status::Finished);
    ASSERT_NE(m_consumer->coeffs, nullptr);
    ASSERT_EQ(m_consumer->coeffs->length(), 3);
    EXPECT_DOUBLE_EQ((*m_consumer->coeffs)[0], 10.0);
    EXPECT_DOUBLE_EQ((*m_consumer->coeffs)[1], 20.0);
    EXPECT_DOUBLE_EQ((*m_consumer->coeffs)[2], 30.0);
}

TEST_F(ArgumentBindingTest, SerializeRoundTripsTheReferenceNotTheValue)
{
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(kBoundDocument, message))
        << message;

    const nlohmann::json document =
        initializer.serialize({"consumer", "prov"}, message);
    ASSERT_FALSE(document.empty()) << message;

    const nlohmann::json &payload =
        document["components"][0]["arguments"]["coeffs"];
    ASSERT_TRUE(payload.contains("@from"))
        << "the produced value was baked into the document: "
        << payload.dump();
    EXPECT_EQ(payload["@from"]["component"], "prov");
    EXPECT_FALSE(payload.contains("values"));
}

TEST_F(ArgumentBindingTest, ABoundProviderRunsOnceForAllItsConsumers)
{
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(R"({
        "components": [
            { "id": "consumer",
              "arguments": { "coeffs": { "@from": {
                  "component": "prov", "output": "produced" } } } },
            { "id": "second",
              "arguments": { "coeffs": { "@from": {
                  "component": "prov", "output": "produced" } } } },
            { "id": "prov" }
        ]
    })", message)) << message;

    EXPECT_EQ(m_provider->prepared, 1);
    ASSERT_NE(m_second->coeffs, nullptr);
    EXPECT_DOUBLE_EQ((*m_second->coeffs)[2], 30.0);
}

TEST_F(ArgumentBindingTest, AProviderFailureIsANamedError)
{
    m_provider->failOnUpdate = true;
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_FALSE(initializer.initializeFromJsonText(kBoundDocument, message));
    EXPECT_NE(message.find("prov"), std::string::npos) << message;
    EXPECT_NE(message.find("broke a tooth"), std::string::npos) << message;
}

TEST_F(ArgumentBindingTest, ABindingToAMissingOutputNamesIt)
{
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_FALSE(initializer.initializeFromJsonText(R"({
        "components": [
            { "id": "consumer",
              "arguments": { "coeffs": { "@from": {
                  "component": "prov", "output": "ghost" } } } },
            { "id": "prov" }
        ]
    })", message));
    EXPECT_NE(message.find("ghost"), std::string::npos) << message;
    EXPECT_NE(message.find("no output"), std::string::npos) << message;
}

TEST_F(ArgumentBindingTest, ProgressReportsProvidersBeforeConsumers)
{
    ModelInitializer initializer(m_resolver);
    struct Event
    {
        int         stage;
        int         stageCount;
        std::string component;
        std::string activity;
    };
    std::vector<Event> events;
    initializer.setProgressCallback(
        [&events](int stage, int stageCount, const std::string &component,
                  const std::string &activity) {
            events.push_back({stage, stageCount, component, activity});
        });

    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(kBoundDocument, message))
        << message;

    ASSERT_GE(events.size(), 3u);
    // Stage 0 initializes the provider; stage 1 runs it for the binding
    // and then initializes the consumer.
    EXPECT_EQ(events[0].component, "prov");
    EXPECT_EQ(events[0].activity, "initializing");
    EXPECT_EQ(events[0].stage, 0);
    EXPECT_EQ(events[0].stageCount, 2);
    const auto running = std::find_if(events.begin(), events.end(),
                                      [](const Event &event) {
                                          return event.activity ==
                                                 "running provider";
                                      });
    ASSERT_NE(running, events.end());
    EXPECT_EQ(running->component, "prov");
    const auto consumer = std::find_if(events.begin(), events.end(),
                                       [](const Event &event) {
                                           return event.component ==
                                                  "consumer";
                                       });
    ASSERT_NE(consumer, events.end());
    EXPECT_EQ(consumer->stage, 1);
    // The provider run happens INSIDE the consumer's apply — its argument
    // is what needs the run — so it follows the consumer's begin event.
    EXPECT_GT(running - events.begin(), consumer - events.begin());
}
