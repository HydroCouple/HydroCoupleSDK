// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for Composition Specification v1: structural parsing of every
// block, the round trip back to a document, and one negative test per
// diagnosable failure mode — the message must name the offending element.

#include <gtest/gtest.h>

#include "hydrocouplesdk/io/compositionspec.h"

#include <string>

using namespace HydroCouple::SDK::IO;

namespace {

/*!
 * A document exercising every block of the specification.
 */
constexpr const char *kFullDocument = R"({
  "schema_version": "1.0",
  "metadata": {
    "id": "study-1",
    "caption": "Study 1",
    "author": "Caleb Buahin",
    "sdk_version": "2.1.0"
  },
  "components": [
    { "id": "catchment",
      "caption": "Catchment",
      "info": { "library": "libcatchment.so",
                "component_info_id": "CatchmentInfo" },
      "arguments": { "coefficients": { "values": [0.3, 0.2] } } },
    { "id": "channel",
      "execution": { "mode": "open", "results": "prior/run.json" },
      "arguments": {} }
  ],
  "connections": [
    { "from": { "component": "catchment", "output": "runoff",
                "adapted_outputs": [
                  { "factory": "TemporalFactory", "id": "linear",
                    "arguments": { "method": { "value": "linear" } } }
                ] },
      "to":   { "component": "channel", "input": "lateral_inflow",
                "role": "upstream" } }
  ],
  "workflow": {
    "strategy": "time_stepped",
    "id": "wf",
    "iterations_per_group": 3,
    "max_steps": 50
  },
  "writers": [
    { "type": "hdf5_ugrid", "path": "out/results.h5", "mesh": "reaches" },
    { "type": "geopackage", "path": "out/results.gpkg",
      "simulation": "run-1",
      "fields": [ { "item": "flow", "units": "m3/s" } ] }
  ],
  "run": { "manifest": "out/run.json" }
})";

CompositionSpec parseOrFail(const char *text)
{
    CompositionSpec spec;
    std::string     message;
    EXPECT_TRUE(CompositionSpec::parse(nlohmann::json::parse(text), spec,
                                       message))
        << message;
    return spec;
}

std::string parseError(const char *text)
{
    CompositionSpec spec;
    std::string     message;
    EXPECT_FALSE(CompositionSpec::parse(nlohmann::json::parse(text), spec,
                                        message));
    return message;
}

} // namespace

// ── Parsing ───────────────────────────────────────────────────────────────

TEST(CompositionSpecTest, ParsesEveryBlock)
{
    const CompositionSpec spec = parseOrFail(kFullDocument);

    EXPECT_EQ(spec.schemaVersion, "1.0");
    EXPECT_EQ(spec.metadata.id, "study-1");
    EXPECT_EQ(spec.metadata.author, "Caleb Buahin");
    EXPECT_FALSE(spec.metadata.empty());

    ASSERT_EQ(spec.components.size(), 2u);
    const ComponentSpec &catchment = spec.components[0];
    EXPECT_EQ(catchment.id, "catchment");
    EXPECT_EQ(catchment.caption, "Catchment");
    EXPECT_EQ(catchment.info.library, "libcatchment.so");
    EXPECT_EQ(catchment.info.componentInfoId, "CatchmentInfo");
    EXPECT_EQ(catchment.mode, ExecutionMode::Run);
    EXPECT_TRUE(catchment.arguments.contains("coefficients"));

    const ComponentSpec &channel = spec.components[1];
    EXPECT_EQ(channel.mode, ExecutionMode::Open);
    EXPECT_EQ(channel.resultsManifest, "prior/run.json");
    EXPECT_TRUE(channel.info.empty());

    ASSERT_EQ(spec.connections.size(), 1u);
    const ConnectionSpec &link = spec.connections[0];
    EXPECT_EQ(link.fromComponent, "catchment");
    EXPECT_EQ(link.output, "runoff");
    EXPECT_EQ(link.toComponent, "channel");
    EXPECT_EQ(link.input, "lateral_inflow");
    EXPECT_EQ(link.role, "upstream");
    ASSERT_EQ(link.adaptedOutputs.size(), 1u);
    EXPECT_EQ(link.adaptedOutputs[0].factory, "TemporalFactory");
    EXPECT_EQ(link.adaptedOutputs[0].id, "linear");
    EXPECT_TRUE(link.adaptedOutputs[0].arguments.contains("method"));

    EXPECT_EQ(spec.workflow.strategy, WorkflowStrategy::TimeStepped);
    EXPECT_EQ(spec.workflow.id, "wf");
    EXPECT_EQ(spec.workflow.iterationsPerGroup, 3);
    EXPECT_EQ(spec.workflow.maxSteps, 50);

    ASSERT_EQ(spec.writers.size(), 2u);
    EXPECT_EQ(spec.writers[0].type, "hdf5_ugrid");
    EXPECT_EQ(spec.writers[0].path, "out/results.h5");
    EXPECT_EQ(spec.writers[1].config["simulation"], "run-1");

    EXPECT_EQ(spec.runManifest, "out/run.json");
    ASSERT_NE(spec.component("channel"), nullptr);
    EXPECT_EQ(spec.component("missing"), nullptr);
}

