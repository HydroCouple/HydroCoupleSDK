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
 *
 * The guide chapter "Compositions" walks through a complete initialization,
 * and the chapter "Composition document reference" lists every field.
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
 * The initializer never creates or loads a model component itself; the
 * only components it creates are ResultsModelComponent substitutes for
 * \c execution.mode "open" blocks, which it owns (openedComponents()).
 *
 * Typical use: construct with a resolver, optionally install a progress
 * callback and an adapter-factory resolver, call initializeFromFile(), then
 * read spec() for the blocks the host must act on (workflow, writers, run
 * manifest) and component() for the live instances. Each initialize() call
 * replaces spec() but adds to openedComponents() and adaptedOutputs()
 * rather than clearing them, so use one initializer per composition.
 *
 * The resolver is called for every component id during validation and
 * application, so it must be callable (an empty \c std::function makes
 * those calls throw \c std::bad_function_call) and must return the same
 * instance for the same id. The components it returns stay owned by the
 * host and must outlive the initializer's use of them.
 *
 * \threadsafety Not thread-safe; drive from one thread. The progress
 * callback runs on that thread.
 */
class HYDROCOUPLESDK_EXPORT ModelInitializer
{
public:
    /*!
     * \brief Maps a component id from the document to a live instance;
     *        returns nullptr for unknown ids.
     *
     * Non-owning: the host keeps every instance alive while the initializer
     * and the wired composition are in use. A null answer is an error for a
     * "run" block ("Unknown component id") and the expected answer for an
     * "open" block whose results come from a run manifest.
     */
    using ComponentResolver =
        std::function<HydroCouple::IModelComponent *(const std::string &id)>;

    /*!
     * \brief Construct with the component resolver.
     *
     * \param[in] resolver Maps document ids to host-owned instances; must
     *                     be callable (see the class description).
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
    /*!
     * \brief Deleted: copying would duplicate ownership of the opened
     *        components and adapted outputs.
     */
    ModelInitializer &operator=(const ModelInitializer &) = delete;
    /*!
     * \brief Moves the resolver, callbacks, parsed spec and every owned
     *        substitute and adapted output.
     *
     * Pointers obtained from the source (component(), adaptedOutputs())
     * stay valid, because the owned objects themselves do not move.
     */
    ModelInitializer(ModelInitializer &&) noexcept;
    /*!
     * \brief Move assignment; destroys what this initializer owned first.
     *
     * \return This initializer.
     */
    ModelInitializer &operator=(ModelInitializer &&) noexcept;
    /*!
     * \brief Destroys the owned ResultsModelComponent substitutes and
     *        adapted outputs.
     *
     * The SDK's adapted outputs detach from their adaptees when destroyed
     * but do not clear the provider of the inputs they feed, so destroy the
     * initializer only after the composition is no longer run or read.
     */
    ~ModelInitializer();

    // ── Loading ───────────────────────────────────────────────────────────

    /*!
     * \brief Parses and applies a composition document from a file.
     *
     * `.yaml`/`.yml` (exact, lower-case extension) converts through
     * yaml-cpp when the SDK was built with it (otherwise fails with a clear
     * message); every other extension is parsed as JSON. YAML scalars
     * become the narrowest faithful JSON value: boolean, then 64-bit
     * integer, then double, then string.
     *
     * Before parsing, the document directory becomes the absolute parent
     * of \p path, so relative \@uri references, sidecar references (when
     * no sidecar directory is set) and \c execution.results paths resolve
     * beside the document.
     *
     * \param[in]  path    Composition document path.
     * \param[out] message Failure description when returning false:
     *                     "Cannot open composition document: \<path\>",
     *                     "YAML parse error in \<path\>: ...", "JSON parse
     *                     error: ...", the no-yaml-cpp message, or any
     *                     message of initialize().
     * \return True when the document was applied in full.
     */
    [[nodiscard]] bool initializeFromFile(const std::filesystem::path &path,
                                          std::string &message);

    /*!
     * \brief Parses and applies a composition document from JSON text.
     *
     * Leaves the document directory as it is (see setDocumentDirectory()).
     * A \c std::exception thrown anywhere during initialize(), not only a
     * JSON syntax error, is caught and reported with the prefix
     * "JSON parse error: ".
     *
     * \param[in]  text    The JSON document.
     * \param[out] message Failure description when returning false.
     * \return True when the document was applied in full.
     */
    [[nodiscard]] bool initializeFromJsonText(const std::string &text,
                                              std::string &message);

