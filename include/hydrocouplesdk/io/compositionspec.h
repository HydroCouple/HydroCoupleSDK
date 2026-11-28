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
 * documents ship alongside for editors and external tooling: the source
 * tree holds them in \c share/schema/ and an installed SDK in
 * \c share/hydrocouplesdk/schema/ (\c composition-1.0.json and
 * \c composition-1.1.json). The parser is the authority: it ignores keys
 * the schemas forbid (\c additionalProperties is false there) and accepts
 * any string as a writer type, while it adds checks the schemas cannot
 * express (declared components, binding cycles).
 *
 * The guide chapters "Compositions" and "Composition document reference"
 * walk through every block and field.
 *
 * \threadsafety Plain value types; no shared state. Distinct objects may be
 * used from distinct threads; one object must not be written by one thread
 * while another reads it.
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
 * \brief The composition specification version this SDK writes ("1.1").
 *
 * CompositionSpec::parse() reads any 1.x document; a document without
 * \c schema_version parses as "1.0". CompositionSpec::toJson() writes the
 * spec's own CompositionSpec::schemaVersion (raised to "1.1" when the
 * document carries an argument binding), and ModelInitializer::serialize()
 * always writes this value.
 */
inline constexpr const char *kCompositionSchemaVersion = "1.1";

/*!
 * \brief Key marking an argument payload as a binding to another
 *        component's output ("\@from", schema 1.1).
 *
 * A payload of the form
 * \code{.json}
 * { "@from": { "component": "meshgen", "output": "mesh" } }
 * \endcode
 * says the argument's value is produced by that component rather than
 * authored in the document. The key joins the \@uri key of uriresolver.h
 * and the \@ref key of sidecar.h: a payload that *refers* instead of
 * *containing*. ModelInitializer resolves it by running the provider to
 * Finished and handing the named output to
 * IArgument::initialize(const IComponentDataItem&); the reference — never
 * the produced value — is what the document round-trips.
 *
 * CompositionSpec::parse() accepts a binding only when the payload object
 * has this one key, its value is an object with non-empty string
 * \c component and \c output, an optional \c selector is null, the provider
 * is declared in \c components[], the provider is not the consumer, and
 * the bindings form no cycle.
 */
inline constexpr const char *kArgumentBindingKey = "@from";

/*!
 * \brief What a component should do when the composition is initialized.
 *
 * Authored as \c execution.mode in a component block; ModelInitializer acts
 * on it while applying the document.
 */
enum class ExecutionMode
{
    /*!
     * Compute normally. Results are never silently reused — the default,
     * and the only mode that guarantees reproducibility from inputs alone.
     * Spelled "run".
     */
    Run,
    /*!
     * Load previously recorded results and finish immediately. A component
     * that knows how to load its own results may do so; otherwise the host
     * substitutes a ResultsModelComponent built from a run manifest.
     * Spelled "open"; requires \c execution.results.
     */
    Open,
    /*!
     * Restart from a checkpoint (reserved; pairs with
     * ICheckpointableModelComponent). Documents may declare it and
     * CompositionSpec::parse() accepts it; ModelInitializer refuses it as
     * reserved and not yet implemented. Spelled "resume".
     */
    Resume
};

/*!
 * \brief Document spelling of an execution mode: "run", "open" or "resume".
 *
 * \param[in] mode The mode to name.
 * \return The lower-case spelling used in \c execution.mode; "run" for a
 *         value outside the enumeration.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string executionModeName(
    ExecutionMode mode);

/*!
 * \brief Parses an execution mode spelling.
 *
 * Matching is exact and case-sensitive.
 *
 * \param[in]  name The spelling: "run", "open" or "resume".
 * \param[out] mode Set to the parsed mode on success; unchanged otherwise.
 * \return False when \p name is not one of the three spellings.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool executionModeFromName(
    const std::string &name, ExecutionMode &mode);

/*!
 * \brief Optional provenance block of a composition document (\c metadata).
 *
 * Every field is optional free text. The parser checks only that each
 * present field is a string; it does not check the \c created timestamp
 * format, which the schema declares as date-time.
 */