TEST(CompositionSpecTest, MinimalDocumentStillParses)
{
    // Documents predating the specification carry only these two keys.
    const CompositionSpec spec = parseOrFail(R"({
        "components": [ { "id": "a" } ],
        "connections": []
    })");

    EXPECT_EQ(spec.schemaVersion, "1.0"); // defaulted
    EXPECT_TRUE(spec.metadata.empty());
    EXPECT_EQ(spec.workflow.strategy, WorkflowStrategy::None);
    EXPECT_TRUE(spec.writers.empty());
    EXPECT_TRUE(spec.runManifest.empty());
    EXPECT_EQ(spec.components[0].mode, ExecutionMode::Run);
}

TEST(CompositionSpecTest, RoundTripsThroughJson)
{
    const CompositionSpec first = parseOrFail(kFullDocument);

    CompositionSpec second;
    std::string     message;
    ASSERT_TRUE(CompositionSpec::parse(first.toJson(), second, message))
        << message;

    // Re-emitting the re-parsed spec reproduces the same document.
    EXPECT_EQ(first.toJson(), second.toJson());

    EXPECT_EQ(second.components[1].mode, ExecutionMode::Open);
    EXPECT_EQ(second.connections[0].adaptedOutputs[0].id, "linear");
    EXPECT_EQ(second.workflow.iterationsPerGroup, 3);
    EXPECT_EQ(second.writers.size(), 2u);
    EXPECT_EQ(second.runManifest, "out/run.json");
}

// ── Enum spellings ────────────────────────────────────────────────────────

TEST(CompositionSpecTest, EnumNamesRoundTrip)
{
    for (ExecutionMode mode : {ExecutionMode::Run, ExecutionMode::Open,
                               ExecutionMode::Resume})
    {
        ExecutionMode parsed{};
        ASSERT_TRUE(executionModeFromName(executionModeName(mode), parsed));
        EXPECT_EQ(parsed, mode);
    }
    ExecutionMode unused{};
    EXPECT_FALSE(executionModeFromName("sideways", unused));

    for (WorkflowStrategy strategy :
         {WorkflowStrategy::None, WorkflowStrategy::PullDriven,
          WorkflowStrategy::TimeStepped})
    {
        WorkflowStrategy parsed{};
        ASSERT_TRUE(
            workflowStrategyFromName(workflowStrategyName(strategy), parsed));
        EXPECT_EQ(parsed, strategy);
    }
    WorkflowStrategy unusedStrategy{};
    EXPECT_FALSE(workflowStrategyFromName("chaotic", unusedStrategy));
}

