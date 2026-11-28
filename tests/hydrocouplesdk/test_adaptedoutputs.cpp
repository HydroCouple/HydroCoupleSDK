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

#include <algorithm>
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
    ChainTestComponent(std::string_view id, AbstractModelComponentInfo *info)
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

TEST_F(AdaptedChainTest, RefusesAnUnknownAdapterNamingBothSourcesSearched)
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

    // The refusal must say what was searched: the fixer's next move differs
    // between "the component has no factories" and "no factory offers it".
    EXPECT_NE(message.find("no_such_adapter"), std::string::npos) << message;
    EXPECT_NE(message.find("up"), std::string::npos) << message;
    EXPECT_NE(message.find("standalone"), std::string::npos) << message;
}

namespace {

/*!
 * Offers "linear_transform" too, but bakes a betraying +1000 offset into
 * every instance, so a gate can tell WHICH factory built a chain link.
 */
class ShadowFactory final : public AbstractAdaptedOutputFactory
{
public:
    ShadowFactory()
        : AbstractAdaptedOutputFactory("shadow.factory"),
          m_offering(std::make_unique<Identity>(
              SdkAdaptedOutputFactory::kLinearTransformId, "Shadowed linear"))
    {}

    std::vector<HydroCouple::IIdentity *> getAvailableAdaptedOutputIds(
        const HydroCouple::IOutput *provider,
        const HydroCouple::IInput * = nullptr) override
    {
        if (!provider)
            return {};
        return {m_offering.get()};
    }

    std::unique_ptr<HydroCouple::IAdaptedOutput> createAdaptedOutput(
        HydroCouple::IIdentity *adaptedProviderId,
        HydroCouple::IOutput *provider,
        HydroCouple::IInput * = nullptr) override
    {
        if (!adaptedProviderId || !provider ||
            adaptedProviderId->id() != SdkAdaptedOutputFactory::kLinearTransformId)
            return nullptr;

        auto created = std::make_unique<LinearTransformAdaptedOutput>(
            "shadowed_linear", provider, this);
        std::string message;
        for (HydroCouple::IArgument *argument : created->arguments())
            if (argument && argument->id() == "offset")
                (void)argument->initialize(R"({"values": [1000.0]})",
                                           AT::JSON, message);
        provider->addAdaptedOutput(created.get());
        return created;
    }

private:
    std::unique_ptr<Identity> m_offering;
};

//! Info registering only the shadow factory.
class ShadowInfo final : public AbstractModelComponentInfo
{
public:
    ShadowInfo() : AbstractModelComponentInfo("shadow.info")
    {
        addAdaptedOutputFactory(&m_shadow);
    }

    std::unique_ptr<HydroCouple::IModelComponent>
    createComponentInstance() override
    {
        return nullptr;
    }

private:
    ShadowFactory m_shadow;
};

} // namespace

TEST_F(AdaptedChainTest, StandaloneFactoriesAreSearchedAfterComponentInfoFactories)
{
    static constexpr const char *kDocument = R"({
        "components": [ { "id": "provider" }, { "id": "consumer" } ],
        "connections": [
            { "from": { "component": "provider", "output": "flow",
                        "adapted_outputs": [
                            { "id": "linear_transform",
                              "arguments": { "multiplier": { "values": [2.0] } } }
                        ] },
              "to": { "component": "consumer", "input": "inflow" } }
        ]})";

    // (1) Reachability: a provider whose component has no componentInfo()
    // at all can still adapt when the host installs standalone factories —
    // the whole point of the seam.
    {
        ChainTestComponent bare("provider", nullptr);
        ChainTestComponent sink("consumer", nullptr);
        ModelInitializer initializer(
            [&](const std::string &id) -> HydroCouple::IModelComponent * {
                if (id == "provider") return &bare;
                if (id == "consumer") return &sink;
                return nullptr;
            });
        initializer.setAdapterFactoryResolver([] {
            return std::vector<HydroCouple::IAdaptedOutputFactory *>{
                SdkAdaptedOutputFactory::instance()};
        });

        std::string message;
        ASSERT_TRUE(initializer.initializeFromJsonText(kDocument, message))
            << message;
        EXPECT_EQ(readAll(*sink.inflow->provider()),
                  (std::vector<double>{20.0, 40.0, 60.0}));
    }

    // (2) Precedence: when the component's own info offers the same id,
    // the component-info factory wins — its baked +1000 offset survives.
    {
        ShadowInfo shadowInfo;
        ChainTestComponent owned("provider", &shadowInfo);
        ChainTestComponent sink("consumer", &shadowInfo);
        ModelInitializer initializer(
            [&](const std::string &id) -> HydroCouple::IModelComponent * {
                if (id == "provider") return &owned;
                if (id == "consumer") return &sink;
                return nullptr;
            });
        initializer.setAdapterFactoryResolver([] {
            return std::vector<HydroCouple::IAdaptedOutputFactory *>{
                SdkAdaptedOutputFactory::instance()};
        });

        std::string message;
        ASSERT_TRUE(initializer.initializeFromJsonText(kDocument, message))
            << message;
        EXPECT_EQ(readAll(*sink.inflow->provider()),
                  (std::vector<double>{1020.0, 1040.0, 1060.0}))
            << "the standalone factory shadowed the component's own";
    }
}

