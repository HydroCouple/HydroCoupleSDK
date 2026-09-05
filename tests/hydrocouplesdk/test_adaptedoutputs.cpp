// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file  test_adaptedoutputs.cpp
 * \brief Gates for the SDK's concrete adapted outputs and their factory
 *        (CONNECT A2), and for ModelInitializer's chain building (A3).
 *
 * The chain-building machinery predates every concrete adapter in the
 * ecosystem, so until this file nothing had ever run it.
 */

#include "hydrocouplesdk/data/lineartransformadaptedoutput.h"
#include "hydrocouplesdk/data/relaxationadaptedoutput.h"
#include "hydrocouplesdk/data/sdkadaptedoutputfactory.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/data/exchangeitems2d.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple::SDK;
using AT = HydroCouple::IArgument::ArgumentInputType;

namespace {

//! Reads a rank-1 Double item's whole slab.
std::vector<double> readAll(const HydroCouple::IComponentDataItem &item)
{
    const std::vector<int64_t> shape = item.shape();
    if (shape.size() != 1 || shape[0] < 0)
        return {};

    std::vector<double> values(static_cast<size_t>(shape[0]));

    HydroCouple::BufferDescriptor destination;
    destination.data  = values.data();
    destination.kind  = HydroCouple::DataKind::Float64;
    destination.rank  = 1;
    destination.shape = shape.data();

    const int64_t start = 0;
    std::string message;
    if (!item.getValuesInto(destination,
                            std::span<const int64_t>(&start, 1),
                            std::span<const int64_t>(&shape[0], 1), &message))
    {
        ADD_FAILURE() << "getValuesInto failed: " << message;
        return {};
    }
    return values;
}

//! Applies one JSON payload to a named argument, the chain builder's way.
void applyArgument(HydroCouple::IAdaptedOutput &adapted,
                   const std::string &argumentId, const std::string &json)
{
    for (HydroCouple::IArgument *argument : adapted.arguments())
    {
        if (argument && argument->id() == argumentId)
        {
            std::string message;
            ASSERT_TRUE(argument->initialize(json, AT::JSON, message))
                << argumentId << ": " << message;
            return;
        }
    }
    FAIL() << "no argument \"" << argumentId << "\"";
}

//! A free-standing provider output the adapters can wrap.
struct ProviderRig
{
    Dimension dimension{"scalar"};
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Flow")};
    Output1DDouble output{"produced", &dimension, 3, quantity.get(), nullptr};

    ProviderRig()
    {
        output[0] = 10.0;
        output[1] = 20.0;
        output[2] = 30.0;
    }
};

} // namespace

// ── A2: the factory and its adapters ────────────────────────────────────────

TEST(AdaptedOutputTest, FactoryRegistersTheCreatedAdapterWithItsProvider)
{
    ProviderRig rig;
    SdkAdaptedOutputFactory *factory = SdkAdaptedOutputFactory::instance();

    const std::vector<HydroCouple::IIdentity *> available =
        factory->getAvailableAdaptedOutputIds(&rig.output);
    ASSERT_FALSE(available.empty());

    std::unique_ptr<HydroCouple::IAdaptedOutput> adapted =
        factory->createAdaptedOutput(available[0], &rig.output);
    ASSERT_NE(adapted, nullptr);

    // The interface contract: "the returned IAdaptedOutput will already be
    // registered with the provider". Without it, the provider's refresh
    // fan-out never reaches the adapter and every chained value goes stale.
    const std::vector<HydroCouple::IAdaptedOutput *> registered =
        rig.output.adaptedOutputs();
    ASSERT_EQ(registered.size(), 1u);
    EXPECT_EQ(registered[0], adapted.get());

    EXPECT_EQ(adapted->adaptee(), &rig.output);
    EXPECT_EQ(adapted->adaptedOutputFactory(), factory);

    // The registry is non-owning: unregister before the instance dies.
    EXPECT_TRUE(rig.output.removeAdaptedOutput(adapted.get()));
}

TEST(AdaptedOutputTest, LinearTransformAppliesMultiplierAndOffsetOnRefresh)
{
    ProviderRig rig;
    LinearTransformAdaptedOutput adapted(
        "linear", &rig.output, SdkAdaptedOutputFactory::instance());

    // Arguments land as JSON before initialize() — exactly the chain
    // builder's order.
    ASSERT_NO_FATAL_FAILURE(
        applyArgument(adapted, "multiplier", R"({"values": [2.0]})"));
    ASSERT_NO_FATAL_FAILURE(
        applyArgument(adapted, "offset", R"([5.0])"));

    adapted.initialize();
    EXPECT_EQ(readAll(adapted), (std::vector<double>{25.0, 45.0, 65.0}));

    // The adaptee moves; refresh() must follow it.
    rig.output[0] = 1.0;
    rig.output[1] = 2.0;
    rig.output[2] = 3.0;
    adapted.refresh();
    EXPECT_EQ(readAll(adapted), (std::vector<double>{7.0, 9.0, 11.0}));
}

