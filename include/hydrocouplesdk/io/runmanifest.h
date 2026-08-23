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
 * \threadsafety RunManifest is a value type. RunRecorder is driven from
 * the orchestrating thread, after the IOThread has finalized its writers.
 */

#ifndef HYDROCOUPLESDK_IO_RUNMANIFEST_H
#define HYDROCOUPLESDK_IO_RUNMANIFEST_H

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
 * \brief How a run ended.
 */
enum class RunStatus
{
    Completed, //!< Ran to completion.
    Failed,    //!< A component or writer failed; results remain readable.
    Stopped    //!< Ended early on request (cooperative stop).
};

/*!
 * \brief Run status ↔ manifest spelling.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string runStatusName(
    RunStatus status);

/*!
 * \brief Parses a run status spelling; false when unrecognized.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool runStatusFromName(
    const std::string &name, RunStatus &status);

/*!
 * \brief One component's outcome in a run.
 */
struct HYDROCOUPLESDK_EXPORT ComponentOutcome
{
    std::string              id;          //!< Component identifier.
    std::string              caption;     //!< Display name, when known.
    std::string              finalStatus; //!< ComponentStatus spelling.
    std::vector<std::string> errors;      //!< Drained diagnostics.
};

/*!
 * \brief The record of one executed composition.
 */
struct HYDROCOUPLESDK_EXPORT RunManifest
{
    std::string schemaVersion = kRunManifestSchemaVersion;

    std::string id;           //!< Run identifier (generated when empty).
    std::string caption;      //!< Display name.
    std::string started;      //!< ISO-8601 UTC.
    std::string finished;     //!< ISO-8601 UTC.
    double      wallSeconds = 0.0;
    RunStatus   status = RunStatus::Completed;
    std::string host;

    std::string sdkVersion;       //!< SDK that produced the run.
    std::string interfaceVersion; //!< Interface version it targeted.

    std::string    compositionPath;   //!< Document path, when referenced.
    std::string    compositionSha256; //!< Its hash, when computed.
    nlohmann::json compositionEmbedded; //!< Full document, when embedded.

    std::vector<ComponentOutcome> components;
    std::vector<ResultEntry>      results;

    /*!
     * \brief Directory the manifest was read from; artifact paths in
     *        results[] resolve against it. Set by read().
     */
    std::filesystem::path directory;

    /*!
     * \brief Entries produced by one component.
     */
    [[nodiscard]] std::vector<const ResultEntry *> entriesFor(
        const std::string &componentId) const;

    /*!
     * \brief The entry for one component's item, or nullptr.
     */
    [[nodiscard]] const ResultEntry *entry(const std::string &componentId,
                                           const std::string &itemId) const;

    /*!
     * \brief Absolute path of an entry's artifact (resolved against
     *        directory when relative).
     */
    [[nodiscard]] std::filesystem::path artifactPath(
        const ResultEntry &entry) const;

    /*!
     * \brief Serializes to the run manifest document shape.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /*!
     * \brief Parses a run manifest document.
     */
    [[nodiscard]] static bool fromJson(const nlohmann::json &json,
                                       RunManifest &manifest,
                                       std::string &message);

    /*!
     * \brief Reads a manifest from a file, recording its directory so
     *        relative artifact paths resolve.
     */
    [[nodiscard]] static bool read(const std::filesystem::path &path,
                                   RunManifest &manifest,
                                   std::string &message);

    /*!
     * \brief Writes the manifest as pretty-printed JSON.
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
 */
class HYDROCOUPLESDK_EXPORT RunRecorder
{
public:
    /*!
     * \brief Starts recording (stamps the start time and a run id).
     * \param[in] runId Identifier; a UUID is generated when empty.
     */
    explicit RunRecorder(std::string runId = {});

    /*!
     * \brief Display name for the run.
     */
    void setCaption(std::string caption);

    /*!
     * \brief Records the composition this run executes: its path, an
     *        optional hash, and optionally the document itself.
     */
    void setComposition(const std::filesystem::path &path,
                        const nlohmann::json &document = {},
                        bool embed = false);

    /*!
     * \brief Registers a writer whose catalog() is collected at finish().
     *        Non-owning; the writer must outlive the recorder's finish().
     */
    void addWriter(std::shared_ptr<IOutputWriter> writer);

    /*!
     * \brief Registers a component whose final status and diagnostics are
     *        recorded at finish(). Non-owning.
     */
    void addComponent(HydroCouple::IModelComponent *component);

    /*!
     * \brief Closes the record: stamps the end time, drains component
     *        statuses and writer catalogs, and returns the manifest.
     *
     * Call after IOThread::finish(), whose finalize() calls are what make
     * writer catalogs valid.
     */
    [[nodiscard]] RunManifest finish(RunStatus status = RunStatus::Completed);

    /*!
     * \brief finish() written straight to a manifest file.
     * \param[in]  path    Manifest destination (parents created).
     * \param[out] message Failure description when returning false.
     */
    [[nodiscard]] bool finishToFile(const std::filesystem::path &path,
                                    std::string &message,
                                    RunStatus status = RunStatus::Completed);

private:
    RunManifest                                 m_manifest;
    std::vector<std::shared_ptr<IOutputWriter>> m_writers;
    std::vector<HydroCouple::IModelComponent *> m_components;
    std::chrono::steady_clock::time_point       m_start;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_RUNMANIFEST_H
