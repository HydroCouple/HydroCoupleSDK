// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   modelinitializer.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  ModelInitializer — YAML/JSON composition initialization targeted
 *         at IArgument, with symmetric round-trip persistence.
 *
 * Arguments are the serialization unit: a component's entire persistent
 * configuration flows through its arguments(), and the SDK never
 * serializes component internals directly. The composition document lists
 * components with per-argument payloads and output→input connections:
 *
 * \code{.json}
 * {
 *   "components": [
 *     { "id": "upstream",
 *       "arguments": { "roughness": { "values": [0.013] } } },
 *     { "id": "downstream", "arguments": {} }
 *   ],
 *   "connections": [
 *     { "from": { "component": "upstream",   "output": "flow" },
 *       "to":   { "component": "downstream", "input":  "inflow" } }
 *   ]
 * }
 * \endcode
 *
 * One schema serves JSON and YAML alike: YAML is converted to JSON at the
 * boundary (when yaml-cpp is available), so argument implementations only
 * ever see JSON. The document is validated with actionable error messages
 * before any component is touched; persistence walks arguments() calling
 * the symmetric IArgument::serialize() to emit the same document shape
 * back out (round-trip guarantee).
 */

#ifndef HYDROCOUPLESDK_IO_MODELINITIALIZER_H
#define HYDROCOUPLESDK_IO_MODELINITIALIZER_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"

#include <filesystem>
#include <functional>
#include <string>

#include <nlohmann/json.hpp>

namespace HydroCouple::SDK::IO {

/*!
 * \brief Reads composition documents and applies them to model components;
 *        writes the symmetric document back out.
 *
 * Component instances are supplied by the caller through a resolver
 * (id → IModelComponent*), keeping the initializer independent of how
 * components are created (static registry, dlopen'd libraries, tests).
 *
 * \threadsafety Not thread-safe; drive from one thread.
 */
class HYDROCOUPLESDK_EXPORT ModelInitializer
{
public:
    /*!
     * \brief Maps a component id from the document to a live instance;
     *        returns nullptr for unknown ids.
     */
    using ComponentResolver =
        std::function<HydroCouple::IModelComponent *(const std::string &id)>;

    /*!
     * \brief Construct with the component resolver.
     */
    explicit ModelInitializer(ComponentResolver resolver);

    // ── Loading ───────────────────────────────────────────────────────────

    /*!
     * \brief Parses and applies a composition document from a file.
     *
     * `.json` parses directly; `.yaml`/`.yml` converts through yaml-cpp
     * when the SDK was built with it (otherwise fails with a clear
     * message).
     *
     * \param[in]  path    Composition document path.
     * \param[out] message Failure description when returning false.
     */
    [[nodiscard]] bool initializeFromFile(const std::filesystem::path &path,
                                          std::string &message);

    /*!
     * \brief Parses and applies a composition document from JSON text.
     */
    [[nodiscard]] bool initializeFromJsonText(const std::string &text,
                                              std::string &message);

    /*!
     * \brief Validates and applies an already-parsed composition document.
     *
     * Order per component: every argument payload feeds
     * IArgument::initialize(payload, JSON, message); then the component's
     * initialize() runs and must reach ComponentStatus::Initialized.
     * Connections are wired afterwards via IInput::setProvider(), with
     * IMultiInput::addProvider() used when the target is a multi-input.
     * Validation of the whole document happens before anything is applied.
     */
    [[nodiscard]] bool initialize(const nlohmann::json &document,
                                  std::string &message);

    // ── Persistence (round trip) ─────────────────────────────────────────

    /*!
     * \brief Serializes components back to the composition document shape.
     *
     * Walks each component's arguments() calling
     * IArgument::serialize(JSON); connection blocks are reproduced from
     * the live provider wiring of each component's inputs.
     *
     * \param[in]  componentIds Document order of components to persist.
     * \param[out] message      Failure description when returning false.
     * \return The composition document; empty JSON on failure.
     */
    [[nodiscard]] nlohmann::json serialize(
        const std::vector<std::string> &componentIds,
        std::string &message) const;

    /*!
     * \brief serialize() written to a file (pretty-printed JSON).
     */
    [[nodiscard]] bool serializeToFile(
        const std::vector<std::string> &componentIds,
        const std::filesystem::path &path, std::string &message) const;

    /*!
     * \brief True when this build can read YAML documents.
     */
    [[nodiscard]] static bool yamlSupported() noexcept;

private:
    [[nodiscard]] bool validate(const nlohmann::json &document,
                                std::string &message) const;
    [[nodiscard]] bool applyComponents(const nlohmann::json &document,
                                       std::string &message);
    [[nodiscard]] bool applyConnections(const nlohmann::json &document,
                                        std::string &message);

    ComponentResolver m_resolver;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_MODELINITIALIZER_H
