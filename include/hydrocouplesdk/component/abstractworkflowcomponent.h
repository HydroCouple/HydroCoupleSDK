// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractworkflowcomponent.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Abstract base implementation of HydroCouple::IWorkflowComponent.
 *
 * The workflow component is the orchestration layer above the default
 * request-reply (pull) execution mode: where the pull chain is driven
 * implicitly by a trigger input at the most downstream point, a workflow
 * drives the composition's lifecycle explicitly — initialize, validate the
 * composition as a whole, prepare, step to synchronization points, and
 * finish — with cooperative pause/stop control and a diagnostics queue
 * mirroring the IModelComponent error contract.
 *
 * This base class owns everything strategy-independent: the status machine
 * and its signal, the role-labeled component registry, the workflow
 * back-pointer bookkeeping on managed components, the error queue, and the
 * cooperative control flags. Concrete strategies (PullDrivenWorkflow,
 * TimeSteppedWorkflow) implement performStep() and the validation /
 * preparation hooks.
 *
 * \threadsafety The lifecycle methods must be called from one driving
 * thread. requestStop() / requestPause() are safe to call from any thread
 * (atomics) and are honored at the next synchronization point. status()
 * and errors() may be read from any thread.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTWORKFLOWCOMPONENT_H
#define HYDROCOUPLESDK_ABSTRACTWORKFLOWCOMPONENT_H

#include "hydrocouplesdk/component/connectionvalidator.h"
#include "hydrocouplesdk/core/identity.h"
#include "hydrocouplesdk/core/signal.h"
#include "hydrocouple.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Abstract base class for HydroCouple::IWorkflowComponent.
 *
 * Lifecycle
 * ---------
 * 1. Created      → addModelComponent() wires the composition
 * 2. initialize() → Initializing → initializes managed components
 *                   → onInitialize() → Initialized or Failed
 * 3. validate()   → Validating → required roles + component validate()
 *                   + validateConnections() + validateComposition()
 *                   → Validated, or Invalid when problems were found
 * 4. prepare()    → Preparing → component prepare() + onPrepare()
 *                   (schedule building, exchange-pattern negotiation)
 *                   → Prepared or Failed
 * 5. update()     → Updating → performStep() → Updated / Paused / Done /
 *                   Failed (stop and pause honored between steps)
 * 6. finish()     → Finishing → component finish() + onFinish()
 *                   → Finished
 *
 * Subclasses implement performStep() and may override onInitialize(),
 * validateConnections(), validateComposition(), onPrepare(), onFinish(), and
 * isCompositionDone().
 *
 * A call made in the wrong status is handled per method: initialize()
 * queues a Warning and returns; validate() returns one message and changes
 * nothing; prepare() and update() fail the workflow. In particular,
 * update() after Done, or while Paused, fails it.
 *
 * Every status change is checked against the normative table
 * (Helpers::isValidWorkflowStatusTransition); a rejected transition is
 * performed and reported once per (from, to) pair with
 * Diagnostic::IllegalStatusTransition.
 *
 * Ownership: the workflow does not own its model components. It keeps raw
 * pointers in insertion order and its destructor calls setWorkflow(nullptr)
 * on each one that still points to it, so every managed component must
 * outlive the workflow or be removed with removeModelComponent() before it
 * is destroyed. The role labels are owned by the workflow.
 */