TEST(AdaptedOutputTest, RelaxationBlendsSuccessiveRefreshesByItsFactor)
{
    ProviderRig rig;
    RelaxationAdaptedOutput adapted(
        "relax", &rig.output, SdkAdaptedOutputFactory::instance());

    // 0.25, not 0.5: an even split is symmetric, so a blend with prev and
    // x swapped would read identically and the gate would prove nothing.
    ASSERT_NO_FATAL_FAILURE(
        applyArgument(adapted, "relaxation_factor", R"({"values": [0.25]})"));

    // First refresh has no history: the adaptee passes through.
    adapted.initialize();
    EXPECT_EQ(readAll(adapted), (std::vector<double>{10.0, 20.0, 30.0}));

    // Second refresh blends: y = 0.75*prev + 0.25*x.
    rig.output[0] = 20.0;
    rig.output[1] = 40.0;
    rig.output[2] = 60.0;
    adapted.refresh();
    EXPECT_EQ(readAll(adapted), (std::vector<double>{12.5, 25.0, 37.5}));

    // And it keeps converging toward a held adaptee, which is what makes it
    // damping rather than a one-off average.
    adapted.refresh();
    EXPECT_EQ(readAll(adapted), (std::vector<double>{14.375, 28.75, 43.125}));
}

TEST(AdaptedOutputTest, FactoryOffersLinearTransformOnlyForOneDimensionalDoubleOutputs)
{
    SdkAdaptedOutputFactory *factory = SdkAdaptedOutputFactory::instance();

    ProviderRig rig;
    EXPECT_EQ(factory->getAvailableAdaptedOutputIds(&rig.output).size(), 2u);

    // A rank-2 output is not an element-wise-transformable slab here.
    Dimension rows{"rows"};
    Dimension columns{"columns"};
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Field")};
    Output2DDouble field("field", &rows, &columns, 2, 2, quantity.get(),
                         nullptr);
    EXPECT_TRUE(factory->getAvailableAdaptedOutputIds(&field).empty())
        << "a rank-2 output was offered an element-wise rank-1 adapter";

    // An integer output is not a Double slab.
    Dimension scalar{"scalar"};
    std::unique_ptr<Quantity> count{Quantity::unitLess("Count")};
    Output1DInt counts("counts", &scalar, 3, count.get(), nullptr);
    EXPECT_TRUE(factory->getAvailableAdaptedOutputIds(&counts).empty())
        << "an Int32 output was offered a Float64 adapter";
}

// ── A3: ModelInitializer builds chains from the spec ────────────────────────

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/component/abstractmodelcomponentinfo.h"
#include "hydrocouplesdk/io/modelinitializer.h"

namespace {

using HydroCouple::SDK::IO::ModelInitializer;

//! Info whose factories the chain builder must find.
class ChainTestInfo final : public AbstractModelComponentInfo
{
public:
    ChainTestInfo() : AbstractModelComponentInfo("chain.test.info")
    {
        addAdaptedOutputFactory(SdkAdaptedOutputFactory::instance());
    }

    std::unique_ptr<HydroCouple::IModelComponent>
    createComponentInstance() override
    {
        return nullptr; // the tests construct their instances directly
    }
};

/*!
 * Minimal component with one rank-1 Double output "flow" (published with
 * nonzero values so a chain built at initialize time reads something
 * distinguishable) and one input "inflow".
 */
class ChainTestComponent : public AbstractModelComponent
{
public:
    ChainTestComponent(std::string_view id, ChainTestInfo *info)
        : AbstractModelComponent(id, info)
    {}

    std::vector<std::string> validate() override { return {}; }
    void prepare() override {}
    void update(const std::vector<HydroCouple::IOutput *> & = {}) override {}
    void finish() override {}

    Output1DDouble *flow   = nullptr;
    Input1DDouble  *inflow = nullptr;

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        inflow = new Input1DDouble("inflow", &m_dimension, 3,
                                   m_quantity.get(), this);
        addInput(inflow);
    }
    void createOutputs() override
    {
        flow = new Output1DDouble("flow", &m_dimension, 3, m_quantity.get(),
                                  this);
        (*flow)[0] = 10.0;
        (*flow)[1] = 20.0;
        (*flow)[2] = 30.0;
        addOutput(flow);
    }
    void initializeFailureCleanUp() override {}

private:
    Dimension m_dimension{"i", "Index"};
    std::unique_ptr<Quantity> m_quantity{Quantity::unitLess("ChainQ")};
};

class AdaptedChainTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_up   = std::make_unique<ChainTestComponent>("up", &m_info);
        m_down = std::make_unique<ChainTestComponent>("down", &m_info);
        m_resolver = [this](const std::string &id)
            -> HydroCouple::IModelComponent * {
            if (id == "up")   return m_up.get();
            if (id == "down") return m_down.get();
            return nullptr;
        };
    }

    ChainTestInfo                       m_info;
    std::unique_ptr<ChainTestComponent> m_up, m_down;
    ModelInitializer::ComponentResolver m_resolver;
};

} // namespace

TEST_F(AdaptedChainTest, BuildsTheChainFromTheSpecAndWiresTheConsumerToItsTail)
{
    // Two stacked steps; the second names no factory, exercising the
    // any-factory match. The consumer must see the TAIL, and each link's
    // adaptee must walk back to the raw output.
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(R"({
        "components": [ { "id": "up" }, { "id": "down" } ],
        "connections": [
            { "from": { "component": "up", "output": "flow",
                        "adapted_outputs": [
                            { "factory": "hydrocouple.sdk.adapters",
                              "id": "linear_transform",
                              "arguments": { "multiplier": { "values": [2.0] },
                                             "offset":     { "values": [5.0] } } },
                            { "id": "linear_transform",
                              "arguments": { "offset": { "values": [100.0] } } }
                        ] },
              "to": { "component": "down", "input": "inflow" } }
        ]})", message)) << message;

    auto *tail = dynamic_cast<HydroCouple::IAdaptedOutput *>(
        m_down->inflow->provider());
    ASSERT_NE(tail, nullptr) << "the consumer is wired to the raw output, "
                                "not the chain tail";

    auto *first = dynamic_cast<HydroCouple::IAdaptedOutput *>(tail->adaptee());
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->adaptee(), static_cast<HydroCouple::IOutput *>(m_up->flow));

    // flow {10,20,30} -> x2+5 -> {25,45,65} -> +100 -> {125,145,165}.
    EXPECT_EQ(readAll(*tail), (std::vector<double>{125.0, 145.0, 165.0}));

    // The chain tail carries the consumer.
    const std::vector<HydroCouple::IInput *> consumers = tail->consumers();
    ASSERT_EQ(consumers.size(), 1u);
    EXPECT_EQ(consumers[0], static_cast<HydroCouple::IInput *>(m_down->inflow));

    // And the provider's push path reaches through both links: this is what
    // a coupled run leans on every step.
    (*m_up->flow)[0] = 1.0;
    (*m_up->flow)[1] = 2.0;
    (*m_up->flow)[2] = 3.0;
    // Through the interface: Output1D's zero-arg updateValues() hides the
    // query overload on the derived type.
    static_cast<HydroCouple::IOutput *>(m_up->flow)
        ->updateValues(m_down->inflow);
    EXPECT_EQ(readAll(*tail), (std::vector<double>{107.0, 109.0, 111.0}));
}

TEST_F(AdaptedChainTest, AppliesChainStepArgumentsBeforeInitialize)
{
    // initialize() latches the coefficients, so values read straight after
    // apply — before any refresh — betray the order: arguments-first gives
    // 3x, initialize-first would have latched the defaults and give 1x.
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(R"({
        "components": [ { "id": "up" }, { "id": "down" } ],
        "connections": [
            { "from": { "component": "up", "output": "flow",
                        "adapted_outputs": [
                            { "id": "linear_transform",
                              "arguments": { "multiplier": { "values": [3.0] } } }
                        ] },
              "to": { "component": "down", "input": "inflow" } }
        ]})", message)) << message;

    HydroCouple::IOutput *provider = m_down->inflow->provider();
    ASSERT_NE(provider, nullptr);
    EXPECT_EQ(readAll(*provider), (std::vector<double>{30.0, 60.0, 90.0}));
}

TEST_F(AdaptedChainTest, RefusesAnUnknownAdapterByName)
{
    ModelInitializer initializer(m_resolver);
    std::string message;
    ASSERT_FALSE(initializer.initializeFromJsonText(R"({
        "components": [ { "id": "up" }, { "id": "down" } ],
        "connections": [
            { "from": { "component": "up", "output": "flow",
                        "adapted_outputs": [ { "id": "no_such_adapter" } ] },
              "to": { "component": "down", "input": "inflow" } }
        ]})", message));

    EXPECT_NE(message.find("no_such_adapter"), std::string::npos) << message;
    EXPECT_NE(message.find("up"), std::string::npos) << message;
}
