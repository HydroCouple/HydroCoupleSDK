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
#include "hydrocouplesdk/io/compositionspec.h"
#include "hydrocouple.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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

    /*!
     * \brief Move-only: the initializer owns the components and adapted
     *        outputs it created (`std::unique_ptr` members), so copying it
     *        would duplicate ownership.
     *
     * Declared rather than left implicit because this class is exported:
     * `__declspec(dllexport)` instantiates *every* member of the class,
     * including the implicit copy assignment, which then fails to compile
     * against the move-only members. Other toolchains only instantiate it
     * if something calls it, so an implicit declaration compiles there and
     * breaks on MSVC alone.
     */
    ModelInitializer(const ModelInitializer &)            = delete;
    ModelInitializer &operator=(const ModelInitializer &) = delete;
    ModelInitializer(ModelInitializer &&) noexcept;
    ModelInitializer &operator=(ModelInitializer &&) noexcept;
    ~ModelInitializer();

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

    // ── Composition Specification v1 ─────────────────────────────────────

    /*!
     * \brief The specification parsed by the most recent initialize().
     *
     * Blocks the initializer does not apply itself — \c workflow, \c writers,
     * \c run, and each component's \c execution mode — are carried here as
     * validated data for the host to act on (writers are feature-gated and
     * workflows live outside SDK::IO, so building them is the host's job).
     */
    [[nodiscard]] const CompositionSpec &spec() const noexcept;

    /*!
     * \brief Structural + semantic validation without applying anything.
     *
     * Never loads component libraries: naming a library in a document makes
     * it executable content, so resolution stays with the caller's resolver
     * and happens only during initialize().
     *
     * \param[in]  document Composition document.
     * \param[out] message  Actionable failure description when false.
     */
    [[nodiscard]] bool validateDocument(const nlohmann::json &document,
                                        std::string &message) const;

    /*!
     * \brief Components the initializer substituted for \c execution.mode
     *        "open" entries (owned).
     *
     * When a component block declares mode "open", the initializer first
     * lets the resolver's component load its own results; a component that
     * finishes during initialize() is taken at its word. Otherwise — and
     * whenever the resolver has no instance at all, which is the norm for
     * an analysis host with no model libraries — a ResultsModelComponent is
     * built from the declared run manifest and used in its place. Anything
     * referring to that component id, including connections, resolves to
     * the substitute.
     */
    [[nodiscard]] const std::vector<
        std::unique_ptr<HydroCouple::IModelComponent>> &
    openedComponents() const noexcept;

    /*!
     * \brief The live instance backing a component id: the substitute when
     *        one was created, otherwise the resolver's component.
     */
    [[nodiscard]] HydroCouple::IModelComponent *component(
        const std::string &id) const;

    /*!
     * \brief Adapted outputs created while wiring connections (owned).
     *
     * IAdaptedOutputFactory::createAdaptedOutput() yields owning handles;
     * the initializer keeps them alive for the composition's lifetime.
     */
    [[nodiscard]] const std::vector<std::unique_ptr<HydroCouple::IAdaptedOutput>> &
    adaptedOutputs() const noexcept;

    // ── Sidecar binary references (efficiency contract, plan §4.7) ────────

    /*!
     * \brief Enables sidecar mode: serialize() replaces argument payloads
     *        larger than the byte threshold with \c \@ref objects whose
     *        bulk bytes are written to \p directory, and initialize()
     *        resolves relative references against it.
     *
     * Without a sidecar directory, initialize() resolves relative
     * references against the composition document's directory.
     */
    void setSidecarDirectory(std::filesystem::path directory);

    /*!
     * \brief Payload size (in value bytes) above which serialize() emits a
     *        sidecar reference instead of an inline array (default 1024).
     */
    void setSidecarThresholdBytes(int64_t bytes);

private:
    /*!
     * \brief Semantic validation of a parsed spec against live components.
     */
    [[nodiscard]] bool validateSpec(const CompositionSpec &spec,
                                    std::string &message) const;
    [[nodiscard]] bool applyComponents(std::string &message);
    [[nodiscard]] bool applyConnections(std::string &message);

    /*!
     * \brief Builds a connection's adaptation chain, returning the output
     *        the consumer should be wired to.
     */
    [[nodiscard]] HydroCouple::IOutput *buildAdaptedChain(
        const ConnectionSpec &link, HydroCouple::IOutput *provider,
        HydroCouple::IInput *consumer, const std::string &context,
        std::string &message);

    /*!
     * \brief Builds a ResultsModelComponent for an "open" component block.
     */
    [[nodiscard]] HydroCouple::IModelComponent *openRecordedComponent(
        const ComponentSpec &block, std::string &message);

    ComponentResolver     m_resolver;
    CompositionSpec       m_spec;
    std::vector<std::unique_ptr<HydroCouple::IModelComponent>> m_opened;
    std::vector<std::unique_ptr<HydroCouple::IAdaptedOutput>> m_adaptedOutputs;
    std::filesystem::path m_sidecarDirectory; //!< Empty = sidecar mode off.
    std::filesystem::path m_documentDirectory; //!< Base for relative refs.
    int64_t               m_sidecarThresholdBytes = 1024;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_MODELINITIALIZER_H
