// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 8: adapted outputs declared in a composition document.
 *         IO::ModelInitializer builds the chain from "adapted_outputs",
 *         applies each step's arguments before initialize(), wires the
 *         consumer to the tail, and writes the chain back in serialize().
 */

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/component/abstractmodelcomponentinfo.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/data/sdkadaptedoutputfactory.h"
#include "hydrocouplesdk/io/modelinitializer.h"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;

namespace {

int failures = 0;

void check(bool ok, const std::string &what)
{
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok)
        ++failures;
}

//! [component]
//! A minimal component with one rank-1 Float64 output "flow" and one input
//! "inflow", enough to carry a connection. AbstractModelComponent only lists
//! the items it is given, so the component owns them, declared after the
//! dimension and quantity they point to.
class Reach final : public AbstractModelComponent
{
private:
    Dimension m_dimension{"reaches", "Reaches"};
    std::unique_ptr<Quantity> m_quantity{Quantity::unitLess("Flow")};

public:
    Reach(std::string_view id, AbstractModelComponentInfo *info)
        : AbstractModelComponent(id, info)
    {}

    std::vector<std::string> validate() override { return {}; }
    void prepare() override {}
    void update(const std::vector<IOutput *> & = {}) override {}
    void finish() override {}

    std::unique_ptr<Output1DDouble> flow;
    std::unique_ptr<Input1DDouble>  inflow;

protected:
    void createArguments() override {}
    bool initializeArguments(std::string &message) override
    {
        message.clear();
        return true;
    }
    void createInputs() override
    {
        inflow = std::make_unique<Input1DDouble>("inflow", &m_dimension, 3, m_quantity.get(), this);
        addInput(inflow.get());
    }
    void createOutputs() override
    {
        flow = std::make_unique<Output1DDouble>("flow", &m_dimension, 3, m_quantity.get(), this);
        (*flow)[0] = 1.0;
        (*flow)[1] = 2.0;
        (*flow)[2] = 3.0;
        addOutput(flow.get());
    }
    void initializeFailureCleanUp() override {}
};

//! Component info with no factories of its own: the chain must come from
//! the host's standalone factories.
class ReachInfo final : public AbstractModelComponentInfo
{
public:
    ReachInfo() : AbstractModelComponentInfo("org.example.reach") {}
    std::unique_ptr<IModelComponent> createComponentInstance() override { return nullptr; }
};
//! [component]

std::vector<double> values(const Input1DDouble &input) { return input.rawData(); }

} // namespace

int main()
{
    ReachInfo info;
    Reach upstream("upstream", &info);
    Reach downstream("downstream", &info);

    //! [document]
    // Two steps on one connection: x * 2 + 5, then under-relaxation with
    // factor 1 (a passthrough on the first refresh). Step arguments use the
    // same JSON an argument's initialize() accepts.
    const char *document = R"({
      "components": [ { "id": "upstream" }, { "id": "downstream" } ],
      "connections": [
        { "from": { "component": "upstream", "output": "flow",
                    "adapted_outputs": [
                      { "factory": "hydrocouple.sdk.adapters", "id": "linear_transform",
                        "arguments": { "multiplier": [2.0], "offset": { "values": [5.0] } } },
                      { "id": "relaxation",
                        "arguments": { "relaxation_factor": [1.0] } } ] },
          "to": { "component": "downstream", "input": "inflow" } }
      ]
    })";
    //! [document]

    // The initializer is scoped so it is destroyed before the components: it
    // owns the adapters, and they deregister from the upstream output then.
    std::string message;
    {
        //! [build]
        IO::ModelInitializer initializer([&](const std::string &id) -> IModelComponent * {
            if (id == "upstream")
                return &upstream;
            if (id == "downstream")
                return &downstream;
            return nullptr;
        });
        initializer.setAdapterFactoryResolver(
            [] { return std::vector<IAdaptedOutputFactory *>{SdkAdaptedOutputFactory::instance()}; });
        const bool built = initializer.initializeFromJsonText(document, message);

        auto *tail = dynamic_cast<IAdaptedOutput *>(downstream.inflow->provider());
        auto *head = tail ? dynamic_cast<IAdaptedOutput *>(tail->adaptee()) : nullptr;
        std::printf("built: %s; tail %s <- %s <- %s\n", built ? "yes" : message.c_str(),
                    tail ? tail->id().c_str() : "?", head ? head->id().c_str() : "?",
                    head ? head->adaptee()->id().c_str() : "?");
        //! [build]
        check(built && tail && head && head->adaptee() == upstream.flow.get(),
              "downstream.inflow <- relaxation <- linear_transform <- upstream.flow");

        //! [push]
        // A coupled step: the provider's component writes new values, the
        // output refreshes its adapters, the consumer pulls from the tail.
        (*upstream.flow)[0] = 10.0;
        (*upstream.flow)[1] = 20.0;
        (*upstream.flow)[2] = 30.0;
        upstream.flow->refreshAdaptedOutputs();
        downstream.inflow->retrieveValuesFromProvider();
        const std::vector<double> got = values(*downstream.inflow);
        std::printf("downstream.inflow: %g %g %g\n", got[0], got[1], got[2]);
        //! [push]
        check(got == std::vector<double>{25.0, 45.0, 65.0}, "values pass through both steps");

        //! [serialize]
        const nlohmann::json written = initializer.serialize({"upstream", "downstream"}, message);
        const nlohmann::json &steps = written["connections"][0]["from"]["adapted_outputs"];
        std::printf("serialized steps:\n%s\n", steps.dump(2).c_str());
        //! [serialize]
        check(steps.size() == 2 && steps[0]["id"] == "linear_transform" &&
                  steps[0]["factory"] == "hydrocouple.sdk.adapters" &&
                  steps[0]["arguments"]["multiplier"]["values"][0] == 2.0,
              "serialize() writes the offering ids, factory and live arguments");

        //! [unknown]
        Reach a("a", &info), b("b", &info);
        IO::ModelInitializer refusing([&](const std::string &id) -> IModelComponent * {
            return id == "a" ? &a : id == "b" ? &b : nullptr;
        });
        refusing.setAdapterFactoryResolver(
            [] { return std::vector<IAdaptedOutputFactory *>{SdkAdaptedOutputFactory::instance()}; });
        std::string refusal;
        const bool refused = !refusing.initializeFromJsonText(R"({
          "components": [ { "id": "a" }, { "id": "b" } ],
          "connections": [ { "from": { "component": "a", "output": "flow",
                                       "adapted_outputs": [ { "id": "unit_conversion" } ] },
                             "to": { "component": "b", "input": "inflow" } } ] })",
                                                              refusal);
        std::printf("unknown step: %s\n", refusal.c_str());
        //! [unknown]
        check(refused && refusal.find("no adapted output \"unit_conversion\"") != std::string::npos,
              "an unknown offering id is refused with what was searched");
    }

    std::printf("%s\n", failures ? "ch08_composition_chain FAILED" : "ch08_composition_chain passed");
    return failures ? 1 : 0;
}