// ── A5: serialize() reproduces chains and roles ─────────────────────────────

#include "hydrocouplesdk/data/abstractinput.h"

namespace {

/*!
 * Multi-input with two named provider roles. The role→provider map never
 * leaves AbstractMultiInput, which is exactly why serialize() must recover
 * roles from the authored spec.
 */
class RoleInput final : public AbstractMultiInput
{
public:
    RoleInput(std::string_view id, Dimension *dimension,
              ValueDefinition *valueDefinition,
              HydroCouple::IModelComponent *component)
        : AbstractMultiInput(id, {dimension}, valueDefinition, component)
    {
        m_labels.push_back(std::make_unique<Identity>("primary", "Primary"));
        m_labels.push_back(
            std::make_unique<Identity>("secondary", "Secondary"));
    }

    [[nodiscard]] std::vector<HydroCouple::IIdentity *>
    providerLabels() const override
    {
        std::vector<HydroCouple::IIdentity *> labels;
        for (const std::unique_ptr<Identity> &label : m_labels)
            labels.push_back(label.get());
        return labels;
    }

    bool canConsume(HydroCouple::IOutput *, std::string &message) const override
    {
        message.clear();
        return true;
    }

    bool canConsume(HydroCouple::IOutput *, std::string &message,
                    const HydroCouple::IIdentity *) const override
    {
        message.clear();
        return true;
    }

    void retrieveValuesFromProvider() override {}
    void applyData() override {}

    [[nodiscard]] std::vector<int64_t> shape() const override { return {1}; }
    [[nodiscard]] HydroCouple::DataKind dataKind() const override
    {
        return HydroCouple::DataKind::Float64;
    }
    [[nodiscard]] bool getValuesInto(const HydroCouple::BufferDescriptor &,
                                     std::span<const int64_t>,
                                     std::span<const int64_t>,
                                     std::string *message) const override
    {
        if (message)
            *message = "RoleInput serves no values";
        return false;
    }
    [[nodiscard]] bool setValuesFrom(const HydroCouple::BufferDescriptor &,
                                     std::span<const int64_t>,
                                     std::span<const int64_t>,
                                     std::string *message) override
    {
        if (message)
            message->clear();
        return true;
    }

private:
    std::vector<std::unique_ptr<Identity>> m_labels;
};

//! Component whose only input is the two-role multi-input "operands".
class MultiSinkComponent : public AbstractModelComponent
{
public:
    MultiSinkComponent(std::string_view id, AbstractModelComponentInfo *info)
        : AbstractModelComponent(id, info)
    {}

    std::vector<std::string> validate() override { return {}; }
    void prepare() override {}
    void update(const std::vector<HydroCouple::IOutput *> & = {}) override {}
    void finish() override {}

    RoleInput *operands = nullptr;

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        operands = new RoleInput("operands", &m_dimension, m_quantity.get(),
                                 this);
        addInput(operands);
    }
    void createOutputs() override {}
    void initializeFailureCleanUp() override {}

private:
    Dimension m_dimension{"i", "Index"};
    std::unique_ptr<Quantity> m_quantity{Quantity::unitLess("SinkQ")};
};