// ── Negative cases: every message names the offending element ─────────────

TEST(CompositionSpecTest, RejectsUnsupportedSchemaVersion)
{
    const std::string message = parseError(
        R"({"schema_version": "2.0", "components": []})");
    EXPECT_NE(message.find("2.0"), std::string::npos);
    EXPECT_NE(message.find("1.x"), std::string::npos);
}

TEST(CompositionSpecTest, RejectsMissingComponentsArray)
{
    EXPECT_NE(parseError(R"({"connections": []})").find("components"),
              std::string::npos);
}

TEST(CompositionSpecTest, RejectsDuplicateComponentIds)
{
    const std::string message = parseError(
        R"({"components": [{"id": "a"}, {"id": "a"}]})");
    EXPECT_NE(message.find("Duplicate"), std::string::npos);
    EXPECT_NE(message.find("\"a\""), std::string::npos);
}

TEST(CompositionSpecTest, RejectsUnknownExecutionMode)
{
    const std::string message = parseError(R"({
        "components": [ { "id": "a", "execution": { "mode": "maybe" } } ]
    })");
    EXPECT_NE(message.find("maybe"), std::string::npos);
    EXPECT_NE(message.find("execution.mode"), std::string::npos);
}

TEST(CompositionSpecTest, RejectsOpenModeWithoutResultsManifest)
{
    const std::string message = parseError(R"({
        "components": [ { "id": "a", "execution": { "mode": "open" } } ]
    })");
    EXPECT_NE(message.find("results"), std::string::npos);
}

TEST(CompositionSpecTest, RejectsConnectionToUndeclaredComponent)
{
    const std::string message = parseError(R"({
        "components": [ { "id": "a" } ],
        "connections": [ { "from": { "component": "a", "output": "o" },
                           "to":   { "component": "ghost", "input": "i" } } ]
    })");
    EXPECT_NE(message.find("ghost"), std::string::npos);
    EXPECT_NE(message.find("not declared"), std::string::npos);
}

TEST(CompositionSpecTest, RejectsAdaptedOutputWithoutId)
{
    const std::string message = parseError(R"({
        "components": [ { "id": "a" }, { "id": "b" } ],
        "connections": [ { "from": { "component": "a", "output": "o",
                                     "adapted_outputs": [ { "factory": "f" } ] },
                           "to":   { "component": "b", "input": "i" } } ]
    })");
    EXPECT_NE(message.find("adapted_outputs[0]"), std::string::npos);
}

TEST(CompositionSpecTest, RejectsUnknownWorkflowStrategy)
{
    const std::string message = parseError(R"({
        "components": [],
        "workflow": { "strategy": "vibes" }
    })");
    EXPECT_NE(message.find("vibes"), std::string::npos);
}

TEST(CompositionSpecTest, RejectsPullDrivenWorkflowWithoutTrigger)
{
    const std::string message = parseError(R"({
        "components": [ { "id": "a" } ],
        "workflow": { "strategy": "pull_driven" }
    })");
    EXPECT_NE(message.find("trigger"), std::string::npos);
}

TEST(CompositionSpecTest, RejectsTriggerOnUndeclaredComponent)
{
    const std::string message = parseError(R"({
        "components": [ { "id": "a" } ],
        "workflow": { "strategy": "pull_driven",
                      "trigger": { "component": "ghost", "input": "i" } }
    })");
    EXPECT_NE(message.find("ghost"), std::string::npos);
}

TEST(CompositionSpecTest, RejectsBadIterationCount)
{
    const std::string message = parseError(R"({
        "components": [],
        "workflow": { "strategy": "time_stepped", "iterations_per_group": 0 }
    })");
    EXPECT_NE(message.find("iterations_per_group"), std::string::npos);
}

TEST(CompositionSpecTest, RejectsWriterWithoutPath)
{
    const std::string message = parseError(R"({
        "components": [],
        "writers": [ { "type": "csv" } ]
    })");
    EXPECT_NE(message.find("path"), std::string::npos);
    EXPECT_NE(message.find("csv"), std::string::npos);
}

