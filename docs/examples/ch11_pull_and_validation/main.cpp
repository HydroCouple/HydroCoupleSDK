// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 11: the PullDrivenWorkflow, composition validation with
 *         its link findings, and the workflow's control and failure rules.
 *
 * Every component here is a counter: on update n it reads its input, adds
 * n times its increment, publishes the sum and logs its id, so the update
 * order and the values show what each workflow did.
 */

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/component/pulldrivenworkflow.h"
#include "hydrocouplesdk/component/timesteppedworkflow.h"
#include "hydrocouplesdk/core/diagnosticcodes.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/unit.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using WorkflowStatus  = IWorkflowComponent::WorkflowStatus;
using ComponentStatus = IModelComponent::ComponentStatus;

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

//! Units, quantities and the axis the counters' items borrow.
struct Quantities
{
    std::unique_ptr<Unit> cms{Unit::flowInCMS()};
    std::unique_ptr<Unit> cfs{Unit::flowInCFS()};
    Quantity flow{"Flow", typeid(double), cms.get()};
    Quantity flowUS{"Flow", typeid(double), cfs.get()};
    Dimension scalar{"scalar", "Single value"};

    Quantities()
    {
        flow.setValueKind(ValueKind::Intensive);
        flowUS.setValueKind(ValueKind::Intensive);
    }
};

/*!
 * out = in + n increment on update n; Done after maxSteps updates. The
 * output can be declared in ft3/s to make a link that fails validation.
 */
class Counter : public AbstractModelComponent
{
public:
    Counter(std::string_view id, Quantities &q, std::vector<std::string> &log,
            double increment, int maxSteps, bool usUnits = false)
        : AbstractModelComponent(id), m_q(q), m_log(log), m_increment(increment),
          m_maxSteps(maxSteps), m_usUnits(usUnits) {}

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
        applyInputValues();
        ++m_steps;
        (*out)[0] = (*in)[0] + m_increment * m_steps;
        m_log.push_back(id());
        setStatus(m_steps >= m_maxSteps ? ComponentStatus::Done : ComponentStatus::Updated);
    }
    void finish() override
    {
        setStatus(ComponentStatus::Finishing);
        setStatus(ComponentStatus::Finished);
    }

    std::unique_ptr<Input1DDouble>  in;
    std::unique_ptr<Output1DDouble> out;

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        in = std::make_unique<Input1DDouble>("in", &m_q.scalar, 1, &m_q.flow, this);
        addInput(in.get());
    }
    void createOutputs() override
    {
        out = std::make_unique<Output1DDouble>("out", &m_q.scalar, 1,
                                               m_usUnits ? &m_q.flowUS : &m_q.flow, this);
        addOutput(out.get());
    }
    void initializeFailureCleanUp() override {}

private:
    Quantities               &m_q;
    std::vector<std::string> &m_log;
    double                    m_increment;
    int                       m_maxSteps;
    bool                      m_usUnits;
    int                       m_steps = 0;
};

std::string joined(const std::vector<std::string> &items)
{
    std::string text;
    for (const std::string &item : items)
        text += (text.empty() ? "" : " ") + item;
    return text;
}

void addAll(AbstractWorkflowComponent &workflow, const std::vector<IModelComponent *> &components)
{
    for (IModelComponent *component : components)
    {
        std::string message;
        check(workflow.addModelComponent(component, nullptr, &message), message);
    }
}

} // namespace

