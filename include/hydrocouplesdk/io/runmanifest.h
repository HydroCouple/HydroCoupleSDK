// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   runmanifest.h
 * \author Caleb Buahin
 * \version 2.1.0
 * \brief  RunManifest and RunRecorder — the record of an executed
 *         composition and the index into its results.
 *
 * A completed run is described explicitly, never inferred from whatever
 * files happen to exist on disk. The manifest records what ran, by which
 * SDK and interface versions, how it ended, each component's final status,
 * and a catalog mapping every recorded data item to the artifact holding
 * its values. That catalog is what lets analysis and visualization tools
 * reopen a run *without any model binary* — see ResultsModelComponent.
 *
 * Writing is deliberately opt-in: a composition that does not configure a
 * manifest simply produces artifacts, exactly as before. Reuse of results
 * is therefore always a declared choice (see ExecutionMode::Open), which
 * is what keeps a rerun reproducible.
 *
 * The manifest document follows the schema \c run-1.0.json (source tree
 * \c share/schema/, installed SDK \c share/hydrocouplesdk/schema/); the
 * guide chapter "Run manifest reference" lists every field.
 *
 * \threadsafety RunManifest is a value type. RunRecorder is driven from
 * the orchestrating thread, after the IOThread has finalized its writers.
 */

#ifndef HYDROCOUPLESDK_IO_RUNMANIFEST_H
#define HYDROCOUPLESDK_IO_RUNMANIFEST_H

#include "hydrocouplesdk/io/compositionspec.h"
#include "hydrocouplesdk/io/outputwriter.h"
#include "hydrocouplesdk/io/resultentry.h"
#include "hydrocouple.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace HydroCouple::SDK::IO {

/*!
 * \brief How a run ended (\c run.status).
 *
 * Chosen by the host and passed to RunRecorder::finish(); the SDK does not
 * infer it from component statuses.
 */
enum class RunStatus
{
    Completed, //!< Ran to completion ("completed").
    Failed,    //!< A component or writer failed; results remain readable ("failed").
    Stopped    //!< Ended early on request (cooperative stop) ("stopped").
};

/*!
 * \brief Run status ↔ manifest spelling.
 *
 * \param[in] status The status to name.
 * \return "completed", "failed" or "stopped"; "completed" for a value
 *         outside the enumeration.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string runStatusName(
    RunStatus status);

/*!
 * \brief Parses a run status spelling; false when unrecognized.
 *
 * \param[in]  name   "completed", "failed" or "stopped" (case-sensitive).
 * \param[out] status Set on success; unchanged otherwise.
 * \return False for any other spelling.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool runStatusFromName(
    const std::string &name, RunStatus &status);

/*!
 * \brief One component's outcome in a run (an entry of \c components[]).
 */
struct HYDROCOUPLESDK_EXPORT ComponentOutcome
{
    std::string              id;          //!< Component identifier (\c id).
    std::string              caption;     //!< Display name, when known (\c caption).
    std::string              finalStatus; //!< ComponentStatus spelling of AbstractModelComponent::statusToString(), for example "Finished" (\c final_status).
    std::vector<std::string> errors;      //!< Messages of the component's errors() at finish, read without clearing (\c errors).
};

/*!
 * \brief The record of one executed composition (a \c run.json document).
 *
 * Built by RunRecorder, written by write(), read back by read(). Plain
 * value type.
 */
struct HYDROCOUPLESDK_EXPORT RunManifest
{
    std::string schemaVersion = kRunManifestSchemaVersion; //!< \c schema_version; any 1.x is read.

    std::string id;           //!< Run identifier (\c run.id; RunRecorder generates a UUID when none is given).
    std::string caption;      //!< Display name (\c run.caption).
    std::string started;      //!< ISO-8601 UTC, "YYYY-MM-DDTHH:MM:SSZ" (\c run.started).
    std::string finished;     //!< ISO-8601 UTC (\c run.finished).
    double      wallSeconds = 0.0; //!< Elapsed wall-clock seconds between RunRecorder construction and finish() (\c run.wall_seconds; written only when positive).
    RunStatus   status = RunStatus::Completed; //!< How the run ended (\c run.status).
    std::string host;         //!< Free text naming the machine (\c run.host); RunRecorder leaves it empty.

