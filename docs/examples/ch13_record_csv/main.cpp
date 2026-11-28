// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Chapter 13 example: record a small run with the CSV writer and reopen it.
// A gauge component produces two outputs for six hourly steps; an IOThread
// carries one Snapshot per step to a CSVWriter; a RunRecorder writes the run
// manifest. The run is then reopened with no model code (ResultsModelComponent
// and an "open" composition), moved to another directory and reopened again,
// and reopened once more with its CSV missing. Every value served back is
// compared with the live run. Returns nonzero on any difference.

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/component/resultsmodelcomponent.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/unit.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/io/csvwriter.h"
#include "hydrocouplesdk/io/iothread.h"
#include "hydrocouplesdk/io/modelinitializer.h"
#include "hydrocouplesdk/io/runmanifest.h"
#include "hydrocouplesdk/io/snapshot.h"

#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::IO;

namespace {

int failures = 0;

void check(bool condition, const std::string &what)
{
    if (!condition)
    {
        std::cout << "CHECK FAILED: " << what << '\n';
        ++failures;
    }
}

constexpr int    kCells = 3;
constexpr int    kSteps = 6;
constexpr double kStart = 2461370.5; // Julian day of the first record

//! A component with two outputs whose values are awkward on purpose:
//! negative, fractional and large, so "identical" means something.
class Gauge : public AbstractModelComponent
{
public:
    explicit Gauge(std::string_view id) : AbstractModelComponent(id, "Gauge") {}

    std::vector<std::string> validate() override
    {
        setStatus(ComponentStatus::Validating);
        setStatus(ComponentStatus::Valid);
        return {};
    }
    void prepare() override
    {
        setStatus(ComponentStatus::Preparing);
        setStatus(ComponentStatus::Updated);
    }
    void update(const std::vector<IOutput *> & = {}) override
    {
        setStatus(ComponentStatus::Updating);
        for (int cell = 0; cell < kCells; ++cell)
            (*flow)[cell] = (m_step - 2.5) * 1.0e3 + cell * 0.125 - 1.0 / 3.0;
        (*level)[0] = 100.0 + m_step / 7.0;
        ++m_step;
        setStatus(m_step >= kSteps ? ComponentStatus::Done
                                   : ComponentStatus::Updated);
    }
    void finish() override
    {
        setStatus(ComponentStatus::Finishing);
        setStatus(ComponentStatus::Finished);
    }

    Output1DDouble *flow  = nullptr;
    Output1DDouble *level = nullptr;

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
        m_flow  = std::make_unique<Output1DDouble>("flow", &m_cells, kCells,
                                                   m_flowQuantity.get(), this);
        m_level = std::make_unique<Output1DDouble>("level", &m_gauge, 1,
                                                   m_levelQuantity.get(), this);
        flow  = m_flow.get();
        level = m_level.get();
        addOutput(flow);
        addOutput(level);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension                       m_cells{"cell", "Cell"};
    Dimension                       m_gauge{"gauge", "Gauge"};
    std::unique_ptr<Unit>           m_cfs{Unit::flowInCFS()};
    std::unique_ptr<Unit>           m_meters{Unit::lengthInMeters()};
    std::unique_ptr<Quantity>       m_flowQuantity{
        new Quantity("Flow", typeid(double), m_cfs.get())};
    std::unique_ptr<Quantity>       m_levelQuantity{
        new Quantity("Water level", typeid(double), m_meters.get())};
    std::unique_ptr<Output1DDouble> m_flow;
    std::unique_ptr<Output1DDouble> m_level;
    int                             m_step = 0;
};

//! Reads a whole recorded item into a flat vector.
bool readAll(IComponentDataItem &item, std::vector<double> &values,
             std::string &message)
{
    const std::vector<int64_t> shape = item.shape();
    int64_t count = 1;
    for (int64_t extent : shape)
        count *= extent;
    values.assign(static_cast<size_t>(count), 0.0);

    BufferDescriptor into;
    into.data  = values.data();
    into.kind  = DataKind::Float64;
    into.rank  = 1;
    into.shape = &count;
    into.space = MemorySpace::Host;
    const std::vector<int64_t> start(shape.size(), 0);
    return item.getValuesInto(into, start, shape, &message);
}

//! Reopens a manifest and compares every served value with the live run.
bool reopenAndCompare(const std::filesystem::path &manifestPath,
                      const std::vector<double> &liveFlow,
                      const std::vector<double> &liveLevel,
                      std::string &message)
{
    std::unique_ptr<ResultsModelComponent> reopened =
        ResultsModelComponent::open(manifestPath, "gauge", message);
    if (!reopened)
        return false;
    reopened->initialize();
    if (reopened->status() != IModelComponent::ComponentStatus::Finished)
    {
        message = reopened->errors().empty()
                      ? "not finished"
                      : reopened->errors().back().message;
        return false;
    }
    bool identical = true;
    for (IComponentDataItem *item : reopened->results())
    {
        std::vector<double> stored;
        if (!readAll(*item, stored, message))
            return false;
        const std::vector<double> &live =
            item->id() == "flow" ? liveFlow : liveLevel;
        identical = identical && stored.size() == live.size() &&
                    std::memcmp(stored.data(), live.data(),
                                live.size() * sizeof(double)) == 0;
    }
    return identical;
}

} // namespace

