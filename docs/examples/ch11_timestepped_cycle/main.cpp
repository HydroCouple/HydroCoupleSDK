// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 11: a TimeSteppedWorkflow over a composition with a
 *         feedback loop.
 *
 * A source delivers inflow I(n) [m3/s] to a reservoir. The reservoir's
 * storage S [m3] is read by an operator, whose release R [m3/s] the
 * reservoir reads back, so reservoir and operator form a cycle:
 *
 *     source -> reservoir <-> operator
 *
 * Over a step of dt seconds the reservoir computes S += dt (I - R) and the
 * operator computes R = c max(S - target, 0). The workflow derives the
 * schedule from the links, collapses the cycle into one group, and runs the
 * group's members in insertion order: the reservoir (added first) reads the
 * release the operator computed in the previous step. The program checks
 * the run against the same recurrence written as a plain loop, then runs a
 * second composition with two sweeps per cyclic group to show that every
 * sweep is a full update().
 */

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/component/timesteppedworkflow.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/signal.h"
#include "hydrocouplesdk/core/unit.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using WorkflowStatus = IWorkflowComponent::WorkflowStatus;

namespace {

int g_failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok)
    {
        std::printf("CHECK FAILED: %s\n", what.c_str());
        ++g_failures;
    }
}

constexpr int    kSteps   = 6;
constexpr double kDt      = 600.0;   // s
constexpr double kTarget  = 5000.0;  // m3
constexpr double kGain    = 2.0e-3;  // 1/s
constexpr std::array<double, kSteps> kInflow{4.0, 9.0, 12.0, 8.0, 5.0, 3.0}; // m3/s

//! Units and quantities shared by the three components (borrowed by their items).
struct Quantities
{
    std::unique_ptr<Unit> cms{Unit::flowInCMS()};
    std::unique_ptr<Unit> m3{Unit::volumeInCubicMeters()};
    Quantity flow{"Flow", typeid(double), cms.get()};
    Quantity volume{"Storage", typeid(double), m3.get()};
    Dimension scalar{"scalar", "Single value"};

    Quantities()
    {
        flow.setValueKind(ValueKind::Intensive);
        volume.setValueKind(ValueKind::Extensive);
    }
};

//! [stepped]
/*!
 * Lifecycle boilerplate shared by the three components: no arguments,
 * Valid at once, Done after kSteps updates. Subclasses create their items
 * and implement step().
 */
class SteppedComponent : public AbstractModelComponent
{
public:
    SteppedComponent(std::string_view id, Quantities &q)
        : AbstractModelComponent(id), m_q(q) {}

    std::vector<std::string> validate() override
    {
        setStatus(ComponentStatus::Validating);
        setStatus(ComponentStatus::Valid);
        return {};
    }

    void prepare() override
    {
        setStatus(ComponentStatus::Preparing);
        m_step = 0;
        setStatus(ComponentStatus::Updated);
    }

    void update(const std::vector<IOutput *> & = {}) override
    {
        setStatus(ComponentStatus::Updating);
        applyInputValues(); // the workflow does not move input values
        step(m_step);
        ++m_step;
        ++updates;
        setStatus(m_step >= kSteps ? ComponentStatus::Done : ComponentStatus::Updated);
    }

    void finish() override
    {
        setStatus(ComponentStatus::Finishing);
        setStatus(ComponentStatus::Finished);
    }

    int updates = 0;

protected:
    virtual void step(int n) = 0;

    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void initializeFailureCleanUp() override {}

    Quantities &m_q;
    int m_step = 0;
};
//! [stepped]

class Source : public SteppedComponent
{
public:
    using SteppedComponent::SteppedComponent;
    std::unique_ptr<Output1DDouble> inflow;

protected:
    void createInputs() override {}
    void createOutputs() override
    {
        inflow = std::make_unique<Output1DDouble>("inflow", &m_q.scalar, 1, &m_q.flow, this);
        addOutput(inflow.get());
    }
    void step(int n) override { (*inflow)[0] = kInflow[static_cast<size_t>(n)]; }
};

class Reservoir : public SteppedComponent
{
public:
    using SteppedComponent::SteppedComponent;
    std::unique_ptr<Input1DDouble>  inflow, release;
    std::unique_ptr<Output1DDouble> storage;
    std::vector<double> history;

protected:
    void createInputs() override
    {
        inflow  = std::make_unique<Input1DDouble>("inflow", &m_q.scalar, 1, &m_q.flow, this);
        release = std::make_unique<Input1DDouble>("release", &m_q.scalar, 1, &m_q.flow, this);
        addInput(inflow.get());
        addInput(release.get());
    }
    void createOutputs() override
    {
        storage = std::make_unique<Output1DDouble>("storage", &m_q.scalar, 1, &m_q.volume, this);
        addOutput(storage.get());
    }
    void step(int) override
    {
        (*storage)[0] += kDt * ((*inflow)[0] - (*release)[0]);
        history.push_back((*storage)[0]);
    }
};

class Operator : public SteppedComponent
{
public:
    using SteppedComponent::SteppedComponent;
    std::unique_ptr<Input1DDouble>  storage;
    std::unique_ptr<Output1DDouble> release;
    std::vector<double> history;

protected:
    void createInputs() override
    {
        storage = std::make_unique<Input1DDouble>("storage", &m_q.scalar, 1, &m_q.volume, this);
        addInput(storage.get());
    }
    void createOutputs() override
    {
        release = std::make_unique<Output1DDouble>("release", &m_q.scalar, 1, &m_q.flow, this);
        addOutput(release.get());
    }
    void step(int) override
    {
        (*release)[0] = kGain * std::max((*storage)[0] - kTarget, 0.0);
        history.push_back((*release)[0]);
    }
};