    std::string sdkVersion;       //!< SDK that produced the run (\c versions.sdk).
    std::string interfaceVersion; //!< Interface version it targeted (\c versions.interface); RunRecorder writes the SDK version, the two being released in lockstep.

    std::string    compositionPath;   //!< Document path, when referenced (\c composition.path), as given to RunRecorder::setComposition().
    std::string    compositionSha256; //!< Its hash (\c composition.sha256); RunRecorder never sets it, a host may.
    nlohmann::json compositionEmbedded; //!< Full document, when embedded (\c composition.embedded); null otherwise.

    std::vector<ComponentOutcome> components; //!< One outcome per registered component (\c components[]).
    std::vector<ResultEntry>      results;    //!< The results catalog (\c results[]).

    /*!
     * \brief Directory the manifest was read from; artifact paths in
     *        results[] resolve against it. Set by read().
     */
    std::filesystem::path directory;

    /*!
     * \brief Entries produced by one component.
     *
     * \param[in] componentId Component id; "" selects unattributed entries.
     * \return Pointers into #results in catalog order, valid until
     *         #results changes.
     */
    [[nodiscard]] std::vector<const ResultEntry *> entriesFor(
        const std::string &componentId) const;

    /*!
     * \brief The entry for one component's item, or nullptr.
     *
     * \param[in] componentId Producing component id.
     * \param[in] itemId      Recorded item id.
     * \return The first matching entry (an item recorded by several writers
     *         has several), valid until #results changes.
     */
    [[nodiscard]] const ResultEntry *entry(const std::string &componentId,
                                           const std::string &itemId) const;

    /*!
     * \brief Path of an entry's artifact (resolved against directory when
     *        relative).
     *
     * The result is absolute only when the artifact or #directory is.
     *
     * \param[in] entry An entry of this manifest.
     * \return \c entry.artifact unchanged when absolute or when #directory
     *         is empty; otherwise #directory / \c entry.artifact.
     */
    [[nodiscard]] std::filesystem::path artifactPath(
        const ResultEntry &entry) const;

    /*!
     * \brief Serializes to the run manifest document shape.
     *
     * Always writes \c schema_version, \c run (with \c id and
     * \c status) and \c results; \c versions, \c composition and
     * \c components only when they hold something. #directory is not
     * written.
     *
     * \return The manifest document.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /*!
     * \brief Parses a run manifest document.
     *
     * Requires an object with a \c run object and a \c results array; a
     * present \c schema_version must be a string whose major version is 1;
     * a present \c run.status must be one of the three spellings. Every
     * \c results[] entry goes through ResultEntry::fromJson(). Missing
     * optional fields keep their defaults; a non-object \c versions or
     * \c composition is ignored. A present field of the wrong JSON type
     * (for example a numeric \c run.id or a non-string \c run.status) makes
     * nlohmann::json throw \c nlohmann::json::type_error.
     *
     * \param[in]  json     The manifest document.
     * \param[out] manifest Reset first; the parsed manifest on success.
     *                      #directory stays empty.
     * \param[out] message  Failure description; cleared on success.
     * \return True when the document is a valid manifest.
     */
    [[nodiscard]] static bool fromJson(const nlohmann::json &json,
                                       RunManifest &manifest,
                                       std::string &message);

