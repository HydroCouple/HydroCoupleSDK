// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
/*!
 * \file   differentiableworkflow.h
 * \brief  Reverse-mode differentiation of a time-stepped composition
 *         (Phase G2.1 of HydroCouple/plans/hydrocouple/DIFFERENTIABLE_INTERFACE_ALTERNATIVES_2026-09-28.md).
 *
 * A DifferentiableWorkflow is a TimeSteppedWorkflow that can run backwards.
 * Forward, it behaves exactly like its base, except that before every step
 * it checkpoints each component (ICheckpointableModelComponent). backward()
 * then visits the recorded steps last-first and, inside each step, the
 * schedule last-first:
 *
 *  - a component's vjp() receives the cotangents that have reached its
 *    outputs (from the caller's loss, and from every consumer downstream)
 *    and the cotangent of its state carried back from the following step;
 *  - the cotangents it returns for its inputs are ADDED to the output each
 *    input reads -- a provider's root output, or an adapted output on the
 *    way to it;
 *  - before a component's own vjp(), the adapted outputs hanging off its
 *    outputs are reversed, leaves first: each adapter's vjp() runs once per
 *    step, seeded with the cotangent its consumers added and the cotangent
 *    of its state carried back from the following step, and ADDS the
 *    cotangent it returns for its adaptee to that adaptee (G2.4);
 *  - argument cotangents (of components and adapters) are summed over steps
 *    into the gradients returned;
 *  - the state cotangent a component or a stateful adapter returns is
 *    carried to the previous step, and after the first step it is the
 *    gradient with respect to the initial state.
 *
 * The interface differentiates the most recent update() and refresh(), so
 * every step but the last is replayed first: every component, and every
 * adapter that carries state (IAdaptedOutput::states()), is restored to its
 * checkpoint from before that step and the step is run again. When
 * backward() returns, the composition is restored to where the forward run
 * left it.
 *
 * Refused, with a message naming the offender, rather than guessed:
 * feedback loops (iterative coupling is Phase G2b), including a component
 * that reads its own output, directly or through adapters (a lagged
 * self-coupling); a component that is not
 * differentiable; an adapted connection through an adapter that is not
 * (the interpolation adapter, whose record grows, among them); an input
 * whose provider's output the provider does not differentiate; multi-inputs;
 * more than one recorded step with a component lacking
 * Capability::Checkpointing, or with an adapter that carries state but is
 * not an ICheckpointableAdaptedOutput; a stateful adapter on a connection
 * from a provider outside the workflow (nothing replays its refreshes); a
 * seed on an output that a non-differentiable adapter stands between; and
 * a component that finished before the others, which would change what a
 * step meant. An adapter between a provider outside the workflow and a
 * managed consumer is differentiated at its latest refresh, as a constant
 * function of its arguments.
 *
 * Memory (G2.2). With CheckpointPolicy::EveryStep a run of N steps holds N
 * composition checkpoints and replays N - 1 steps. With
 * CheckpointPolicy::Logarithmic the forward run keeps a dyadic ladder of
 * checkpoints -- those whose distance d from the newest is a multiple of the
 * largest power of two not above d, so at most two per power of two -- and
 * backward() reverses each gap between ladder rungs by recursive bisection,
 * holding one temporary checkpoint per level. Held checkpoints then grow as
 * O(log N) and replays as O(N log N); the gradients are identical, because
 * every step is still differentiated at exactly its own linearization point.
 *
 * Releasing (ICheckpointableModelComponent::releaseState and its adapter
 * counterpart). Every checkpoint this workflow saves is released exactly
 * once, as soon as nothing can restore it: a rung the ladder drops, a
 * bisection temporary when its half is done, the final state backward()
 * returns to once it is back, and every remaining rung on clearRecording(),
 * prepare() and finish(). The one exception is the token of an adapter that
 * has since left the composition: the adapter may be gone, so its token is
 * dropped unreleased. The workflow's destructor does NOT release (the
 * components may already be gone): finish the workflow, or call
 * clearRecording(), before destroying it.
 *
 * Float64 items only in G2.1. Nothing here depends on LibTorch: this is the
 * Torch-free floor; the optional Torch layer (G2.3) sits on top of it.
 *
 * Forward mode. The workflow drives only vjp(): there is no SDK driver for
 * jvp() through a composition. A caller that needs a Jacobian-vector
 * product calls each component's IDifferentiableModelComponent::jvp() on
 * its most recent step directly.
 *
 * In formulas: a step maps \f$ (s_{n-1}, u_n, p) \mapsto (s_n, y_n) \f$ for
 * state \f$ s \f$, inputs \f$ u \f$, arguments \f$ p \f$ and outputs
 * \f$ y \f$. For a loss \f$ L = \sum_n \ell_n(y_n) \f$ the seed function
 * supplies \f$ \bar y_n = \partial \ell_n / \partial y_n \f$ at each step,
 * and backward() returns
 * \f[ \frac{dL}{dp} = \sum_{n=1}^{N} \bar p_n, \qquad
 *     \frac{dL}{ds_0} = \bar s_0, \f]
 * where each step's vjp() maps \f$ (\bar y_n, \bar s_n) \f$ to
 * \f$ (\bar u_n, \bar p_n, \bar s_{n-1}) \f$ and \f$ \bar u_n \f$ is added
 * to the cotangent of the output the input reads.
 */