//! One composition: the three components and the workflow over them.
struct Composition
{
    Quantities quantities;
    Source     source{"source", quantities};
    Reservoir  reservoir{"reservoir", quantities};
    Operator   control{"operator", quantities};
    // Declared last, so it is destroyed first: its destructor still reads
    // the components it manages.
    TimeSteppedWorkflow workflow{"stepped"};
};

//! [compose]
void compose(Composition &c)
{
    // Insertion order decides the order inside a cyclic group.
    for (IModelComponent *component :
         std::vector<IModelComponent *>{&c.source, &c.reservoir, &c.control})
    {
        std::string message;
        check(c.workflow.addModelComponent(component, nullptr, &message), message);
    }
    c.workflow.initialize(); // initializes the components, which creates their items

    // Wiring needs the items, so it happens after initialize().
    c.source.inflow->addConsumer(c.reservoir.inflow.get());
    c.reservoir.storage->addConsumer(c.control.storage.get());
    c.control.release->addConsumer(c.reservoir.release.get());
}
//! [compose]

const char *name(WorkflowStatus status)
{
    static std::string text;
    text = AbstractWorkflowComponent::statusToString(status);
    return text.c_str();
}

} // namespace

int main()
{
    std::printf("time-lagged cycle (1 sweep)\n");
    Composition c;

    //! [events]
    using Event = const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &;
    std::vector<WorkflowStatus> seen;
    auto slot = std::make_shared<FunctionSlot<Event>>(
        [&seen](const ISignal<Event> &, Event event) { seen.push_back(event->status()); });
    c.workflow.connect(slot);
    //! [events]

    compose(c);
    //! [run]
    const std::vector<std::string> problems = c.workflow.validate();
    check(problems.empty() && c.workflow.status() == WorkflowStatus::Validated, "validated");
    check(c.workflow.errors().empty(), "no link findings for matching declarations");

    c.workflow.prepare(); // builds the schedule
    check(c.workflow.status() == WorkflowStatus::Prepared, "prepared");
    for (const auto &group : c.workflow.schedule())
    {
        std::printf("  group:");
        for (IModelComponent *member : group)
            std::printf(" %s", member->id().c_str());
        std::printf("%s\n", group.size() > 1 ? "  (feedback loop)" : "");
    }

    int steps = 0;
    while (c.workflow.status() != WorkflowStatus::Done &&
           c.workflow.status() != WorkflowStatus::Failed)
    {
        c.workflow.update();
        ++steps;
        std::printf("  step %d: inflow %5.1f m3/s, storage %8.1f m3, release %6.3f m3/s\n",
                    steps, (*c.source.inflow)[0], c.reservoir.history.back(),
                    c.control.history.back());
    }
    c.workflow.finish();
    //! [run]

    const auto &schedule = c.workflow.schedule();
    check(schedule.size() == 2 && schedule[0].size() == 1 && schedule[0][0] == &c.source &&
              schedule[1].size() == 2 && schedule[1][0] == &c.reservoir,
          "source first, then the reservoir-operator loop in insertion order");
    check(steps == kSteps && c.reservoir.status() == IModelComponent::ComponentStatus::Finished,
          "six steps, then Finished");

    //! [reference]
    // The same recurrence as a plain loop: the reservoir uses the release
    // of the previous step, the operator the storage of this step.
    double storage = 0.0, release = 0.0;
    for (int n = 0; n < kSteps; ++n)
    {
        storage += kDt * (kInflow[static_cast<size_t>(n)] - release);
        release = kGain * std::max(storage - kTarget, 0.0);
        check(std::abs(storage - c.reservoir.history[static_cast<size_t>(n)]) < 1e-9 &&
                  std::abs(release - c.control.history[static_cast<size_t>(n)]) < 1e-12,
              "step " + std::to_string(n + 1) + " matches the reference loop");
    }
    //! [reference]

    std::printf("  workflow statuses:");
    for (WorkflowStatus status : seen)
        std::printf(" %s", name(status));
    std::printf("\n");
    check(seen.front() == WorkflowStatus::Initializing && seen.back() == WorkflowStatus::Finished,
          "status events from Initializing to Finished");

    // ── Two sweeps per cyclic group ──────────────────────────────────────
    std::printf("\ntwo sweeps per cyclic group\n");
    //! [sweeps]
    Composition twice;
    compose(twice);
    twice.workflow.setIterationsPerGroup(2);
    (void)twice.workflow.validate();
    twice.workflow.prepare();
    twice.workflow.update();
    std::printf("  after one step: source %d update, reservoir %d updates, operator %d updates\n",
                twice.source.updates, twice.reservoir.updates, twice.control.updates);
    //! [sweeps]
    check(twice.source.updates == 1 && twice.reservoir.updates == 2 && twice.control.updates == 2,
          "a cyclic group runs every sweep as a full update()");
    int more = 1;
    while (twice.workflow.status() == WorkflowStatus::Updated)
    {
        twice.workflow.update();
        ++more;
    }
    std::printf("  the reservoir reached Done after %d workflow steps; the run took %d\n",
                static_cast<int>(twice.reservoir.history.size()) / 2, more);
    check(more == kSteps, "the source still sets the length of the run");
    check(twice.reservoir.history.size() == static_cast<size_t>(kSteps),
          "the reservoir's own clock ran out after kSteps updates");
    twice.workflow.finish();

    std::printf("\n%s\n", g_failures == 0 ? "all checks passed" : "CHECKS FAILED");
    return g_failures == 0 ? 0 : 1;
}
