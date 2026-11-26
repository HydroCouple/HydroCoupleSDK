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
 * Diagnostics: a failed initialize() queues two Fatal entries, one with the
 * codes below and one, code 0, from setStatus(Failed, ...), both carrying
 * the same message.
 *
 * | Code | Message begins | Cause |
 * | --- | --- | --- |
 * | 1 | run "ID" records no results for ... | The manifest's catalog has no entry for the component id (or none at all). |
 * | 2 | item "ITEM": ... | No reader exists for the entry's format in this build. |
 * | 3 | item "ITEM": ... | The reader could not open the artifact file. |
 * | 4 | no readable artifact for the recorded items (...) | Every entry's format is unsupported by this build. |
 * | 5 | item ITEM: ... | A unit, geometry, layering or time axis in the entry is invalid or missing. |
 *
 * Recorded items report TimeKind::Unknown, TimeInterpolation::None and
 * TimeExtrapolation::Refuse, refuse setValuesFrom(), and serve times in
 * Julian days (entries recorded in elapsed seconds are converted with the
 * entry's epoch).
 *
 * Ownership: the readers, dimensions, units and value definitions are
 * owned by the component; the recorded items are created with \c new,
 * registered as outputs and never deleted (the base registry does not own
 * them). finish() closes the readers, after which reading an item's values
 * uses a destroyed reader; read the values before calling finish(), or do
 * not call it (the workflows never finish a component that is already
 * Finished).
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
     * \details Opens nothing yet: initialize() reads the artifacts. The
     *          caption is "Recorded results".
     *
     * \param[in] manifest    The run manifest (copied; carries the
     *                        directory relative artifacts resolve against).
     * \param[in] componentId Which component's entries to expose. When
     *                        empty, every entry in the catalog is exposed.
     * \param[in] id          Identifier for this component; defaults to
     *                        \p componentId so a reopened component keeps
     *                        the identity it had during the run. When both
     *                        are empty a UUID is generated.
     */
    ResultsModelComponent(IO::RunManifest manifest,
                          std::string componentId,
                          std::string_view id = {});

    //! Destroys the readers and metadata; the recorded items are not deleted.
    ~ResultsModelComponent() override;

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    ResultsModelComponent(const ResultsModelComponent &)            = delete;
    //! Not copy-assignable (see the deleted copy constructor).
    ResultsModelComponent &operator=(const ResultsModelComponent &) = delete;

    /*!
     * \brief Convenience: reads the manifest from disk, then constructs.
     * \details The new component is Created; call initialize() to load it.
     * \param[in]  manifestPath  Path of the run manifest (JSON).
     * \param[in]  componentId   Component whose entries to expose; empty
     *                           exposes every entry. Also the new
     *                           component's id.
     * \param[out] message Failure description when returning nullptr;
     *                     cleared on success.
     * \return The new component, owned by the caller, or nullptr when the
     *         manifest cannot be read.
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
     * the component Failed with a Fatal diagnostic naming the item (see the
     * code table in the class description).
     *
     * The status is not checked first: call it once, on a Created instance.
     * On success the status walks Initializing, Initialized ("results
     * loaded from run ..."), Finishing and Finished ("recorded run ...
     * ready for reading"). When the same item was recorded in several
     * formats, the first readable one in the order hdf5_ugrid,
     * netcdf_ugrid, geopackage, csv is exposed. Items appear in catalog
     * order. Items created before a failure stay registered.
     */
    void initialize() override;

    /*!
     * \brief Nothing to validate: a recorded run is what it is.
     * \details Changes no status.
     * \return An empty vector.
     */
    std::vector<std::string> validate() override;

    /*!
     * \brief No-op — values are already computed.
     */
    void prepare() override;

    /*!
     * \brief No-op — the component is Finished from initialize() onward.
     * \param[in] requiredOutputs  Ignored.
     */
    void update(const std::vector<HydroCouple::IOutput *> &requiredOutputs = {})
        override;

    /*!
     * \brief Closes the readers.
     * \details Sets Finished when the status is anything else (from Failed
     *          this is a transition the table rejects, and it is reported).
     *          The recorded items stay registered but can no longer read
     *          values: their reader pointers dangle.
     */
    void finish() override;

    /*!
     * \brief The recorded items (also exposed through outputs()).
     * \return Non-owning pointers in catalog order; empty before initialize().
     */
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> results()
        const override;

    // ── Recorded-run accessors ────────────────────────────────────────────

    /*!
     * \brief The manifest this component was opened from.
     * \return Reference to the component's copy.
     */
    [[nodiscard]] const IO::RunManifest &manifest() const noexcept;

    /*!
     * \brief The component id whose entries are exposed ("" = all).
     * \return Reference to the id given at construction.
     */
    [[nodiscard]] const std::string &sourceComponentId() const noexcept;

    /*!
     * \brief The catalog entry behind a recorded item, or nullptr.
     * \param[in] itemId  Id of a recorded item.
     * \return The item's copy of its entry, valid while the component
     *         lives; nullptr when no recorded item has that id.
     */
    [[nodiscard]] const IO::ResultEntry *entryFor(
        const std::string &itemId) const;

    /*!
     * \brief Time coordinate values of a recorded item (from the artifact
     *        when it stores them, otherwise from the recorded axis).
     * \param[in]  itemId   Id of a recorded item.
     * \param[out] values   Replaced with the item's times, in Julian days;
     *                      empty for an item without a time axis.
     * \param[out] message  Cleared on success; "no recorded item \"ID\""
     *                      otherwise.
     * \return \c false when no recorded item has that id.
     */
    [[nodiscard]] bool times(const std::string &itemId,
                             std::vector<double> &values,
                             std::string &message) const;

protected:
    //! No arguments: does nothing.
    void createArguments() override;
    //! Not called (initialize() is overridden); clears \p message and
    //! returns \c true.
    //! \param[out] message  Cleared.
    //! \return \c true.
    bool initializeArguments(std::string &message) override;
    //! No inputs: does nothing.
    void createInputs() override;
    //! Does nothing; initialize() registers the recorded items itself.
    void createOutputs() override;
    //! Closes the readers. Not called by this class's initialize().
    void initializeFailureCleanUp() override;

private:
    struct Impl;
    //! Readers, metadata and items (see Impl).
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_COMPONENT_RESULTSMODELCOMPONENT_H