int main()
{
    std::filesystem::remove_all("run");
    std::filesystem::remove_all("moved");
    std::filesystem::remove_all("incomplete");

    Gauge gauge("gauge");
    gauge.initialize();
    (void)gauge.validate();
    gauge.prepare();

    //! [record]
    auto csv = std::make_shared<CSVWriter>("run/results.csv");

    IOThread io(/*capacity=*/8);
    io.addWriter(csv);

    RunRecorder recorder("chapter-13-run");
    recorder.setCaption("Chapter 13 recording example");
    recorder.addWriter(csv);       // its catalog goes into the manifest
    recorder.addComponent(&gauge); // its status and outputs' metadata too

    if (!io.start())
    {
        std::cout << "writer failed to start: " << io.errorMessage() << '\n';
        return 1;
    }

    std::vector<double> liveFlow, liveLevel; // what the run produced
    std::string         message;
    for (int step = 0; step < kSteps; ++step)
    {
        gauge.update();
        Snapshot snapshot(kStart + step / 24.0); // hourly, in Julian days
        if (!snapshot.capture(*gauge.flow, message) ||
            !snapshot.capture(*gauge.level, message))
        {
            std::cout << message << '\n';
            return 1;
        }
        for (int cell = 0; cell < kCells; ++cell)
            liveFlow.push_back((*gauge.flow)[cell]);
        liveLevel.push_back((*gauge.level)[0]);
        if (!io.enqueue(std::move(snapshot)))
            break; // the IO thread is not running
    }
    gauge.finish();

    io.finish(); // drains the queue and finalizes the writer
    if (io.errorCode() != 0)
    {
        std::cout << "writer error " << io.errorCode() << ": "
                  << io.errorMessage() << '\n';
        return 1;
    }
    // Catalogs are complete only now, after finalize().
    if (!recorder.finishToFile("run/run.json", message, RunStatus::Completed))
    {
        std::cout << "manifest: " << message << '\n';
        return 1;
    }
    //! [record]

    //! [manifest]
    RunManifest manifest;
    if (!RunManifest::read("run/run.json", manifest, message))
    {
        std::cout << message << '\n';
        return 1;
    }
    std::cout << "run " << manifest.id << " (" << manifest.caption << "): "
              << runStatusName(manifest.status) << ", sdk "
              << manifest.sdkVersion << '\n';
    for (const ComponentOutcome &outcome : manifest.components)
        std::cout << "  component " << outcome.id << " ended "
                  << outcome.finalStatus << '\n';
    for (const ResultEntry &entry : manifest.results)
    {
        std::cout << "  result " << entry.componentId << "/" << entry.itemId
                  << " in " << entry.artifact << " (" << entry.format
                  << "), shape {";
        for (size_t d = 0; d < entry.shape.size(); ++d)
            std::cout << (d ? ", " : "") << entry.shape[d];
        std::cout << "}, dimensions {";
        for (size_t d = 0; d < entry.dimensions.size(); ++d)
            std::cout << (d ? ", " : "") << entry.dimensions[d];
        std::cout << "}, units " << entry.units << ", time "
                  << std::setprecision(10) << entry.time->start << " to "
                  << entry.time->end << " " << entry.time->units << '\n';
    }
    //! [manifest]
    std::cout << "the flow entry as written in run.json:\n"
              << manifest.entry("gauge", "flow")->toJson().dump(2) << '\n';

    check(manifest.results.size() == 2, "two catalog entries");
    const ResultEntry *flowEntry = manifest.entry("gauge", "flow");
    check(flowEntry && flowEntry->shape == std::vector<int64_t>{kSteps, kCells},
          "flow is { time, cell }");
    check(flowEntry && flowEntry->dimensions ==
                           std::vector<std::string>{"time", "cell"},
          "dimension names come from the output");
    check(manifest.components.size() == 1 &&
              manifest.components[0].finalStatus == "Finished",
          "the component outcome");

    //! [reopen]
    std::unique_ptr<ResultsModelComponent> reopened =
        ResultsModelComponent::open("run/run.json", "gauge", message);
    if (!reopened)
    {
        std::cout << message << '\n';
        return 1;
    }
    reopened->initialize(); // opens the CSV; ends in Finished

    std::vector<double> storedFlow;
    for (IComponentDataItem *item : reopened->results())
    {
        std::vector<double> times;
        std::string reason;
        (void)reopened->times(item->id(), times, reason);
        std::cout << "reopened " << item->id() << ": " << times.size()
                  << " times from " << std::setprecision(12) << times.front()
                  << " to " << times.back() << '\n';
        if (item->id() == "flow" && !readAll(*item, storedFlow, message))
        {
            std::cout << message << '\n';
            return 1;
        }
    }
    //! [reopen]
    check(reopened->status() == IModelComponent::ComponentStatus::Finished,
          "the reopened component is Finished");
    check(storedFlow.size() == liveFlow.size() &&
              std::memcmp(storedFlow.data(), liveFlow.data(),
                          liveFlow.size() * sizeof(double)) == 0,
          "the CSV serves back the exact doubles of the live run");
    std::cout << "flow at the last step: " << std::setprecision(17)
              << storedFlow[storedFlow.size() - 3] << ", "
              << storedFlow[storedFlow.size() - 2] << ", "
              << storedFlow.back() << '\n';

    //! [open_composition]
    // The analysis route: a composition of "open" blocks, and no resolver
    // that knows any model component.
    const CompositionSpec analysis = compositionForRun(manifest, "run/run.json");
    ModelInitializer initializer(
        [](const std::string &) -> IModelComponent * { return nullptr; });
    if (!initializer.initialize(analysis.toJson(), message))
    {
        std::cout << "cannot reopen: " << message << '\n';
        return 1;
    }
    IModelComponent *substitute = initializer.component("gauge");
    std::cout << "open composition: component " << substitute->id()
              << " serves " << substitute->outputs().size() << " outputs\n";
    //! [open_composition]
    check(initializer.openedComponents().size() == 1 &&
              substitute->outputs().size() == 2,
          "the initializer substitutes one ResultsModelComponent");

    //! [move]
    // A run is the manifest plus the artifacts beside it: move them together.
    std::filesystem::create_directories("moved");
    std::filesystem::copy("run", "moved/run",
                          std::filesystem::copy_options::recursive);
    const bool movedOk = reopenAndCompare("moved/run/run.json", liveFlow,
                                          liveLevel, message);
    std::cout << "moved run reopens " << (movedOk ? "identically" : "badly")
              << '\n';

    // The manifest alone is not a run.
    std::filesystem::create_directories("incomplete");
    std::filesystem::copy_file("run/run.json", "incomplete/run.json");
    const bool incompleteOk = reopenAndCompare("incomplete/run.json",
                                               liveFlow, liveLevel, message);
    std::cout << "manifest without its CSV: " << message << '\n';
    //! [move]
    check(movedOk, "a moved run reopens with identical values");
    check(!incompleteOk &&
              message.find("cannot open result artifact") != std::string::npos,
          "a missing artifact is a named failure");

    std::cout << (failures == 0 ? "all checks passed" : "checks failed")
              << '\n';
    return failures == 0 ? 0 : 1;
}
