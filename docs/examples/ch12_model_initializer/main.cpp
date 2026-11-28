// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Chapter 12 example: apply a composition document to live components with
// ModelInitializer. The document (composition.json, and composition.yaml for
// the YAML path) uses an inline argument, an @uri reference to a file beside
// the document, an @uri reference a host-installed UriResolver serves, an
// @from binding to another component's output, and a connection through an
// adapted output. The program checks every value it received and writes the
// composition back out. It returns nonzero when anything differs from what
// the guide states.

#include "components.h"

#include "hydrocouplesdk/data/sdkadaptedoutputfactory.h"
#include "hydrocouplesdk/io/modelinitializer.h"
#include "hydrocouplesdk/io/uriresolver.h"

#include <filesystem>
#include <iostream>
#include <sstream>
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

std::vector<double> values(Argument1DDouble &argument)
{
    std::vector<double> result;
    for (int i = 0; i < argument.length(); ++i)
        result.push_back(argument[i]);
    return result;
}

std::string text(const std::vector<double> &numbers)
{
    std::ostringstream result;
    result << '[';
    for (size_t i = 0; i < numbers.size(); ++i)
        result << (i ? ", " : "") << numbers[i];
    result << ']';
    return result.str();
}

//! [resolver]
// A host-side resolver for "memory:" URIs. A real host installs one backed
// by an HTTP client or a data service; the SDK only defines the seam.
class MemoryResolver final : public UriResolver
{
public:
    bool handles(const std::string &scheme) const override
    {
        return scheme == "memory";
    }
    bool resolve(const std::string &uri, std::string &contents,
                 std::string &message) override
    {
        if (uri == "memory:weights")
        {
            contents = "[0.5, 0.25, 0.25]";
            return true;
        }
        message = "nothing stored under " + uri;
        return false;
    }
};
//! [resolver]

//! [instances]
// The host owns the components; the initializer asks for them by id.
struct Instances
{
    example::Producer profile{"profile"};
    example::Producer upstream{"upstream"};
    example::Consumer channel{"channel"};

    ModelInitializer::ComponentResolver resolver()
    {
        return [this](const std::string &id) -> IModelComponent * {
            if (id == "profile")  return &profile;
            if (id == "upstream") return &upstream;
            if (id == "channel")  return &channel;
            return nullptr;
        };
    }
};
//! [instances]

void checkApplied(Instances &live, ModelInitializer &initializer,
                  const std::string &label)
{
    // The bound provider ran to Finished: three steps of scale * step.
    check(live.profile.status() == IModelComponent::ComponentStatus::Finished,
          label + ": the bound provider ran to Finished");
    check(values(*live.channel.initial) ==
              std::vector<double>{30.0, 60.0, 90.0},
          label + ": initial holds the provider's last series");
    check(values(*live.upstream.scale) == std::vector<double>{2.0, 4.0, 6.0},
          label + ": scale read from scale.json beside the document");
    check(values(*live.channel.weights) ==
              std::vector<double>{0.5, 0.25, 0.25},
          label + ": weights served by the installed resolver");
    check(live.upstream.status() ==
              IModelComponent::ComponentStatus::Initialized,
          label + ": a component without bindings stops at Initialized");

    auto *tail =
        dynamic_cast<IAdaptedOutput *>(live.channel.inflow->provider());
    check(tail && tail->adaptee() ==
                      static_cast<IOutput *>(live.upstream.series),
          label + ": inflow is wired to the adapted output of upstream");
    check(initializer.adaptedOutputs().size() == 1,
          label + ": the initializer owns one adapted output");
}

} // namespace

