// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/io/modelinitializer.h"
#include "hydrocouplesdk/io/sidecar.h"
#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouplesdk/data/bufferio.h"

#include <fstream>
#include <sstream>

#ifdef HYDROCOUPLESDK_HAVE_YAML_CPP
#include <yaml-cpp/yaml.h>
#endif

namespace HydroCouple::SDK::IO {

namespace {

using AT = HydroCouple::IArgument::ArgumentInputType;

/*!
 * Finds an element by IIdentity::id() in a component collection.
 */
template <typename T>
T *findById(const std::vector<T *> &items, const std::string &id)
{
    for (T *item : items)
        if (item && item->id() == id)
            return item;
    return nullptr;
}

#ifdef HYDROCOUPLESDK_HAVE_YAML_CPP
/*!
 * Converts a yaml-cpp node to the equivalent nlohmann::json value, so
 * argument implementations only ever see JSON.
 */
nlohmann::json yamlToJson(const YAML::Node &node)
{
    switch (node.Type())
    {
    case YAML::NodeType::Null:
        return nullptr;
    case YAML::NodeType::Scalar:
    {
        // Try the narrowest faithful representation.
        if (bool b; YAML::convert<bool>::decode(node, b))
            return b;
        if (int64_t i; YAML::convert<int64_t>::decode(node, i))
            return i;
        if (double d; YAML::convert<double>::decode(node, d))
            return d;
        return node.as<std::string>();
    }
    case YAML::NodeType::Sequence:
    {
        auto array = nlohmann::json::array();
        for (const auto &child : node)
            array.push_back(yamlToJson(child));
        return array;
    }
    case YAML::NodeType::Map:
    {
        auto object = nlohmann::json::object();
        for (const auto &pair : node)
            object[pair.first.as<std::string>()] = yamlToJson(pair.second);
        return object;
    }
    default:
        return nullptr;
    }
}
#endif

/*!
 * Read-only IComponentDataItem over sidecar bytes. Passing it to
 * IArgument::initialize(const IComponentDataItem&) lets every argument
 * implementation resize and bulk-copy through the typed data plane —
 * sidecar payloads never touch a JSON DOM.
 */
class SidecarSourceItem final
    : public HydroCouple::SDK::AbstractComponentDataItem
{
public:
    SidecarSourceItem(SidecarRef ref, std::vector<std::byte> bytes)
        : AbstractComponentDataItem("sidecar", {}, nullptr, nullptr),
          m_ref(std::move(ref)),
          m_bytes(std::move(bytes))
    {
    }

    [[nodiscard]] std::vector<int64_t> shape() const override
    {
        return m_ref.shape;
    }

    [[nodiscard]] DataKind dataKind() const override { return m_ref.kind; }

    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr)
        const override
    {
        switch (m_ref.kind)
        {
#define HC_SIDECAR_CASE(K, T)                                                \
    case DataKind::K:                                                        \
        return HydroCouple::SDK::detail::readHyperslab<T>(                   \
            reinterpret_cast<const T *>(m_bytes.data()), m_ref.shape,        \
            destination, start, count, message)
        HC_SIDECAR_CASE(Float64, double);
        HC_SIDECAR_CASE(Float32, float);
        HC_SIDECAR_CASE(Int64, int64_t);
        HC_SIDECAR_CASE(UInt64, uint64_t);
        HC_SIDECAR_CASE(Int32, int32_t);
        HC_SIDECAR_CASE(UInt32, uint32_t);
        HC_SIDECAR_CASE(Int16, int16_t);
        HC_SIDECAR_CASE(UInt16, uint16_t);
        HC_SIDECAR_CASE(Int8, int8_t);
        HC_SIDECAR_CASE(UInt8, uint8_t);
        HC_SIDECAR_CASE(Boolean, bool);
#undef HC_SIDECAR_CASE
        default:
            if (message) *message = "sidecar has an unsupported DataKind";
            return false;
        }
    }

    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &,
                                     std::span<const int64_t>,
                                     std::span<const int64_t>,
                                     std::string *message = nullptr) override
    {
        if (message) *message = "sidecar source items are read-only";
        return false;
    }

private:
    SidecarRef             m_ref;
    std::vector<std::byte> m_bytes;
};

} // namespace

void ModelInitializer::setSidecarDirectory(std::filesystem::path directory)
{
    m_sidecarDirectory = std::move(directory);
}

void ModelInitializer::setSidecarThresholdBytes(int64_t bytes)
{
    m_sidecarThresholdBytes = std::max<int64_t>(1, bytes);
}

ModelInitializer::ModelInitializer(ComponentResolver resolver)
    : m_resolver(std::move(resolver))
{
}

bool ModelInitializer::yamlSupported() noexcept
{
#ifdef HYDROCOUPLESDK_HAVE_YAML_CPP
    return true;
#else
    return false;
#endif
}

// ── Loading ───────────────────────────────────────────────────────────────

bool ModelInitializer::initializeFromFile(const std::filesystem::path &path,
                                          std::string &message)
{
    std::ifstream file(path);
    if (!file)
    {
        message = "Cannot open composition document: " + path.string();
        return false;
    }
    std::ostringstream text;
    text << file.rdbuf();
    m_documentDirectory = path.has_parent_path() ? path.parent_path()
                                                 : std::filesystem::path(".");

    const auto extension = path.extension().string();
    if (extension == ".yaml" || extension == ".yml")
    {
#ifdef HYDROCOUPLESDK_HAVE_YAML_CPP
        try
        {
            const nlohmann::json document =
                yamlToJson(YAML::Load(text.str()));
            return initialize(document, message);
        }
        catch (const std::exception &e)
        {
            message = std::string("YAML parse error in ") + path.string() +
                      ": " + e.what();
            return false;
        }
#else
        message = "Composition document '" + path.string() +
                  "' is YAML, but this build has no yaml-cpp support. "
                  "Rebuild with the vcpkg manifest (yaml-cpp is a core "
                  "dependency) or provide the document as JSON.";
        return false;
#endif
    }
    return initializeFromJsonText(text.str(), message);
}

bool ModelInitializer::initializeFromJsonText(const std::string &text,
                                              std::string &message)
{
    try
    {
        return initialize(nlohmann::json::parse(text), message);
    }
    catch (const std::exception &e)
    {
        message = std::string("JSON parse error: ") + e.what();
        return false;
    }
}

bool ModelInitializer::initialize(const nlohmann::json &document,
                                  std::string &message)
{
    // Structural parse first, then semantics against live components —
    // nothing is applied until both pass.
    if (!CompositionSpec::parse(document, m_spec, message))
        return false;
    if (!validateSpec(m_spec, message))
        return false;
    if (!applyComponents(message))
        return false;
    return applyConnections(message);
}

const CompositionSpec &ModelInitializer::spec() const noexcept
{
    return m_spec;
}

const std::vector<std::unique_ptr<HydroCouple::IAdaptedOutput>> &
ModelInitializer::adaptedOutputs() const noexcept
{
    return m_adaptedOutputs;
}

bool ModelInitializer::validateDocument(const nlohmann::json &document,
                                        std::string &message) const
{
    CompositionSpec spec;
    if (!CompositionSpec::parse(document, spec, message))
        return false;
    return validateSpec(spec, message);
}

// ── Validation (before anything is applied) ───────────────────────────────

bool ModelInitializer::validateSpec(const CompositionSpec &spec,
                                    std::string &message) const
{
    for (const ComponentSpec &block : spec.components)
    {
        HydroCouple::IModelComponent *component = m_resolver(block.id);
        if (!component)
        {
            message = "Unknown component id \"" + block.id +
                      "\" — the resolver returned no instance for it";
            return false;
        }

        if (block.mode == ExecutionMode::Resume)
        {
            message = "Component \"" + block.id +
                      "\": execution mode \"resume\" is reserved and not yet "
                      "implemented (see ICheckpointableModelComponent)";
            return false;
        }

        for (const auto &[argId, payload] : block.arguments.items())
        {
            if (!findById(component->arguments(), argId))
            {
                message = "Component \"" + block.id + "\" has no argument \"" +
                          argId + "\"";
                return false;
            }
            (void)payload;
        }
    }

    int index = 0;
    for (const ConnectionSpec &link : spec.connections)
    {
        const std::string context = "connections[" + std::to_string(index) + "]";
        HydroCouple::IModelComponent *source = m_resolver(link.fromComponent);
        HydroCouple::IModelComponent *target = m_resolver(link.toComponent);
        if (!source || !target)
        {
            message = context + ": unknown component id";
            return false;
        }

        // Exchange items exist only after a component initializes, so item
        // level checks are deferred to apply time; role and factory names
        // are checkable now for components already past Created.
        if (!link.role.empty())
        {
            HydroCouple::IInput *input =
                findById(target->inputs(), link.input);
            if (auto *multi =
                    dynamic_cast<HydroCouple::IMultiInput *>(input))
            {
                bool known = false;
                for (HydroCouple::IIdentity *label : multi->providerLabels())
                {
                    if (label && label->id() == link.role)
                    {
                        known = true;
                        break;
                    }
                }
                if (!known)
                {
                    message = context + ".to.role \"" + link.role +
                              "\" is not a provider role of input \"" +
                              link.input + "\"";
                    return false;
                }
            }
            else if (input)
            {
                message = context + ".to.role was given but input \"" +
                          link.input + "\" is not an IMultiInput";
                return false;
            }
        }
        ++index;
    }

    message.clear();
    return true;
}

