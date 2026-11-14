// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   compositionspec.h
 * \author Caleb Buahin
 * \version 2.1.0
 * \brief  Composition Specification v1 — the SDK-owned, validatable
 *         description of a coupled model composition.
 *
 * A composition document is JSON (or YAML, converted to JSON at the
 * boundary, so one specification governs both). It describes *what* a
 * composition is — components, their arguments, how they are connected,
 * how execution is orchestrated, and where results are written — without
 * naming any host-specific machinery. It replaces the XML compositions
 * that previously lived in HydroCoupleComposer, so a GUI, a CLI runner, a
 * test harness, and a cloud worker all read the same file.
 *
 * This header holds the parsed, strongly typed form. Parsing performs
 * *structural* validation (shapes, enums, required fields, cross
 * references between blocks); ModelInitializer performs *semantic*
 * validation against live components (does this argument exist, can this
 * input consume that output) before applying anything. Formal JSON Schema
 * documents ship alongside for editors and external tooling — see
 * \c share/hydrocouplesdk/schema/composition-1.1.json.
 *
 * \threadsafety Plain value types; no shared state.
 */

#ifndef HYDROCOUPLESDK_IO_COMPOSITIONSPEC_H
#define HYDROCOUPLESDK_IO_COMPOSITIONSPEC_H

#include "hydrocouplesdk/export.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace HydroCouple::SDK::IO {

/*!
 * \brief The composition specification version this SDK reads and writes.
 */
inline constexpr const char *kCompositionSchemaVersion = "1.1";

/*!
 * \brief Key marking an argument payload as a binding to another
 *        component's output.
 *
 * A payload of the form
 * \code{.json}
 * { "@from": { "component": "meshgen", "output": "mesh" } }
 * \endcode
 * says the argument's value is produced by that component rather than
 * authored in the document. The sigil joins \c @uri and \c @ref: a payload
 * that *refers* instead of *containing*. ModelInitializer resolves it by
 * running the provider to Finished and handing the named output to
 * IArgument::initialize(const IComponentDataItem&); the reference — never
 * the produced value — is what the document round-trips.
 */
inline constexpr const char *kArgumentBindingKey = "@from";

/*!
 * \brief What a component should do when the composition is initialized.
 */
enum class ExecutionMode
{
    /*!
     * Compute normally. Results are never silently reused — the default,
     * and the only mode that guarantees reproducibility from inputs alone.
     */
    Run,
    /*!
     * Load previously recorded results and finish immediately. A component
     * that knows how to load its own results may do so; otherwise the host
     * substitutes a ResultsModelComponent built from a run manifest.
     */
    Open,
    /*!
     * Restart from a checkpoint (reserved; pairs with
     * ICheckpointableModelComponent). Documents may declare it; the SDK
     * currently diagnoses it as unimplemented.
     */
    Resume
};

/*!
 * \brief Human-readable name of an execution mode ("run"/"open"/"resume").
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string executionModeName(
    ExecutionMode mode);

/*!
 * \brief Parses an execution mode name; false when unrecognized.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool executionModeFromName(
    const std::string &name, ExecutionMode &mode);

/*!
 * \brief Optional provenance block of a composition document.
 */
struct HYDROCOUPLESDK_EXPORT CompositionMetadata
{
    std::string id;                //!< Stable composition identifier.
    std::string caption;           //!< Display name.
    std::string description;       //!< Free text.
    std::string author;            //!< Who authored the composition.
    std::string created;           //!< ISO-8601 timestamp.
    std::string sdkVersion;        //!< SDK that wrote the document.
    std::string interfaceVersion;  //!< Interface version it targets.

    [[nodiscard]] bool empty() const noexcept;
};

/*!
 * \brief How a component instance can be created without a code-side
 *        resolver (what a UI needs to round-trip a composition).
 *
 * Both fields are optional: a host that resolves components itself (a test,
 * a static registry) simply omits them. Note that naming a library makes a
 * document executable content — the SDK never loads libraries during
 * validation, and any loader is opt-in and allow-listed by the host.
 */