class HYDROCOUPLESDK_EXPORT AbstractWorkflowComponent
    : public Identity,
      public virtual HydroCouple::IWorkflowComponent
{
public:
    /*!
     * \brief Construct with an id and optional component-info pointer.
     * \details The status starts at Created, with no roles and no components.
     * \param[in] id   Workflow identifier; copied. Empty is replaced by a
     *                 generated UUID v4.
     * \param[in] info Metadata/factory object (borrowed; may be nullptr).
     */
    explicit AbstractWorkflowComponent(
        std::string_view id,
        HydroCouple::IWorkflowComponentInfo *info = nullptr);

    /*!
     * \brief Detaches the managed components and destroys the workflow.
     * \details Calls setWorkflow(nullptr) on every managed component whose
     *          workflow() is this one; the components are not finished or
     *          deleted. Every managed component must still exist.
     */
    ~AbstractWorkflowComponent() override;

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    AbstractWorkflowComponent(const AbstractWorkflowComponent &)            = delete;
    //! Not copy-assignable (see the deleted copy constructor).
    AbstractWorkflowComponent &operator=(const AbstractWorkflowComponent &) = delete;

    // ── IWorkflowComponent ────────────────────────────────────────────────

    /*!
     * \brief The metadata object given at construction.
     * \return The borrowed info, or nullptr.
     */
    [[nodiscard]] HydroCouple::IWorkflowComponentInfo *componentInfo() const override final;

    /*!
     * \brief The role labels registered with addModelComponentLabel().
     * \return Non-owning pointers, in registration order; valid while the
     *         workflow lives. Empty for the SDK's own strategies, which
     *         register no roles.
     */
    [[nodiscard]] std::vector<HydroCouple::IIdentity *> modelComponentLabels() const override;

    /*!
     * \brief Whether a role must be filled before validate() succeeds.
     * \param[in] modelComponentLabel  Role label, matched by id(); may be
     *                                 nullptr.
     * \return The role's required flag; \c false for nullptr or an unknown
     *         role.
     */
    [[nodiscard]] bool isRequiredModelComponent(
        const HydroCouple::IIdentity *modelComponentLabel) const override;

    /*!
     * \brief Initializes every managed component still in the Created state,
     *        then calls onInitialize().
     * \details Legal from Created and Failed; in any other status the call
     *          queues a Warning ("initialize() ignored in state ...") and
     *          returns. Clears pending stop and pause requests, sets
     *          Initializing, then visits the components in insertion order:
     *          a component that is Created or Failed is initialized, a
     *          component in any other status is left as it is, and a
     *          component that is Failed afterwards fails the workflow
     *          ("component 'id' failed to initialize"). onInitialize()
     *          returning \c false fails it with its message. Otherwise the
     *          status becomes Initialized.
     */
    void initialize() override;

    /*!
     * \brief Validates the composition: required roles filled, every managed
     *        component's validate() clean, every link's two ends declaring
     *        compatible semantics (validateConnections()), then
     *        validateComposition().
     * \details Legal from Initialized, Validated and Invalid; otherwise the
     *          call returns one message ("validate() requires the
     *          Initialized, Validated or Invalid state (current: ...)") and
     *          changes nothing. Each component's validate() is called
     *          whatever its status, and its problems are prefixed with
     *          "component 'id': ".
     *          A link finding of Error severity is a problem: it is returned,
     *          queued on errors() with its Diagnostic code, and makes the
     *          workflow Invalid. A Warning (a declaration missing on one end)
     *          is queued on errors() and does not. Every other problem is
     *          queued at Error with code 0. The status ends Validated when
     *          nothing was found, otherwise Invalid ("composition validation
     *          found N problem(s)"), from which validate() may be called again
     *          after the composition is fixed.
     * \return Empty when valid; otherwise one description per problem: role
     *         problems, component problems, validateComposition() problems,
     *         then link errors.
     */
    [[nodiscard]] std::vector<std::string> validate() override;

    /*!
     * \brief Drives prepare() on every managed component, then onPrepare()
     *        (where strategies build execution schedules).
     * \details Legal from Validated only; otherwise the workflow fails
     *          ("prepare() requires the Validated state (current: ...)").
     *          Components are prepared in insertion order, whatever their
     *          status; one that is Failed afterwards fails the workflow
     *          ("component 'id' failed to prepare"). onPrepare() returning
     *          \c false fails it with its message. Otherwise Prepared.
     */
    void prepare() override;

    /*!
     * \brief One orchestration step: performStep(), then honors pending
     *        stop/pause requests and Done detection.
     * \details Legal from Prepared and Updated; otherwise the workflow fails
     *          ("update() requires the Prepared or Updated state (current:
     *          ...)"), so stop calling update() once the status is Done,
     *          and call resume() before stepping a Paused workflow. Sets
     *          Updating and calls performStep(); \c false fails the workflow
     *          with the step's message. After a successful step, in this
     *          order: a stop request ends the run as Done ("stopped on
     *          request"); isCompositionDone() ends it as Done; a pause
     *          request is consumed and the status becomes Paused ("paused
     *          on request"); otherwise Updated.
     */
    void update() override;

    /*!
     * \brief Drives finish() on every managed component, then onFinish().
     * \details Does nothing when already Finished. Otherwise sets Finishing
     *          (from any status; from Created this is a transition the
     *          table rejects and is reported), calls finish() on every
     *          component that is neither Finished nor Created, calls
     *          onFinish() and sets Finished. The workflow cannot be
     *          initialized again.
     */
    void finish() override;

    /*!
     * \brief Cooperative stop: takes effect after the in-flight step.
     * \details Sets a flag; safe from any thread. The next update() that
     *          completes its step ends the run as Done. The flag stays set
     *          until initialize() clears it.
     */
    void requestStop() override;

    /*!
     * \brief Cooperative pause: takes effect after the in-flight step.
     * \details Sets a flag; safe from any thread. The next update() that
     *          completes its step, and is neither stopped nor done, becomes
     *          Paused and clears the flag.
     */
    void requestPause() override;

    /*!
     * \brief Resumes a Paused workflow (Paused → Updated); no-op otherwise.
     * \details Clears a pending pause request and sets Updated ("resumed").
     */
    void resume() override;

    /*!
     * \brief The current workflow status.
     * \return The status; safe to read from any thread (atomic).
     */
    [[nodiscard]] WorkflowStatus status() const override final;

    /*!
     * \brief The managed components.
     * \return A copy of the non-owning pointers, in insertion order.
     */
    [[nodiscard]] std::vector<HydroCouple::IModelComponent *> modelComponents() const override;

    /*!
     * \brief Adds a component (optionally into a role slot) and sets its
     *        workflow back-pointer.
     * \details Allowed in any status; a component added after initialize()
     *          is not initialized by the workflow. The component is not
     *          owned and must outlive the workflow or be removed first.
     * \param[in]  component            Component to manage.
     * \param[in]  modelRoleIdentifier  Role to fill, matched by id(), or
     *                                  nullptr to add the component without
     *                                  a role.
     * \param[out] message              Optional; cleared on success,
     *                                  otherwise "component is null",
     *                                  "component 'id' is already managed",
     *                                  "unknown role 'r'" or "role 'r' is
     *                                  already filled by 'id'".
     * \return False with \p message set when the component is null, the
     *         role is unknown, the role is already filled, or the component
     *         is already managed.
     */
    [[nodiscard]] bool addModelComponent(
        HydroCouple::IModelComponent *component,
        const HydroCouple::IIdentity *modelRoleIdentifier = nullptr,
        std::string *message = nullptr) override;

    /*!
     * \brief Stops managing a component.
     * \details Empties any role the component filled and clears its
     *          workflow back-pointer when it points to this workflow. The
     *          component's status is not changed.
     * \param[in] component  Component to remove.
     * \return \c false when \p component is not managed.
     */
    [[nodiscard]] bool removeModelComponent(
        HydroCouple::IModelComponent *component) override;

    /*!
     * \brief Drains the workflow diagnostics queue (see IWorkflowComponent).
     * \details The queue holds the workflow's own entries (source = the
     *          workflow id), including the problems and link findings of
     *          validate() and a Fatal entry for every transition to Failed.
     *          It does not copy the managed components' queues: read those
     *          from each component. Thread-safe.
     * \param[in] clearAfterRead  When \c true, empties the queue after
     *                            copying it.
     * \return The entries in the order they were queued.
     */
    [[nodiscard]] std::vector<ErrorEntry> errors(bool clearAfterRead = false) override;

    // ── ISignal<const shared_ptr<IWorkflowComponentStatusChangeEventArgs>&> ─

    /*!
     * \brief Connects a slot to the workflow status signal.
     * \details Every setStatus() call delivers one event, synchronously, on
     *          the driving thread. Weak reference; null and duplicate slots
     *          are ignored.
     * \param[in] slot  Slot to connect.
     */
    void connect(const std::shared_ptr<ISlot<
        const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &>> &slot) override;

    /*!
     * \brief Disconnects a slot from the workflow status signal.
     * \param[in] slot  Slot to disconnect; null and unknown slots are ignored.
     */
    void disconnect(const std::shared_ptr<ISlot<
        const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &>> &slot) override;

    /*!
     * \brief Blocks or unblocks both the property-change and status signals.
     * \param[in] block  \c true to block; \c false to unblock.
     */
    void blockSignals(bool block) override;

    // ── Non-interface helpers ─────────────────────────────────────────────

    /*!
     * \brief The component filling a role slot, or nullptr.
     * \param[in] roleId  Id of the role label.
     * \return The component, or nullptr when the role is unknown or empty.
     */
    [[nodiscard]] HydroCouple::IModelComponent *componentForRole(
        const std::string &roleId) const;

    /*!
     * \brief Returns a human-readable label for a WorkflowStatus value.
     * \param[in] status  Status to name.
     * \return The enumerator's name, or "Unknown" for a value outside the
     *         enumeration.
     */
    static std::string statusToString(WorkflowStatus status);

protected:
    // ── Strategy hooks ────────────────────────────────────────────────────

    /*!
     * \brief Executes one orchestration step (the strategy).
     * \details Called by update() while the status is Updating.
     * \param[out] message  On failure, the reason.
     * \return False when the step failed; the base transitions to Failed
     *         using \p message.
     */
    virtual bool performStep(std::string &message) = 0;

    /*!
     * \brief Called at the end of initialize(); default no-op.
     * \param[out] message  On failure, the reason.
     * \return False with \p message set to fail initialization; the default
     *         returns \c true.
     */
    virtual bool onInitialize(std::string &message);

    /*!
     * \brief Checks every wired link of every managed component.
     * \details The default runs validateConnection() over each input's
     *          provider (each of an IMultiInput's providers) and returns every
     *          finding. Override to add checks, or to drop findings a strategy
     *          knows are handled elsewhere; return an empty vector to opt out.
     *          Inputs that belong to no managed component (a pull trigger
     *          outside the composition, for example) are not checked.
     * \return The findings, Errors and Warnings mixed.
     */
    [[nodiscard]] virtual std::vector<ConnectionFinding> validateConnections();

    /*!
     * \brief Strategy-specific composition checks appended by validate();
     *        default adds nothing.
     * \param[in,out] problems  Problems found so far; append one message per
     *                          new problem.
     */
    virtual void validateComposition(std::vector<std::string> &problems);

    /*!
     * \brief Called at the end of prepare() — schedule building lives here;
     *        default no-op.
     * \param[out] message  On failure, the reason.
     * \return False with \p message set to fail preparation; the default
     *         returns \c true.
     */
    virtual bool onPrepare(std::string &message);

    /*!
     * \brief Called at the end of finish(); default no-op.
     */
    virtual void onFinish();

    /*!
     * \brief Done detection after a successful step: default is true when
     *        every managed component reports Done or Finished.
     * \return \c true when the run is complete; also \c true when the
     *         workflow manages no component.
     */
    [[nodiscard]] virtual bool isCompositionDone() const;

    // ── Role registration (called by subclass constructors) ──────────────

    /*!
     * \brief Registers a role slot components can be added into.
     * \details Duplicate ids are not checked; lookups use the first match.
     * \param[in] label    Role identity (owned by the workflow).
     * \param[in] required When true, validate() fails while the slot is empty.
     */
    void addModelComponentLabel(std::unique_ptr<Identity> label, bool required);

    // ── Diagnostics ───────────────────────────────────────────────────────

    /*!
     * \brief Queues a diagnostic entry on the workflow error queue.
     * \details The entry's source is the workflow id. Thread-safe.
     * \param[in] severity  Severity of the entry.
     * \param[in] code      ErrorEntry::code (0, or a Diagnostic code).
     * \param[in] message   Description; copied.
     */
    void reportError(ErrorEntry::Severity severity, int32_t code,
                     const std::string &message);

    // ── Status management ─────────────────────────────────────────────────

    /*!
     * \brief Sets the status without a message.
     * \details Same as setStatus(status, "", 0).
     * \param[in] status  New status.
     */
    void setStatus(WorkflowStatus status);
    /*!
     * \brief Sets the status with a message.
     * \details Same as setStatus(status, message, 0).
     * \param[in] status   New status.
     * \param[in] message  Message for the event.
     */
    void setStatus(WorkflowStatus status, const std::string &message);
    /*!
     * \brief Sets the status, checks the transition, queues a Fatal entry on
     *        failure and notifies.
     * \details A transition the normative table rejects is performed and
     *          reported once per (from, to) pair at Error with
     *          Diagnostic::IllegalStatusTransition. When \p status is Failed,
     *          a Fatal entry, code 0, is queued with \p message (or "workflow
     *          failed" when it is empty). Then one event is emitted. Unlike
     *          AbstractModelComponent, no propertyChanged("Status") fires.
     * \param[in] status    New status.
     * \param[in] message   Message for the event and the Fatal entry.
     * \param[in] progress  Percentage for the event, nominally 0 to 100.
     */
    void setStatus(WorkflowStatus status, const std::string &message,
                   float progress);

    /*!
     * \brief True once requestStop() has been called, until initialize()
     *        clears it.
     * \details Declared but not defined in the SDK library: a subclass that
     *          calls it fails to link.
     * \return The stop flag.
     */
    [[nodiscard]] bool stopRequested() const noexcept;

    /*!
     * \brief True while a pause request is pending.
     * \details Declared but not defined in the SDK library: a subclass that
     *          calls it fails to link.
     * \return The pause flag; cleared when update() pauses, by resume() and
     *         by initialize().
     */
    [[nodiscard]] bool pauseRequested() const noexcept;

    // ── Protected ISignal emit (required pure-virtual) ────────────────────

    /*!
     * \brief Emits a status event to the connected slots.
     * \param[in] args  Event to deliver.
     */
    void emit(const std::shared_ptr<
        IWorkflowComponentStatusChangeEventArgs> &args) override;

private:
    //! One role a component can fill.
    struct RoleSlot
    {
        std::unique_ptr<Identity>      label;              //!< Owned role label.
        bool                           required  = false;  //!< validate() needs it filled.
        HydroCouple::IModelComponent  *component = nullptr; //!< Filling component, or nullptr.
    };

    HydroCouple::IWorkflowComponentInfo *m_componentInfo = nullptr; //!< Borrowed info.

    //! Atomic per the interface's threading convention (requestStop() and
    //! status() may be called from another thread while update() runs).
    std::atomic<WorkflowStatus> m_status{WorkflowStatus::Created};

    //! (from, to) pairs already reported as illegal (see setStatus()).
    std::set<std::pair<int, int>> m_reportedIllegalTransitions;

    std::vector<RoleSlot>                            m_roles;      //!< Roles, registration order.
    std::vector<HydroCouple::IModelComponent *>      m_components; //!< Insertion order.

    std::atomic<bool> m_stopRequested{false};  //!< Set by requestStop().
    std::atomic<bool> m_pauseRequested{false}; //!< Set by requestPause().

    std::vector<ErrorEntry> m_errorQueue; //!< Entries returned by errors().
    mutable std::mutex      m_errorMutex; //!< Guards the queue and reported pairs.

    //! The status signal behind connect(), disconnect() and emit().
    Signal<const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &>
        m_statusChanged;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTWORKFLOWCOMPONENT_H
