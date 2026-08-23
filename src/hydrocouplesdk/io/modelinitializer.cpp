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
    if (!validate(document, message))
        return false;
    if (!applyComponents(document, message))
        return false;
    return applyConnections(document, message);
}

// ── Validation (before anything is applied) ───────────────────────────────

bool ModelInitializer::validate(const nlohmann::json &document,
                                std::string &message) const
{
    if (!document.is_object() || !document.contains("components") ||
        !document["components"].is_array())
    {
        message = "Composition document must be an object with a "
                  "\"components\" array";
        return false;
    }

    for (const auto &block : document["components"])
    {
        if (!block.is_object() || !block.contains("id") ||
            !block["id"].is_string())
        {
            message = "Every components[] entry must be an object with a "
                      "string \"id\"";
            return false;
        }
        const std::string id = block["id"].get<std::string>();

        HydroCouple::IModelComponent *component = m_resolver(id);
        if (!component)
        {
            message = "Unknown component id \"" + id +
                      "\" — the resolver returned no instance for it";
            return false;
        }
        if (block.contains("arguments"))
        {
            if (!block["arguments"].is_object())
            {
                message = "components[\"" + id +
                          "\"].arguments must be an object of "
                          "argumentId → payload";
                return false;
            }
            for (const auto &[argId, payload] : block["arguments"].items())
            {
                if (!findById(component->arguments(), argId))
                {
                    message = "Component \"" + id +
                              "\" has no argument \"" + argId + "\"";
                    return false;
                }
                (void)payload;
            }
        }
    }

    if (document.contains("connections"))
    {
        if (!document["connections"].is_array())
        {
            message = "\"connections\" must be an array";
            return false;
        }
        int index = 0;
        for (const auto &connection : document["connections"])
        {
            for (const char *end : {"from", "to"})
            {
                if (!connection.contains(end) ||
                    !connection[end].is_object() ||
                    !connection[end].contains("component"))
                {
                    message = "connections[" + std::to_string(index) +
                              "] must carry \"from\"/\"to\" objects with a "
                              "\"component\" id";
                    return false;
                }
            }
            ++index;
        }
    }

    message.clear();
    return true;
}

// ── Application ───────────────────────────────────────────────────────────

bool ModelInitializer::applyComponents(const nlohmann::json &document,
                                       std::string &message)
{
    for (const auto &block : document["components"])
    {
        const std::string id = block["id"].get<std::string>();
        HydroCouple::IModelComponent *component = m_resolver(id);

        if (block.contains("arguments"))
        {
            for (const auto &[argId, payload] : block["arguments"].items())
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
        if (component->status() !=
            HydroCouple::IModelComponent::ComponentStatus::Initialized)
        {
            message = "Component \"" + id +
                      "\" failed to reach Initialized after initialize()";
            for (const auto &entry : component->errors())
                message += "\n  - " + entry.message;
            return false;
        }
    }
    message.clear();
    return true;
}

bool ModelInitializer::applyConnections(const nlohmann::json &document,
                                        std::string &message)
{
    if (!document.contains("connections"))
        return true;

    int index = 0;
    for (const auto &connection : document["connections"])
    {
        const auto &from = connection["from"];
        const auto &to   = connection["to"];

        HydroCouple::IModelComponent *source =
            m_resolver(from["component"].get<std::string>());
        HydroCouple::IModelComponent *target =
            m_resolver(to["component"].get<std::string>());
        if (!source || !target)
        {
            message = "connections[" + std::to_string(index) +
                      "]: unknown component id";
            return false;
        }

        const std::string outputId = from.value("output", "");
        const std::string inputId  = to.value("input", "");

        HydroCouple::IOutput *output = findById(source->outputs(), outputId);
        if (!output)
        {
            message = "connections[" + std::to_string(index) +
                      "]: component \"" + source->id() +
                      "\" has no output \"" + outputId + "\"";
            return false;
        }
        HydroCouple::IInput *input = findById(target->inputs(), inputId);
        if (!input)
        {
            message = "connections[" + std::to_string(index) +
                      "]: component \"" + target->id() +
                      "\" has no input \"" + inputId + "\"";
            return false;
        }

        bool wired = false;
        if (auto *multi = dynamic_cast<HydroCouple::IMultiInput *>(input))
            wired = multi->addProvider(output);
        else
            wired = input->setProvider(output);
        if (!wired)
        {
            std::string why;
            (void)input->canConsume(output, why);
            message = "connections[" + std::to_string(index) + "]: input \"" +
                      inputId + "\" rejected output \"" + outputId +
                      "\"" + (why.empty() ? "" : (": " + why));
            return false;
        }
        ++index;
    }
    message.clear();
    return true;
}

// ── Persistence (round trip) ──────────────────────────────────────────────

nlohmann::json ModelInitializer::serialize(
    const std::vector<std::string> &componentIds, std::string &message) const
{
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
