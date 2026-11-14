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
 * (atomics) and are honored at the next synchronization point.
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
 *                   → Validated or Failed
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
 */
class HYDROCOUPLESDK_EXPORT AbstractWorkflowComponent
    : public Identity,
      public virtual HydroCouple::IWorkflowComponent
{
public:
    /*!
     * \brief Construct with an id and optional component-info pointer.
     * \param[in] id   Workflow identifier.
     * \param[in] info Metadata/factory object (borrowed; may be nullptr).
     */
    explicit AbstractWorkflowComponent(
        std::string_view id,
        HydroCouple::IWorkflowComponentInfo *info = nullptr);

    ~AbstractWorkflowComponent() override;

    //! Not copyable: owns non-copyable state (unique_ptr / mutex /
    //! atomic). Declared rather than left implicit because the class is
    //! exported, and __declspec(dllexport) instantiates every member --
    //! including the implicit copy assignment, which cannot compile
    //! against those members. Other toolchains only instantiate it on use.
    AbstractWorkflowComponent(const AbstractWorkflowComponent &)            = delete;
    AbstractWorkflowComponent &operator=(const AbstractWorkflowComponent &) = delete;

    // ── IWorkflowComponent ────────────────────────────────────────────────

    [[nodiscard]] HydroCouple::IWorkflowComponentInfo *componentInfo() const override final;

    [[nodiscard]] std::vector<HydroCouple::IIdentity *> modelComponentLabels() const override;

    [[nodiscard]] bool isRequiredModelComponent(
        const HydroCouple::IIdentity *modelComponentLabel) const override;

    /*!
     * \brief Initializes every managed component still in the Created state,
     *        then calls onInitialize().
     */
    void initialize() override;

    /*!
     * \brief Validates the composition: required roles filled, every managed
     *        component's validate() clean, every link's two ends declaring
     *        compatible semantics (validateConnections()), then
     *        validateComposition().
     * \details A link finding of Error severity is a problem: it is returned,
     *          queued on errors() with its Diagnostic code, and fails the
     *          workflow. A Warning (a declaration missing on one end) is
     *          queued on errors() and does not.
     * \return Empty when valid; otherwise one description per problem.
     */
    [[nodiscard]] std::vector<std::string> validate() override;

    /*!
     * \brief Drives prepare() on every managed component, then onPrepare()
     *        (where strategies build execution schedules).
     */
    void prepare() override;

    /*!
     * \brief One orchestration step: performStep(), then honors pending
     *        stop/pause requests and Done detection.
     */
    void update() override;

    /*!
     * \brief Drives finish() on every managed component, then onFinish().
     */
    void finish() override;

    /*!
     * \brief Cooperative stop: takes effect after the in-flight step.
     */
    void requestStop() override;

    /*!
     * \brief Cooperative pause: takes effect after the in-flight step.
     */
    void requestPause() override;

    /*!
     * \brief Resumes a Paused workflow (Paused → Updated); no-op otherwise.
     */
    void resume() override;

    [[nodiscard]] WorkflowStatus status() const override final;

    [[nodiscard]] std::vector<HydroCouple::IModelComponent *> modelComponents() const override;

    /*!
     * \brief Adds a component (optionally into a role slot) and sets its
     *        workflow back-pointer.
     * \return False with \p message set when the role is unknown, the role is
     *         already filled, or the component is already managed.
     */
    [[nodiscard]] bool addModelComponent(
        HydroCouple::IModelComponent *component,
        const HydroCouple::IIdentity *modelRoleIdentifier = nullptr,
        std::string *message = nullptr) override;

    [[nodiscard]] bool removeModelComponent(
        HydroCouple::IModelComponent *component) override;

    /*!
     * \brief Drains the workflow diagnostics queue (see IWorkflowComponent).
     */
    [[nodiscard]] std::vector<ErrorEntry> errors(bool clearAfterRead = false) override;

    // ── ISignal<const shared_ptr<IWorkflowComponentStatusChangeEventArgs>&> ─

    void connect(const std::shared_ptr<ISlot<
        const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &>> &slot) override;

    void disconnect(const std::shared_ptr<ISlot<
        const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &>> &slot) override;

    void blockSignals(bool block) override;

    // ── Non-interface helpers ─────────────────────────────────────────────

    /*!
     * \brief The component filling a role slot, or nullptr.
     */
    [[nodiscard]] HydroCouple::IModelComponent *componentForRole(
        const std::string &roleId) const;

    /*!
     * \brief Returns a human-readable label for a WorkflowStatus value.
     */
    static std::string statusToString(WorkflowStatus status);

protected:
    // ── Strategy hooks ────────────────────────────────────────────────────

    /*!
     * \brief Executes one orchestration step (the strategy).
     * \return False when the step failed; the base transitions to Failed
     *         using \p message.
     */
    virtual bool performStep(std::string &message) = 0;

    /*!
     * \brief Called at the end of initialize(); default no-op.
     * \return False with \p message set to fail initialization.
     */
    virtual bool onInitialize(std::string &message);

    /*!
     * \brief Checks every wired link of every managed component.
     * \details The default runs validateConnection() over each input's
     *          provider (each of an IMultiInput's providers) and returns every
     *          finding. Override to add checks, or to drop findings a strategy
     *          knows are handled elsewhere; return an empty vector to opt out.
     */
    [[nodiscard]] virtual std::vector<ConnectionFinding> validateConnections();

    /*!
     * \brief Strategy-specific composition checks appended by validate();
     *        default adds nothing.
     */
    virtual void validateComposition(std::vector<std::string> &problems);

    /*!
     * \brief Called at the end of prepare() — schedule building lives here;
     *        default no-op.
     * \return False with \p message set to fail preparation.
     */
    virtual bool onPrepare(std::string &message);

    /*!
     * \brief Called at the end of finish(); default no-op.
     */
    virtual void onFinish();

    /*!
     * \brief Done detection after a successful step: default is true when
     *        every managed component reports Done or Finished.
     */
    [[nodiscard]] virtual bool isCompositionDone() const;

    // ── Role registration (called by subclass constructors) ──────────────

    /*!
     * \brief Registers a role slot components can be added into.
     * \param[in] label    Role identity (owned by the workflow).
     * \param[in] required When true, validate() fails while the slot is empty.
     */
    void addModelComponentLabel(std::unique_ptr<Identity> label, bool required);

    // ── Diagnostics ───────────────────────────────────────────────────────

    /*!
     * \brief Queues a diagnostic entry on the workflow error queue.
     */
    void reportError(ErrorEntry::Severity severity, int32_t code,
                     const std::string &message);

    // ── Status management ─────────────────────────────────────────────────

    void setStatus(WorkflowStatus status);
    void setStatus(WorkflowStatus status, const std::string &message);
    void setStatus(WorkflowStatus status, const std::string &message,
                   float progress);

    /*!
     * \brief True once requestStop() has been called (sticky until finish()).
     */
    [[nodiscard]] bool stopRequested() const noexcept;

    /*!
     * \brief True while a pause request is pending.
     */
    [[nodiscard]] bool pauseRequested() const noexcept;

    // ── Protected ISignal emit (required pure-virtual) ────────────────────

    void emit(const std::shared_ptr<
        IWorkflowComponentStatusChangeEventArgs> &args) override;

private:
    struct RoleSlot
    {
        std::unique_ptr<Identity>      label;
        bool                           required  = false;
        HydroCouple::IModelComponent  *component = nullptr;
    };

    HydroCouple::IWorkflowComponentInfo *m_componentInfo = nullptr;

    //! Atomic per the interface's threading convention (requestStop() and
    //! status() may be called from another thread while update() runs).
    std::atomic<WorkflowStatus> m_status{WorkflowStatus::Created};

    //! (from, to) pairs already reported as illegal (see setStatus()).
    std::set<std::pair<int, int>> m_reportedIllegalTransitions;

    std::vector<RoleSlot>                            m_roles;
    std::vector<HydroCouple::IModelComponent *>      m_components; //!< Insertion order.

    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_pauseRequested{false};

    std::vector<ErrorEntry> m_errorQueue;
    mutable std::mutex      m_errorMutex;

    Signal<const std::shared_ptr<IWorkflowComponentStatusChangeEventArgs> &>
        m_statusChanged;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTWORKFLOWCOMPONENT_H
