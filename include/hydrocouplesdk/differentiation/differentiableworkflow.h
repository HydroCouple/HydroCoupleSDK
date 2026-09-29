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
 *  - the cotangents it returns for its inputs travel back along each input's
 *    connection -- through every adapted output on the way, by that
 *    adapter's own vjp() -- to the provider's root output, where they are
 *    ADDED to whatever else that output fed;
 *  - argument cotangents are summed over steps into the gradients returned;
 *  - the state cotangent it returns is carried to the previous step, and
 *    after the first step it is the gradient with respect to the initial
 *    state.
 *
 * The interface differentiates the most recent update(), so every step but
 * the last is replayed first: every component is restored to its checkpoint
 * from before that step and the step is run again. When backward() returns,
 * the composition is restored to where the forward run left it.
 *
 * Refused, with a message naming the offender, rather than guessed:
 * feedback loops (iterative coupling is Phase G2b); a component that is not
 * differentiable; an adapted connection through an adapter that is not
 * (the stateful relaxation and interpolation adapters among them); an input
 * whose provider's output the provider does not differentiate; multi-inputs;
 * more than one recorded step without Capability::Checkpointing; and a
 * component that finished before the others, which would change what a
 * step meant.
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
 * Releasing (ICheckpointableModelComponent::releaseState). Every checkpoint
 * this workflow saves is released exactly once, as soon as nothing can
 * restore it: a rung the ladder drops, a bisection temporary when its half
 * is done, the final state backward() returns to once it is back, and every
 * remaining rung on clearRecording(), prepare() and finish(). The workflow's
 * destructor does NOT release (the components may already be gone): finish
 * the workflow, or call clearRecording(), before destroying it.
 *
 * Float64 items only in G2.1. Nothing here depends on LibTorch: this is the
 * Torch-free floor; the optional Torch layer (G2.3) sits on top of it.
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
    const HydroCouple::IIdentity *owner = nullptr;
    HydroCouple::IArgument *argument = nullptr;
    std::vector<int64_t> shape;
    std::vector<double> values;
};

/*!
 * \brief d(loss)/d(state before the first recorded step).
 */
struct StateGradient
{
    HydroCouple::IModelComponent *component = nullptr;
    HydroCouple::IComponentDataItem *state = nullptr;
    std::vector<int64_t> shape;
    std::vector<double> values;
};

/*!
 * \brief Everything one backward() pass produced.
 */
struct HYDROCOUPLESDK_EXPORT Gradients
{
    std::vector<ArgumentGradient> arguments;
    std::vector<StateGradient> initialStates;

    //! The gradient of one argument, or nullptr if nothing reached it.
    [[nodiscard]] const ArgumentGradient *
    find(const HydroCouple::IIdentity *owner,
         const HydroCouple::IArgument *argument) const;

    //! The gradient of one component's initial state item, or nullptr.
    [[nodiscard]] const StateGradient *
    findState(const HydroCouple::IModelComponent *component,
              const HydroCouple::IComponentDataItem *state) const;
};

/*!
 * \brief Where a caller deposits d(loss)/d(output) for one recorded step.
 */
class HYDROCOUPLESDK_EXPORT Seeds
{
public:
    /*!
     * \brief Adds \p cotangent (row-major, the output's whole shape) to the
     *        cotangent of \p output at this step. Repeated adds accumulate.
     * \returns false if the length does not match the output's shape.
     */
    bool add(const HydroCouple::IOutput *output,
             std::span<const double> cotangent);

private:
    friend class DifferentiableWorkflow;
    std::map<const HydroCouple::IOutput *, std::vector<double>> m_values;
};

/*!
 * \brief Called once per recorded step, last step first, with the step's
 *        1-based index; deposits that step's loss cotangents.
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
    explicit DifferentiableWorkflow(
        std::string_view id,
        HydroCouple::IWorkflowComponentInfo *info = nullptr);
    ~DifferentiableWorkflow() override = default;

    /*!
     * \brief Why this composition cannot be differentiated (empty when it
     *        can). Meaningful after prepare(); also checked by backward().
     */
    [[nodiscard]] std::vector<std::string> differentiabilityProblems() const;

    //! Steps recorded since prepare() (or clearRecording()).
    [[nodiscard]] int recordedSteps() const noexcept;

    //! Steps the most recent backward() had to replay.
    [[nodiscard]] int replays() const noexcept;

    //! The checkpointing policy (default EveryStep). A change applies to
    //! steps recorded after it; switching to Logarithmic thins at once.
    void setCheckpointPolicy(CheckpointPolicy policy);
    [[nodiscard]] CheckpointPolicy checkpointPolicy() const noexcept;

    //! Composition checkpoints held now (one per checkpointed step).
    [[nodiscard]] int storedCheckpoints() const noexcept;

    //! The most composition checkpoints held at once during the last
    //! backward(): the recorded ones, the final state it returns to, and the
    //! temporaries its bisection makes.
    [[nodiscard]] int peakCheckpoints() const noexcept;

    //! The steps k (state after k steps) whose checkpoint is held, ascending.
    [[nodiscard]] std::vector<int> checkpointedSteps() const;

    //! Forgets the recorded steps, releasing every checkpoint still held;
    //! the next step starts a new record.
    void clearRecording();

    //! Releases the record, then finishes as a TimeSteppedWorkflow does.
    void finish() override;

    //! Checkpoint tokens released so far (one per component per checkpoint),
    //! and how many of those releases a component refused.
    [[nodiscard]] int releasedTokens() const noexcept;
    [[nodiscard]] int refusedReleases() const noexcept;

    /*!
     * \brief Reverse pass over every recorded step.
     * \param seeds    deposits d(loss)/d(output) per step.
     * \param gradients receives argument and initial-state gradients.
     * \param message  receives the reason on failure.
     * \returns true on success; on success the composition is back where the
     *          forward run left it.
     */
    bool backward(const SeedFunction &seeds, Gradients &gradients,
                  std::string &message);

protected:
    bool onPrepare(std::string &message) override;
    bool performStep(std::string &message) override;

private:
    struct Record
    {
        //! Components the step actually updated.
        std::vector<bool> updated;
    };
    struct Pass;   //!< the working state of one backward(), in the .cpp

    [[nodiscard]] std::vector<HydroCouple::IModelComponent *> scheduleOrder() const;
    bool saveAll(std::vector<std::string> &tokens, std::string &message);
    bool restoreAll(const std::vector<std::string> &tokens, std::string &message);
    void releaseAll(const std::vector<std::string> &tokens);
    void thinLadder();
    void notePeak(int extra);
    bool replayStep(int step, std::string &message);
    bool advance(int from, int to, std::string &message);
    bool reverseRange(int lo, int hi, Pass &pass, std::string &message);
    bool sweepStep(int step, Pass &pass, std::string &message);

    std::vector<Record> m_records;
    //! Composition checkpoints: key k is the state after k recorded steps
    //! (before step k + 1); one token per component in schedule order, empty
    //! for a component without Capability::Checkpointing.
    std::map<int, std::vector<std::string>> m_checkpoints;
    CheckpointPolicy m_policy = CheckpointPolicy::EveryStep;
    int m_linearizedAt = -1;   //!< 1-based step whose update() is the latest
    int m_replays = 0;
    int m_peak = 0;
    int m_released = 0;
    int m_refusedReleases = 0;
};

} // namespace HydroCouple::SDK::Differentiation

#endif // HYDROCOUPLESDK_DIFFERENTIABLEWORKFLOW_H