struct HYDROCOUPLESDK_EXPORT ComponentInfoRef
{
    std::string library;          //!< Shared library path.
    std::string componentInfoId;  //!< IComponentInfo id within the library.

    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] bool operator==(const ComponentInfoRef &) const = default;
};

/*!
 * \brief One component block.
 */
struct HYDROCOUPLESDK_EXPORT ComponentSpec
{
    std::string      id;       //!< Component identifier (required).
    std::string      caption;  //!< Optional display name.
    ComponentInfoRef info;     //!< Optional instantiation hint.

    ExecutionMode mode = ExecutionMode::Run; //!< execution.mode.

    /*!
     * Run manifest supplying this component's recorded results when
     * \c mode is Open. Relative paths resolve against the document.
     */
    std::string resultsManifest;

    /*!
     * Argument payloads keyed by argument id, exactly as authored
     * (including sidecar \c @ref blocks). Applied through
     * IArgument::initialize().
     */
    nlohmann::json arguments = nlohmann::json::object();

    /*!
     * Member-wise, arguments JSON included — this is change detection for
     * hosts diffing two documents, not identity (a component's identity is
     * its \c id alone).
     */
    [[nodiscard]] bool operator==(const ComponentSpec &) const = default;
};

/*!
 * \brief One link in a connection's adaptation chain.
 */
struct HYDROCOUPLESDK_EXPORT AdaptedOutputSpec
{
    std::string    factory;   //!< IAdaptedOutputFactory id.
    std::string    id;        //!< Adapted-output id offered by that factory.
    nlohmann::json arguments = nlohmann::json::object(); //!< Its arguments.

    [[nodiscard]] bool operator==(const AdaptedOutputSpec &) const = default;
};

/*!
 * \brief One provider → consumer connection.
 */
struct HYDROCOUPLESDK_EXPORT ConnectionSpec
{
    std::string fromComponent; //!< Provider component id.
    std::string output;        //!< Provider output id.
    std::string toComponent;   //!< Consumer component id.
    std::string input;         //!< Consumer input id.

    /*!
     * Ordered adaptation chain applied to the provider before the
     * consumer sees it (empty for a direct connection).
     */
    std::vector<AdaptedOutputSpec> adaptedOutputs;

    /*!
     * Provider role identifier when the consumer is an IMultiInput;
     * empty for single inputs or role-agnostic multi-inputs.
     */
    std::string role;

    /*!
     * Member-wise, the adaptation chain included — change detection, not
     * identity. A connection's identity is its endpoints plus role: two
     * chains on the same (output, input, role) would be a double-wire, so
     * hosts deduplicate by endpoints and treat the chain as content.
     */
    [[nodiscard]] bool operator==(const ConnectionSpec &) const = default;
};

/*!
 * \brief Which workflow strategy drives the composition.
 */
enum class WorkflowStrategy
{
    None,        //!< The host drives components itself.
    PullDriven,  //!< Request-reply from a declared trigger input.
    TimeStepped  //!< Schedule derived from the connection graph.
};

/*!
 * \brief Human-readable strategy name ("none"/"pull_driven"/"time_stepped").
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string workflowStrategyName(
    WorkflowStrategy strategy);

/*!
 * \brief Parses a strategy name; false when unrecognized.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool workflowStrategyFromName(
    const std::string &name, WorkflowStrategy &strategy);

/*!
 * \brief Orchestration configuration.
 */
struct HYDROCOUPLESDK_EXPORT WorkflowSpec
{
    WorkflowStrategy strategy = WorkflowStrategy::None;

    std::string id = "workflow"; //!< Workflow component identifier.

    std::string triggerComponent; //!< Trigger input's component (pull mode).
    std::string triggerInput;     //!< Trigger input id (pull mode).

    int iterationsPerGroup = 1;   //!< Sweeps per cyclic group (stepped mode).