    /*!
     * \brief Reads a manifest from a file, recording its directory so
     *        relative artifact paths resolve.
     *
     * JSON syntax errors are caught and reported; type errors from
     * fromJson() propagate as \c nlohmann::json::type_error.
     *
     * \param[in]  path     Manifest file.
     * \param[out] manifest The parsed manifest; #directory is set to the
     *                      parent of \p path ("." when it has none), as
     *                      given, not made absolute.
     * \param[out] message  "Cannot open run manifest \"\<path\>\"", "Run
     *                      manifest \"\<path\>\" is not valid JSON: ...", or
     *                      "Run manifest \"\<path\>\": " followed by the
     *                      fromJson() message.
     * \return True when the file was read and parsed.
     */
    [[nodiscard]] static bool read(const std::filesystem::path &path,
                                   RunManifest &manifest,
                                   std::string &message);

    /*!
     * \brief Writes the manifest as pretty-printed JSON.
     *
     * Creates missing parent directories and overwrites an existing file.
     * Artifacts are referenced as stored in each entry, so write the
     * manifest in the directory the relative artifact paths are relative to.
     *
     * \param[in]  path    Destination file.
     * \param[out] message "Cannot write run manifest \"\<path\>\"" or
     *                     "Failed while writing run manifest \"\<path\>\""
     *                     on failure; cleared on success.
     * \return True when the file was written.
     */
    [[nodiscard]] bool write(const std::filesystem::path &path,
                             std::string &message) const;
};

/*!
 * \brief Assembles a RunManifest from a live composition.
 *
 * Usage: construct before the run, note the writers whose catalogs should
 * be collected, add the components taking part, then finish() after the
 * IOThread has finalized (writer catalogs are only valid then).
 *
 * Call finish() or finishToFile() once: each call appends the component
 * outcomes and writer catalogs again to the manifest it keeps.
 *
 * \threadsafety Not thread-safe; drive from the orchestrating thread.
 */
class HYDROCOUPLESDK_EXPORT RunRecorder
{
public:
    /*!
     * \brief Starts recording (stamps the start time and a run id).
     *
     * Also records the SDK version as both \c versions.sdk and
     * \c versions.interface, and starts the wall-clock timer.
     *
     * \param[in] runId Identifier; a UUID is generated when empty.
     */
    explicit RunRecorder(std::string runId = {});

    /*!
     * \brief Display name for the run.
     *
     * \param[in] caption Written as \c run.caption.
     */
    void setCaption(std::string caption);

    /*!
     * \brief Records the composition this run executes: its path and
     *        optionally the document itself.
     *
     * No hash is computed: \c composition.sha256 stays empty unless the
     * host sets RunManifest::compositionSha256 on the returned manifest.
     *
     * \param[in] path     Written as \c composition.path, as given.
     * \param[in] document The composition document to embed.
     * \param[in] embed    When true and \p document is not null, the
     *                     document is copied into \c composition.embedded.
     */
    void setComposition(const std::filesystem::path &path,
                        const nlohmann::json &document = {},
                        bool embed = false);

    /*!
     * \brief Registers a writer whose catalog() is collected at finish().
     *
     * The recorder holds shared ownership, so the writer lives at least as
     * long as the recorder. nullptr is ignored.
     *
     * \param[in] writer The writer, usually also registered with the
     *                   IOThread that drives it.
     */
    void addWriter(std::shared_ptr<IOutputWriter> writer);

    /*!
     * \brief Registers a component whose final status and diagnostics are
     *        recorded at finish(). Non-owning.
     *
     * The components are also used to attribute catalog entries and, in
     * finishToFile(), to read each recorded output's metadata.
     *
     * \param[in] component Host-owned component that must outlive finish();
     *                      nullptr is ignored.
     */
    void addComponent(HydroCouple::IModelComponent *component);

    /*!
     * \brief Closes the record: stamps the end time, collects component
     *        statuses and writer catalogs, and returns the manifest.
     *
     * Call after IOThread::finish(), whose finalize() calls are what make
     * writer catalogs valid.
     *
     * For each registered component: id, caption, status spelling and the
     * messages of errors() (not cleared). For each writer, in registration
     * order, every catalog entry; an entry without a component id is given
     * the first registered component that has an output or a result with
     * the entry's item id. finish() adds no units, quantity or geometry;
     * finishToFile() does.
     *
     * \param[in] status How the run ended.
     * \return A copy of the assembled manifest (the manifest's \c directory
     *         empty).
     */
    [[nodiscard]] RunManifest finish(RunStatus status = RunStatus::Completed);