    /*!
     * \brief Validates and applies an already-parsed composition document.
     *
     * Components are applied in bindingStages() order — document order
     * until \@from bindings introduce dependencies, and then providers
     * strictly before their consumers. Per component, arguments are applied
     * in the order the JSON object stores them (sorted by argument id):
     * a binding runs its provider to Finished and hands the named live
     * output to IArgument::initialize(const IComponentDataItem&); a sidecar
     * \@ref payload is read from its file and handed over as a read-only
     * data item; every other payload, with a relative \@uri rebased on the
     * document directory, feeds IArgument::initialize(payload, JSON,
     * message). Then the component's initialize() runs and must reach
     * ComponentStatus::Initialized.
     *
     * An "open" block applies no arguments: its resolver instance (when
     * there is one) is initialized and kept if it reaches Finished;
     * otherwise a ResultsModelComponent is opened from the block's run
     * manifest and must reach Finished.
     *
     * Connections are wired afterwards: each adaptation chain is built
     * (see setAdapterFactoryResolver()), then IOutput::addConsumer() is
     * called on the chain's last output, which carries the provider role
     * when the target is a multi-input. Structural and semantic validation
     * of the whole document happens before anything is applied; a failure
     * while applying leaves the components already applied as they are.
     *
     * \param[in]  document Composition document (JSON form).
     * \param[out] message  Actionable failure description when returning
     *                      false; cleared on success.
     * \return True when every component was applied and every connection
     *         wired.
     */
    [[nodiscard]] bool initialize(const nlohmann::json &document,
                                  std::string &message);

    /*!
     * \brief Notified as the staged apply advances.
     *
     * Called on the initializing thread as each component's apply begins
     * (\p activity "initializing") and again when a bound provider is run
     * to completion (\p activity "running provider"). \p stage and
     * \p stageCount describe the bindingStages() position, so a host can
     * show providers → consumers progression without re-deriving it.
     *
     * Parameters: \p stage is the zero-based stage index; \p stageCount
     * the number of stages; \p componentId the component being applied or,
     * for "running provider", the provider about to run; \p activity one of
     * the two strings above. The "running provider" call is made for every
     * binding, also when the provider has already finished.
     */
    using ProgressCallback =
        std::function<void(int stage, int stageCount,
                           const std::string &componentId,
                           const std::string &activity)>;

    /*!
     * \brief Installs (or clears, with nullptr) the progress callback.
     *
     * \param[in] callback Called synchronously during initialize(); an
     *                     exception it throws propagates out of
     *                     initialize() (initializeFromJsonText() and
     *                     initializeFromFile() catch it as a message).
     */
    void setProgressCallback(ProgressCallback callback);

    /*!
     * \brief Supplies standalone adapted-output factories — host-owned,
     *        for example realized from IAdaptedOutputFactoryComponentInfo
     *        libraries, or SdkAdaptedOutputFactory::instance().
     *
     * A list, not an id→factory lookup, because a chain step that names no
     * factory must enumerate every candidate. Consulted by the chain
     * builder after the provider component's own componentInfo() factories,
     * so a component may deliberately shadow a standalone factory id. The
     * resolver is called once per chain step; the factories stay owned by
     * the host and must outlive the adapted outputs they create.
     */
    using AdapterFactoryResolver =
        std::function<std::vector<HydroCouple::IAdaptedOutputFactory *>()>;

    /*!
     * \brief Installs (or clears, with nullptr) the standalone-factory
     *        resolver.
     *
     * Without one, an adaptation chain can only use the factories of the
     * provider's component info; a step that finds no factory at all fails
     * with "adapted output ... requested but the provider's component
     * exposes no adapter factories and no standalone adapter factories are
     * installed".
     *
     * \param[in] resolver Returns the host's standalone factories.
     */
    void setAdapterFactoryResolver(AdapterFactoryResolver resolver);

    // ── Persistence (round trip) ─────────────────────────────────────────