// ── Application ───────────────────────────────────────────────────────────

bool ModelInitializer::applyComponents(std::string &message)
{
    for (const ComponentSpec &block : m_spec.components)
    {
        const std::string id = block.id;
        HydroCouple::IModelComponent *component = m_resolver(id);

        {
            for (const auto &[argId, payload] : block.arguments.items())
            {
                HydroCouple::IArgument *argument =
                    findById(component->arguments(), argId);
                std::string argMessage;
                if (isSidecarRef(payload))
                {
                    SidecarRef ref;
                    if (!parseSidecarRef(payload, ref, argMessage))
                    {
                        message = "Component \"" + id + "\", argument \"" +
                                  argId + "\": " + argMessage;
                        return false;
                    }
                    if (ref.path.is_relative())
                    {
                        const std::filesystem::path base =
                            !m_sidecarDirectory.empty() ? m_sidecarDirectory
                            : !m_documentDirectory.empty()
                                ? m_documentDirectory
                                : std::filesystem::path(".");
                        ref.path = base / ref.path;
                    }
                    std::vector<std::byte> bytes;
                    if (!readSidecarBytes(ref, bytes, argMessage))
                    {
                        message = "Component \"" + id + "\", argument \"" +
                                  argId + "\": " + argMessage;
                        return false;
                    }
                    SidecarSourceItem source(std::move(ref),
                                             std::move(bytes));
                    if (!argument->initialize(source, argMessage))
                    {
                        message = "Component \"" + id + "\", argument \"" +
                                  argId + "\": " + argMessage;
                        return false;
                    }
                }
                else if (!argument->initialize(payload.dump(), AT::JSON,
                                               argMessage))
                {
                    message = "Component \"" + id + "\", argument \"" +
                              argId + "\": " + argMessage;
                    return false;
                }
            }
        }

        component->initialize();

        // A component opened from recorded results legitimately finishes
        // during initialize() — its values are already computed. Every
        // other mode must reach Initialized.
        using Status = HydroCouple::IModelComponent::ComponentStatus;
        const Status status = component->status();
        const bool acceptable =
            status == Status::Initialized ||
            (block.mode == ExecutionMode::Open && status == Status::Finished);
        if (!acceptable)
        {
            message = "Component \"" + id + "\" failed to reach " +
                      (block.mode == ExecutionMode::Open
                           ? "Initialized or Finished"
                           : "Initialized") +
                      " after initialize()";
            for (const auto &entry : component->errors())
                message += "\n  - " + entry.message;
            return false;
        }
    }
    message.clear();
    return true;
}