struct HYDROCOUPLESDK_EXPORT CompositionMetadata
{
    std::string id;                //!< Stable composition identifier (\c metadata.id).
    std::string caption;           //!< Display name (\c metadata.caption).
    std::string description;       //!< Free text (\c metadata.description).
    std::string author;            //!< Who authored the composition (\c metadata.author).
    std::string created;           //!< ISO-8601 timestamp (\c metadata.created), not validated.
    std::string sdkVersion;        //!< SDK that wrote the document (\c metadata.sdk_version).
    std::string interfaceVersion;  //!< Interface version it targets (\c metadata.interface_version).

    /*!
     * \brief Whether every field is empty.
     *
     * \return True when no field is set; CompositionSpec::toJson() then
     *         omits the \c metadata block.
     */
    [[nodiscard]] bool empty() const noexcept;
};

/*!
 * \brief How a component instance can be created without a code-side
 *        resolver (what a UI needs to round-trip a composition).
 *
 * Authored as a component block's \c info object. Both fields are optional:
 * a host that resolves components itself (a test, a static registry)
 * simply omits them. Note that naming a library makes a document
 * executable content — the SDK never loads libraries during validation or
 * initialization; ModelInitializer only calls the host's resolver, and any
 * loader is opt-in and allow-listed by the host.
 */
struct HYDROCOUPLESDK_EXPORT ComponentInfoRef
{
    std::string library;          //!< Shared library path (\c info.library).
    std::string componentInfoId;  //!< IComponentInfo id within the library (\c info.component_info_id).

    /*!
     * \brief Whether both fields are empty.
     *
     * \return True when the block names neither a library nor a component
     *         info id; CompositionSpec::toJson() then omits \c info.
     */
    [[nodiscard]] bool empty() const noexcept;

    /*!
     * \brief Member-wise equality of the library and component info id.
     *
     * \return True when both fields are equal.
     */
    [[nodiscard]] bool operator==(const ComponentInfoRef &) const = default;
};

/*!
 * \brief One component block of \c components[].
 */
struct HYDROCOUPLESDK_EXPORT ComponentSpec
{
    std::string      id;       //!< Component identifier (required, unique in the document).
    std::string      caption;  //!< Optional display name.
    ComponentInfoRef info;     //!< Optional instantiation hint (\c info).

    ExecutionMode mode = ExecutionMode::Run; //!< \c execution.mode; Run when absent.

    /*!
     * Run manifest supplying this component's recorded results
     * (\c execution.results); required when \c mode is Open. A relative
     * path resolves against the document's directory (see
     * ModelInitializer::setDocumentDirectory()).
     */
    std::string resultsManifest;

    /*!
     * Argument payloads keyed by argument id, exactly as authored
     * (including sidecar \@ref blocks, \@uri references and \@from
     * bindings). Applied through IArgument::initialize(); always an
     * object, empty when the block has no \c arguments.
     */
    nlohmann::json arguments = nlohmann::json::object();

    /*!
     * \brief Member-wise equality, arguments JSON included.
     *
     * This is change detection for hosts diffing two documents, not
     * identity (a component's identity is its \c id alone).
     *
     * \return True when every member, including the argument JSON, is equal.
     */
    [[nodiscard]] bool operator==(const ComponentSpec &) const = default;
};

/*!
 * \brief One link in a connection's adaptation chain
 *        (\c from.adapted_outputs[]).
 */
struct HYDROCOUPLESDK_EXPORT AdaptedOutputSpec
{
    std::string    factory;   //!< IAdaptedOutputFactory id; empty lets ModelInitializer search every factory.
    std::string    id;        //!< Adapted-output id offered by that factory (required).
    nlohmann::json arguments = nlohmann::json::object(); //!< Argument payloads keyed by argument id, applied before the adapted output initializes.

    /*!
     * \brief Member-wise equality, arguments JSON included.
     *
     * \return True when the factory, id and arguments are equal.
     */
    [[nodiscard]] bool operator==(const AdaptedOutputSpec &) const = default;
};

/*!
 * \brief One provider → consumer connection of \c connections[].
 */
struct HYDROCOUPLESDK_EXPORT ConnectionSpec
{
    std::string fromComponent; //!< Provider component id (\c from.component, required).
    std::string output;        //!< Provider output id (\c from.output).
    std::string toComponent;   //!< Consumer component id (\c to.component, required).
    std::string input;         //!< Consumer input id (\c to.input).

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
     * \brief Member-wise equality, the adaptation chain included.
     *
     * This is change detection, not identity. A connection's identity is
     * its endpoints plus role: two chains on the same (output, input, role)
     * would be a double-wire, so hosts deduplicate by endpoints and treat
     * the chain as content.
     *
     * \return True when the endpoints, role and chain are equal.
     */
    [[nodiscard]] bool operator==(const ConnectionSpec &) const = default;
};

