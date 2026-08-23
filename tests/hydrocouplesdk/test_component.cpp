// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for AbstractModelComponent: lifecycle status transitions, the v2
// error queue, capabilities, and status-change signals.

#include <gtest/gtest.h>

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/data/abstractargument.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <memory>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;

namespace {

/*!
 * Minimal concrete component exercising the base-class machinery.
 */
class TestComponent : public AbstractModelComponent
{
public:
    explicit TestComponent(std::string_view id)
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
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Updated,
                  "prepared");
    }

    void update(const std::vector<HydroCouple::IOutput *> & = {}) override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Updating);
        ++updateCount;
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Updated,
                  "stepped");
    }

    void finish() override
    {
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Finishing);
        setStatus(HydroCouple::IModelComponent::ComponentStatus::Finished);
    }

    // Expose protected machinery for testing.
    using AbstractModelComponent::reportError;
    using AbstractModelComponent::setStatus;

    int  updateCount   = 0;
    bool argsCreated   = false;
    bool inputsCreated = false;

protected:
    void createArguments() override { argsCreated = true; }
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return !failArgInit;
    }
    void createInputs()  override { inputsCreated = true; }
    void createOutputs() override {}
    void initializeFailureCleanUp() override { cleanedUp = true; }

public:
    bool failArgInit = false;
    bool cleanedUp   = false;
};

using Status = HydroCouple::IModelComponent::ComponentStatus;

} // namespace

TEST(ModelComponent, StartsCreated)
{
    TestComponent c("comp");
    EXPECT_EQ(c.status(), Status::Created);
}

TEST(ModelComponent, InitializeTransitionsToInitialized)
{
    TestComponent c("comp");
    c.initialize();
    EXPECT_EQ(c.status(), Status::Initialized);
    EXPECT_TRUE(c.argsCreated);
    EXPECT_TRUE(c.inputsCreated);
}

TEST(ModelComponent, InitializeFailureSetsFailedAndCleansUp)
{
    TestComponent c("comp");
    c.failArgInit = true;
    c.initialize();
    EXPECT_EQ(c.status(), Status::Failed);
    EXPECT_TRUE(c.cleanedUp);

    // Failure must queue a Fatal diagnostic (normative failure channel).
    const auto errs = c.errors();
    ASSERT_FALSE(errs.empty());
    EXPECT_EQ(errs.back().severity, ErrorEntry::Severity::Fatal);
}

TEST(ModelComponent, FullLifecycleRunsToFinished)
{
    TestComponent c("comp");
    c.initialize();
    EXPECT_TRUE(c.validate().empty());
    c.prepare();
    c.update();
    c.update();
    c.finish();
    EXPECT_EQ(c.status(), Status::Finished);
    EXPECT_EQ(c.updateCount, 2);
}

TEST(ModelComponent, CapabilitiesDefaultEmpty)
{
    TestComponent c("comp");
    EXPECT_TRUE(c.capabilities().empty());
}

TEST(ModelComponent, ErrorQueueDrainsWhenRequested)
{
    TestComponent c("comp");
    c.reportError(ErrorEntry::Severity::Warning, 1001, "first");
    c.reportError(ErrorEntry::Severity::Error,   2001, "second");

    auto peek = c.errors(false);
    ASSERT_EQ(peek.size(), 2u);
    EXPECT_EQ(peek[0].code, 1001);
    EXPECT_EQ(peek[1].severity, ErrorEntry::Severity::Error);

    auto drained = c.errors(true);
    EXPECT_EQ(drained.size(), 2u);
    EXPECT_TRUE(c.errors().empty());
}

TEST(ModelComponent, StatusChangedSignalFires)
{
    TestComponent c("comp");

    std::vector<Status> observed;
    using EventPtr =
        const std::shared_ptr<HydroCouple::IComponentStatusChangeEventArgs> &;
    auto slot = std::make_shared<FunctionSlot<EventPtr>>(
        [&](const ISignal<EventPtr> &, EventPtr args) {
            observed.push_back(args->status());
        });
    c.connect(slot);

    c.initialize();
    ASSERT_FALSE(observed.empty());
    EXPECT_EQ(observed.back(), Status::Initialized);
}

TEST(ModelComponent, SetStatusFailedQueuesFatalError)
{
    TestComponent c("comp");
    c.setStatus(Status::Failed, "boom");
    const auto errs = c.errors();
    ASSERT_EQ(errs.size(), 1u);
    EXPECT_EQ(errs[0].severity, ErrorEntry::Severity::Fatal);
    EXPECT_EQ(errs[0].message, "boom");
}