int main()
{
    const std::filesystem::path source = DOCS_EXAMPLE_SOURCE_DIR;

    //! [install]
    MemoryResolver memory;
    setUriResolver(&memory); // before any component is initialized
    //! [install]

    //! [initialize]
    Instances live;
    ModelInitializer initializer(live.resolver());
    initializer.setAdapterFactoryResolver(
        [] { return std::vector<IAdaptedOutputFactory *>{
                 SdkAdaptedOutputFactory::instance()}; });
    initializer.setProgressCallback(
        [](int stage, int stageCount, const std::string &component,
           const std::string &activity) {
            std::cout << "stage " << stage + 1 << "/" << stageCount << ": "
                      << activity << " " << component << '\n';
        });

    std::string message;
    if (!initializer.initializeFromFile(source / "composition.json", message))
    {
        std::cout << "composition failed: " << message << '\n';
        return 1;
    }
    //! [initialize]

    //! [inspect]
    std::cout << "profile is " << (live.profile.status() ==
                                           IModelComponent::ComponentStatus::Finished
                                       ? "Finished"
                                       : "not finished")
              << "; channel.initial = " << text(values(*live.channel.initial))
              << '\n'
              << "upstream.scale = " << text(values(*live.upstream.scale))
              << " (from scale.json)\n"
              << "channel.weights = " << text(values(*live.channel.weights))
              << " (from memory:weights)\n"
              << "channel.inflow provider: "
              << live.channel.inflow->provider()->id() << '\n';

    // Blocks the initializer carries but does not act on.
    const CompositionSpec &spec = initializer.spec();
    std::cout << "host must build: workflow "
              << workflowStrategyName(spec.workflow.strategy) << ", "
              << spec.writers.size() << " writer(s), run manifest "
              << spec.runManifest << '\n';
    //! [inspect]
    checkApplied(live, initializer, "json");
    check(spec.writers.size() == 1 && spec.runManifest == "out/run.json",
          "spec() carries writers and run");

    //! [validate]
    nlohmann::json misspelled = spec.toJson();
    misspelled["components"][2]["arguments"]["friction"] = {
        {"values", {0.03}}};
    if (!initializer.validateDocument(misspelled, message))
        std::cout << "validateDocument: " << message << '\n';
    //! [validate]
    check(message == "Component \"channel\" has no argument \"friction\"",
          "validateDocument names the unknown argument");

    //! [serialize]
    if (!initializer.serializeToFile({"profile", "upstream", "channel"},
                                     "composition.out.json", message))
    {
        std::cout << "serialization failed: " << message << '\n';
        return 1;
    }
    const nlohmann::json out =
        initializer.serialize({"profile", "upstream", "channel"}, message);
    std::cout << "written back: channel.initial = "
              << out["components"][2]["arguments"]["initial"].dump()
              << "\n              channel.weights @uri = "
              << out["components"][2]["arguments"]["weights"]["@uri"]
              << "\n              upstream.scale values = "
              << out["components"][1]["arguments"]["scale"]["values"].dump()
              << "\n              chain step = "
              << out["connections"][0]["from"]["adapted_outputs"][0]["id"]
              << '\n';
    //! [serialize]
    check(isArgumentBinding(out["components"][2]["arguments"]["initial"]),
          "the binding is written back as a binding");
    check(out["components"][2]["arguments"]["weights"]["@uri"] ==
              "memory:weights",
          "a remote source is written back as its URI");
    check(!out["components"][1]["arguments"]["scale"].contains("@uri") &&
              out["components"][1]["arguments"]["scale"]["values"] ==
                  nlohmann::json::parse("[2.0, 4.0, 6.0]"),
          "a local file is written back as its values");

    //! [yaml]
    if (ModelInitializer::yamlSupported())
    {
        Instances fromYaml;
        ModelInitializer yaml(fromYaml.resolver());
        yaml.setAdapterFactoryResolver(
            [] { return std::vector<IAdaptedOutputFactory *>{
                     SdkAdaptedOutputFactory::instance()}; });
        if (!yaml.initializeFromFile(source / "composition.yaml", message))
        {
            std::cout << "YAML composition failed: " << message << '\n';
            return 1;
        }
        std::cout << "composition.yaml applied the same values: "
                  << text(values(*fromYaml.channel.initial)) << '\n';
        checkApplied(fromYaml, yaml, "yaml");
    }
    //! [yaml]

    setUriResolver(nullptr);
    std::cout << (failures == 0 ? "all checks passed" : "checks failed")
              << '\n';
    return failures == 0 ? 0 : 1;
}