HydroCouple::IOutput *ModelInitializer::buildAdaptedChain(
    const ConnectionSpec &link, HydroCouple::IOutput *provider,
    HydroCouple::IInput *consumer, const std::string &context,
    std::string &message)
{
    HydroCouple::IOutput *current = provider;

    for (const AdaptedOutputSpec &step : link.adaptedOutputs)
    {
        HydroCouple::IModelComponent *owner = current->modelComponent();
        HydroCouple::IModelComponentInfo *info =
            owner ? owner->componentInfo() : nullptr;
        if (!info)
        {
            message = context + ": adapted output \"" + step.id +
                      "\" requested but the provider's component exposes no "
                      "componentInfo() to find factories on";
            return nullptr;
        }

        HydroCouple::IAdaptedOutput *created = nullptr;
        for (HydroCouple::IAdaptedOutputFactory *factory :
             info->adaptedOutputFactories())
        {
            if (!factory)
                continue;
            if (!step.factory.empty() && factory->id() != step.factory)
                continue;

            for (HydroCouple::IIdentity *available :
                 factory->getAvailableAdaptedOutputIds(current, consumer))
            {
                if (!available || available->id() != step.id)
                    continue;
                std::unique_ptr<HydroCouple::IAdaptedOutput> adapted =
                    factory->createAdaptedOutput(available, current, consumer);
                if (!adapted)
                {
                    message = context + ": factory \"" + factory->id() +
                              "\" returned no adapted output for \"" +
                              step.id + "\"";
                    return nullptr;
                }
                created = adapted.get();
                m_adaptedOutputs.push_back(std::move(adapted));
                break;
            }
            if (created)
                break;
        }

        if (!created)
        {
            message = context + ": no factory on component \"" +
                      (owner ? owner->id() : std::string("?")) +
                      "\" offers adapted output \"" + step.id + "\"" +
                      (step.factory.empty()
                           ? std::string()
                           : (" from factory \"" + step.factory + "\""));
            return nullptr;
        }

        // Adapted-output arguments are applied before initialize(), the
        // same contract component arguments follow.
        for (const auto &[argId, payload] : step.arguments.items())
        {
            HydroCouple::IArgument *argument =
                findById(created->arguments(), argId);
            if (!argument)
            {
                message = context + ": adapted output \"" + step.id +
                          "\" has no argument \"" + argId + "\"";
                return nullptr;
            }
            std::string argMessage;
            if (!argument->initialize(payload.dump(), AT::JSON, argMessage))
            {
                message = context + ": adapted output \"" + step.id +
                          "\", argument \"" + argId + "\": " + argMessage;
                return nullptr;
            }
        }

        created->initialize();
        current = created;
    }

    return current;
}

bool ModelInitializer::applyConnections(std::string &message)
{
    int index = 0;
    for (const ConnectionSpec &link : m_spec.connections)
    {
        const std::string context = "connections[" + std::to_string(index) + "]";

        HydroCouple::IModelComponent *source = m_resolver(link.fromComponent);
        HydroCouple::IModelComponent *target = m_resolver(link.toComponent);
        if (!source || !target)
        {
            message = context + ": unknown component id";
            return false;
        }

        HydroCouple::IOutput *output = findById(source->outputs(), link.output);
        if (!output)
        {
            message = context + ": component \"" + source->id() +
                      "\" has no output \"" + link.output + "\"";
            return false;
        }
        HydroCouple::IInput *input = findById(target->inputs(), link.input);
        if (!input)
        {
            message = context + ": component \"" + target->id() +
                      "\" has no input \"" + link.input + "\"";
            return false;
        }

        HydroCouple::IOutput *effective =
            buildAdaptedChain(link, output, input, context, message);
        if (!effective)
            return false;

        bool wired = false;
        if (auto *multi = dynamic_cast<HydroCouple::IMultiInput *>(input))
        {
            HydroCouple::IIdentity *role = nullptr;
            if (!link.role.empty())
            {
                for (HydroCouple::IIdentity *label : multi->providerLabels())
                {
                    if (label && label->id() == link.role)
                    {
                        role = label;
                        break;
                    }
                }
                if (!role)
                {
                    message = context + ": input \"" + link.input +
                              "\" has no provider role \"" + link.role + "\"";
                    return false;
                }
            }
            wired = multi->addProvider(effective, role);
        }
        else
        {
            wired = input->setProvider(effective);
        }

        if (!wired)
        {
            std::string why;
            (void)input->canConsume(effective, why);
            message = context + ": input \"" + link.input +
                      "\" rejected output \"" + effective->id() + "\"" +
                      (why.empty() ? "" : (": " + why));
            return false;
        }

        // Keep the provider's consumer list symmetric with the wiring.
        effective->addConsumer(input);
        ++index;
    }
    message.clear();
    return true;
}

// ── Persistence (round trip) ──────────────────────────────────────────────