TEST(CompositionSpecTest, RejectsMistypedFields)
{
    EXPECT_NE(parseError(R"({"components": [], "metadata": 7})").find("metadata"),
              std::string::npos);
    EXPECT_NE(parseError(R"({"components": [{"id": "a", "arguments": 3}]})")
                  .find("arguments"),
              std::string::npos);
    EXPECT_NE(parseError(R"({"components": [], "writers": {}})").find("writers"),
              std::string::npos);
}

// ── Argument bindings (spec v1.1) ─────────────────────────────────────────

namespace {

/*!
 * A mesh generator whose output initializes a solver's mesh argument, plus
 * a bystander with no bindings at all.
 */
constexpr const char *kBoundDocument = R"({
  "schema_version": "1.1",
  "components": [
    { "id": "solver",
      "arguments": {
        "mesh": { "@from": { "component": "meshgen", "output": "mesh",
                             "selector": null } },
        "dt":   { "value": 60.0 }
      } },
    { "id": "meshgen", "arguments": {} },
    { "id": "bystander", "arguments": {} }
  ]
})";

} // namespace

TEST(CompositionSpecTest, ParsesAnArgumentBindingAndDerivesItsTypedForm)
{
    const CompositionSpec spec = parseOrFail(kBoundDocument);

    const ComponentSpec *solver = spec.component("solver");
    ASSERT_NE(solver, nullptr);
    EXPECT_TRUE(isArgumentBinding(solver->arguments["mesh"]));
    EXPECT_FALSE(isArgumentBinding(solver->arguments["dt"]));

    const std::vector<ArgumentBindingSpec> bindings = argumentBindings(spec);
    ASSERT_EQ(bindings.size(), 1u);
    EXPECT_EQ(bindings[0].component, "solver");
    EXPECT_EQ(bindings[0].argument, "mesh");
    EXPECT_EQ(bindings[0].provider, "meshgen");
    EXPECT_EQ(bindings[0].output, "mesh");
}

TEST(CompositionSpecTest, ABindingRoundTripsAsTheReferenceItIs)
{
    const CompositionSpec spec     = parseOrFail(kBoundDocument);
    const nlohmann::json  document = spec.toJson();

    const nlohmann::json &payload =
        document["components"][0]["arguments"]["mesh"];
    ASSERT_TRUE(payload.contains("@from"));
    EXPECT_EQ(payload["@from"]["component"], "meshgen");
    EXPECT_EQ(payload["@from"]["output"], "mesh");
}

TEST(CompositionSpecTest, AOneOhDocumentWithoutBindingsKeepsItsVersion)
{
    const CompositionSpec spec = parseOrFail(kFullDocument);
    EXPECT_EQ(spec.toJson()["schema_version"], "1.0");
}

TEST(CompositionSpecTest, ABindingUpgradesAOneOhDocumentToOneOne)
{
    // A 1.0 claim on a document that uses 1.1 syntax parses (same major),
    // but must not survive the round trip: a strict 1.0 reader would be
    // told it can read what it cannot.
    const CompositionSpec spec = parseOrFail(R"({
      "schema_version": "1.0",
      "components": [
        { "id": "a",
          "arguments": { "x": { "@from": { "component": "b",
                                           "output": "y" } } } },
        { "id": "b" }
      ]
    })");
    EXPECT_EQ(spec.toJson()["schema_version"], "1.1");
}

TEST(CompositionSpecTest, RejectsABindingToAnUndeclaredProvider)
{
    const std::string message = parseError(R"({
      "components": [
        { "id": "solver",
          "arguments": { "mesh": { "@from": { "component": "ghost",
                                              "output": "mesh" } } } }
      ]
    })");
    EXPECT_NE(message.find("ghost"), std::string::npos) << message;
    EXPECT_NE(message.find("not declared"), std::string::npos) << message;
}

