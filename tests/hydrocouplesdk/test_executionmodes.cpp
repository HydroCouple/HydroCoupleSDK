// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Execution modes and composition-level reuse: a document declaring
// execution.mode "open" initializes a composition whose upstream is a
// recorded run, and driving a live downstream from that stored upstream
// reproduces the all-live result exactly.

#include <gtest/gtest.h>

#include "hydrocouplesdk/io/modelinitializer.h"
#include "hydrocouplesdk/io/runmanifest.h"
#include "hydrocouplesdk/io/csvwriter.h"
#include "hydrocouplesdk/io/iothread.h"
#include "hydrocouplesdk/io/snapshot.h"
#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/data/timesliceoutput.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <array>
#include <filesystem>
#include <memory>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::IO;

namespace {

constexpr int kSteps   = 5;
constexpr int kReaches = 2;

std::filesystem::path modesDir()
{
    const std::filesystem::path dir =
        std::filesystem::path(TEST_ARTIFACTS_DIR) / "modes";
    std::filesystem::create_directories(dir);
    return dir;
}

/*!
 * Upstream: emits a deterministic series.
 */
class Upstream : public AbstractModelComponent
{
public:
    Upstream() : AbstractModelComponent("upstream", "Upstream") {}

    static double value(int step, int reach)
    {
        return 10.0 * step + reach + 0.5;
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

    Output1DDouble *flow = nullptr;
    int             m_step = 0;

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &m) override { m.clear(); return true; }
    void createInputs() override {}
    void createOutputs() override
    {
        m_quantity.reset(Quantity::unitLess("Q"));
        flow = new Output1DDouble("flow", &m_dimension, kReaches,
                                  m_quantity.get(), this);
        addOutput(flow);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension                 m_dimension{"reach", "Reach"};
    std::unique_ptr<Quantity> m_quantity;
};

/*!
 * Downstream: doubles whatever it is given, recording each step.
 */
class Downstream : public AbstractModelComponent
{
public:
    Downstream() : AbstractModelComponent("downstream", "Downstream") {}

    std::vector<std::string> validate() override { return {}; }
    void prepare() override { m_step = 0; history.clear(); }

    void update(const std::vector<IOutput *> & = {}) override
    {
        // Pull from whatever provider is wired — live or recorded.
        if (IOutput *provider = inflow->provider())
        {
            const std::array<int64_t, 1> shape{kReaches};
            std::array<double, kReaches>  values{};
            BufferDescriptor destination;
            destination.data  = values.data();
            destination.kind  = DataKind::Float64;
            destination.rank  = 1;
            destination.shape = shape.data();
            destination.space = MemorySpace::Host;

            // The provider is always rank-1 here: a live output, or a
            // TimeSliceOutput over a recorded { time, reach } variable.
            std::string message;
            const std::array<int64_t, 1> start{0};
            (void)provider->getValuesInto(destination, start, shape, &message);
            for (int reach = 0; reach < kReaches; ++reach)
                (*routed)[reach] = 2.0 * values[reach];
        }

        history.push_back({(*routed)[0], (*routed)[1]});
        ++m_step;
        setStatus(m_step >= kSteps ? ComponentStatus::Done
                                   : ComponentStatus::Updated);
    }

    void finish() override { setStatus(ComponentStatus::Finished); }

