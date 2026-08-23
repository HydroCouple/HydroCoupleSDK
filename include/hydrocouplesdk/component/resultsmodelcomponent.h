// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   resultsmodelcomponent.h
 * \author Caleb Buahin
 * \version 2.1.0
 * \brief  ResultsModelComponent — a previously executed model, reopened
 *         from its run manifest.
 *
 * This is what makes analysis and visualization model-agnostic: a stored
 * run is presented as an ordinary IModelComponent whose recorded items
 * serve values straight out of the run's artifacts. The model's own
 * library is never loaded — the manifest's results catalog carries
 * everything needed (artifact, format, variable, kind, shape, units, mesh
 * attachment, time axis), so a UI can browse and plot results for a
 * component it has no code for.
 *
 * Lifecycle: initialize() opens the readers, builds one lazily-backed item
 * per catalog entry, and then walks Initializing → Initialized →
 * Finishing → Finished, because a recorded run has, by definition, already
 * computed its values. The Finished state releases *computational*
 * resources; the data items stay readable, which is the whole point.
 *
 * The recorded items appear in both results() — the standard's channel for
 * outputs that were never part of a coupling — and outputs(), so a stored
 * run can also drive a *new* composition as a boundary condition. The
 * workflows already skip Done/Finished components while still refreshing
 * their outputs, so this needs no special casing (pair with a
 * time-interpolating adapted output when step alignment differs).
 *
 * \threadsafety Not thread-safe; drive from one thread. Values are read
 * on demand, so concurrent reads of one item are not safe without
 * external synchronization.
 */

#ifndef HYDROCOUPLESDK_COMPONENT_RESULTSMODELCOMPONENT_H
#define HYDROCOUPLESDK_COMPONENT_RESULTSMODELCOMPONENT_H

#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/io/runmanifest.h"
#include "hydrocouplesdk/io/resultreader.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief A model component backed by a recorded run rather than a solver.
 */
class HYDROCOUPLESDK_EXPORT ResultsModelComponent final
    : public AbstractModelComponent
{
public:
    /*!
     * \brief Reopens one component's results from a manifest.
     *
     * \param[in] manifest    The run manifest (copied; carries the
     *                        directory relative artifacts resolve against).
     * \param[in] componentId Which component's entries to expose. When
     *                        empty, every entry in the catalog is exposed.
     * \param[in] id          Identifier for this component; defaults to
     *                        \p componentId so a reopened component keeps
     *                        the identity it had during the run.
     */
    ResultsModelComponent(IO::RunManifest manifest,
                          std::string componentId,
                          std::string_view id = {});

    ~ResultsModelComponent() override;

    /*!
     * \brief Convenience: reads the manifest from disk, then constructs.
     * \param[out] message Failure description when returning nullptr.
     */
    [[nodiscard]] static std::unique_ptr<ResultsModelComponent> open(
        const std::filesystem::path &manifestPath,
        const std::string &componentId, std::string &message);

    // ── IModelComponent ───────────────────────────────────────────────────

    /*!
     * \brief Opens the artifacts and exposes the recorded items, finishing
     *        immediately: a recorded run has already computed its values.
     *
     * Failure to open an artifact or find a reader for its format leaves
     * the component Failed with a Fatal diagnostic naming the file.
     */
    void initialize() override;

    /*!
     * \brief Nothing to validate: a recorded run is what it is.
     */
    std::vector<std::string> validate() override;

    /*!
     * \brief No-op — values are already computed.
     */
    void prepare() override;

    /*!
     * \brief No-op — the component is Finished from initialize() onward.
     */
    void update(const std::vector<HydroCouple::IOutput *> &requiredOutputs = {})
        override;

    /*!
     * \brief Closes the readers.
     */
    void finish() override;

    /*!
     * \brief The recorded items (also exposed through outputs()).
     */
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> results()
        const override;

    // ── Recorded-run accessors ────────────────────────────────────────────

    /*!
     * \brief The manifest this component was opened from.
     */
    [[nodiscard]] const IO::RunManifest &manifest() const noexcept;

    /*!
     * \brief The component id whose entries are exposed ("" = all).
     */
    [[nodiscard]] const std::string &sourceComponentId() const noexcept;

    /*!
     * \brief The catalog entry behind a recorded item, or nullptr.
     */
    [[nodiscard]] const IO::ResultEntry *entryFor(
        const std::string &itemId) const;

    /*!
     * \brief Time coordinate values of a recorded item (from the artifact
     *        when it stores them, otherwise from the recorded axis).
     */
    [[nodiscard]] bool times(const std::string &itemId,
                             std::vector<double> &values,
                             std::string &message) const;

protected:
    void createArguments() override;
    bool initializeArguments(std::string &message) override;
    void createInputs() override;
    void createOutputs() override;
    void initializeFailureCleanUp() override;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_COMPONENT_RESULTSMODELCOMPONENT_H
