// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the workflow orchestration layer (plan §5.5): the
// AbstractWorkflowComponent status machine, role registry, diagnostics, and
// cooperative pause/stop; PullDrivenWorkflow's request-reply chain from a
// declared trigger; and TimeSteppedWorkflow's DAG-of-SCCs schedule with
// feedback-loop iteration groups.

#include <gtest/gtest.h>

#include "hydrocouplesdk/component/abstractworkflowcomponent.h"
#include "hydrocouplesdk/component/pulldrivenworkflow.h"
#include "hydrocouplesdk/component/timesteppedworkflow.h"
#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;

namespace {

using WStatus = IWorkflowComponent::WorkflowStatus;
using CStatus = IModelComponent::ComponentStatus;

/*!
 * Chain component: one "in" input, one "out" output (length 1).
 * update() applies inputs, adds +1, and records itself in a shared update
 * log so tests can assert execution order. Reports Done after maxSteps.
 */
class ChainComponent : public AbstractModelComponent
{
public:
    ChainComponent(std::string_view id, std::vector<std::string> *updateLog,
                   int maxSteps = 3)
        : AbstractModelComponent(id),
          m_updateLog(updateLog),
          m_maxSteps(maxSteps)
    {}

    std::vector<std::string> validate() override
    {
        setStatus(CStatus::Validating);
        setStatus(CStatus::Valid);
        return {};
    }

    void prepare() override
    {
        setStatus(CStatus::Preparing);
        setStatus(CStatus::Updated, "prepared");
    }

    void update(const std::vector<IOutput *> & = {}) override
    {
        setStatus(CStatus::Updating);
        applyInputValues();
        (*out)[0] = (*in)[0] + 1.0;
        ++updateCount;
        if (m_updateLog)
            m_updateLog->push_back(id());
        setStatus(updateCount >= m_maxSteps ? CStatus::Done : CStatus::Updated,
                  "stepped");
    }

    void finish() override
    {
        setStatus(CStatus::Finishing);
        setStatus(CStatus::Finished);
    }

    Input1DDouble  *in  = nullptr;
    Output1DDouble *out = nullptr;
    int             updateCount = 0;

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        in = new Input1DDouble("in", &m_dim, 1, m_quantity.get(), this);
        addInput(in);
    }
    void createOutputs() override
    {
        m_quantity.reset(Quantity::unitLess("ChainQuantity"));
        out = new Output1DDouble("out", &m_dim, 1, m_quantity.get(), this);
        addOutput(out);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension m_dim{"i", "Index"};
    std::unique_ptr<Quantity> m_quantity;
    std::vector<std::string> *m_updateLog;
    int                       m_maxSteps;
};

/*!
 * Component whose update() fails on the given step.
 */
class FailingComponent : public ChainComponent
{
public:
    FailingComponent(std::string_view id, std::vector<std::string> *log,
                     int failOnStep)
        : ChainComponent(id, log, 1000), m_failOnStep(failOnStep)
    {}

    void update(const std::vector<IOutput *> &required = {}) override
    {
        if (updateCount + 1 >= m_failOnStep)
        {
            setStatus(CStatus::Failed, "deliberate failure");
            return;
        }
        ChainComponent::update(required);
    }

private:
    int m_failOnStep;
};

/*!
 * Records every workflow status transition fired through the signal.
 */
class RecordingSlot final
    : public ISlot<const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &>
{
public:
    void operator()(
        const ISignal<
            const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &> &,
        const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &args)
        override
    {
        statuses.push_back(args->status());
        messages.push_back(args->message());
    }

    std::vector<WStatus>     statuses;
    std::vector<std::string> messages;
};

/*!
 * Workflow with one required and one optional role, exposing the registry
 * for testing the base class in isolation. performStep() is a no-op.
 */
class RoleWorkflow : public AbstractWorkflowComponent
{
public:
    explicit RoleWorkflow(std::string_view id)
        : AbstractWorkflowComponent(id)
    {
        addModelComponentLabel(
            std::make_unique<Identity>("driver", "Driving model"), true);
        addModelComponentLabel(
            std::make_unique<Identity>("observer", "Optional observer"), false);
    }

protected:
    bool performStep(std::string &message) override
    {
        message.clear();
        return true;
    }
};

/*!
 * Connects provider.out → consumer.in (both directions of the link).
 */
void link(ChainComponent &provider, ChainComponent &consumer)
{
    ASSERT_TRUE(consumer.in->setProvider(provider.out));
    provider.out->addConsumer(consumer.in);
}

} // namespace

// ── AbstractWorkflowComponent ─────────────────────────────────────────────

TEST(WorkflowBaseTest, RoleRegistryAndLabels)
{
    RoleWorkflow workflow("wf");
    const auto labels = workflow.modelComponentLabels();
    ASSERT_EQ(labels.size(), 2u);
    EXPECT_EQ(labels[0]->id(), "driver");
    EXPECT_TRUE(workflow.isRequiredModelComponent(labels[0]));
    EXPECT_FALSE(workflow.isRequiredModelComponent(labels[1]));
}

TEST(WorkflowBaseTest, AddModelComponentDiagnostics)
{
    RoleWorkflow workflow("wf");
    std::vector<std::string> log;
    ChainComponent a("a", &log), b("b", &log);
    Identity driver("driver"), unknown("nope");
    std::string message;

    EXPECT_FALSE(workflow.addModelComponent(nullptr, nullptr, &message));
    EXPECT_FALSE(message.empty());

    EXPECT_TRUE(workflow.addModelComponent(&a, &driver, &message)) << message;
    EXPECT_EQ(a.workflow(), &workflow);

    EXPECT_FALSE(workflow.addModelComponent(&a, nullptr, &message));
    EXPECT_NE(message.find("already managed"), std::string::npos);

    EXPECT_FALSE(workflow.addModelComponent(&b, &unknown, &message));
    EXPECT_NE(message.find("unknown role"), std::string::npos);

    EXPECT_FALSE(workflow.addModelComponent(&b, &driver, &message));
    EXPECT_NE(message.find("already filled"), std::string::npos);

    EXPECT_TRUE(workflow.removeModelComponent(&a));
    EXPECT_EQ(a.workflow(), nullptr);
    EXPECT_EQ(workflow.componentForRole("driver"), nullptr);
}

TEST(WorkflowBaseTest, ValidateFailsWhileRequiredRoleEmpty)
{
    RoleWorkflow workflow("wf");
    workflow.initialize();
    ASSERT_EQ(workflow.status(), WStatus::Initialized);

    const auto problems = workflow.validate();
    ASSERT_FALSE(problems.empty());
    EXPECT_NE(problems[0].find("driver"), std::string::npos);
    EXPECT_EQ(workflow.status(), WStatus::Failed);

    const auto errors = workflow.errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors.back().severity, ErrorEntry::Severity::Fatal);
}

TEST(WorkflowBaseTest, LifecycleEnforcesPhaseOrder)
{
    RoleWorkflow workflow("wf");
    workflow.update(); // Created → invalid
    EXPECT_EQ(workflow.status(), WStatus::Failed);
}

// ── PullDrivenWorkflow ────────────────────────────────────────────────────

class PullWorkflowTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        a = std::make_unique<ChainComponent>("a", &log);
        b = std::make_unique<ChainComponent>("b", &log);
        c = std::make_unique<ChainComponent>("c", &log);

        workflow = std::make_unique<PullDrivenWorkflow>("pull");
        std::string message;
        ASSERT_TRUE(workflow->addModelComponent(a.get(), nullptr, &message));
        ASSERT_TRUE(workflow->addModelComponent(b.get(), nullptr, &message));
        ASSERT_TRUE(workflow->addModelComponent(c.get(), nullptr, &message));

        workflow->initialize();
        ASSERT_EQ(workflow->status(), WStatus::Initialized);

        // a → b → c, trigger consumes c.out.
        link(*a, *b);
        link(*b, *c);
        m_quantity.reset(Quantity::unitLess("TriggerQuantity"));
        trigger = std::make_unique<Input1DDouble>(
            "trigger", &m_dim, 1, m_quantity.get(), nullptr);
        ASSERT_TRUE(trigger->setProvider(c->out));
        c->out->addConsumer(trigger.get());
        workflow->setTrigger(trigger.get());
    }

    std::vector<std::string>            log;
    std::unique_ptr<ChainComponent>     a, b, c;
    std::unique_ptr<PullDrivenWorkflow> workflow;
    std::unique_ptr<Input1DDouble>      trigger;
    Dimension                           m_dim{"i", "Index"};
    std::unique_ptr<Quantity>           m_quantity;
};

TEST_F(PullWorkflowTest, ValidateRequiresTrigger)
{
    PullDrivenWorkflow bare("bare");
    bare.initialize();
    const auto problems = bare.validate();
    ASSERT_FALSE(problems.empty());
    EXPECT_NE(problems[0].find("trigger"), std::string::npos);
}

TEST_F(PullWorkflowTest, ServicesChainUpstreamFirst)
{
    ASSERT_TRUE(workflow->validate().empty());
    workflow->prepare();
    ASSERT_EQ(workflow->status(), WStatus::Prepared);

    workflow->update();
    EXPECT_EQ(workflow->status(), WStatus::Updated);

    // One pull services every upstream component exactly once, most
    // upstream first.
    EXPECT_EQ(log, (std::vector<std::string>{"a", "b", "c"}));

    // Values propagate down the chain: a.out=1, b.out=2, c.out=3, and the
    // trigger retrieved c's value.
    EXPECT_DOUBLE_EQ((*a->out)[0], 1.0);
    EXPECT_DOUBLE_EQ((*b->out)[0], 2.0);
    EXPECT_DOUBLE_EQ((*c->out)[0], 3.0);
    EXPECT_DOUBLE_EQ((*trigger)[0], 3.0);
}

TEST_F(PullWorkflowTest, RunsToDoneAndFinishes)
{
    ASSERT_TRUE(workflow->validate().empty());
    workflow->prepare();

    int guard = 0;
    while (workflow->status() != WStatus::Done && ++guard < 10)
        workflow->update();

    EXPECT_EQ(workflow->status(), WStatus::Done);
    EXPECT_EQ(a->updateCount, 3);
    EXPECT_EQ(c->updateCount, 3);

    workflow->finish();
    EXPECT_EQ(workflow->status(), WStatus::Finished);
    EXPECT_EQ(a->status(), CStatus::Finished);
}

TEST_F(PullWorkflowTest, ComponentFailurePropagates)
{
    FailingComponent bad("bad", &log, 1);
    std::string message;
    ASSERT_TRUE(workflow->addModelComponent(&bad, nullptr, &message));
    bad.initialize();
    // Splice bad between b and c: c consumes bad, bad consumes b.
    ASSERT_TRUE(b->out->removeConsumer(c->in));
    link(*b, bad);
    link(bad, *c);

    ASSERT_TRUE(workflow->validate().empty());
    workflow->prepare();
    workflow->update();

    EXPECT_EQ(workflow->status(), WStatus::Failed);
    const auto errors = workflow->errors();
    ASSERT_FALSE(errors.empty());
    EXPECT_EQ(errors.back().severity, ErrorEntry::Severity::Fatal);
    EXPECT_NE(errors.back().message.find("bad"), std::string::npos);

    // The local component must not outlive its registration.
    EXPECT_TRUE(workflow->removeModelComponent(&bad));
}

// ── TimeSteppedWorkflow ───────────────────────────────────────────────────

class TimeSteppedWorkflowTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        a = std::make_unique<ChainComponent>("a", &log);
        b = std::make_unique<ChainComponent>("b", &log);
        c = std::make_unique<ChainComponent>("c", &log);

        workflow = std::make_unique<TimeSteppedWorkflow>("stepped");
        std::string message;
        ASSERT_TRUE(workflow->addModelComponent(c.get(), nullptr, &message));
        ASSERT_TRUE(workflow->addModelComponent(b.get(), nullptr, &message));
        ASSERT_TRUE(workflow->addModelComponent(a.get(), nullptr, &message));
        workflow->initialize();
        ASSERT_EQ(workflow->status(), WStatus::Initialized);
    }

    void validateAndPrepare()
    {
        ASSERT_TRUE(workflow->validate().empty());
        workflow->prepare();
        ASSERT_EQ(workflow->status(), WStatus::Prepared);
    }

    std::vector<std::string>              log;
    std::unique_ptr<ChainComponent>       a, b, c;
    std::unique_ptr<TimeSteppedWorkflow>  workflow;
};

TEST_F(TimeSteppedWorkflowTest, ScheduleOrdersProvidersFirst)
{
    // a → b → c despite reversed insertion order.
    link(*a, *b);
    link(*b, *c);
    validateAndPrepare();

    const auto &schedule = workflow->schedule();
    ASSERT_EQ(schedule.size(), 3u);
    EXPECT_EQ(schedule[0][0]->id(), "a");
    EXPECT_EQ(schedule[1][0]->id(), "b");
    EXPECT_EQ(schedule[2][0]->id(), "c");

    workflow->update();
    EXPECT_EQ(log, (std::vector<std::string>{"a", "b", "c"}));
    EXPECT_DOUBLE_EQ((*c->out)[0], 3.0);
}

TEST_F(TimeSteppedWorkflowTest, FeedbackLoopCollapsesIntoOneGroup)
{
    // a → b ⇄ c: b and c form a feedback loop.
    link(*a, *b);
    link(*b, *c);
    link(*c, *b);
    validateAndPrepare();

    const auto &schedule = workflow->schedule();
    ASSERT_EQ(schedule.size(), 2u);
    EXPECT_EQ(schedule[0].size(), 1u);
    EXPECT_EQ(schedule[0][0]->id(), "a");
    EXPECT_EQ(schedule[1].size(), 2u);

    // Two fixed-point sweeps per step for the cyclic group only.
    workflow->setIterationsPerGroup(2);
    workflow->update();
    EXPECT_EQ(a->updateCount, 1);
    EXPECT_EQ(b->updateCount, 2);
    EXPECT_EQ(c->updateCount, 2);
}

TEST_F(TimeSteppedWorkflowTest, StatusSignalTracesLifecycle)
{
    link(*a, *b);
    link(*b, *c);

    auto slot = std::make_shared<RecordingSlot>();
    workflow->connect(slot);

    validateAndPrepare();
    workflow->update();

    const auto &s = slot->statuses;
    ASSERT_GE(s.size(), 6u);
    EXPECT_EQ(s[0], WStatus::Validating);
    EXPECT_EQ(s[1], WStatus::Validated);
    EXPECT_EQ(s[2], WStatus::Preparing);
    EXPECT_EQ(s[3], WStatus::Prepared);
    EXPECT_EQ(s[4], WStatus::Updating);
    EXPECT_EQ(s[5], WStatus::Updated);
}

TEST_F(TimeSteppedWorkflowTest, PauseAtSynchronizationPointAndResume)
{
    link(*a, *b);
    link(*b, *c);
    validateAndPrepare();

    workflow->requestPause();
    workflow->update();
    EXPECT_EQ(workflow->status(), WStatus::Paused);
    EXPECT_EQ(a->updateCount, 1); // in-flight step completed first

    workflow->resume();
    EXPECT_EQ(workflow->status(), WStatus::Updated);
    workflow->update();
    EXPECT_EQ(a->updateCount, 2);
}

TEST_F(TimeSteppedWorkflowTest, StopRequestEndsRunAsDone)
{
    link(*a, *b);
    link(*b, *c);
    validateAndPrepare();

    workflow->requestStop();
    workflow->update();
    EXPECT_EQ(workflow->status(), WStatus::Done);
    EXPECT_EQ(a->updateCount, 1);

    workflow->finish();
    EXPECT_EQ(workflow->status(), WStatus::Finished);
}

TEST_F(TimeSteppedWorkflowTest, RunsToCompositionDone)
{
    link(*a, *b);
    link(*b, *c);
    validateAndPrepare();

    int guard = 0;
    while (workflow->status() != WStatus::Done && ++guard < 10)
        workflow->update();

    EXPECT_EQ(workflow->status(), WStatus::Done);
    EXPECT_EQ(a->updateCount, 3);
    EXPECT_EQ(b->updateCount, 3);
    EXPECT_EQ(c->updateCount, 3);
}