int main()
{
    Quantities q;

    // ── Pull-driven: a chain serviced from a trigger ─────────────────────
    std::printf("pull-driven chain\n");
    std::vector<std::string> log;
    {
        //! [pull]
        Counter a("a", q, log, 1.0, 3), b("b", q, log, 10.0, 3);
        // The trigger is a probe the host owns; it belongs to no component.
        Input1DDouble trigger("trigger", &q.scalar, 1, &q.flow, nullptr);
        PullDrivenWorkflow workflow("pull");
        addAll(workflow, {&b, &a}); // insertion order does not matter here
        workflow.initialize();
        a.out->addConsumer(b.in.get());
        b.out->addConsumer(&trigger);

        std::vector<std::string> problems = workflow.validate();
        std::printf("  without a trigger: %s\n", problems.empty() ? "valid" : problems[0].c_str());
        check(workflow.status() == WorkflowStatus::Invalid, "Invalid without a trigger");

        workflow.setTrigger(&trigger);
        problems = workflow.validate(); // Invalid -> Validating is allowed
        check(problems.empty() && workflow.status() == WorkflowStatus::Validated,
              "valid once the trigger is declared");
        workflow.prepare();
        while (workflow.status() == WorkflowStatus::Prepared ||
               workflow.status() == WorkflowStatus::Updated)
        {
            workflow.update();
            std::printf("  pulled %.1f, updates so far: %s\n", trigger[0], joined(log).c_str());
        }
        check(workflow.status() == WorkflowStatus::Done, "Done when a and b are Done");
        workflow.finish();
        //! [pull]
        check(log == std::vector<std::string>{"a", "b", "a", "b", "a", "b"},
              "upstream first, each component once per pull");
        check(trigger[0] == 33.0, "the trigger read b's last value");
    }

    // ── Validation: a link whose units contradict ────────────────────────
    std::printf("\nlink validation\n");
    {
        //! [validate]
        std::vector<std::string> unused;
        Counter us("us-gauge", q, unused, 1.0, 1, /*usUnits*/ true);
        Counter si("si-gauge", q, unused, 1.0, 1);
        Counter channel("channel", q, unused, 1.0, 1);
        TimeSteppedWorkflow workflow("checked");
        addAll(workflow, {&us, &si, &channel});
        workflow.initialize();
        us.out->addConsumer(channel.in.get()); // ft3/s into an m3/s input

        const std::vector<std::string> problems = workflow.validate();
        std::printf("  status %s, %zu problem(s)\n",
                    AbstractWorkflowComponent::statusToString(workflow.status()).c_str(),
                    problems.size());
        for (const ErrorEntry &entry : workflow.errors(true))
            std::printf("  error queue: code %d: %s\n", entry.code, entry.message.c_str());
        check(workflow.status() == WorkflowStatus::Invalid && problems.size() == 1,
              "one link error makes the composition Invalid");

        // Fix the composition and validate again; no re-initialization needed.
        check(us.out->removeConsumer(channel.in.get()), "unwire the ft3/s output");
        si.out->addConsumer(channel.in.get());
        check(workflow.validate().empty() && workflow.status() == WorkflowStatus::Validated,
              "valid after rewiring");
        std::printf("  after rewiring: %s\n",
                    AbstractWorkflowComponent::statusToString(workflow.status()).c_str());
        //! [validate]
        workflow.finish();
    }

    // ── Unconnected components and the schedule ──────────────────────────
    std::printf("\nschedule without links\n");
    {
        //! [order]
        std::vector<std::string> unused;
        Counter x("x", q, unused, 0.0, 1), y("y", q, unused, 0.0, 1), z("z", q, unused, 0.0, 1);
        TimeSteppedWorkflow workflow("unlinked");
        addAll(workflow, {&x, &y, &z});
        workflow.initialize();
        (void)workflow.validate();
        workflow.prepare();
        std::string order;
        for (const auto &group : workflow.schedule())
            order += (order.empty() ? "" : " ") + group[0]->id();
        std::printf("  added x y z, scheduled %s\n", order.c_str());
        //! [order]
        check(order == "z y x", "groups without a path between them run in reverse insertion order");
        workflow.finish();
    }

    // ── Pause, resume, stop, and a step too many ─────────────────────────
    std::printf("\ncontrol\n");
    {
        //! [control]
        std::vector<std::string> steps;
        Counter tick("tick", q, steps, 1.0, 100);
        TimeSteppedWorkflow workflow("controlled");
        addAll(workflow, {&tick});
        workflow.initialize();
        (void)workflow.validate();
        workflow.prepare();

        workflow.requestPause(); // any thread; honored after the step in flight
        workflow.update();
        std::printf("  after a pause request: %s\n",
                    AbstractWorkflowComponent::statusToString(workflow.status()).c_str());
        workflow.resume();
        workflow.requestStop();
        workflow.update();
        std::printf("  after a stop request: %s, tick updated %zu times\n",
                    AbstractWorkflowComponent::statusToString(workflow.status()).c_str(),
                    steps.size());
        check(workflow.status() == WorkflowStatus::Done && steps.size() == 2,
              "the stop ends the run as Done after the step in flight");

        workflow.update(); // Done is final for update()
        std::printf("  update() after Done: %s\n",
                    AbstractWorkflowComponent::statusToString(workflow.status()).c_str());
        const std::vector<ErrorEntry> errors = workflow.errors();
        check(workflow.status() == WorkflowStatus::Failed && !errors.empty() &&
                  errors.back().severity == ErrorEntry::Severity::Fatal,
              "update() after Done fails the workflow");
        if (!errors.empty())
            std::printf("  error queue: %s\n", errors.back().message.c_str());
        workflow.finish(); // still legal from Failed
        check(tick.status() == ComponentStatus::Finished, "finish() reached the component");
        //! [control]
    }

    std::printf("\n%s\n", g_failures == 0 ? "all checks passed" : "CHECKS FAILED");
    return g_failures == 0 ? 0 : 1;
}