nlohmann::json ModelInitializer::serialize(
    const std::vector<std::string> &componentIds, std::string &message) const
{
    // The write side of Composition Specification v1: preserve the blocks
    // the initializer does not own (metadata, workflow, writers, run,
    // per-component execution modes) and regenerate what it does.
    CompositionSpec out;
    out.schemaVersion = kCompositionSchemaVersion;
    out.metadata      = m_spec.metadata;
    out.workflow      = m_spec.workflow;
    out.writers       = m_spec.writers;
    out.runManifest   = m_spec.runManifest;

    nlohmann::json document;
    document["components"]  = nlohmann::json::array();
    document["connections"] = nlohmann::json::array();

    for (const std::string &id : componentIds)
    {
        HydroCouple::IModelComponent *component = m_resolver(id);
        if (!component)
        {
            message = "serialize: unknown component id \"" + id + "\"";
            return {};
        }

        nlohmann::json block;
        block["id"]        = id;
        if (const ComponentSpec *previous = m_spec.component(id))
        {
            if (!previous->caption.empty())
                block["caption"] = previous->caption;
            if (!previous->info.empty())
            {
                nlohmann::json info = nlohmann::json::object();
                if (!previous->info.library.empty())
                    info["library"] = previous->info.library;
                if (!previous->info.componentInfoId.empty())
                    info["component_info_id"] = previous->info.componentInfoId;
                block["info"] = std::move(info);
            }
            if (previous->mode != ExecutionMode::Run ||
                !previous->resultsManifest.empty())
            {
                nlohmann::json execution;
                execution["mode"] = executionModeName(previous->mode);
                if (!previous->resultsManifest.empty())
                    execution["results"] = previous->resultsManifest;
                block["execution"] = std::move(execution);
            }
        }
        block["arguments"] = nlohmann::json::object();
        for (HydroCouple::IArgument *argument : component->arguments())
        {
            std::string argMessage;
            const int64_t elementSize =
                HydroCouple::Helpers::dataKindSize(argument->dataKind());
            int64_t valueBytes = elementSize;
            for (int64_t extent : argument->shape())
                valueBytes *= extent;
            if (!m_sidecarDirectory.empty() && elementSize > 0 &&
                valueBytes > m_sidecarThresholdBytes)
            {
                nlohmann::json ref;
                const std::filesystem::path sidecarPath =
                    m_sidecarDirectory / (id + "_" + argument->id() + ".bin");
                if (!writeSidecar(*argument, sidecarPath, ref, argMessage))
                {
                    message = "serialize: component \"" + id +
                              "\", argument \"" + argument->id() +
                              "\": " + argMessage;
                    return {};
                }
                block["arguments"][argument->id()] = std::move(ref);
                continue;
            }
            std::string value;
            if (!argument->serialize(AT::JSON, value, argMessage))
            {
                message = "serialize: component \"" + id +
                          "\", argument \"" + argument->id() +
                          "\": " + argMessage;
                return {};
            }
            block["arguments"][argument->id()] = nlohmann::json::parse(value);
        }
        document["components"].push_back(std::move(block));

        // Connections reproduced from live wiring of this component's inputs.
        for (HydroCouple::IInput *input : component->inputs())
        {
            HydroCouple::IOutput *provider = input->provider();
            if (!provider || !provider->modelComponent())
                continue;
            document["connections"].push_back({
                {"from", {{"component", provider->modelComponent()->id()},
                          {"output", provider->id()}}},
                {"to",   {{"component", id}, {"input", input->id()}}},
            });
        }
    }

    // Blocks the initializer carries but does not regenerate.
    nlohmann::json carried = out.toJson();
    document["schema_version"] = carried["schema_version"];
    for (const char *key : {"metadata", "workflow", "writers", "run"})
    {
        if (carried.contains(key))
            document[key] = carried[key];
    }

    message.clear();
    return document;
}

bool ModelInitializer::serializeToFile(
    const std::vector<std::string> &componentIds,
    const std::filesystem::path &path, std::string &message) const
{
    const nlohmann::json document = serialize(componentIds, message);
    if (document.is_null() || (document.empty() && !message.empty()))
        return false;

    std::error_code ec;
    if (path.has_parent_path())
        std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream file(path);
    if (!file)
    {
        message = "Cannot open '" + path.string() + "' for writing";
        return false;
    }
    file << document.dump(2) << '\n';
    return static_cast<bool>(file);
}

} // namespace HydroCouple::SDK::IO