//! The connection whose from.component matches, or null.
const nlohmann::json *linkFrom(const nlohmann::json &document,
                               const std::string &fromComponent)
{
    for (const nlohmann::json &link : document["connections"])
        if (link["from"]["component"] == fromComponent)
            return &link;
    return nullptr;
}

} // namespace

TEST_F(AdaptedChainTest, SerializeReproducesTheAdaptedChainAndMultiInputRoles)
{
    ChainTestComponent a("a", &m_info);
    ChainTestComponent b("b", &m_info);
    MultiSinkComponent sink("sink", &m_info);

    ModelInitializer initializer(
        [&](const std::string &id) -> HydroCouple::IModelComponent * {
            if (id == "a")    return &a;
            if (id == "b")    return &b;
            if (id == "sink") return &sink;
            return nullptr;
        });

    std::string message;
    ASSERT_TRUE(initializer.initializeFromJsonText(R"({
        "components": [ { "id": "a" }, { "id": "b" }, { "id": "sink" } ],
        "connections": [
            { "from": { "component": "a", "output": "flow",
                        "adapted_outputs": [
                            { "id": "linear_transform",
                              "arguments": { "multiplier": { "values": [2.0] } } }
                        ] },
              "to": { "component": "sink", "input": "operands",
                      "role": "primary" } },
            { "from": { "component": "b", "output": "flow" },
              "to": { "component": "sink", "input": "operands",
                      "role": "secondary" } }
        ]})", message)) << message;

    // Mutate the live adapter after the build: the document must carry the
    // adapter's CURRENT argument values, not the ones it was born with.
    HydroCouple::IAdaptedOutput *tail = nullptr;
    for (HydroCouple::IOutput *provider : sink.operands->providers())
        if (auto *adapted =
                dynamic_cast<HydroCouple::IAdaptedOutput *>(provider))
            tail = adapted;
    ASSERT_NE(tail, nullptr);

    // Wired through IOutput::addConsumer() with the role (ABI 5): each
    // provider lists the multi-input among its consumers, which the old
    // direct addProvider() call never told it.
    for (HydroCouple::IOutput *provider : sink.operands->providers())
    {
        const auto consumers = provider->consumers();
        EXPECT_NE(std::find(consumers.begin(), consumers.end(),
                            static_cast<HydroCouple::IInput *>(sink.operands)),
                  consumers.end())
            << provider->id() << " does not know the multi-input consumes it";
    }
    for (HydroCouple::IArgument *argument : tail->arguments())
        if (argument && argument->id() == "multiplier")
            ASSERT_TRUE(argument->initialize(R"({"values": [7.0]})", AT::JSON,
                                             message)) << message;

    const nlohmann::json document =
        initializer.serialize({"a", "b", "sink"}, message);
    ASSERT_FALSE(document.is_null()) << message;

    // Before this fix the adapted link was SILENTLY DROPPED: the chain tail
    // has no modelComponent(), and the old walk gave up right there.
    ASSERT_EQ(document["connections"].size(), 2u) << document.dump(2);

    const nlohmann::json *adaptedLink = linkFrom(document, "a");
    ASSERT_NE(adaptedLink, nullptr) << document.dump(2);
    EXPECT_EQ((*adaptedLink)["from"]["output"], "flow");
    ASSERT_TRUE((*adaptedLink)["from"].contains("adapted_outputs"));
    const nlohmann::json &steps = (*adaptedLink)["from"]["adapted_outputs"];
    ASSERT_EQ(steps.size(), 1u);
    EXPECT_EQ(steps[0]["id"], "linear_transform")
        << "the OFFERING id, not the instance id with its ordinal";
    EXPECT_EQ(steps[0]["factory"], "hydrocouple.sdk.adapters");
    EXPECT_EQ(steps[0]["arguments"]["multiplier"]["values"],
              nlohmann::json::array({7.0}))
        << "the live adapter's mutated argument did not round-trip";
    EXPECT_EQ((*adaptedLink)["to"]["role"], "primary");

    const nlohmann::json *plainLink = linkFrom(document, "b");
    ASSERT_NE(plainLink, nullptr) << document.dump(2);
    EXPECT_FALSE((*plainLink)["from"].contains("adapted_outputs"));
    EXPECT_EQ((*plainLink)["to"]["role"], "secondary");
}