    /*!
     * \brief Serializes components back to the composition document shape.
     *
     * Walks each component's arguments() calling
     * IArgument::serialize(JSON); connection blocks are reproduced from
     * the live provider wiring of each component's inputs, including
     * adaptation chains (whose arguments come from the live adapted
     * outputs). An argument the last initialize() saw as an \@from binding
     * is written back as that binding, never as its value. With a sidecar
     * directory set, a fixed-width argument whose value bytes exceed the
     * threshold is written to "\<directory\>/\<component\>_\<argument\>.bin" and
     * replaced by an \@ref object.
     *
     * Caption, \c info and \c execution of each block, and the
     * \c metadata, \c workflow, \c writers and \c run blocks, are copied
     * from spec(); \c schema_version is always kCompositionSchemaVersion.
     * Each component is looked up through the resolver, not through
     * component(), so a ResultsModelComponent substitute whose id the
     * resolver does not know fails with "serialize: unknown component id".
     *
     * \param[in]  componentIds Document order of components to persist.
     * \param[out] message      Failure description when the result is
     *                          null; cleared on success.
     * \return The composition document; a null JSON value on failure.
     */
    [[nodiscard]] nlohmann::json serialize(
        const std::vector<std::string> &componentIds,
        std::string &message) const;

    /*!
     * \brief serialize() written to a file (pretty-printed JSON).
     *
     * Creates missing parent directories and overwrites an existing file.
     *
     * \param[in]  componentIds Document order of components to persist.
     * \param[in]  path         Destination file.
     * \param[out] message      Failure description when returning false
     *                          (a serialize() message or "Cannot open
     *                          '\<path\>' for writing").
     * \return True when the document was written and the stream is good.
     */
    [[nodiscard]] bool serializeToFile(
        const std::vector<std::string> &componentIds,
        const std::filesystem::path &path, std::string &message) const;

    /*!
     * \brief True when this build can read YAML documents.
     *
     * \return True when the SDK was built with yaml-cpp.
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
     *
     * \return The last parsed spec; it is replaced at the start of every
     *         initialize(), so after a failed call it holds whatever the
     *         failed parse produced. Empty before the first call.
     */
    [[nodiscard]] const CompositionSpec &spec() const noexcept;

    /*!
     * \brief Structural + semantic validation without applying anything.
     *
     * Never loads component libraries: naming a library in a document makes
     * it executable content, so resolution stays with the caller's resolver.
     * Runs CompositionSpec::parse() and then the checks initialize() makes
     * before applying: every "run" block has a resolver instance, no block
     * asks for "resume", every authored argument id exists on its
     * component, and a connection role names a provider role of a
     * multi-input target (checked only when the target's inputs already
     * exist). Item-level checks (does the output exist, can the input
     * consume it) need initialized components and happen in initialize().
     * Calls the resolver but changes nothing, not even spec().
     *
     * \param[in]  document Composition document.
     * \param[out] message  Actionable failure description when false;
     *                      cleared on success.
     * \return True when the document passes both passes.
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
     *
     * \return The substitutes in creation order. They live as long as this
     *         initializer.
     */
    [[nodiscard]] const std::vector<
        std::unique_ptr<HydroCouple::IModelComponent>> &
    openedComponents() const noexcept;

    /*!
     * \brief The live instance backing a component id: the substitute when
     *        one was created, otherwise the resolver's component.
     *
     * \param[in] id Component id from the document.
     * \return Non-owning pointer; nullptr when neither a substitute nor the
     *         resolver knows \p id (or no resolver was given).
     */
    [[nodiscard]] HydroCouple::IModelComponent *component(
        const std::string &id) const;

    /*!
     * \brief Adapted outputs created while wiring connections (owned).
     *
     * IAdaptedOutputFactory::createAdaptedOutput() yields owning handles;
     * the initializer keeps them alive for the composition's lifetime.
     *
     * \return Every adapted output created so far, in creation order.
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
     * references against the composition document's directory (and the
     * current directory when that is empty too).
     *
     * \param[in] directory Sidecar directory; created on demand by
     *                      serialize(). An empty path turns sidecar mode
     *                      off.
     */
    void setSidecarDirectory(std::filesystem::path directory);

    /*!
     * \brief Sets the base for explicit relative resources in an in-memory
     *        edited document.
     *
     * Relative \@uri references in argument payloads, relative sidecar
     * references (when no sidecar directory is set) and relative
     * \c execution.results manifest paths resolve against it.
     * initializeFromFile() sets it to the document's directory; an empty
     * base means the current directory.
     *
     * \param[in] directory Base directory; stored as an absolute,
     *                      lexically normalized path. Empty clears it.
     */
    void setDocumentDirectory(std::filesystem::path directory);