/*!
 * \brief Which workflow strategy drives the composition
 *        (\c workflow.strategy).
 *
 * The SDK IO layer only carries this choice; building the workflow
 * component is the host's job.
 */
enum class WorkflowStrategy
{
    None,        //!< The host drives components itself ("none").
    PullDriven,  //!< Request-reply from a declared trigger input ("pull_driven").
    TimeStepped  //!< Schedule derived from the connection graph ("time_stepped").
};

/*!
 * \brief Document spelling of a strategy: "none", "pull_driven" or
 *        "time_stepped".
 *
 * \param[in] strategy The strategy to name.
 * \return The spelling used in \c workflow.strategy; "none" for a value
 *         outside the enumeration.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string workflowStrategyName(
    WorkflowStrategy strategy);

/*!
 * \brief Parses a strategy spelling.
 *
 * Matching is exact and case-sensitive.
 *
 * \param[in]  name     The spelling: "none", "pull_driven" or "time_stepped".
 * \param[out] strategy Set to the parsed strategy on success; unchanged
 *                      otherwise.
 * \return False when \p name is not one of the three spellings.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool workflowStrategyFromName(
    const std::string &name, WorkflowStrategy &strategy);

/*!
 * \brief Orchestration configuration (the \c workflow block).
 *
 * Carried as validated data: ModelInitializer does not build or run a
 * workflow. CompositionSpec::toJson() writes the block only when
 * \c strategy is not None.
 */
struct HYDROCOUPLESDK_EXPORT WorkflowSpec
{
    WorkflowStrategy strategy = WorkflowStrategy::None; //!< \c workflow.strategy; None when absent.

    std::string id = "workflow"; //!< Workflow component identifier (\c workflow.id).

    std::string triggerComponent; //!< Trigger input's component (\c workflow.trigger.component); required and declared for PullDriven.
    std::string triggerInput;     //!< Trigger input id (\c workflow.trigger.input); required for PullDriven.

    int iterationsPerGroup = 1;   //!< Sweeps per cyclic group (\c workflow.iterations_per_group, integer of at least 1).

    /*!
     * Optional cap on orchestration steps the host should run
     * (\c workflow.max_steps, a non-negative integer); 0 means "until the
     * workflow reports Done".
     */
    int maxSteps = 0;
};

/*!
 * \brief One output writer to construct (an entry of \c writers[]).
 *
 * The SDK does not build writers here — some are feature-gated — so the
 * block is carried as validated data for the host to act on, with the
 * common fields lifted out for convenience. The parser requires a string
 * \c type and a non-empty string \c path; it does not check \c type
 * against the four writer kinds.
 */
struct HYDROCOUPLESDK_EXPORT WriterSpec
{
    std::string    type;   //!< Writer kind; the schema allows "csv", "hdf5_ugrid", "netcdf_ugrid", "geopackage".
    std::string    path;   //!< Destination file (required, non-empty).
    nlohmann::json config = nlohmann::json::object(); //!< The whole writer object as authored, \c type and \c path included.
};

/*!
 * \brief One argument bound to another component's output.
 *
 * The typed form of an \@from payload, always derived from the
 * component blocks' \c arguments — the JSON is the single source of
 * truth, so a spec edited through its arguments cannot drift from a
 * cached edge list.
 */
struct HYDROCOUPLESDK_EXPORT ArgumentBindingSpec
{
    std::string component; //!< Consumer component id.
    std::string argument;  //!< Consumer argument id.
    std::string provider;  //!< Provider component id; empty when the binding is malformed.
    std::string output;    //!< Provider output id; empty when the binding is malformed.
};

/*!
 * \brief Whether an argument payload is an \@from binding reference.
 *
 * True for any object carrying the key — malformed bindings are still
 * bindings, so a consumer never mistakes one for a literal payload and
 * feeds it to a component; parse() is where malformation is refused.
 *
 * \param[in] payload One argument payload.
 * \return True when \p payload is a JSON object containing "\@from".
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool isArgumentBinding(
    const nlohmann::json &payload);

/*!
 * \brief A parsed composition document.
 *
 * Produced by parse() from a JSON document (ModelInitializer converts YAML
 * to JSON first) and written back by toJson(). The blocks the SDK does not
 * apply itself (\c workflow, \c writers, \c run) are carried as validated
 * data for the host.
 */