#ifndef HYDROCOUPLESDK_DIFFERENTIABLEWORKFLOW_H
#define HYDROCOUPLESDK_DIFFERENTIABLEWORKFLOW_H

#include "hydrocouplesdk/component/timesteppedworkflow.h"
#include "hydrocouple.h"

#include <functional>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Differentiation {

/*!
 * \brief d(loss)/d(argument), summed over every recorded step.
 * \details \c owner is the model component or the adapted output the
 *          argument belongs to.
 */
struct ArgumentGradient
{
    const HydroCouple::IIdentity *owner = nullptr; //!< Component or adapted output the argument belongs to.
    HydroCouple::IArgument *argument = nullptr;    //!< The differentiated argument.
    std::vector<int64_t> shape;                    //!< The argument's shape() when the gradient was first added.
    std::vector<double> values;                    //!< Row-major gradient, elementCount(shape) values.
};

/*!
 * \brief d(loss)/d(state before the first recorded step).
 * \details The state belongs to a model component or, for a stateful
 *          adapter (G2.4), to an adapted output; exactly one of
 *          \c component and \c adapter is set, and \c owner is that one.
 *          An adapter's state before step 1 is what its initialize() made
 *          of its adaptee's prepared values, which the engine does not
 *          differentiate: a caller who wants that dependence chains it.
 */
struct StateGradient
{
    const HydroCouple::IIdentity *owner = nullptr;      //!< \c component or \c adapter, as an identity.
    HydroCouple::IModelComponent *component = nullptr;  //!< Owning component, or null for an adapter state.
    HydroCouple::IAdaptedOutput *adapter = nullptr;     //!< Owning adapted output, or null for a component state.
    HydroCouple::IComponentDataItem *state = nullptr;   //!< The state item (one of differentiableStates()).
    std::vector<int64_t> shape;                         //!< The state item's shape.
    std::vector<double> values;                         //!< Row-major gradient, elementCount(shape) values.
};

/*!
 * \brief Everything one backward() pass produced.
 *
 * An argument or state that no cotangent reached has no entry, which is
 * different from a zero gradient: check find() for nullptr.
 */
struct HYDROCOUPLESDK_EXPORT Gradients
{
    //! Argument gradients, in the order the reverse sweep first reached them.
    std::vector<ArgumentGradient> arguments;
    //! Initial-state gradients: component states in schedule order, then
    //! adapter states.
    std::vector<StateGradient> initialStates;

    /*!
     * \brief The gradient of one argument, or nullptr if nothing reached it.
     * \param[in] owner    The component or adapted output the argument
     *                     belongs to (pass the component pointer; it converts).
     * \param[in] argument The argument.
     * \return The entry (owned by this object), or nullptr.
     */
    [[nodiscard]] const ArgumentGradient *
    find(const HydroCouple::IIdentity *owner,
         const HydroCouple::IArgument *argument) const;

    /*!
     * \brief The gradient of one initial state item of a model component or
     *        an adapted output, or nullptr.
     * \param[in] owner The component or adapted output that carries the state.
     * \param[in] state The state item.
     * \return The entry (owned by this object), or nullptr.
     */
    [[nodiscard]] const StateGradient *
    findState(const HydroCouple::IIdentity *owner,
              const HydroCouple::IComponentDataItem *state) const;
};