    /*!
     * \brief finish() written straight to a manifest file, with the
     *        metadata a reader needs to serve values without model code.
     *
     * For every entry whose item is an output of a registered component
     * (the attributed one, when the entry has a component id):
     * - \c description becomes the output's caption;
     * - \c dimensions become "time" followed by the output's dimension ids,
     *   and must match the shape's rank;
     * - when the output's value definition is an IQuantity with a unit and
     *   the entry's units are empty or equal the unit caption, \c units
     *   becomes the unit caption and \c quantity records its SI scale,
     *   offset, dimension powers and value kind;
     * - an "elapsed_s" or "seconds_since_start" time axis gets
     *   \c epoch_julian_day from the component's simulation period, when
     *   the component is an ITimeModelComponent with one;
     * - for an IPolyhedralSurfaceComponentDataItem, the surface's mesh view
     *   is written once per surface to
     *   "geometry_\<uuid\>_\<n\>.json" (schema "hc.recorded-mesh/1.0") beside
     *   the manifest, and \c geometry names it with the entity axis, plus
     *   the layer axis and the recorded interface elevations for a layered
     *   item; \c location is set from the item.
     * Entries for vertical-coordinate slices (see verticalFieldId()) are
     * folded into their items and removed.
     *
     * \param[in]  path    Manifest destination (parents created). Give it a
     *                     directory: a bare file name makes creating the
     *                     parent directory fail (see the guide's
     *                     diagnostics).
     * \param[out] message Failure description when returning false:
     *                     "ambiguous recorded output identity: \<item\>",
     *                     "recorded dimension rank mismatch", "recorded
     *                     surface has no mesh view", "could not write
     *                     recorded geometry", "recorded mesh entity
     *                     dimension is absent", "unsupported recorded mesh
     *                     location", "recorded layer dimension is absent",
     *                     "recording result metadata: " plus an exception
     *                     message, or a RunManifest::write() message.
     * \param[in]  status  How the run ended.
     * \return True when the manifest was written.
     */
    [[nodiscard]] bool finishToFile(const std::filesystem::path &path,
                                    std::string &message,
                                    RunStatus status = RunStatus::Completed);

private:
    RunManifest                                 m_manifest;   //!< The record being assembled.
    std::vector<std::shared_ptr<IOutputWriter>> m_writers;    //!< Writers whose catalogs are collected.
    std::vector<HydroCouple::IModelComponent *> m_components; //!< Non-owning registered components.
    std::chrono::steady_clock::time_point       m_start;      //!< Construction time, for wall_seconds.
};

/*!
 * \brief Builds the composition that reopens a recorded run for analysis.
 *
 * Every component that contributed results becomes an \c execution.mode
 * "open" block pointing at \p manifestPath, with no connections: the
 * result is a document a UI can hand straight to ModelInitializer to
 * browse a finished run — no model libraries involved. Add connections to
 * it (or edit it) to drive a new composition from stored results.
 *
 * \param[in] manifest     The run to reopen.
 * \param[in] manifestPath Path recorded in each block's
 *                         \c execution.results (relative paths resolve
 *                         against the composition document).
 * \return A spec with schema version kCompositionSchemaVersion, metadata
 *         taken from the run (id, caption or "Reopened run \<id\>", finished
 *         time, versions) and one "open" block per distinct component id
 *         in catalog order, with the outcome's caption. Unattributed
 *         entries give one block with an empty id, which
 *         CompositionSpec::parse() accepts; its ResultsModelComponent
 *         substitute then serves every entry of the manifest.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT CompositionSpec compositionForRun(
    const RunManifest &manifest, const std::string &manifestPath);

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_RUNMANIFEST_H