struct HYDROCOUPLESDK_EXPORT CompositionSpec
{
    std::string schemaVersion = kCompositionSchemaVersion; //!< \c schema_version as read; parse() sets "1.0" when the document has none.

    CompositionMetadata        metadata;    //!< The \c metadata block (empty when absent).
    std::vector<ComponentSpec> components;  //!< The \c components[] blocks, in document order.
    std::vector<ConnectionSpec> connections; //!< The \c connections[] blocks, in document order.
    WorkflowSpec               workflow;    //!< The \c workflow block (strategy None when absent).
    std::vector<WriterSpec>    writers;     //!< The \c writers[] blocks, in document order.

    /*!
     * Where a RunRecorder should write this composition's run manifest
     * (\c run.manifest in the document); empty disables recording. The SDK
     * does not act on it: the host passes it to RunRecorder::finishToFile().
     */
    std::string runManifest;

    /*!
     * \brief Structural parse and validation of a composition document.
     *
     * Checks shapes, required fields, enum spellings, duplicate component
     * ids, that every connection endpoint and pull-driven trigger names a
     * declared component, that every \@from binding is well formed, and
     * that the bindings form no cycle. Keys the parser does not know are
     * ignored. Semantic checks against live components happen in
     * ModelInitializer.
     *
     * \param[in]  document JSON composition document.
     * \param[out] spec     Reset first; the parsed specification on
     *                      success, partly filled on failure.
     * \param[out] message  Actionable failure description naming the
     *                      offending element (for example
     *                      "connections[0].to.component \"ghost\" is not
     *                      declared in components[]"); cleared on success.
     * \return True when the document is structurally valid.
     */
    [[nodiscard]] static bool parse(const nlohmann::json &document,
                                    CompositionSpec &spec,
                                    std::string &message);

    /*!
     * \brief The symmetric write side: the document this spec describes.
     *
     * Optional blocks are omitted when empty, so a minimal composition
     * round-trips to a minimal document. Always written: \c schema_version
     * (raised from "1.0" to "1.1" when an argument binding is present),
     * \c components (each with \c arguments) and \c connections (possibly
     * empty). Written when set: \c metadata, \c info, \c execution (when the
     * mode is not Run or a results path is set), \c workflow (when the
     * strategy is not None), \c writers and \c run.
     *
     * \return The composition document as JSON.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /*!
     * \brief The component block with the given id, or nullptr.
     *
     * \param[in] id Component identifier to look up (exact match).
     * \return Pointer into #components, valid until that vector changes;
     *         nullptr when no block has \p id.
     */
    [[nodiscard]] const ComponentSpec *component(const std::string &id) const;
};

/*!
 * \brief Every \@from binding in the composition, in document order.
 *
 * Derived from the argument payloads on demand (see ArgumentBindingSpec);
 * on a spec that parse() accepted, every entry is well-formed. Arguments are
 * visited in the order nlohmann::json stores object keys (sorted by key)
 * within each component.
 *
 * \param[in] spec The composition to scan.
 * \return One entry per binding payload; provider and output stay empty for
 *         a malformed binding.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::vector<ArgumentBindingSpec>
argumentBindings(const CompositionSpec &spec);

/*!
 * \brief Component ids grouped into initialization stages.
 *
 * Stage N holds the components whose longest chain of \@from providers
 * is N deep, so every provider stands in an earlier stage than its
 * consumers and a binding-free composition is one stage in document
 * order. Within a stage, components keep document order. Hosts use this to
 * preview and drive staged initialization; ModelInitializer orders its
 * apply pass by it.
 *
 * Intended for a spec that parse() accepted. On a hand-built spec whose
 * binding names a provider that is not in \c components, the cycle report
 * looks the provider up with \c std::map::at, which throws
 * std::out_of_range.
 *
 * \param[in]  spec    The composition to stage.
 * \param[out] stages  Cleared first; the staged component ids on success,
 *                     empty on failure.
 * \param[out] message Names every edge of the cycle on failure ("Argument
 *                     bindings form a cycle: \"a\" (argument \"x\") needs
 *                     \"b\"; ..."); cleared on success.
 * \returns False when the bindings form a cycle.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool bindingStages(
    const CompositionSpec &spec,
    std::vector<std::vector<std::string>> &stages,
    std::string &message);

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_COMPOSITIONSPEC_H