/*!
 * \brief Where a caller deposits d(loss)/d(output) for one recorded step.
 *
 * The workflow creates one per step and hands it to the SeedFunction. A
 * seed may name a root output of a component in the workflow or an
 * adapted output hanging off one. A seed on an output its component does
 * not list in differentiableOutputs() is ignored; a seed that reaches a
 * non-differentiable adapted output fails backward().
 */
class HYDROCOUPLESDK_EXPORT Seeds
{
public:
    /*!
     * \brief Adds \p cotangent (row-major, the output's whole shape) to the
     *        cotangent of \p output at this step. Repeated adds accumulate.
     * \param[in] output    The output the loss depends on (not null).
     * \param[in] cotangent d(loss)/d(output) for this step, row-major,
     *                      elementCount(output->shape()) values; copied.
     * \returns false if \p output is null or the length does not match the
     *          output's shape.
     */
    bool add(const HydroCouple::IOutput *output,
             std::span<const double> cotangent);

private:
    friend class DifferentiableWorkflow;
    std::map<const HydroCouple::IOutput *, std::vector<double>> m_values; //!< Accumulated cotangent per output.
};

/*!
 * \brief Called once per recorded step, last step first, with the step's
 *        1-based index; deposits that step's loss cotangents.
 *
 * When it runs, the composition holds that step's state and outputs (the
 * step has been replayed if needed), so the function may read outputs to
 * form the cotangent of a misfit. An empty function seeds nothing.
 */
using SeedFunction = std::function<void(int step, Seeds &seeds)>;

/*!
 * \brief How many composition checkpoints a recorded run keeps.
 */
enum class CheckpointPolicy
{
    EveryStep,   //!< One per step: O(N) memory, N - 1 replays (fastest backward)
    Logarithmic  //!< A dyadic ladder: O(log N) memory, O(N log N) replays
};

/*!
 * \brief A TimeSteppedWorkflow that records its steps and differentiates
 *        them in reverse.
 */
class HYDROCOUPLESDK_EXPORT DifferentiableWorkflow : public TimeSteppedWorkflow
{
public:
    /*!
     * \brief Creates an empty workflow; add components as for a
     *        TimeSteppedWorkflow.
     * \param[in] id   Workflow identifier.
     * \param[in] info Optional workflow metadata (borrowed; may be null).
     */
    explicit DifferentiableWorkflow(
        std::string_view id,
        HydroCouple::IWorkflowComponentInfo *info = nullptr);
    //! Destroys the workflow without releasing checkpoints (see the file
    //! comment): call finish() or clearRecording() first.
    ~DifferentiableWorkflow() override = default;

    /*!
     * \brief Why this composition cannot be differentiated (empty when it
     *        can). Meaningful after prepare(); also checked by backward().
     *
     * Checks the schedule and the recorded steps, so a problem that needs
     * more than one step (a component without Capability::Checkpointing)
     * appears only once a second step is recorded.
     *
     * \return One message per problem, naming the component, input or
     *         adapted output at fault.
     */
    [[nodiscard]] std::vector<std::string> differentiabilityProblems() const;

    //! Steps recorded since prepare() (or clearRecording()).
    //! \return The number of successful update() calls since then.
    [[nodiscard]] int recordedSteps() const noexcept;

    //! Steps the most recent backward() had to replay.
    //! \return Replayed steps (N - 1 for N steps under EveryStep when the
    //!         last step is still the latest update(); reset by backward()).
    [[nodiscard]] int replays() const noexcept;

    //! The checkpointing policy (default EveryStep). A change applies to
    //! steps recorded after it; switching to Logarithmic thins at once.
    //! \param[in] policy The new policy.
    void setCheckpointPolicy(CheckpointPolicy policy);
    //! The checkpointing policy in force.
    //! \return EveryStep unless setCheckpointPolicy() changed it.
    [[nodiscard]] CheckpointPolicy checkpointPolicy() const noexcept;

    //! Composition checkpoints held now (one per checkpointed step).
    //! \return The number of held checkpoints.
    [[nodiscard]] int storedCheckpoints() const noexcept;

    //! The most composition checkpoints held at once during the last
    //! backward(): the recorded ones, the final state it returns to, and the
    //! temporaries its bisection makes.
    //! \return The peak (0 before the first backward()).
    [[nodiscard]] int peakCheckpoints() const noexcept;

    //! The steps k (state after k steps) whose checkpoint is held, ascending.
    //! \return 0 to recordedSteps() - 1 under EveryStep; the ladder rungs
    //!         under Logarithmic (0 and recordedSteps() - 1 always among them).
    [[nodiscard]] std::vector<int> checkpointedSteps() const;

