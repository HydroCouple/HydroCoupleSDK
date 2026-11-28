// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractmodelcomponent.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Abstract base implementation of HydroCouple::IModelComponent.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTMODELCOMPONENT_H
#define HYDROCOUPLESDK_ABSTRACTMODELCOMPONENT_H

#include "hydrocouplesdk/core/identity.h"
#include "hydrocouplesdk/core/signal.h"
#include "hydrocouple.h"

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#ifdef USE_MPI
#include <mpi.h>
#else
//! Stand-in for MPI's communicator handle when the SDK is built without MPI
//! (USE_MPI undefined); lets the MPI bookkeeping members compile.
using MPI_Comm  = int;
//! Stand-in for MPI's group handle when the SDK is built without MPI.
using MPI_Group = int;
#endif

namespace HydroCouple::SDK {

class AbstractModelComponentInfo;
class AbstractOutput;
class AbstractInput;
class AbstractArgument;
class ComponentStatusChangeEventArgs;

/*!
 * \brief Abstract base class for HydroCouple::IModelComponent.
 *
 * Provides:
 * - Status-change signal (ISignal of `const shared_ptr<IComponentStatusChangeEventArgs>&`)
 * - Ordered argument / input / output registries
 * - The error queue (errors()), with a Fatal entry queued automatically
 *   when a message overload of setStatus() reports Failed
 * - A check of every status change against the normative transition table
 * - Reference directory management (std::filesystem::path)
 * - MPI resource tracking (process rank, allocated processes)
 * - GPU allocation table (mpiProcess → (gpuPlatform, gpuDevice, maxBlocks))
 * - Component ordering index (used by composition managers)
 *
 * Lifecycle
 * ---------
 * 1. Created → createArguments() is called once, lazily, by the first
 *    arguments() call or by initialize()
 * 2. initialize() → Initializing → calls initializeArguments()
 *    → createInputs(), createOutputs()
 *    → Initialized or Failed
 * 3. validate() → Validating → Valid or Invalid
 * 4. prepare()  → Preparing  → Updated
 * 5. update()   → Updating   → Updated / Done / Failed
 * 6. finish()   → Finishing  → Finished or Created
 *
 * Only step 2 is implemented here. validate(), prepare(), update() and
 * finish() are pure virtual: the subclass performs the transitions of
 * steps 3 to 6 itself with setStatus(), in the order the normative table
 * (Helpers::isValidComponentStatusTransition) allows.
 *
 * Subclasses must implement createArguments(), initializeArguments(),
 * createInputs(), createOutputs(), validate(), prepare(), update(),
 * finish(), and initializeFailureCleanUp().
 *
 * Ownership
 * ---------
 * The argument, input and output registries hold raw, non-owning
 * pointers: addInput() and the other mutators never take ownership, and
 * neither clearInputs() nor the destructor deletes anything. The subclass
 * owns its items (for example in \c std::unique_ptr members) and keeps them
 * alive while they are registered. The component info is borrowed too.
 *
 * Thread safety
 * -------------
 * status() is atomic and errors() is guarded by a mutex, so both may be
 * read from any thread. The status-change signal is thread-safe
 * (std::shared_mutex). The argument/input/output maps are NOT: mutate them
 * only from the owning thread during the initialize/prepare lifecycle
 * phases.
 */
class HYDROCOUPLESDK_EXPORT AbstractModelComponent
    : public Identity,
      public virtual HydroCouple::IModelComponent
{
    friend class AbstractModelComponentInfo;

public:
    //! Assign the composition instance identity before initialization.
    //! Factories name a type by default; a host may create several instances.
    //! \param[in]  id       New instance id; must not be empty.
    //! \param[out] message  Cleared on success; on failure, "a nonempty
    //!                      instance ID can only be assigned before
    //!                      initialization".
    //! \return \c true when the id was replaced (fires propertyChanged("Id"));
    //!         \c false when \p id is empty or the status is not Created.
    [[nodiscard]] bool setInstanceId(std::string_view id, std::string &message);
    /*!
     * \brief Construct with an id and optional component-info pointer.
     * \details The status starts at Created. No hook is called: arguments
     *          are created on the first arguments() or initialize() call.
     * \param[in] id    Instance identifier; copied. Empty is replaced by a
     *                  generated UUID v4.
     * \param[in] info  Metadata object returned by componentInfo();
     *                  borrowed, may be nullptr.
     */
    explicit AbstractModelComponent(std::string_view id,
                                     AbstractModelComponentInfo *info = nullptr);

    /*!
     * \brief Construct with id, caption, and optional component-info.
     * \param[in] id       Instance identifier; copied. Empty is replaced by
     *                     a generated UUID v4.
     * \param[in] caption  Human-readable caption; copied.
     * \param[in] info     Metadata object; borrowed, may be nullptr.
     */
    AbstractModelComponent(std::string_view id,
                            std::string_view caption,
                            AbstractModelComponentInfo *info = nullptr);

    //! Destroys the component. Registered items are not deleted (the
    //! registries do not own them), and finish() is not called.
    ~AbstractModelComponent() override = default;

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    AbstractModelComponent(const AbstractModelComponent &)            = delete;
    //! Not copy-assignable (see the deleted copy constructor).
    AbstractModelComponent &operator=(const AbstractModelComponent &) = delete;

    // ── IModelComponent ───────────────────────────────────────────────────

    /*!
     * \brief The metadata object this instance was created with.
     * \return The info passed to the constructor (borrowed), or nullptr.
     */
    [[nodiscard]] HydroCouple::IModelComponentInfo *componentInfo() const override final;

    /*!
     * \brief The current lifecycle status.
     * \details Safe to call from any thread at any time (atomic load).
     * \return The status last set with setStatus(); Created after
     *         construction.
     */
    [[nodiscard]] HydroCouple::IModelComponent::ComponentStatus status() const override final;

    /*!
     * \brief The component's arguments, in registration order.
     * \details Legal in every status, including Created: the first call
     *          runs createArguments() (once per instance, even though this
     *          method is const). The items are owned by the subclass.
     * \return Non-owning pointers to the registered arguments.
     */
    [[nodiscard]] std::vector<HydroCouple::IArgument *>       arguments() const override final;
    /*!
     * \brief The component's inputs, in registration order.
     * \details Empty until initialize() has run createInputs(). The vector
     *          is a snapshot; the items are owned by the subclass and are
     *          replaced when the component is initialized again.
     * \return Non-owning pointers to the registered inputs.
     */
    [[nodiscard]] std::vector<HydroCouple::IInput *>          inputs()    const override final;
    /*!
     * \brief The component's outputs, in registration order.
     * \details Empty until initialize() has run createOutputs(). Adapted
     *          outputs built on these are not listed (follow each output's
     *          adaptedOutputs()).
     * \return Non-owning pointers to the registered outputs.
     */
    [[nodiscard]] std::vector<HydroCouple::IOutput *>         outputs()   const override final;
    /*!
     * \brief Result items that are not exchange items.
     * \details The base returns an empty vector; ResultsModelComponent
     *          overrides it.
     * \return Non-owning pointers to the result items.
     */
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> results() const override;

    /*!
     * \brief Default states(): none. A component whose update() depends on its
     * past overrides this to list its prognostic items (what a checkpoint must
     * capture; a superset of any differentiableStates()).
     * \return An empty vector.
     */
    [[nodiscard]] std::vector<HydroCouple::IComponentDataItem *> states() const override;

    /*!
     * \brief Default initialize(): transitions Created/Failed/Initialized → Initialized.
     *
     * Legal from Created, Failed and Initialized (re-initialization); from
     * any other status the call returns at once, with no status change and
     * nothing queued. Otherwise, in order:
     *
     * 1. setStatus(Initializing, "Initializing component");
     * 2. createArguments(), if it has not run yet;
     * 3. initializeArguments(message). On \c false: setStatus(Failed,
     *    "Argument initialization failed: " + message), which queues a
     *    Fatal entry, then initializeFailureCleanUp(), and return;
     * 4. clearInputs() and clearOutputs() (the previous items are dropped
     *    from the registries, not deleted), createInputs(), createOutputs(),
     *    initializeAdaptedOutputs();
     * 5. setStatus(Initialized, "Component initialized successfully").
     *
     * createArguments() never runs a second time, so arguments and their
     * values survive re-initialization; inputs and outputs are rebuilt, so
     * connections made to the previous items are lost. An exception thrown
     * by a hook propagates out of initialize() and leaves the status at
     * Initializing.
     */
    void initialize() override;

    /*!
     * \brief Validates the component's current configuration.
     * \details Pure virtual: the base does nothing. An implementation sets
     * Validating, checks what its update() needs (required inputs connected,
     * argument combinations), sets Valid or Invalid, and returns the
     * problems. The interface requires every returned message to be queued
     * on errors() too (reportError(): Error when Invalid, Warning when
     * Valid). Legal from Initialized, Valid and Invalid.
     * \return Empty when valid; otherwise one message per problem.
     */
    std::vector<std::string> validate() override = 0;

    /*!
     * \brief Prepares the component for time stepping.
     * Pure virtual — subclasses implement domain-specific preparation: set
     * Preparing, allocate run state, publish initial output values, then
     * set Updated (there is no Prepared status). Legal from Valid only;
     * on failure set Failed with a message.
     */
    void prepare() override = 0;

    /*!
     * \brief Advances the component by one update step.
     * Pure virtual — subclasses implement time-stepping logic: set
     * Updating, read inputs (applyInputValues()), compute, write outputs,
     * then set Updated, or Done after the last step, or WaitingForData when
     * a provider is behind, or Failed with a message. Legal from Updated and
     * WaitingForData. The SDK workflows do not call update() on a component
     * that is Done or Finished.
     * \param[in] requiredOutputs  Outputs the caller needs refreshed; empty
     *                             means the implementation's choice (the
     *                             SDK workflows always pass an empty
     *                             vector).
     */
    void update(const std::vector<HydroCouple::IOutput *> &requiredOutputs = {})
        override = 0;

    /*!
     * \brief Finalises the component.
     * Pure virtual — subclasses write output files and free resources: set
     * Finishing, release run state, then set Finished (cannot restart) or
     * Created (can be initialized again). Legal from Initialized, Valid,
     * Invalid, Updated, Done and Failed.
     */
    void finish() override = 0;

    // ── Workflow ──────────────────────────────────────────────────────────

    /*!
     * \brief The workflow that manages this component.
     * \return The workflow set with setWorkflow() (borrowed), or nullptr.
     */
    [[nodiscard]] const HydroCouple::IWorkflowComponent *workflow() const override;
    /*!
     * \brief Records the managing workflow and fires propertyChanged("Workflow").
     * \details Called by AbstractWorkflowComponent::addModelComponent() and
     *          removeModelComponent(); the base does not act on it otherwise.
     * \param[in] workflow  The workflow (borrowed), or nullptr to detach.
     */
    void setWorkflow(const HydroCouple::IWorkflowComponent *workflow) override;

    // ── Capabilities and diagnostics (v2 contracts) ───────────────────────

    /*!
     * \brief The optional capabilities this component supports.
     * \details The base implementation returns an empty set; subclasses
     * advertise Capability values for the optional interfaces they implement.
     * Legal in every status.
     * \return The supported capabilities; empty here.
     */
    [[nodiscard]] std::set<Capability> capabilities() const override;

    /*!
     * \brief Drains this component's diagnostic queue.
     * \details The error queue is the normative failure channel of the v2
     * standard. setStatus(Failed, message) queues a Severity::Fatal entry
     * automatically (the overload without a message does not); subclasses
     * may queue additional entries via reportError(). An illegal status
     * transition queues an Error with Diagnostic::IllegalStatusTransition.
     * Legal in every status and from any thread.
     * \param[in] clearAfterRead  When \c true, the queue is emptied after
     *                            it is copied.
     * \return A copy of the queued entries in insertion order.
     */
    [[nodiscard]] std::vector<ErrorEntry> errors(bool clearAfterRead = false) override;

    /*!
     * \brief Queues a diagnostic on this component's error queue on behalf of
     *        one of its data items (the interface's error-channel convention:
     *        a bool-returning method that fails queues a Severity::Error entry).
     * \details AbstractOutput::addConsumer() uses it when a connection is
     *          refused. Thread-safe.
     * \param[in] severity  Severity of the entry.
     * \param[in] code      ErrorEntry::code; 0 or a component-defined value.
     * \param[in] message   Human-readable description; copied.
     */
    void queueError(ErrorEntry::Severity severity, int32_t code,
                    const std::string &message)
    {
        reportError(severity, code, message);
    }

    // ── MPI (SDK transport machinery — not part of the v2 interface) ──────
    //
    // Bookkeeping only: no SDK code reads or writes these values, and the
    // communicator is never set by the SDK.

    /*!
     * \brief Number of processes in the component's communicator.
     * \return 1 without MPI. With MPI, the size of the communicator a
     *         subclass stored in m_ComponentMPIComm, or 1 when it is
     *         MPI_COMM_NULL.
     */
    [[nodiscard]] int             mpiNumOfProcesses()  const;
    /*!
     * \brief The rank recorded with mpiSetProcessRank().
     * \return The recorded rank; 0 by default.
     */
    [[nodiscard]] int             mpiProcessRank()     const;
    /*!
     * \brief Records this component's process rank.
     * \param[in] rank  Rank to record; not checked.
     */
    void mpiSetProcessRank(int rank);
    /*!
     * \brief The ranks recorded with mpiAllocateProcesses().
     * \return A copy of the set; empty by default.
     */
    [[nodiscard]] std::set<int>   mpiAllocatedProcesses()    const;
    /*!
     * \brief Records the ranks allocated to this component.
     * \details Fires propertyChanged("MpiAllocatedProcesses").
     * \param[in] procs  Ranks; copied, also into an ordered array member.
     */
    void mpiAllocateProcesses(const std::set<int> &procs);
    /*!
     * \brief Clears the allocated ranks.
     * \details Fires propertyChanged("MpiAllocatedProcesses").
     */
    void mpiClearAllocatedProcesses();

    // ── GPU ───────────────────────────────────────────────────────────────
    //
    // A table recorded for subclasses; no SDK code reads it.

    /*!
     * \brief The GPU platform recorded for a process.
     * \param[in] processor  MPI process rank used as the table key.
     * \return The platform index, or -1 when nothing is recorded for \p processor.
     */
    [[nodiscard]] int gpuPlatform(int processor)            const;
    /*!
     * \brief The GPU device recorded for a process.
     * \param[in] processor  MPI process rank used as the table key.
     * \return The device index, or -1 when nothing is recorded for \p processor.
     */
    [[nodiscard]] int gpuDevice(int processor)              const;
    /*!
     * \brief The block or work-group limit recorded for a process.
     * \param[in] processor  MPI process rank used as the table key.
     * \return The limit, or 0 when nothing is recorded for \p processor.
     */
    [[nodiscard]] int gpuMaxNumBlocksOrWorkGrps(int processor) const;
    /*!
     * \brief Records (or replaces) the GPU allocation of a process.
     * \param[in] mpiProcess   MPI process rank (table key).
     * \param[in] gpuPlatform  Platform index.
     * \param[in] gpuDevice    Device index.
     * \param[in] maxBlocks    Maximum blocks or work groups.
     */
    void gpuAllocatedResources(int mpiProcess, int gpuPlatform,
                                int gpuDevice, int maxBlocks);
    /*!
     * \brief Clears the GPU allocation table.
     */
    void gpuClearAllocatedResources();

    // ── Reference directory ───────────────────────────────────────────────

    /*!
     * \brief The directory relative file paths resolve against.
     * \return The directory as a string; empty until set.
     */
    [[nodiscard]] std::string referenceDirectory() const override;
    /*!
     * \brief Sets the reference directory and fires propertyChanged("ReferenceDirectory").
     * \details A host typically sets it to the composition document's
     *          directory before initialize(). The path is stored as given,
     *          not checked or normalized.
     * \param[in] dir  Directory path.
     */
    void setReferenceDirectory(const std::string &dir) override;

    // ── ISignal<const shared_ptr<IComponentStatusChangeEventArgs>&> ───────

    /*!
     * \brief Connects a slot to the status-change signal.
     * \details Every setStatus() call, including one that repeats the
     *          current status, delivers one event to every connected slot,
     *          synchronously, on the thread that called setStatus(). The
     *          signal keeps a weak reference; null and duplicate slots are
     *          ignored. Property-change slots connect through the
     *          Description overload instead.
     * \param[in] slot  Slot to connect.
     */
    void connect(const std::shared_ptr<
        ISlot<const std::shared_ptr<IComponentStatusChangeEventArgs> &>> &slot) override;

    /*!
     * \brief Disconnects a slot from the status-change signal.
     * \param[in] slot  Slot to disconnect; null and unknown slots are ignored.
     */
    void disconnect(const std::shared_ptr<
        ISlot<const std::shared_ptr<IComponentStatusChangeEventArgs> &>> &slot) override;

    /*!
     * \brief Gates both the propertyChanged and componentStatusChanged signals.
     * \details Status changes, errors and transition checks still happen
     *          while blocked; only the notifications are dropped.
     * \param[in] block  \c true to block both signals; \c false to unblock.
     */
    void blockSignals(bool block) override;

    // ── Non-interface helpers ─────────────────────────────────────────────

    /*!
     * \brief The ordering index set with setIndex().
     * \return The index; 0 by default. No SDK code reads it.
     */
    [[nodiscard]] int  index() const;
    /*!
     * \brief Records an ordering index for a composition manager.
     * \param[in] index  Any value; not checked.
     */
    void setIndex(int index);

    /*!
     * \brief Whether initialize() has succeeded at least once.
     * \return \c true after the first successful initialize(); never reset,
     *         not even by finish().
     */
    [[nodiscard]] bool isInitialized() const;
    /*!
     * \brief Intended to say whether prepare() has run.
     * \return Always \c false: no code sets the flag behind it (the
     *         subclass's prepare() cannot reach it, as it is private).
     */
    [[nodiscard]] bool isPrepared()    const;

    /*!
     * \brief Applies values from all connected inputs (calls retrieveValuesFromProvider()
     *        + applyData() on each AbstractInput).
     * \details Inputs are visited in registration order. An input without a
     *          provider keeps its values; what retrieval does (and whether it
     *          refreshes the provider) is the input class's choice. Call it at the start of update():
     *          TimeSteppedWorkflow does not move input values for you.
     */
    virtual void applyInputValues();

    /*!
     * \brief Triggers updateValues on each required output (or all outputs if empty).
     * \details Calls the no-argument AbstractOutput::updateValues() on each
     *          AbstractOutput (others are skipped). Adapted outputs are not
     *          refreshed by this call.
     * \param[in] requiredOutputs  Outputs to update; empty means every
     *                             registered output.
     */
    virtual void updateOutputValues(
        const std::vector<HydroCouple::IOutput *> &requiredOutputs = {});

    /*!
     * \brief Calls initialize() on all adapted outputs registered on outputs.
     * \details Delegates to AbstractOutput::initializeAdaptedOutputs() for
     *          each registered AbstractOutput. initialize() calls it after
     *          createOutputs().
     */
    virtual void initializeAdaptedOutputs();

    /*!
     * \brief Converts a filesystem path relative to the reference directory to absolute.
     * \param[in] filePath  Path to resolve.
     * \return \p filePath unchanged when it is absolute; otherwise the
     *         reference directory joined with \p filePath, not normalized.
     *         With no reference directory the result is \p filePath, still
     *         relative.
     */
    [[nodiscard]] std::filesystem::path getAbsoluteFilePath(
        const std::filesystem::path &filePath) const;

    /*!
     * \brief Converts an absolute path to one relative to the reference directory.
     * \param[in] filePath  Path to express relative to the reference directory.
     * \return \p filePath unchanged when no reference directory is set;
     *         otherwise \c std::filesystem::relative(filePath, directory),
     *         which resolves symbolic links and is empty when no relative
     *         path exists.
     * \throws std::filesystem::filesystem_error when the file system cannot
     *         resolve the paths.
     */
    [[nodiscard]] std::filesystem::path getRelativeFilePath(
        const std::filesystem::path &filePath) const;

    /*!
     * \brief Returns a human-readable label for a ComponentStatus value.
     * \param[in] status  Status to name.
     * \return The enumerator's name ("Created", "WaitingForData", ...), or
     *         "Unknown" for a value outside the enumeration.
     */
    static std::string statusToString(HydroCouple::IModelComponent::ComponentStatus status);

protected:
    // ── Pure-virtual lifecycle hooks ──────────────────────────────────────

    /*!
     * \brief Runs createArguments() exactly once (lazy; shared by
     *        arguments() in the Created state and by initialize()).
     * \details The guard flag is set before the hook runs, so a hook that
     *          calls arguments() does not recurse.
     */
    void ensureArgumentsCreated();

    /*!
     * \brief Creates and registers the component's arguments.
     * \details Called once per instance, before initialize() reads them and
     *          possibly while the status is Created, so it must not depend
     *          on argument values. Register each argument with
     *          addArgument(); the subclass owns them.
     */
    virtual void createArguments()  = 0;
    /*!
     * \brief Reads the argument values and initializes the model.
     * \details Called by initialize() while the status is Initializing,
     *          before createInputs(). Open files, check values, allocate
     *          the model here.
     * \param[out] message  On failure, the reason; initialize() prefixes it
     *                      with "Argument initialization failed: ".
     * \return \c true to continue; \c false to fail initialization.
     */
    virtual bool initializeArguments(std::string &message) = 0;
    /*!
     * \brief Creates and registers the inputs.
     * \details Called by initialize() after initializeArguments() succeeds,
     *          after the input registry has been cleared. Register each input
     *          with addInput(); the subclass owns them and must release the
     *          ones from a previous initialization itself.
     */
    virtual void createInputs()     = 0;
    /*!
     * \brief Creates and registers the outputs.
     * \details Called by initialize() right after createInputs(), under the
     *          same rules (addOutput(); subclass ownership).
     */
    virtual void createOutputs()    = 0;

    /*!
     * \brief Called when initialize() fails; subclass releases partial state.
     * \details Called once, after the status is already Failed, only when
     *          initializeArguments() returned \c false. Release what that
     *          hook acquired (open files, model memory).
     */
    virtual void initializeFailureCleanUp() = 0;

    // ── Diagnostics ───────────────────────────────────────────────────────

    /*!
     * \brief Queues a diagnostic entry on this component's error queue.
     * \details The entry's source is id(). Thread-safe.
     * \param[in] severity  Severity of the entry.
     * \param[in] code      ErrorEntry::code; below 1000 for component codes
     *                      (see diagnosticcodes.h).
     * \param[in] message   Human-readable description; copied.
     */
    void reportError(ErrorEntry::Severity severity, int32_t code,
                     const std::string &message);

    // ── Status management ─────────────────────────────────────────────────

    /*!
     * \brief Sets the status and notifies, without a message.
     * \details Stores the status, checks the transition (see commitStatus()),
     *          emits a status event with an empty message, then fires
     *          propertyChanged("Status"). Unlike the message overloads, it
     *          queues no Fatal entry when \p status is Failed; use
     *          setStatus(Failed, message) for failures.
     * \param[in] status  New status.
     */
    void setStatus(HydroCouple::IModelComponent::ComponentStatus status);

    /*!
     * \brief Sets the status with a message and notifies.
     * \details As setStatus(ComponentStatus), and when \p status is Failed
     *          also queues a Severity::Fatal entry, code 0, carrying
     *          \p message. The event carries \p message and no progress.
     * \param[in] status   New status.
     * \param[in] message  Message for the event (and the Fatal entry).
     */
    virtual void setStatus(HydroCouple::IModelComponent::ComponentStatus status,
                            const std::string &message);

    /*!
     * \brief Sets the status with a message and a progress value, and notifies.
     * \details As setStatus(ComponentStatus, const std::string &); the event
     *          reports \p progress, and hasProgressMonitor() is true when
     *          \p progress is greater than 0. Call it with the current status
     *          to publish progress without a transition.
     * \param[in] status    New status.
     * \param[in] message   Message for the event (and the Fatal entry).
     * \param[in] progress  Percentage, nominally 0 to 100; not checked.
     */
    virtual void setStatus(HydroCouple::IModelComponent::ComponentStatus status,
                            const std::string &message, float progress);

    // ── Registry mutators (called by subclasses) ──────────────────────────

    /*!
     * \brief Registers an input.
     * \details Keyed by id: an input whose id is already registered is not
     *          added (the first one stays), though propertyChanged("Inputs")
     *          still fires. Not owned.
     * \param[in] input  Input to register; nullptr is ignored.
     */
    void addInput(AbstractInput *input);
    /*!
     * \brief Unregisters an input.
     * \details Fires propertyChanged("Inputs") on removal. The input is not
     *          deleted and its connections are not touched.
     * \param[in] input  Input to remove.
     * \return \c true when \p input itself was registered and is removed;
     *         \c false for nullptr, an unknown id, or another object with
     *         the same id.
     */
    bool removeInput(AbstractInput *input);
    /*!
     * \brief Unregisters every input without deleting them or notifying.
     */
    void clearInputs();
    /*!
     * \brief The registered inputs by id.
     * \return A copy of the id-to-input map.
     */
    [[nodiscard]] std::unordered_map<std::string, AbstractInput *> inputsInternal() const;

    /*!
     * \brief Registers an output.
     * \details Same rules as addInput(); fires propertyChanged("Outputs").
     * \param[in] output  Output to register; nullptr is ignored.
     */
    void addOutput(AbstractOutput *output);
    /*!
     * \brief Unregisters an output.
     * \details Same rules as removeInput(); fires propertyChanged("Outputs")
     *          on removal.
     * \param[in] output  Output to remove.
     * \return \c true when \p output itself was registered and is removed.
     */
    bool removeOutput(AbstractOutput *output);
    /*!
     * \brief Unregisters every output without deleting them or notifying.
     */
    void clearOutputs();
    /*!
     * \brief The registered outputs by id.
     * \return A copy of the id-to-output map.
     */
    [[nodiscard]] std::unordered_map<std::string, AbstractOutput *> outputsInternal() const;

    /*!
     * \brief Registers an argument.
     * \details Same rules as addInput(); fires propertyChanged("Arguments").
     * \param[in] argument  Argument to register; nullptr is ignored.
     */
    void addArgument(AbstractArgument *argument);
    /*!
     * \brief Unregisters an argument.
     * \details Same rules as removeInput(); fires
     *          propertyChanged("Arguments") on removal.
     * \param[in] argument  Argument to remove.
     * \return \c true when \p argument itself was registered and is removed.
     */
    bool removeArgument(AbstractArgument *argument);
    /*!
     * \brief Unregisters every argument without deleting them or notifying.
     */
    void clearArguments();
    /*!
     * \brief The registered arguments by id.
     * \return A copy of the id-to-argument map.
     */
    [[nodiscard]] std::unordered_map<std::string, AbstractArgument *> argumentsInternal() const;

    // ── MPI communicator ─────────────────────────────────────────────────

    /*!
     * \brief The communicator stored in m_ComponentMPIComm.
     * \return The stored handle; value-initialized unless a subclass set it.
     */
    [[nodiscard]] MPI_Comm mpiCommunicator() const;

    // ── Protected ISignal emit (required pure-virtual) ────────────────────

    /*!
     * \brief Emits a status event to the connected slots.
     * \details setStatus() calls it for you; call it directly only to
     *          deliver an event without changing the status.
     * \param[in] args  Event to deliver.
     */
    void emit(const std::shared_ptr<IComponentStatusChangeEventArgs> &args) override;

    // MPI state exposed for subclasses
    int                   m_mpiProcess  = 0;              //!< Rank recorded by mpiSetProcessRank().
    MPI_Comm              m_ComponentMPIComm{};           //!< Communicator; the SDK never sets it.
    std::set<int>         m_mpiAllocatedProcesses;        //!< Ranks recorded by mpiAllocateProcesses().
    std::vector<int>      m_mpiAllocatedProcessesArray;   //!< The same ranks, ascending, as an array.

private:
    //! Declared but not defined anywhere; never called.
    void initializeIdentifierArgument();

    //! Atomic per the interface's threading convention: status() may be read
    //! from any thread (a proxy updates it from the transport's thread).
    std::atomic<HydroCouple::IModelComponent::ComponentStatus> m_status{
        HydroCouple::IModelComponent::ComponentStatus::Created};

    //! Records the transition and checks it against the normative table.
    //! A transition the table rejects is still performed, and reported once
    //! per distinct (from, to) pair as an Error with
    //! Diagnostic::IllegalStatusTransition; a repeated status is not a
    //! transition and is never reported.
    //! \param[in] status  New status.
    //! \return The status the component held before.
    HydroCouple::IModelComponent::ComponentStatus
    commitStatus(HydroCouple::IModelComponent::ComponentStatus status);

    //! (from, to) pairs already reported as illegal, so a component that
    //! shortcuts every time step reports the defect once, not per step.
    std::set<std::pair<int, int>> m_reportedIllegalTransitions;

    // Ordered insertion lists for deterministic iteration
    std::vector<AbstractInput *>    m_orderedInputs;    //!< Inputs, registration order.
    std::vector<AbstractOutput *>   m_orderedOutputs;   //!< Outputs, registration order.
    std::vector<AbstractArgument *> m_orderedArguments; //!< Arguments, registration order.

    // Fast O(1) lookup by id
    std::unordered_map<std::string, AbstractInput *>    m_inputs;    //!< Inputs by id.
    std::unordered_map<std::string, AbstractOutput *>   m_outputs;   //!< Outputs by id.
    std::unordered_map<std::string, AbstractArgument *> m_arguments; //!< Arguments by id.

    AbstractModelComponentInfo              *m_componentInfo  = nullptr;    //!< Borrowed info.
    const HydroCouple::IWorkflowComponent  *m_workflowComponent = nullptr; //!< Borrowed workflow.

    bool m_initialized = false;      //!< Set by the first successful initialize().
    bool m_prepared    = false;      //!< Never set; see isPrepared().
    bool m_argumentsCreated = false; //!< Guard of ensureArgumentsCreated().
    int  m_index       = 0;          //!< Value of index().

    std::filesystem::path m_referenceDir; //!< Value of referenceDirectory().

    // GPU allocation: mpiProcess → (gpuPlatform, gpuDevice, maxBlocks)
    std::unordered_map<int, std::tuple<int, int, int>> m_gpuAllocation; //!< GPU table by rank.

    // MPI group handles (non-owning when USE_MPI is off)
    MPI_Group m_worldGroup{};               //!< Unused.
    MPI_Group m_componentMPIGroup{};        //!< Unused.
    bool      m_mpiResourcesInitialized = false; //!< Unused.

    std::vector<ErrorEntry> m_errorQueue;  //!< Entries returned by errors().
    mutable std::mutex      m_errorMutex;  //!< Guards the queue and the reported pairs.

    //! The status-change signal behind connect(), disconnect() and emit().
    Signal<const std::shared_ptr<IComponentStatusChangeEventArgs> &> m_statusChanged;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTMODELCOMPONENT_H
