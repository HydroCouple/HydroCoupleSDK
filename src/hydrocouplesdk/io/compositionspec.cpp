// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/io/compositionspec.h"

#include <map>
#include <set>

namespace HydroCouple::SDK::IO {

namespace {

/*!
 * Reads an optional string field; false (with a message) when present but
 * of the wrong type.
 */
bool optionalString(const nlohmann::json &object, const char *key,
                    const std::string &context, std::string &out,
                    std::string &message)
{
    if (!object.contains(key) || object[key].is_null())
        return true;
    if (!object[key].is_string())
    {
        message = context + "." + key + " must be a string";
        return false;
    }
    out = object[key].get<std::string>();
    return true;
}

/*!
 * Reads an optional object field into \p out; false when present but not
 * an object.
 */
bool optionalObject(const nlohmann::json &object, const char *key,
                    const std::string &context, nlohmann::json &out,
                    std::string &message)
{
    if (!object.contains(key) || object[key].is_null())
        return true;
    if (!object[key].is_object())
    {
        message = context + "." + key + " must be an object";
        return false;
    }
    out = object[key];
    return true;
}

/*!
 * Validates one \c @from payload. \p seen must hold every declared
 * component id — providers may be declared after their consumers, since
 * staging orders initialization, not the document.
 */
bool validateBinding(const nlohmann::json &payload, const std::string &context,
                     const std::string &consumerId,
                     const std::set<std::string> &seen, std::string &message)
{
    if (payload.size() != 1)
    {
        message = context + " is a binding, so \"@from\" must be its only "
                            "key — a reference carries no other payload";
        return false;
    }
    const auto &from = payload[kArgumentBindingKey];
    if (!from.is_object())
    {
        message = context + ".@from must be an object with string "
                            "\"component\" and \"output\"";
        return false;
    }
    for (const auto &[key, value] : from.items())
    {
        if (key == "component" || key == "output")
        {
            if (!value.is_string() || value.get<std::string>().empty())
            {
                message = context + ".@from." + key +
                          " must be a non-empty string";
                return false;
            }
            continue;
        }
        if (key == "selector")
        {
            if (!value.is_null())
            {
                message = context + ".@from.selector is reserved and not "
                                    "yet implemented; bind the whole output";
                return false;
            }
            continue;
        }
        message = context + ".@from carries unknown key \"" + key + "\"";
        return false;
    }
    if (!from.contains("component") || !from.contains("output"))
    {
        message = context + ".@from must name a \"component\" and an "
                            "\"output\"";
        return false;
    }

    const std::string provider = from["component"].get<std::string>();
    if (!seen.count(provider))
    {
        message = context + ".@from.component \"" + provider +
                  "\" is not declared in components[]";
        return false;
    }
    if (provider == consumerId)
    {
        message = context + ".@from binds \"" + consumerId +
                  "\" to its own output";
        return false;
    }
    return true;
}

} // namespace

// ── Enum names ────────────────────────────────────────────────────────────

std::string executionModeName(ExecutionMode mode)
{
    switch (mode)
    {
        case ExecutionMode::Run:    return "run";
        case ExecutionMode::Open:   return "open";
        case ExecutionMode::Resume: return "resume";
    }
    return "run";
}

bool executionModeFromName(const std::string &name, ExecutionMode &mode)
{
    if (name == "run")    { mode = ExecutionMode::Run;    return true; }
    if (name == "open")   { mode = ExecutionMode::Open;   return true; }
    if (name == "resume") { mode = ExecutionMode::Resume; return true; }
    return false;
}

std::string workflowStrategyName(WorkflowStrategy strategy)
{
    switch (strategy)
    {
        case WorkflowStrategy::None:        return "none";
        case WorkflowStrategy::PullDriven:  return "pull_driven";
        case WorkflowStrategy::TimeStepped: return "time_stepped";
    }
    return "none";
}

bool workflowStrategyFromName(const std::string &name,
                              WorkflowStrategy &strategy)
{
    if (name == "none")
    {
        strategy = WorkflowStrategy::None;
        return true;
    }
    if (name == "pull_driven")
    {
        strategy = WorkflowStrategy::PullDriven;
        return true;
    }
    if (name == "time_stepped")
    {
        strategy = WorkflowStrategy::TimeStepped;
        return true;
    }
    return false;
}

// ── Small value types ─────────────────────────────────────────────────────

bool CompositionMetadata::empty() const noexcept
{
    return id.empty() && caption.empty() && description.empty() &&
           author.empty() && created.empty() && sdkVersion.empty() &&
           interfaceVersion.empty();
}

bool ComponentInfoRef::empty() const noexcept
{
    return library.empty() && componentInfoId.empty();
}

// ── Parsing ───────────────────────────────────────────────────────────────

bool CompositionSpec::parse(const nlohmann::json &document,
                            CompositionSpec &spec, std::string &message)
{
    spec = CompositionSpec{};

    if (!document.is_object())
    {
        message = "Composition document must be a JSON object";
        return false;
    }

    // schema_version: optional for documents predating the specification —
    // those are 1.0-era documents, not documents in whatever version this
    // SDK happens to write. When present the MAJOR component must be one
    // this SDK understands.
    spec.schemaVersion = "1.0";
    if (document.contains("schema_version"))
    {
        if (!document["schema_version"].is_string())
        {
            message = "\"schema_version\" must be a string (e.g. \"1.0\")";
            return false;
        }
        spec.schemaVersion = document["schema_version"].get<std::string>();
        const std::string major =
            spec.schemaVersion.substr(0, spec.schemaVersion.find('.'));
        if (major != "1")
        {
            message = "Unsupported composition schema_version \"" +
                      spec.schemaVersion + "\"; this SDK reads version 1.x";
            return false;
        }
    }

    // ── metadata ─────────────────────────────────────────────────────────
    if (document.contains("metadata"))
    {
        if (!document["metadata"].is_object())
        {
            message = "\"metadata\" must be an object";
            return false;
        }
        const auto &meta = document["metadata"];
        if (!optionalString(meta, "id", "metadata", spec.metadata.id, message) ||
            !optionalString(meta, "caption", "metadata", spec.metadata.caption,
                            message) ||
            !optionalString(meta, "description", "metadata",
                            spec.metadata.description, message) ||
            !optionalString(meta, "author", "metadata", spec.metadata.author,
                            message) ||
            !optionalString(meta, "created", "metadata", spec.metadata.created,
                            message) ||
            !optionalString(meta, "sdk_version", "metadata",
                            spec.metadata.sdkVersion, message) ||
            !optionalString(meta, "interface_version", "metadata",
                            spec.metadata.interfaceVersion, message))
            return false;
    }

    // ── components ───────────────────────────────────────────────────────
    if (!document.contains("components") || !document["components"].is_array())
    {
        message = "Composition document must be an object with a "
                  "\"components\" array";
        return false;
    }

    std::set<std::string> seen;
    int index = 0;
    for (const auto &block : document["components"])
    {
        const std::string context = "components[" + std::to_string(index) + "]";
        if (!block.is_object() || !block.contains("id") ||
            !block["id"].is_string())
        {
            message = "Every components[] entry must be an object with a "
                      "string \"id\"";
            return false;
        }

        ComponentSpec component;
        component.id = block["id"].get<std::string>();
        if (!seen.insert(component.id).second)
        {
            message = "Duplicate component id \"" + component.id +
                      "\" in components[]";
            return false;
        }
        const std::string named = "components[\"" + component.id + "\"]";

        if (!optionalString(block, "caption", named, component.caption,
                            message))
            return false;

        // info: instantiation hint
        nlohmann::json info = nlohmann::json::object();
        if (!optionalObject(block, "info", named, info, message))
            return false;
        if (!optionalString(info, "library", named + ".info",
                            component.info.library, message) ||
            !optionalString(info, "component_info_id", named + ".info",
                            component.info.componentInfoId, message))
            return false;

        // execution: mode + results manifest
        nlohmann::json execution = nlohmann::json::object();
        if (!optionalObject(block, "execution", named, execution, message))
            return false;
        std::string modeName;
        if (!optionalString(execution, "mode", named + ".execution", modeName,
                            message))
            return false;
        if (!modeName.empty() &&
            !executionModeFromName(modeName, component.mode))
        {
            message = named + ".execution.mode must be one of \"run\", "
                              "\"open\", \"resume\" (got \"" + modeName + "\")";
            return false;
        }
        if (!optionalString(execution, "results", named + ".execution",
                            component.resultsManifest, message))
            return false;
        if (component.mode == ExecutionMode::Open &&
            component.resultsManifest.empty())
        {
            message = named + ".execution: mode \"open\" requires a "
                              "\"results\" run manifest path";
            return false;
        }

        // arguments
        if (block.contains("arguments") && !block["arguments"].is_null())
        {
            if (!block["arguments"].is_object())
            {
                message = named + ".arguments must be an object of "
                                  "argumentId → payload";
                return false;
            }
            component.arguments = block["arguments"];
        }

        spec.components.push_back(std::move(component));
        ++index;
    }

    // ── argument bindings ────────────────────────────────────────────────
    // Validated after the whole components[] pass because a provider may
    // legitimately be declared after its consumer — staging orders
    // initialization, not the document.
    for (const ComponentSpec &component : spec.components)
    {
        for (const auto &[argumentId, payload] : component.arguments.items())
        {
            if (!isArgumentBinding(payload))
                continue;
            const std::string context = "components[\"" + component.id +
                                        "\"].arguments." + argumentId;
            if (!validateBinding(payload, context, component.id, seen,
                                 message))
                return false;
        }
    }
    {
        std::vector<std::vector<std::string>> stages;
        if (!bindingStages(spec, stages, message))
            return false;
    }

    // ── connections ──────────────────────────────────────────────────────
    if (document.contains("connections") && !document["connections"].is_null())
    {
        if (!document["connections"].is_array())
        {
            message = "\"connections\" must be an array";
            return false;
        }
        index = 0;
        for (const auto &connection : document["connections"])
        {
            const std::string context =
                "connections[" + std::to_string(index) + "]";
            if (!connection.is_object())
            {
                message = context + " must be an object";
                return false;
            }
            for (const char *end : {"from", "to"})
            {
                if (!connection.contains(end) || !connection[end].is_object() ||
                    !connection[end].contains("component") ||
                    !connection[end]["component"].is_string())
                {
                    message = context +
                              " must carry \"from\"/\"to\" objects with a "
                              "string \"component\" id";
                    return false;
                }
            }

            ConnectionSpec link;
            const auto &from = connection["from"];
            const auto &to   = connection["to"];
            link.fromComponent = from["component"].get<std::string>();
            link.toComponent   = to["component"].get<std::string>();
            if (!optionalString(from, "output", context + ".from", link.output,
                                message) ||
                !optionalString(to, "input", context + ".to", link.input,
                                message) ||
                !optionalString(to, "role", context + ".to", link.role,
                                message))
                return false;

            if (!seen.count(link.fromComponent))
            {
                message = context + ".from.component \"" + link.fromComponent +
                          "\" is not declared in components[]";
                return false;
            }
            if (!seen.count(link.toComponent))
            {
                message = context + ".to.component \"" + link.toComponent +
                          "\" is not declared in components[]";
                return false;
            }

            if (from.contains("adapted_outputs") &&
                !from["adapted_outputs"].is_null())
            {
                if (!from["adapted_outputs"].is_array())
                {
                    message = context + ".from.adapted_outputs must be an array";
                    return false;
                }
                int chainIndex = 0;
                for (const auto &adapted : from["adapted_outputs"])
                {
                    const std::string adaptedContext =
                        context + ".from.adapted_outputs[" +
                        std::to_string(chainIndex) + "]";
                    if (!adapted.is_object() || !adapted.contains("id") ||
                        !adapted["id"].is_string())
                    {
                        message = adaptedContext +
                                  " must be an object with a string \"id\"";
                        return false;
                    }
                    AdaptedOutputSpec step;
                    step.id = adapted["id"].get<std::string>();
                    if (!optionalString(adapted, "factory", adaptedContext,
                                        step.factory, message))
                        return false;
                    if (!optionalObject(adapted, "arguments", adaptedContext,
                                        step.arguments, message))
                        return false;
                    link.adaptedOutputs.push_back(std::move(step));
                    ++chainIndex;
                }
            }

            spec.connections.push_back(std::move(link));
            ++index;
        }
    }

    // ── workflow ─────────────────────────────────────────────────────────
    if (document.contains("workflow") && !document["workflow"].is_null())
    {
        if (!document["workflow"].is_object())
        {
            message = "\"workflow\" must be an object";
            return false;
        }
        const auto &workflow = document["workflow"];

        std::string strategyName;
        if (!optionalString(workflow, "strategy", "workflow", strategyName,
                            message))
            return false;
        if (!strategyName.empty() &&
            !workflowStrategyFromName(strategyName, spec.workflow.strategy))
        {
            message = "workflow.strategy must be one of \"none\", "
                      "\"pull_driven\", \"time_stepped\" (got \"" +
                      strategyName + "\")";
            return false;
        }
        if (!optionalString(workflow, "id", "workflow", spec.workflow.id,
                            message))
            return false;

        nlohmann::json trigger = nlohmann::json::object();
        if (!optionalObject(workflow, "trigger", "workflow", trigger, message))
            return false;
        if (!optionalString(trigger, "component", "workflow.trigger",
                            spec.workflow.triggerComponent, message) ||
            !optionalString(trigger, "input", "workflow.trigger",
                            spec.workflow.triggerInput, message))
            return false;

        if (workflow.contains("iterations_per_group"))
        {
            if (!workflow["iterations_per_group"].is_number_integer() ||
                workflow["iterations_per_group"].get<int>() < 1)
            {
                message = "workflow.iterations_per_group must be an integer "
                          "≥ 1";
                return false;
            }
            spec.workflow.iterationsPerGroup =
                workflow["iterations_per_group"].get<int>();
        }
        if (workflow.contains("max_steps"))
        {
            if (!workflow["max_steps"].is_number_integer() ||
                workflow["max_steps"].get<int>() < 0)
            {
                message = "workflow.max_steps must be a non-negative integer";
                return false;
            }
            spec.workflow.maxSteps = workflow["max_steps"].get<int>();
        }

        if (spec.workflow.strategy == WorkflowStrategy::PullDriven)
        {
            if (spec.workflow.triggerComponent.empty() ||
                spec.workflow.triggerInput.empty())
            {
                message = "workflow.strategy \"pull_driven\" requires "
                          "workflow.trigger.component and .input";
                return false;
            }
            if (!seen.count(spec.workflow.triggerComponent))
            {
                message = "workflow.trigger.component \"" +
                          spec.workflow.triggerComponent +
                          "\" is not declared in components[]";
                return false;
            }
        }
    }

    // ── writers ──────────────────────────────────────────────────────────
    if (document.contains("writers") && !document["writers"].is_null())
    {
        if (!document["writers"].is_array())
        {
            message = "\"writers\" must be an array";
            return false;
        }
        index = 0;
        for (const auto &writer : document["writers"])
        {
            const std::string context = "writers[" + std::to_string(index) + "]";
            if (!writer.is_object() || !writer.contains("type") ||
                !writer["type"].is_string())
            {
                message = context +
                          " must be an object with a string \"type\"";
                return false;
            }
            WriterSpec spec_writer;
            spec_writer.type = writer["type"].get<std::string>();
            if (!optionalString(writer, "path", context, spec_writer.path,
                                message))
                return false;
            if (spec_writer.path.empty())
            {
                message = context + " (\"" + spec_writer.type +
                          "\") requires a \"path\"";
                return false;
            }
            spec_writer.config = writer;
            spec.writers.push_back(std::move(spec_writer));
            ++index;
        }
    }

    // ── run ──────────────────────────────────────────────────────────────
    if (document.contains("run") && !document["run"].is_null())
    {
        if (!document["run"].is_object())
        {
            message = "\"run\" must be an object";
            return false;
        }
        if (!optionalString(document["run"], "manifest", "run",
                            spec.runManifest, message))
            return false;
    }

    message.clear();
    return true;
}

// ── Serialization ─────────────────────────────────────────────────────────

nlohmann::json CompositionSpec::toJson() const
{
    nlohmann::json document;
    // A binding is 1.1 syntax: a document that carries one must not claim
    // an earlier version, or a strict 1.0 reader would be told it can read
    // what it cannot.
    document["schema_version"] =
        (schemaVersion == "1.0" && !argumentBindings(*this).empty())
            ? "1.1"
            : schemaVersion;

    if (!metadata.empty())
    {
        nlohmann::json meta = nlohmann::json::object();
        if (!metadata.id.empty())          meta["id"] = metadata.id;
        if (!metadata.caption.empty())     meta["caption"] = metadata.caption;
        if (!metadata.description.empty()) meta["description"] = metadata.description;
        if (!metadata.author.empty())      meta["author"] = metadata.author;
        if (!metadata.created.empty())     meta["created"] = metadata.created;
        if (!metadata.sdkVersion.empty())  meta["sdk_version"] = metadata.sdkVersion;
        if (!metadata.interfaceVersion.empty())
            meta["interface_version"] = metadata.interfaceVersion;
        document["metadata"] = std::move(meta);
    }

    document["components"] = nlohmann::json::array();
    for (const ComponentSpec &component : components)
    {
        nlohmann::json block;
        block["id"] = component.id;
        if (!component.caption.empty())
            block["caption"] = component.caption;
        if (!component.info.empty())
        {
            nlohmann::json info = nlohmann::json::object();
            if (!component.info.library.empty())
                info["library"] = component.info.library;
            if (!component.info.componentInfoId.empty())
                info["component_info_id"] = component.info.componentInfoId;
            block["info"] = std::move(info);
        }
        if (component.mode != ExecutionMode::Run ||
            !component.resultsManifest.empty())
        {
            nlohmann::json execution;
            execution["mode"] = executionModeName(component.mode);
            if (!component.resultsManifest.empty())
                execution["results"] = component.resultsManifest;
            block["execution"] = std::move(execution);
        }
        block["arguments"] = component.arguments;
        document["components"].push_back(std::move(block));
    }

    document["connections"] = nlohmann::json::array();
    for (const ConnectionSpec &link : connections)
    {
        nlohmann::json from;
        from["component"] = link.fromComponent;
        from["output"]    = link.output;
        if (!link.adaptedOutputs.empty())
        {
            from["adapted_outputs"] = nlohmann::json::array();
            for (const AdaptedOutputSpec &step : link.adaptedOutputs)
            {
                nlohmann::json adapted;
                adapted["id"] = step.id;
                if (!step.factory.empty())
                    adapted["factory"] = step.factory;
                if (!step.arguments.empty())
                    adapted["arguments"] = step.arguments;
                from["adapted_outputs"].push_back(std::move(adapted));
            }
        }

        nlohmann::json to;
        to["component"] = link.toComponent;
        to["input"]     = link.input;
        if (!link.role.empty())
            to["role"] = link.role;

        document["connections"].push_back(
            {{"from", std::move(from)}, {"to", std::move(to)}});
    }

    if (workflow.strategy != WorkflowStrategy::None)
    {
        nlohmann::json block;
        block["strategy"] = workflowStrategyName(workflow.strategy);
        block["id"]       = workflow.id;
        if (!workflow.triggerComponent.empty())
            block["trigger"] = {{"component", workflow.triggerComponent},
                                {"input", workflow.triggerInput}};
        if (workflow.iterationsPerGroup != 1)
            block["iterations_per_group"] = workflow.iterationsPerGroup;
        if (workflow.maxSteps != 0)
            block["max_steps"] = workflow.maxSteps;
        document["workflow"] = std::move(block);
    }

    if (!writers.empty())
    {
        document["writers"] = nlohmann::json::array();
        for (const WriterSpec &writer : writers)
        {
            nlohmann::json block = writer.config;
            block["type"] = writer.type;
            block["path"] = writer.path;
            document["writers"].push_back(std::move(block));
        }
    }

    if (!runManifest.empty())
        document["run"] = {{"manifest", runManifest}};

    return document;
}

const ComponentSpec *CompositionSpec::component(const std::string &id) const
{
    for (const ComponentSpec &component : components)
    {
        if (component.id == id)
            return &component;
    }
    return nullptr;
}

// ── Argument bindings ─────────────────────────────────────────────────────

bool isArgumentBinding(const nlohmann::json &payload)
{
    return payload.is_object() && payload.contains(kArgumentBindingKey);
}

std::vector<ArgumentBindingSpec> argumentBindings(const CompositionSpec &spec)
{
    std::vector<ArgumentBindingSpec> bindings;
    for (const ComponentSpec &component : spec.components)
    {
        for (const auto &[argumentId, payload] : component.arguments.items())
        {
            if (!isArgumentBinding(payload))
                continue;
            const auto &from = payload[kArgumentBindingKey];
            ArgumentBindingSpec binding;
            binding.component = component.id;
            binding.argument  = argumentId;
            if (from.is_object())
            {
                if (from.contains("component") && from["component"].is_string())
                    binding.provider = from["component"].get<std::string>();
                if (from.contains("output") && from["output"].is_string())
                    binding.output = from["output"].get<std::string>();
            }
            bindings.push_back(std::move(binding));
        }
    }
    return bindings;
}

bool bindingStages(const CompositionSpec &spec,
                   std::vector<std::vector<std::string>> &stages,
                   std::string &message)
{
    stages.clear();

    // provider → consumers, and how many providers each consumer waits on.
    std::map<std::string, std::vector<std::string>> consumersOf;
    std::map<std::string, int> waitingOn;
    for (const ComponentSpec &component : spec.components)
        waitingOn[component.id] = 0;
    for (const ArgumentBindingSpec &binding : argumentBindings(spec))
    {
        consumersOf[binding.provider].push_back(binding.component);
        ++waitingOn[binding.component];
    }

    // Peel zero-dependency components a level at a time, in document order,
    // so a stage index is the longest provider chain above the component.
    std::set<std::string> placed;
    while (placed.size() < spec.components.size())
    {
        std::vector<std::string> stage;
        for (const ComponentSpec &component : spec.components)
        {
            if (!placed.count(component.id) && waitingOn[component.id] == 0)
                stage.push_back(component.id);
        }
        if (stage.empty())
            break; // Everything left waits on something left: a cycle.
        for (const std::string &id : stage)
        {
            placed.insert(id);
            for (const std::string &consumer : consumersOf[id])
                --waitingOn[consumer];
        }
        stages.push_back(std::move(stage));
    }

    if (placed.size() == spec.components.size())
    {
        message.clear();
        return true;
    }

    // Name the cycle: walk consumer → provider among the leftovers until a
    // component repeats, then print the loop the walk closed.
    std::map<std::string, ArgumentBindingSpec> providerOf;
    for (const ArgumentBindingSpec &binding : argumentBindings(spec))
    {
        if (!placed.count(binding.component) && !placed.count(binding.provider))
            providerOf.emplace(binding.component, binding);
    }
    std::string current = providerOf.begin()->first;
    std::vector<std::string> path;
    std::set<std::string> onPath;
    while (!onPath.count(current))
    {
        path.push_back(current);
        onPath.insert(current);
        current = providerOf.at(current).provider;
    }
    message = "Argument bindings form a cycle: ";
    bool inLoop = false;
    for (const std::string &id : path)
    {
        if (id == current)
            inLoop = true;
        if (!inLoop)
            continue;
        message += "\"" + id + "\" (argument \"" +
                   providerOf.at(id).argument + "\") needs \"" +
                   providerOf.at(id).provider + "\"; ";
    }
    message.pop_back();
    message.back() = '.';
    stages.clear();
    return false;
}

} // namespace HydroCouple::SDK::IO