    //! Forgets the recorded steps, releasing every checkpoint still held;
    //! the next step starts a new record. The components' current state is
    //! not changed.
    void clearRecording();

    /*!
     * \brief Returns every component (and every adapter that carries state)
     *        to the state before the first recorded
     *        step and forgets the record (releasing its checkpoints), so the
     *        same run can be repeated -- with new argument values, say, in a
     *        training loop -- without preparing again, which the workflow
     *        lifecycle does not allow once it has stepped.
     * \param[out] message The failure reason ("nothing to rewind: no step
     *                     has been recorded", or a restore failure);
     *                     cleared on success.
     * \returns false (with a message) when nothing is recorded or a
     *          component cannot restore.
     */
    bool rewind(std::string &message);

    //! Releases the record, then finishes as a TimeSteppedWorkflow does.
    void finish() override;

    //! Checkpoint tokens released so far (one per checkpointed component or
    //! adapter per checkpoint), and how many of those releases were refused.
    //! \return Releases that succeeded over the workflow's lifetime (never
    //!         reset).
    [[nodiscard]] int releasedTokens() const noexcept;
    //! Releases a component or adapter refused (releaseState() returned
    //! false) over the workflow's lifetime.
    //! \return The count (never reset).
    [[nodiscard]] int refusedReleases() const noexcept;

    /*!
     * \brief Reverse pass over every recorded step.
     *
     * Refuses up front, before any vjp() runs, when
     * differentiabilityProblems() is not empty (the problems joined with
     * "; "). Otherwise saves the current state, sweeps the steps last-first
     * (replaying from checkpoints as needed), and restores the saved state.
     * The record is kept, so backward() may be called again, and further
     * steps may be recorded after it.
     *
     * \param[in]  seeds     deposits d(loss)/d(output) per step (may be
     *                       empty: then nothing is seeded).
     * \param[out] gradients receives argument and initial-state gradients;
     *                       reset at the start, so it is empty on failure.
     * \param[out] message   receives the reason on failure; cleared on
     *                       success.
     * \returns true on success; on success the composition is back where the
     *          forward run left it.
     */
    bool backward(const SeedFunction &seeds, Gradients &gradients,
                  std::string &message);

protected:
    /*!
     * \brief Clears the record, then builds the schedule as
     *        TimeSteppedWorkflow does.
     * \param[out] message The base class's failure reason.
     * \return The base class's result.
     */
    bool onPrepare(std::string &message) override;
    /*!
     * \brief Records one step: checkpoints every component and checkpointable
     *        adapter, runs the step, keeps the checkpoint, and thins the
     *        ladder under Logarithmic.
     * \param[out] message "checkpoint of 'X' failed: ..." or the base step's
     *                     failure ("component 'X' failed during update").
     * \return False when the checkpoint or the step failed; the checkpoint
     *         of a failed step is released and nothing is recorded.
     */
    bool performStep(std::string &message) override;

private:
    /*!
     * \brief What one recorded step did.
     */
    struct Record
    {
        //! Components the step actually updated.
        std::vector<bool> updated;
    };
    //! A token one checkpointable adapter gave. The id guards against a
    //! different adapter that has since been made at the same address.
    struct AdapterToken
    {
        HydroCouple::IAdaptedOutput *adapter = nullptr; //!< The adapter that saved the token.
        std::string id;                                 //!< Its id when the token was saved.
        std::string token;                              //!< The opaque token.
    };
    //! One composition checkpoint: a token per component in schedule order
    //! (empty for one without Capability::Checkpointing) and a token per
    //! checkpointable adapter.
    struct Checkpoint
    {
        std::vector<std::string> components; //!< Token per component, schedule order.
        std::vector<AdapterToken> adapters;  //!< Token per checkpointable managed adapter.
    };
    struct Pass;   //!< the working state of one backward(), in the .cpp