    Input1DDouble  *inflow = nullptr;
    Output1DDouble *routed = nullptr;
    std::vector<std::array<double, kReaches>> history;
    int m_step = 0;

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &m) override { m.clear(); return true; }
    void createInputs() override
    {
        inflow = new Input1DDouble("inflow", &m_dimension, kReaches,
                                   m_quantity.get(), this);
        addInput(inflow);
    }
    void createOutputs() override
    {
        m_quantity.reset(Quantity::unitLess("Q"));
        routed = new Output1DDouble("routed", &m_dimension, kReaches,
                                    m_quantity.get(), this);
        addOutput(routed);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension                 m_dimension{"reach", "Reach"};
    std::unique_ptr<Quantity> m_quantity;
};

/*!
 * Records an upstream-only run and returns its manifest path.
 */
std::filesystem::path recordUpstream(const std::filesystem::path &directory)
{
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    Upstream upstream;
    upstream.initialize();
    upstream.prepare();

    RunRecorder recorder("upstream-run");
    IOThread    io(16);
    auto csv = std::make_shared<CSVWriter>(directory / "upstream.csv");
    io.addWriter(csv);
    recorder.addWriter(csv);
    recorder.addComponent(&upstream);
    EXPECT_TRUE(io.start());

    for (int step = 0; step < kSteps; ++step)
    {
        upstream.update();
        Snapshot    snapshot(500.0 + step);
        std::string message;
        EXPECT_TRUE(snapshot.capture(*upstream.flow, message)) << message;
        EXPECT_TRUE(io.enqueue(std::move(snapshot)));
    }
    io.finish();
    upstream.finish();

    const auto manifestPath = directory / "run.json";
    std::string message;
    EXPECT_TRUE(recorder.finishToFile(manifestPath, message)) << message;
    return manifestPath;
}

} // namespace

// ── execution.mode "open" ─────────────────────────────────────────────────

TEST(ExecutionModeTest, OpenSubstitutesRecordedResultsWithoutALiveComponent)
{
    const auto directory = modesDir() / "substitute";
    const auto manifest  = recordUpstream(directory);

    // The analysis case: the host has no upstream component at all.
    ModelInitializer initializer(
        [](const std::string &) -> IModelComponent * { return nullptr; });

    nlohmann::json document = {
        {"schema_version", "1.0"},
        {"components",
         {{{"id", "upstream"},
           {"execution",
            {{"mode", "open"}, {"results", manifest.string()}}}}}}};

    std::string message;
    ASSERT_TRUE(initializer.initialize(document, message)) << message;

    // A ResultsModelComponent was substituted and is already Finished.
    ASSERT_EQ(initializer.openedComponents().size(), 1u);
    IModelComponent *opened = initializer.component("upstream");
    ASSERT_NE(opened, nullptr);
    EXPECT_EQ(opened->status(), IModelComponent::ComponentStatus::Finished);
    ASSERT_EQ(opened->results().size(), 1u);
    EXPECT_EQ(opened->results()[0]->id(), "flow");
}

TEST(ExecutionModeTest, OpenLetsAComponentLoadItsOwnResultsFirst)
{
    const auto directory = modesDir() / "self-loading";
    const auto manifest  = recordUpstream(directory);

    // A component that finishes during initialize() is taken at its word:
    // no substitute is created.
    struct SelfLoading : Upstream
    {
        void initialize() override
        {
            setStatus(ComponentStatus::Initializing);
            setStatus(ComponentStatus::Initialized);
            setStatus(ComponentStatus::Finishing);
            setStatus(ComponentStatus::Finished, "loaded own results");
        }
    } selfLoading;

    ModelInitializer initializer(
        [&](const std::string &id) -> IModelComponent * {
            return id == "upstream" ? &selfLoading : nullptr;
        });

    nlohmann::json document = {
        {"schema_version", "1.0"},
        {"components",
         {{{"id", "upstream"},
           {"execution",
            {{"mode", "open"}, {"results", manifest.string()}}}}}}};

    std::string message;
    ASSERT_TRUE(initializer.initialize(document, message)) << message;
    EXPECT_TRUE(initializer.openedComponents().empty());
    EXPECT_EQ(initializer.component("upstream"),
              static_cast<IModelComponent *>(&selfLoading));
}

TEST(ExecutionModeTest, ResumeIsDiagnosedAsReserved)
{
    Upstream upstream;
    ModelInitializer initializer(
        [&](const std::string &id) -> IModelComponent * {
            return id == "upstream" ? &upstream : nullptr;
        });

    std::string message;
    EXPECT_FALSE(initializer.initialize(
        nlohmann::json::parse(R"({"components": [
            {"id": "upstream", "execution": {"mode": "resume"}}]})"),
        message));
    EXPECT_NE(message.find("resume"), std::string::npos);
    EXPECT_NE(message.find("reserved"), std::string::npos);
}

// ── Stored run as a boundary condition ────────────────────────────────────

TEST(ExecutionModeTest, StoredUpstreamReproducesTheAllLiveResult)
{
    // 1. All live: upstream drives downstream directly.
    std::vector<std::array<double, kReaches>> liveHistory;
    {
        Upstream   upstream;
        Downstream downstream;
        upstream.initialize();
        downstream.initialize();
        ASSERT_TRUE(downstream.inflow->setProvider(upstream.flow));
        upstream.flow->addConsumer(downstream.inflow);

        upstream.prepare();
        downstream.prepare();
        for (int step = 0; step < kSteps; ++step)
        {
            upstream.update();
            downstream.update();
        }
        liveHistory = downstream.history;
    }
    ASSERT_EQ(liveHistory.size(), static_cast<size_t>(kSteps));

    // 2. Reopened upstream drives the same live downstream.
    const auto directory = modesDir() / "boundary";
    const auto manifest  = recordUpstream(directory);

    Downstream downstream;
    ModelInitializer initializer(
        [&](const std::string &id) -> IModelComponent * {
            return id == "downstream" ? &downstream : nullptr;
        });

    nlohmann::json document = {
        {"schema_version", "1.0"},
        {"components",
         {{{"id", "upstream"},
           {"execution", {{"mode", "open"}, {"results", manifest.string()}}}},
          {{"id", "downstream"}}}}};

    std::string message;
    ASSERT_TRUE(initializer.initialize(document, message)) << message;

    // The recorded item is { time, reach } — history — while the live
    // input wants { reach } — the present. TimeSliceOutput bridges them;
    // the initializer does not guess, because which level is "now" is the
    // host's decision.
    IModelComponent *opened = initializer.component("upstream");
    ASSERT_NE(opened, nullptr);
    ASSERT_EQ(opened->results().size(), 1u);
    TimeSliceOutput slice("flow", opened->results()[0]);
    EXPECT_EQ(slice.timeCount(), kSteps);
    EXPECT_EQ(slice.shape(), (std::vector<int64_t>{kReaches}));

    ASSERT_TRUE(downstream.inflow->setProvider(&slice));
    slice.addConsumer(downstream.inflow);

    downstream.prepare();
    for (int step = 0; step < kSteps; ++step)
    {
        slice.setTimeIndex(step);
        downstream.update();
    }

    ASSERT_EQ(downstream.history.size(), liveHistory.size());
    for (size_t step = 0; step < liveHistory.size(); ++step)
    {
        for (int reach = 0; reach < kReaches; ++reach)
            EXPECT_DOUBLE_EQ(downstream.history[step][reach],
                             liveHistory[step][reach])
                << "step " << step << " reach " << reach;
    }
}