// ── A7: temporal interpolation ──────────────────────────────────────────────

#include "hydrocouplesdk/temporal/temporalinterpolationadaptedoutput.h"
#include "hydrocouplesdk/temporal/timeseriesexchangeitems.h"

TEST(AdaptedOutputTest, TemporalInterpolationServesTheValueAtTheQueriedTime)
{
    using HydroCouple::SDK::Temporal::TemporalInterpolationAdaptedOutput;
    using HydroCouple::SDK::Temporal::TimeData;
    using HydroCouple::SDK::Temporal::TimeSeriesOutputDouble;

    Dimension timeDimension{"time"};
    std::unique_ptr<Quantity> quantity{Quantity::unitLess("Stage")};
    TimeSeriesOutputDouble series("stage", {&timeDimension}, quantity.get(),
                                  nullptr);
    series.addTimeStep(std::make_shared<TimeData>("Time", 0.0), {10.0});
    series.addTimeStep(std::make_shared<TimeData>("Time", 1.0), {20.0});

    SdkAdaptedOutputFactory *factory = SdkAdaptedOutputFactory::instance();

    // The temporal offering is for time-series providers only — and it is
    // the ONLY offering there (the element-wise transforms want rank-1).
    HydroCouple::IIdentity *offering = nullptr;
    const std::vector<HydroCouple::IIdentity *> available =
        factory->getAvailableAdaptedOutputIds(&series);
    ASSERT_EQ(available.size(), 1u);
    offering = available[0];
    EXPECT_EQ(offering->id(), SdkAdaptedOutputFactory::kTemporalInterpolationId);

    ProviderRig plain;
    for (HydroCouple::IIdentity *identity :
         factory->getAvailableAdaptedOutputIds(&plain.output))
        EXPECT_NE(identity->id(),
                  SdkAdaptedOutputFactory::kTemporalInterpolationId)
            << "a plain rank-1 output was offered temporal interpolation";

    std::unique_ptr<HydroCouple::IAdaptedOutput> created =
        factory->createAdaptedOutput(offering, &series);
    ASSERT_NE(created, nullptr);
    auto *adapted =
        dynamic_cast<TemporalInterpolationAdaptedOutput *>(created.get());
    ASSERT_NE(adapted, nullptr);

    // initialize() seeds the whole history the adaptee already holds.
    adapted->initialize();
    EXPECT_EQ(adapted->sampleCount(), 2);
    adapted->setQueryTime(0.5);
    EXPECT_EQ(readAll(*adapted), (std::vector<double>{15.0}));

    // refresh() appends the newest sample; a same-time refresh is skipped
    // (the harmless-double-refresh contract).
    series.addTimeStep(std::make_shared<TimeData>("Time", 2.0), {40.0});
    adapted->refresh();
    EXPECT_EQ(adapted->sampleCount(), 3);
    adapted->refresh();
    EXPECT_EQ(adapted->sampleCount(), 3) << "a same-time refresh duplicated";
    adapted->setQueryTime(1.5);
    EXPECT_EQ(readAll(*adapted), (std::vector<double>{30.0}));

    // "previous" holds from the left instead of interpolating.
    std::unique_ptr<HydroCouple::IAdaptedOutput> held =
        factory->createAdaptedOutput(offering, &series);
    ASSERT_NE(held, nullptr);
    ASSERT_NO_FATAL_FAILURE(
        applyArgument(*held, "method", R"({"values": ["previous"]})"));
    held->initialize();
    auto *heldAdapted =
        dynamic_cast<TemporalInterpolationAdaptedOutput *>(held.get());
    ASSERT_NE(heldAdapted, nullptr);
    heldAdapted->setQueryTime(0.5);
    EXPECT_EQ(readAll(*heldAdapted), (std::vector<double>{10.0}));

    EXPECT_TRUE(series.removeAdaptedOutput(created.get()));
    EXPECT_TRUE(series.removeAdaptedOutput(held.get()));
}
