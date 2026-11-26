// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Chapter 12 example: read and validate a composition document without any
// model component. CompositionSpec::parse() checks the structure of every
// block; the program prints what it found, stages the argument bindings,
// writes the document back out, and shows the message each kind of broken
// document produces. It returns nonzero when anything differs from what the
// guide states.

#include "hydrocouplesdk/io/compositionspec.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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

//! How an argument payload refers to its value.
std::string payloadKind(const nlohmann::json &payload)
{
    if (isArgumentBinding(payload))
        return "binding";
    if (payload.is_object() && payload.contains("@uri"))
        return "uri";
    if (payload.is_object() && payload.contains("@ref"))
        return "sidecar";
    return "inline";
}

} // namespace

int main()
{
    //! [parse]
    const std::string path =
        std::string(DOCS_EXAMPLE_SOURCE_DIR) + "/composition.json";
    std::ifstream file(path);
    if (!file)
    {
        std::cout << "cannot open " << path << '\n';
        return 1;
    }
    const nlohmann::json document = nlohmann::json::parse(file);

    CompositionSpec spec;
    std::string     message;
    if (!CompositionSpec::parse(document, spec, message))
    {
        std::cout << "invalid composition: " << message << '\n';
        return 1;
    }
    //! [parse]

    //! [inspect]
    std::cout << "schema_version " << spec.schemaVersion << ", composition \""
              << spec.metadata.caption << "\" by " << spec.metadata.author
              << '\n';

    for (const ComponentSpec &component : spec.components)
    {
        std::cout << "component " << component.id << ": mode "
                  << executionModeName(component.mode);
        if (!component.info.empty())
            std::cout << ", library " << component.info.library;
        if (!component.resultsManifest.empty())
            std::cout << ", results " << component.resultsManifest;
        for (const auto &[argument, payload] : component.arguments.items())
            std::cout << ", " << argument << " (" << payloadKind(payload)
                      << ")";
        std::cout << '\n';
    }

    for (const ConnectionSpec &link : spec.connections)
    {
        std::cout << "connection " << link.fromComponent << "." << link.output
                  << " -> " << link.toComponent << "." << link.input;
        for (const AdaptedOutputSpec &step : link.adaptedOutputs)
            std::cout << " via " << step.factory << "/" << step.id;
        std::cout << '\n';
    }

    std::cout << "workflow " << workflowStrategyName(spec.workflow.strategy)
              << " \"" << spec.workflow.id << "\", "
              << spec.workflow.iterationsPerGroup << " iterations per group, "
              << spec.workflow.maxSteps << " steps at most\n";
    for (const WriterSpec &writer : spec.writers)
        std::cout << "writer " << writer.type << " -> " << writer.path
                  << '\n';
    std::cout << "run manifest " << spec.runManifest << '\n';
    //! [inspect]

    check(spec.schemaVersion == "1.1", "schema version");
    check(spec.components.size() == 4, "four components");
    check(spec.component("gauge") &&
              spec.component("gauge")->mode == ExecutionMode::Open,
          "gauge is opened from a run");
    check(spec.connections.size() == 2 &&
              spec.connections[0].adaptedOutputs.size() == 1,
          "one adapted connection");
    check(spec.workflow.strategy == WorkflowStrategy::TimeStepped &&
              spec.workflow.iterationsPerGroup == 2 &&
              spec.workflow.maxSteps == 48,
          "workflow block");
    check(spec.writers.size() == 2 &&
              spec.writers[1].config["simulation"] == "lower-creek",
          "writer blocks keep their extra keys");

    //! [bindings]
    for (const ArgumentBindingSpec &binding : argumentBindings(spec))
        std::cout << "binding " << binding.component << "." << binding.argument
                  << " <- " << binding.provider << "." << binding.output
                  << '\n';

    std::vector<std::vector<std::string>> stages;
    if (!bindingStages(spec, stages, message))
    {
        std::cout << message << '\n';
        return 1;
    }
    for (size_t stage = 0; stage < stages.size(); ++stage)
    {
        std::cout << "stage " << stage << ":";
        for (const std::string &id : stages[stage])
            std::cout << ' ' << id;
        std::cout << '\n';
    }
    //! [bindings]

    check(stages.size() == 2, "two initialization stages");
    check(stages.size() == 2 &&
              stages[1] == std::vector<std::string>{"channel"},
          "the consumer of a binding is initialized last");

    //! [roundtrip]
    const nlohmann::json written = spec.toJson();
    CompositionSpec      reread;
    if (!CompositionSpec::parse(written, reread, message))
    {
        std::cout << "the written document does not parse: " << message
                  << '\n';
        return 1;
    }
    bool same = reread.components.size() == spec.components.size() &&
                reread.connections.size() == spec.connections.size();
    for (size_t i = 0; same && i < spec.components.size(); ++i)
        same = reread.components[i] == spec.components[i];
    for (size_t i = 0; same && i < spec.connections.size(); ++i)
        same = reread.connections[i] == spec.connections[i];
    std::cout << "round trip " << (same ? "preserves" : "changes")
              << " every component and connection\n";
    //! [roundtrip]
    check(same, "round trip");

    //! [version]
    // A document without schema_version is read as 1.0. Writing it back
    // keeps 1.0, unless it uses a 1.1 feature (an argument binding).
    CompositionSpec legacy;
    check(CompositionSpec::parse(
              nlohmann::json::parse(R"({"components": [{"id": "a"}]})"),
              legacy, message),
          "minimal document");
    std::cout << "minimal document: read as " << legacy.schemaVersion
              << ", written as " << legacy.toJson()["schema_version"] << '\n';

    CompositionSpec upgraded;
    check(CompositionSpec::parse(nlohmann::json::parse(R"({
        "components": [
          {"id": "a"},
          {"id": "b", "arguments": {"x": {"@from": {"component": "a",
                                                   "output": "out"}}}}]})"),
                                 upgraded, message),
          "binding without schema_version");
    std::cout << "document with a binding: read as "
              << upgraded.schemaVersion << ", written as "
              << upgraded.toJson()["schema_version"] << '\n';
    //! [version]
    check(legacy.toJson()["schema_version"] == "1.0", "1.0 kept");
    check(upgraded.toJson()["schema_version"] == "1.1", "1.1 forced");

    //! [errors]
    struct Broken
    {
        const char *document;
        const char *expected; // a fragment of the message
    };
    const std::vector<Broken> broken = {
        {R"({"schema_version": "2.0", "components": []})",
         "this SDK reads version 1.x"},
        {R"({"connections": []})", "\"components\" array"},
        {R"({"components": [{"id": "a"}, {"id": "a"}]})",
         "Duplicate component id"},
        {R"({"components": [{"id": "a", "execution": {"mode": "replay"}}]})",
         "execution.mode must be one of"},
        {R"({"components": [{"id": "a", "execution": {"mode": "open"}}]})",
         "requires a \"results\" run manifest path"},
        {R"({"components": [{"id": "a"}], "connections": [
             {"from": {"component": "a", "output": "q"},
              "to": {"component": "ghost", "input": "q"}}]})",
         "is not declared in components[]"},
        {R"({"components": [{"id": "a", "arguments": {"x": {"@from":
             {"component": "a", "output": "q"}}}}]})",
         "to its own output"},
        {R"({"components": [
             {"id": "a", "arguments": {"x": {"@from": {"component": "b", "output": "q"}}}},
             {"id": "b", "arguments": {"y": {"@from": {"component": "a", "output": "q"}}}}]})",
         "form a cycle"},
        {R"({"components": [{"id": "a"}], "workflow": {"strategy": "pull_driven"}})",
         "requires workflow.trigger.component and .input"},
        {R"({"components": [{"id": "a"}], "writers": [{"type": "csv"}]})",
         "requires a \"path\""},
    };
    for (const Broken &entry : broken)
    {
        CompositionSpec rejected;
        const bool parsed = CompositionSpec::parse(
            nlohmann::json::parse(entry.document), rejected, message);
        std::cout << "refused: " << message << '\n';
        check(!parsed && message.find(entry.expected) != std::string::npos,
              std::string("message containing ") + entry.expected);
    }
    //! [errors]

    std::cout << (failures == 0 ? "all checks passed" : "checks failed")
              << '\n';
    return failures == 0 ? 0 : 1;
}