// ── TimeSliceOutput ───────────────────────────────────────────────────────

TEST(TimeSliceOutputTest, PresentsOneLevelOfARecordedVariable)
{
    const auto manifestPath = recordUpstream(modesDir() / "slice");
    std::string message;

    ModelInitializer initializer(
        [](const std::string &) -> IModelComponent * { return nullptr; });
    nlohmann::json document = {
        {"components",
         {{{"id", "upstream"},
           {"execution",
            {{"mode", "open"}, {"results", manifestPath.string()}}}}}}};
    ASSERT_TRUE(initializer.initialize(document, message)) << message;

    IComponentDataItem *recorded =
        initializer.component("upstream")->results()[0];
    TimeSliceOutput slice("flow", recorded);

    EXPECT_EQ(slice.timeCount(), kSteps);
    EXPECT_EQ(slice.dataKind(), DataKind::Float64);
    EXPECT_EQ(slice.shape(), (std::vector<int64_t>{kReaches}));
    ASSERT_EQ(slice.dimensions().size(), 1u);

    std::array<double, kReaches> values{};
    const std::array<int64_t, 1> shape{kReaches};
    const std::array<int64_t, 1> start{0};
    BufferDescriptor destination;
    destination.data  = values.data();
    destination.kind  = DataKind::Float64;
    destination.rank  = 1;
    destination.shape = shape.data();
    destination.space = MemorySpace::Host;

    for (int step = 0; step < kSteps; ++step)
    {
        slice.setTimeIndex(step);
        ASSERT_TRUE(slice.getValuesInto(destination, start, shape, &message))
            << message;
        for (int reach = 0; reach < kReaches; ++reach)
            EXPECT_DOUBLE_EQ(values[reach], Upstream::value(step, reach));
    }

    // Walking forward and seeking by time.
    slice.setTimeIndex(0);
    EXPECT_TRUE(slice.advance());
    EXPECT_EQ(slice.timeIndex(), 1);
    slice.setTimeIndex(kSteps - 1);
    EXPECT_FALSE(slice.advance()) << "stops at the last level";
    EXPECT_TRUE(slice.setTime(502.4));
    EXPECT_EQ(slice.timeIndex(), 2) << "nearest recorded level";

    // Out-of-range indexes clamp rather than read garbage.
    slice.setTimeIndex(9999);
    EXPECT_EQ(slice.timeIndex(), kSteps - 1);

    // A slice is a view, not a store.
    EXPECT_FALSE(slice.setValuesFrom(destination, start, shape, &message));
    EXPECT_NE(message.find("read-only"), std::string::npos);
}

// ── Manifest → composition round trip ─────────────────────────────────────

TEST(ExecutionModeTest, CompositionForRunReopensEveryRecordedComponent)
{
    const auto manifestPath = recordUpstream(modesDir() / "roundtrip");

    RunManifest manifest;
    std::string message;
    ASSERT_TRUE(RunManifest::read(manifestPath, manifest, message)) << message;

    const CompositionSpec spec =
        compositionForRun(manifest, manifestPath.string());

    EXPECT_EQ(spec.schemaVersion, kCompositionSchemaVersion);
    EXPECT_EQ(spec.metadata.id, "upstream-run");
    ASSERT_EQ(spec.components.size(), 1u);
    EXPECT_EQ(spec.components[0].id, "upstream");
    EXPECT_EQ(spec.components[0].mode, ExecutionMode::Open);
    EXPECT_EQ(spec.components[0].resultsManifest, manifestPath.string());
    EXPECT_TRUE(spec.connections.empty());

    // The emitted document initializes an analysis composition as-is.
    ModelInitializer initializer(
        [](const std::string &) -> IModelComponent * { return nullptr; });
    ASSERT_TRUE(initializer.initialize(spec.toJson(), message)) << message;
    ASSERT_EQ(initializer.openedComponents().size(), 1u);
    EXPECT_EQ(initializer.component("upstream")->status(),
              IModelComponent::ComponentStatus::Finished);
}