TEST(CompositionSpecTest, RejectsABindingToItself)
{
    const std::string message = parseError(R"({
      "components": [
        { "id": "solver",
          "arguments": { "mesh": { "@from": { "component": "solver",
                                              "output": "mesh" } } } }
      ]
    })");
    EXPECT_NE(message.find("its own output"), std::string::npos) << message;
}

TEST(CompositionSpecTest, RejectsASelectorValueAsNotYetImplemented)
{
    const std::string message = parseError(R"({
      "components": [
        { "id": "solver",
          "arguments": { "mesh": { "@from": { "component": "meshgen",
                                              "output": "mesh",
                                              "selector": "cell[0]" } } } },
        { "id": "meshgen" }
      ]
    })");
    EXPECT_NE(message.find("selector"), std::string::npos) << message;
    EXPECT_NE(message.find("not yet implemented"), std::string::npos)
        << message;
}

TEST(CompositionSpecTest, RejectsABindingCarryingASecondKey)
{
    const std::string message = parseError(R"({
      "components": [
        { "id": "solver",
          "arguments": { "mesh": { "@from": { "component": "meshgen",
                                              "output": "mesh" },
                                   "units": "m" } } },
        { "id": "meshgen" }
      ]
    })");
    EXPECT_NE(message.find("only"), std::string::npos) << message;
}

TEST(CompositionSpecTest, RejectsABindingThatNamesNoOutput)
{
    const std::string message = parseError(R"({
      "components": [
        { "id": "solver",
          "arguments": { "mesh": { "@from": { "component": "meshgen" } } } },
        { "id": "meshgen" }
      ]
    })");
    EXPECT_NE(message.find("output"), std::string::npos) << message;
}

TEST(CompositionSpecTest, RejectsABindingCycleNamingItsEdges)
{
    const std::string message = parseError(R"({
      "components": [
        { "id": "a",
          "arguments": { "x": { "@from": { "component": "b",
                                           "output": "out" } } } },
        { "id": "b",
          "arguments": { "y": { "@from": { "component": "a",
                                           "output": "out" } } } }
      ]
    })");
    EXPECT_NE(message.find("cycle"), std::string::npos) << message;
    EXPECT_NE(message.find("\"a\""), std::string::npos) << message;
    EXPECT_NE(message.find("\"b\""), std::string::npos) << message;
}

TEST(CompositionSpecTest, StagesPutEveryProviderBeforeItsConsumers)
{
    // Declared consumer-first on purpose: staging orders initialization,
    // not the document.
    const CompositionSpec spec = parseOrFail(R"({
      "components": [
        { "id": "c",
          "arguments": { "x": { "@from": { "component": "b",
                                           "output": "out" } } } },
        { "id": "b",
          "arguments": { "x": { "@from": { "component": "a",
                                           "output": "out" } } } },
        { "id": "a" },
        { "id": "d" }
      ]
    })");

    std::vector<std::vector<std::string>> stages;
    std::string message;
    ASSERT_TRUE(bindingStages(spec, stages, message)) << message;
    ASSERT_EQ(stages.size(), 3u);
    EXPECT_EQ(stages[0], (std::vector<std::string>{"a", "d"}));
    EXPECT_EQ(stages[1], (std::vector<std::string>{"b"}));
    EXPECT_EQ(stages[2], (std::vector<std::string>{"c"}));
}

TEST(CompositionSpecTest, ABindingFreeCompositionIsOneStageInDocumentOrder)
{
    const CompositionSpec spec = parseOrFail(kFullDocument);

    std::vector<std::vector<std::string>> stages;
    std::string message;
    ASSERT_TRUE(bindingStages(spec, stages, message)) << message;
    ASSERT_EQ(stages.size(), 1u);
    EXPECT_EQ(stages[0],
              (std::vector<std::string>{"catchment", "channel"}));
}