    /*!
     * \brief Payload size (in value bytes) above which serialize() emits a
     *        sidecar reference instead of an inline array (default 1024).
     *
     * \param[in] bytes Threshold in bytes (element size times element
     *                  count); values below 1 are stored as 1.
     */
    void setSidecarThresholdBytes(int64_t bytes);

private:
    /*!
     * \brief Semantic validation of a parsed spec against live components.
     *
     * \param[in]  spec    Spec that CompositionSpec::parse() accepted.
     * \param[out] message First problem found; cleared on success.
     * \return False for an unknown "run" component, a "resume" block, an
     *         unknown argument id, an unknown connection endpoint, or a
     *         role that does not fit the target input.
     */
    [[nodiscard]] bool validateSpec(const CompositionSpec &spec,
                                    std::string &message) const;
    /*!
     * \brief Applies every component block of m_spec in binding-stage order.
     *
     * \param[out] message First failure, naming the component and argument.
     * \return False at the first argument, provider or initialize() failure.
     */
    [[nodiscard]] bool applyComponents(std::string &message);
    /*!
     * \brief Builds adaptation chains and wires every connection of m_spec.
     *
     * \param[out] message First failure, prefixed "connections[\<i\>]".
     * \return False at the first missing item, chain failure, unknown role
     *         or exception thrown by IOutput::addConsumer().
     */
    [[nodiscard]] bool applyConnections(std::string &message);

    /*!
     * \brief Builds a connection's adaptation chain, returning the output
     *        the consumer should be wired to.
     *
     * For each step, the candidates are the factories of the current
     * output's component info followed by the standalone factories; the
     * first candidate whose id matches (any, when the step names none) and
     * which offers the step id for (current output, consumer) creates it.
     * Its arguments are applied, it is initialized, and it becomes the
     * current output.
     *
     * \param[in]  link     The connection being wired.
     * \param[in]  provider The provider's own output.
     * \param[in]  consumer The consumer input (passed to the factories).
     * \param[in]  context  "connections[\<i\>]", the message prefix.
     * \param[out] message  Failure description when returning nullptr.
     * \return The last output of the chain (\p provider for an empty
     *         chain); nullptr on failure.
     */
    [[nodiscard]] HydroCouple::IOutput *buildAdaptedChain(
        const ConnectionSpec &link, HydroCouple::IOutput *provider,
        HydroCouple::IInput *consumer, const std::string &context,
        std::string &message);

    /*!
     * \brief Builds a ResultsModelComponent for an "open" component block.
     *
     * A relative manifest path resolves against the document directory.
     * The substitute exposes the manifest's entries for the block's id and
     * takes that id; it is appended to openedComponents().
     *
     * \param[in]  block   The "open" component block.
     * \param[out] message "Component \"\<id\>\" (execution mode \"open\"): "
     *                     followed by the manifest error, on failure.
     * \return Non-owning pointer to the new substitute; nullptr when the
     *         manifest cannot be read.
     */
    [[nodiscard]] HydroCouple::IModelComponent *openRecordedComponent(
        const ComponentSpec &block, std::string &message);

    /*!
     * \brief Runs a binding's provider to Finished so its outputs are the
     *        values they will ever be.
     *
     * A provider already Finished — an "open" substitute, or one an
     * earlier binding ran — is left alone. Otherwise: validate, prepare,
     * update until Done, finish; anything short of Finished fails with
     * the provider's own errors appended. A provider that has not reported
     * Done, Finished or Failed after 1,000,000 update() calls fails with
     * "did not report Done within 1000000 update steps".
     *
     * \param[in]  providerId Provider component id.
     * \param[in]  context    "Component \"\<id\>\", argument \"\<arg\>\"", the
     *                        message prefix.
     * \param[out] message    Failure description, listing validation
     *                        problems or the provider's errors.
     * \return True when the provider is Finished.
     */
    [[nodiscard]] bool ensureProviderFinished(const std::string &providerId,
                                              const std::string &context,
                                              std::string &message);

    ComponentResolver      m_resolver;               //!< Host's id-to-instance lookup.
    ProgressCallback       m_progress;               //!< Optional staged-apply observer.
    AdapterFactoryResolver m_adapterFactoryResolver; //!< Optional standalone adapter factories.
    CompositionSpec       m_spec;                    //!< Spec of the most recent initialize().
    std::vector<std::unique_ptr<HydroCouple::IModelComponent>> m_opened; //!< Owned "open" substitutes.
    std::vector<std::unique_ptr<HydroCouple::IAdaptedOutput>> m_adaptedOutputs; //!< Owned adaptation-chain links.
    std::filesystem::path m_sidecarDirectory; //!< Empty = sidecar mode off.
    std::filesystem::path m_documentDirectory; //!< Base for relative refs.
    int64_t               m_sidecarThresholdBytes = 1024; //!< Sidecar threshold in value bytes (at least 1).
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_MODELINITIALIZER_H