    //! The schedule flattened, group by group.
    //! \return Every managed component once, in update order.
    [[nodiscard]] std::vector<HydroCouple::IModelComponent *> scheduleOrder() const;
    //! Every adapted output hanging, at any depth, off an output of a managed
    //! component -- each once, parents before children.
    //! \return The adapters, in discovery order.
    [[nodiscard]] std::vector<HydroCouple::IAdaptedOutput *> managedAdapters() const;
    /*!
     * \brief Saves every component and checkpointable adapter.
     * \param[out] checkpoint The tokens; reset on failure (tokens already
     *                        saved are released).
     * \param[out] message    "checkpoint of 'X' failed: ..." or "checkpoint
     *                        of adapted output 'A' failed: ...".
     * \return True when every save succeeded.
     */
    bool saveAll(Checkpoint &checkpoint, std::string &message);
    /*!
     * \brief Restores every component and stateful adapter to \p checkpoint.
     *
     * Refuses before restoring anything when a component lacks
     * Capability::Checkpointing, a checkpointed adapter has left the
     * composition, or a stateful adapter has no token in the checkpoint.
     * After the components it refreshes the first-tier adapters, then
     * restores the stateful ones.
     *
     * \param[in]  checkpoint The checkpoint to return to.
     * \param[out] message    The refusal or restore failure.
     * \return True when everything was restored.
     */
    bool restoreAll(const Checkpoint &checkpoint, std::string &message);
    /*!
     * \brief Releases every token of \p checkpoint whose owner is still
     *        present, counting releases and refusals.
     * \param[in] checkpoint The checkpoint to release (not modified).
     */
    void releaseAll(const Checkpoint &checkpoint);
    //! Under Logarithmic, drops (and releases) every checkpoint k whose
    //! distance d from the newest is not a multiple of the largest power of
    //! two not above d.
    void thinLadder();
    //! Raises m_peak to the held checkpoints plus \p extra.
    //! \param[in] extra Checkpoints held outside m_checkpoints (the final
    //!                  state backward() returns to).
    void notePeak(int extra);
    /*!
     * \brief Re-runs recorded step \p step from the restored state.
     * \param[in]  step    1-based step.
     * \param[out] message "restoreState left 'X' finished, so replaying
     *                     step N would skip it" or the step's failure.
     * \return True when the step ran; it is then the linearization point.
     */
    bool replayStep(int step, std::string &message);
    /*!
     * \brief Brings the composition to the state after step \p to,
     *        linearized there, starting from the checkpoint after \p from.
     *
     * Does nothing when step \p to is already the latest update().
     *
     * \param[in]  from    Checkpoint key to restore (0 ≤ from < to).
     * \param[in]  to      1-based step to end linearized at.
     * \param[out] message "no checkpoint of the state after step K", a
     *                     restore failure or a replay failure.
     * \return True on success.
     */
    bool advance(int from, int to, std::string &message);
    /*!
     * \brief Sweeps steps hi, hi - 1, ..., lo + 1 given the checkpoint after
     *        step lo, bisecting with a temporary checkpoint at the midpoint.
     * \param[in]     lo      Checkpoint key the range starts from.
     * \param[in]     hi      Last step of the range.
     * \param[in,out] pass    The reverse pass's working state.
     * \param[out]    message Failure reason.
     * \return True when every step of the range was swept.
     */
    bool reverseRange(int lo, int hi, Pass &pass, std::string &message);
    /*!
     * \brief Differentiates one step at its linearization point: seeds,
     *        adapter trees leaves first, then each component's vjp() in
     *        reverse schedule order.
     * \param[in]     step    1-based step; must be the next step due and the
     *                        latest update().
     * \param[in,out] pass    The reverse pass's working state; the state
     *                        cotangents it carries move to step - 1.
     * \param[out]    message A vjp refusal, a cotangent on a
     *                        non-differentiable adapter, or an internal error.
     * \return True when the step was swept.
     */
    bool sweepStep(int step, Pass &pass, std::string &message);

    std::vector<Record> m_records; //!< One record per recorded step.
    //! Composition checkpoints: key k is the state after k recorded steps
    //! (before step k + 1).
    std::map<int, Checkpoint> m_checkpoints;
    CheckpointPolicy m_policy = CheckpointPolicy::EveryStep; //!< Policy in force.
    int m_linearizedAt = -1;   //!< 1-based step whose update() is the latest
    int m_replays = 0;         //!< Replays in the last backward().
    int m_peak = 0;            //!< Peak checkpoints in the last backward().
    int m_released = 0;        //!< Successful releases (lifetime).
    int m_refusedReleases = 0; //!< Refused releases (lifetime).
};

} // namespace HydroCouple::SDK::Differentiation

#endif // HYDROCOUPLESDK_DIFFERENTIABLEWORKFLOW_H