    /*!
     * Optional cap on orchestration steps the host should run; 0 means
     * "until the workflow reports Done".
     */
    int maxSteps = 0;
};

/*!
 * \brief One output writer to construct.
 *
 * The SDK does not build writers here — some are feature-gated — so the
 * block is carried as validated data for the host to act on, with the
 * common fields lifted out for convenience.
 */
struct HYDROCOUPLESDK_EXPORT WriterSpec
{
    std::string    type;   //!< "csv", "hdf5_ugrid", "netcdf_ugrid", "geopackage".
    std::string    path;   //!< Destination file.
    nlohmann::json config = nlohmann::json::object(); //!< Remaining keys.
};

/*!
 * \brief One argument bound to another component's output.
 *
 * The typed form of an \c @from payload, always derived from the
 * component blocks' \c arguments — the JSON is the single source of
 * truth, so a spec edited through its arguments cannot drift from a
 * cached edge list.
 */
struct HYDROCOUPLESDK_EXPORT ArgumentBindingSpec
{
    std::string component; //!< Consumer component id.
    std::string argument;  //!< Consumer argument id.
    std::string provider;  //!< Provider component id.
    std::string output;    //!< Provider output id.
};

/*!
 * \brief Whether an argument payload is an \c @from binding reference.
 *
 * True for any object carrying the key — malformed bindings are still
 * bindings, so a consumer never mistakes one for a literal payload and
 * feeds it to a component; parse() is where malformation is refused.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool isArgumentBinding(
    const nlohmann::json &payload);

/*!
 * \brief A parsed composition document.
 */
struct HYDROCOUPLESDK_EXPORT CompositionSpec
{
    std::string schemaVersion = kCompositionSchemaVersion;

    CompositionMetadata        metadata;
    std::vector<ComponentSpec> components;
    std::vector<ConnectionSpec> connections;
    WorkflowSpec               workflow;
    std::vector<WriterSpec>    writers;

    /*!
     * Where a RunRecorder should write this composition's run manifest
     * (\c run.manifest in the document); empty disables recording.
     */
    std::string runManifest;

    /*!
     * \brief Structural parse and validation of a composition document.
     *
     * Checks shapes, required fields, enum spellings, duplicate component
     * ids, and that every connection endpoint names a declared component.
     * Semantic checks against live components happen in ModelInitializer.
     *
     * \param[in]  document JSON composition document.
     * \param[out] spec     The parsed specification on success.
     * \param[out] message  Actionable failure description otherwise.
     */
    [[nodiscard]] static bool parse(const nlohmann::json &document,
                                    CompositionSpec &spec,
                                    std::string &message);

    /*!
     * \brief The symmetric write side: the document this spec describes.
     *
     * Optional blocks are omitted when empty, so a minimal composition
     * round-trips to a minimal document.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /*!
     * \brief The component block with the given id, or nullptr.
     */
    [[nodiscard]] const ComponentSpec *component(const std::string &id) const;
};

/*!
 * \brief Every \c @from binding in the composition, in document order.
 *
 * Derived from the argument payloads on demand (see ArgumentBindingSpec);
 * on a spec that parse() accepted, every entry is well-formed.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::vector<ArgumentBindingSpec>
argumentBindings(const CompositionSpec &spec);

/*!
 * \brief Component ids grouped into initialization stages.
 *
 * Stage N holds the components whose longest chain of \c @from providers
 * is N deep, so every provider stands in an earlier stage than its
 * consumers and a binding-free composition is one stage in document
 * order. Hosts use this to preview and drive staged initialization;
 * ModelInitializer orders its apply pass by it.
 *
 * \param[in]  spec    The composition to stage.
 * \param[out] stages  The staged component ids on success.
 * \param[out] message Names an edge of the cycle on failure.
 * \returns False when the bindings form a cycle.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool bindingStages(
    const CompositionSpec &spec,
    std::vector<std::vector<std::string>> &stages,
    std::string &message);

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_COMPOSITIONSPEC_H
