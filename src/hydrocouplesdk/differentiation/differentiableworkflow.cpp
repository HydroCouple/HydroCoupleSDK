// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/differentiation/differentiableworkflow.h"
#include "hydrocouplesdk/differentiation/differentials.h"

#include <algorithm>
#include <deque>
#include <set>
#include <sstream>
#include <utility>

namespace HydroCouple::SDK::Differentiation {

using ComponentStatus = HydroCouple::IModelComponent::ComponentStatus;
using HydroCouple::DifferentialEntry;
using HydroCouple::DifferentialRole;
using HydroCouple::DifferentialSet;

namespace {

bool finishedStatus(ComponentStatus status)
{
    return status == ComponentStatus::Done ||
           status == ComponentStatus::Finished;
}

HydroCouple::ICheckpointableModelComponent *
checkpointable(HydroCouple::IModelComponent *component)
{
    if (!component->capabilities().count(HydroCouple::Capability::Checkpointing))
        return nullptr;
    return dynamic_cast<HydroCouple::ICheckpointableModelComponent *>(component);
}

HydroCouple::IDifferentiableModelComponent *
differentiable(HydroCouple::IModelComponent *component)
{
    if (!component->capabilities().count(HydroCouple::Capability::Differentiable))
        return nullptr;
    return dynamic_cast<HydroCouple::IDifferentiableModelComponent *>(component);
}

std::vector<double> &accumulate(std::vector<double> &into,
                                const std::vector<double> &value)
{
    if (into.empty())
        into.assign(value.size(), 0.0);
    for (size_t i = 0; i < into.size() && i < value.size(); ++i)
        into[i] += value[i];
    return into;
}

/*!
 * Owns the storage behind a batch of DifferentialEntry buffers: every
 * vector lives in a deque, so pointers stay valid while entries are added.
 */
class EntryBatch
{
public:
    //! An entry over a copy of \p values (a seed).
    void seed(const HydroCouple::IComponentDataItem *item, DifferentialRole role,
              const std::vector<int64_t> &shape, const std::vector<double> &values)
    {
        m_shapes.push_back(shape);
        m_values.push_back(values);
        push(item, role);
    }

    //! A zeroed entry the callee will overwrite (a result); returns its index.
    size_t result(const HydroCouple::IComponentDataItem *item,
                  DifferentialRole role, const std::vector<int64_t> &shape)
    {
        m_shapes.push_back(shape);
        m_values.emplace_back(static_cast<size_t>(elementCount(shape)), 0.0);
        push(item, role);
        return m_entries.size() - 1;
    }

    [[nodiscard]] DifferentialSet set() const
    {
        return DifferentialSet(m_entries.data(), m_entries.size());
    }
    [[nodiscard]] bool empty() const { return m_entries.empty(); }
    [[nodiscard]] const std::vector<double> &values(size_t index) const
    {
        return m_values[index];
    }

private:
    void push(const HydroCouple::IComponentDataItem *item, DifferentialRole role)
    {
        DifferentialEntry entry;
        entry.item = item;
        entry.role = role;
        entry.value = hostBuffer(m_values.back(), m_shapes.back());
        m_entries.push_back(entry);
    }

    std::deque<std::vector<int64_t>> m_shapes;
    std::deque<std::vector<double>> m_values;
    std::vector<DifferentialEntry> m_entries;
};

//! Argument gradients keyed by (owner, argument), in first-reached order.
class ArgumentSums
{
public:
    void add(const HydroCouple::IIdentity *owner, HydroCouple::IArgument *argument,
             const std::vector<int64_t> &shape, const std::vector<double> &values)
    {
        const auto key = std::make_pair(owner, argument);
        auto found = m_index.find(key);
        if (found == m_index.end())
        {
            m_index[key] = m_sums.size();
            m_sums.push_back(ArgumentGradient{owner, argument, shape, values});
            return;
        }
        accumulate(m_sums[found->second].values, values);
    }

    std::vector<ArgumentGradient> take() { return std::move(m_sums); }

private:
    std::map<std::pair<const HydroCouple::IIdentity *, HydroCouple::IArgument *>,
             size_t> m_index;
    std::vector<ArgumentGradient> m_sums;
};

} // namespace

// ── Gradients ──────────────────────────────────────────────────────────────

const ArgumentGradient *
Gradients::find(const HydroCouple::IIdentity *owner,
                const HydroCouple::IArgument *argument) const
{
    for (const ArgumentGradient &g : arguments)
        if (g.owner == owner && g.argument == argument)
            return &g;
    return nullptr;
}

const StateGradient *
Gradients::findState(const HydroCouple::IModelComponent *component,
                     const HydroCouple::IComponentDataItem *state) const
{
    for (const StateGradient &g : initialStates)
        if (g.component == component && g.state == state)
            return &g;
    return nullptr;
}

// ── Seeds ──────────────────────────────────────────────────────────────────

bool Seeds::add(const HydroCouple::IOutput *output,
                std::span<const double> cotangent)
{
    if (!output)
        return false;
    const std::vector<int64_t> shape = output->shape();
    if (elementCount(shape) != static_cast<int64_t>(cotangent.size()))
        return false;
    accumulate(m_values[output],
               std::vector<double>(cotangent.begin(), cotangent.end()));
    return true;
}

// ── DifferentiableWorkflow ─────────────────────────────────────────────────

DifferentiableWorkflow::DifferentiableWorkflow(
    std::string_view id, HydroCouple::IWorkflowComponentInfo *info)
    : TimeSteppedWorkflow(id, info)
{
}

int DifferentiableWorkflow::recordedSteps() const noexcept
{
    return static_cast<int>(m_records.size());
}

int DifferentiableWorkflow::replays() const noexcept
{
    return m_replays;
}

void DifferentiableWorkflow::clearRecording()
{
    m_records.clear();
    m_checkpoints.clear();
    m_linearizedAt = -1;
}

void DifferentiableWorkflow::setCheckpointPolicy(CheckpointPolicy policy)
{
    m_policy = policy;
    thinLadder();
}

CheckpointPolicy DifferentiableWorkflow::checkpointPolicy() const noexcept
{
    return m_policy;
}

int DifferentiableWorkflow::storedCheckpoints() const noexcept
{
    return static_cast<int>(m_checkpoints.size());
}

int DifferentiableWorkflow::peakCheckpoints() const noexcept
{
    return m_peak;
}

std::vector<int> DifferentiableWorkflow::checkpointedSteps() const
{
    std::vector<int> steps;
    for (const auto &entry : m_checkpoints)
        steps.push_back(entry.first);
    return steps;
}

void DifferentiableWorkflow::thinLadder()
{
    if (m_policy != CheckpointPolicy::Logarithmic || m_checkpoints.empty())
        return;
    // Keep k when k is a multiple of the largest power of two not above its
    // distance d from the newest checkpoint (1 when d is 0). At most two
    // rungs per power of two survive, and a rung dropped now would never be
    // kept later: d only grows, and so does the alignment it needs. The
    // first rung (k = 0, a multiple of everything) and the newest (d = 0,
    // alignment 1) always survive -- which backward() relies on.
    const int newest = m_checkpoints.rbegin()->first;
    for (auto it = m_checkpoints.begin(); it != m_checkpoints.end();)
    {
        const int k = it->first;
        const int d = newest - k;
        int power = 1;
        while (power * 2 <= d)
            power *= 2;
        it = (k % power) == 0 ? std::next(it) : m_checkpoints.erase(it);
    }
}

void DifferentiableWorkflow::notePeak(int extra)
{
    m_peak = std::max(m_peak, static_cast<int>(m_checkpoints.size()) + extra);
}

std::vector<HydroCouple::IModelComponent *>
DifferentiableWorkflow::scheduleOrder() const
{
    std::vector<HydroCouple::IModelComponent *> order;
    for (const auto &group : schedule())
        for (auto *component : group)
            order.push_back(component);
    return order;
}

bool DifferentiableWorkflow::onPrepare(std::string &message)
{
    clearRecording();
    return TimeSteppedWorkflow::onPrepare(message);
}

bool DifferentiableWorkflow::saveAll(std::vector<std::string> &tokens,
                                     std::string &message)
{
    const auto order = scheduleOrder();
    tokens.assign(order.size(), std::string());
    for (size_t i = 0; i < order.size(); ++i)
    {
        auto *cp = checkpointable(order[i]);
        if (!cp)
            continue;
        std::string why;
        if (!cp->saveState(tokens[i], why))
        {
            message = "checkpoint of '" + order[i]->id() + "' failed: " + why;
            return false;
        }
    }
    return true;
}

bool DifferentiableWorkflow::restoreAll(const std::vector<std::string> &tokens,
                                        std::string &message)
{
    const auto order = scheduleOrder();
    for (size_t i = 0; i < order.size(); ++i)
    {
        auto *cp = checkpointable(order[i]);
        if (!cp)
        {
            message = "component '" + order[i]->id() +
                      "' lacks Capability::Checkpointing, so an earlier step "
                      "cannot be replayed";
            return false;
        }
        std::string why;
        if (!cp->restoreState(tokens[i], why))
        {
            message = "restore of '" + order[i]->id() + "' failed: " + why;
            return false;
        }
    }
    return true;
}

bool DifferentiableWorkflow::performStep(std::string &message)
{
    Record record;
    const auto order = scheduleOrder();
    record.updated.resize(order.size());
    for (size_t i = 0; i < order.size(); ++i)
        record.updated[i] = !finishedStatus(order[i]->status());
    std::vector<std::string> tokens;
    if (!saveAll(tokens, message))
        return false;
    if (!TimeSteppedWorkflow::performStep(message))
        return false;
    m_checkpoints[static_cast<int>(m_records.size())] = std::move(tokens);
    m_records.push_back(std::move(record));
    m_linearizedAt = static_cast<int>(m_records.size());
    thinLadder();
    return true;
}

bool DifferentiableWorkflow::advance(int from, int to, std::string &message)
{
    if (m_linearizedAt == to)
        return true;   // already the latest update(): nothing to recompute
    auto found = m_checkpoints.find(from);
    if (found == m_checkpoints.end())
    {
        message = "no checkpoint of the state after step " + std::to_string(from);
        return false;
    }
    if (!restoreAll(found->second, message))
        return false;
    for (int step = from + 1; step <= to; ++step)
        if (!replayStep(step, message))
            return false;
    return true;
}

bool DifferentiableWorkflow::replayStep(int step, std::string &message)
{
    const Record &record = m_records[static_cast<size_t>(step - 1)];
    const auto order = scheduleOrder();
    for (size_t i = 0; i < order.size(); ++i)
    {
        if (record.updated[i] && finishedStatus(order[i]->status()))
        {
            message = "restoreState left '" + order[i]->id() +
                      "' finished, so replaying step " + std::to_string(step) +
                      " would skip it";
            return false;
        }
    }
    if (!TimeSteppedWorkflow::performStep(message))
        return false;
    ++m_replays;
    m_linearizedAt = step;
    return true;
}

std::vector<std::string> DifferentiableWorkflow::differentiabilityProblems() const
{
    std::vector<std::string> problems;
    const auto order = scheduleOrder();
    const std::set<HydroCouple::IModelComponent *> managed(order.begin(), order.end());

    for (const auto &group : schedule())
    {
        if (group.size() < 2)
            continue;
        std::string names;
        for (auto *component : group)
            names += (names.empty() ? "'" : ", '") + component->id() + "'";
        problems.push_back("feedback loop between " + names +
                           ": iterative coupling is not differentiable yet");
    }

    for (auto *component : order)
    {
        auto *d = differentiable(component);
        if (!d)
        {
            problems.push_back("component '" + component->id() +
                               "' is not differentiable");
            continue;
        }
        if (m_records.size() > 1 && !checkpointable(component))
            problems.push_back("component '" + component->id() +
                               "' lacks Capability::Checkpointing, so only a "
                               "single recorded step can be differentiated");

        for (HydroCouple::IInput *input : d->differentiableInputs())
        {
            if (dynamic_cast<HydroCouple::IMultiInput *>(input))
            {
                problems.push_back("input '" + input->id() + "' of '" +
                                   component->id() +
                                   "' is a multi-input, not yet supported");
                continue;
            }
            HydroCouple::IOutput *provider = input->provider();
            bool broken = false;
            while (auto *adapted = dynamic_cast<HydroCouple::IAdaptedOutput *>(provider))
            {
                if (!dynamic_cast<HydroCouple::IDifferentiableAdaptedOutput *>(adapted))
                {
                    problems.push_back("adapted output '" + adapted->id() +
                                       "' on the connection into '" +
                                       component->id() + "." + input->id() +
                                       "' is not differentiable");
                    broken = true;
                    break;
                }
                provider = adapted->adaptee();
            }
            if (broken || !provider)
                continue;
            auto *owner = provider->modelComponent();
            if (!owner || !managed.count(owner))
                continue;   // an unmanaged provider is a constant
            auto *pd = differentiable(owner);
            if (!pd)
                continue;   // already reported above
            const auto outs = pd->differentiableOutputs();
            if (std::find(outs.begin(), outs.end(), provider) == outs.end())
                problems.push_back("'" + component->id() + "." + input->id() +
                                   "' consumes '" + owner->id() + "." +
                                   provider->id() + "', which '" + owner->id() +
                                   "' does not differentiate");
        }
    }

    for (size_t step = 0; step < m_records.size(); ++step)
        for (size_t i = 0; i < order.size(); ++i)
            if (!m_records[step].updated[i])
                problems.push_back("component '" + order[i]->id() +
                                   "' had finished by step " +
                                   std::to_string(step + 1) +
                                   ", so that step is not the same function "
                                   "for every component");
    return problems;
}

// ── backward() ─────────────────────────────────────────────────────────────

//! The working state of one reverse pass.
struct DifferentiableWorkflow::Pass
{
    const SeedFunction *seedFunction = nullptr;
    std::vector<HydroCouple::IModelComponent *> order;
    ArgumentSums argumentSums;
    //! State cotangents carried from the step after the one being swept.
    std::map<HydroCouple::IModelComponent *,
             std::map<HydroCouple::IComponentDataItem *, std::vector<double>>>
        stateCarry;
    //! The next step that must be swept: steps are visited strictly
    //! last-first, and sweepStep() checks it.
    int nextStep = 0;
};

bool DifferentiableWorkflow::sweepStep(int step, Pass &pass, std::string &message)
{
    // Precondition: the composition is linearized at `step`.
    if (step != pass.nextStep || m_linearizedAt != step)
    {
        message = "internal: step " + std::to_string(step) +
                  " swept out of order or away from its linearization point";
        return false;
    }
    --pass.nextStep;

    Seeds seeds;
    if (*pass.seedFunction)
        (*pass.seedFunction)(step, seeds);
    std::map<const HydroCouple::IOutput *, std::vector<double>> outBar =
        std::move(seeds.m_values);

    std::map<HydroCouple::IModelComponent *,
             std::map<HydroCouple::IComponentDataItem *, std::vector<double>>>
        nextCarry;

    const auto &order = pass.order;
    for (auto it = order.rbegin(); it != order.rend(); ++it)
    {
        HydroCouple::IModelComponent *component = *it;
        auto *d = differentiable(component);

        // Seeds: output cotangents reached so far, state from step+1.
        EntryBatch seedBatch;
        for (HydroCouple::IOutput *output : d->differentiableOutputs())
        {
            auto found = outBar.find(output);
            if (found != outBar.end())
                seedBatch.seed(itemOf(output), DifferentialRole::Output,
                               output->shape(), found->second);
        }
        const auto states = d->differentiableStates();
        for (HydroCouple::IComponentDataItem *state : states)
        {
            auto &carried = pass.stateCarry[component];
            auto found = carried.find(state);
            if (found != carried.end())
                seedBatch.seed(state, DifferentialRole::StateAfter,
                               state->shape(), found->second);
        }
        if (seedBatch.empty())
            continue;   // nothing downstream depends on this component

        // Results: every input, argument and state it differentiates.
        EntryBatch results;
        const auto inputs = d->differentiableInputs();
        const auto arguments = d->differentiableArguments();
        std::vector<size_t> inputAt, argumentAt, stateAt;
        for (auto *input : inputs)
            inputAt.push_back(results.result(itemOf(input), DifferentialRole::Input,
                                             input->shape()));
        for (auto *argument : arguments)
            argumentAt.push_back(results.result(itemOf(argument),
                                                DifferentialRole::Argument,
                                                argument->shape()));
        for (auto *state : states)
            stateAt.push_back(results.result(state, DifferentialRole::StateBefore,
                                             state->shape()));

        std::string vjpMessage;
        if (!d->vjp(seedBatch.set(), results.set(), &vjpMessage))
        {
            message = "'" + component->id() + "'.vjp at step " +
                      std::to_string(step) + " refused: " + vjpMessage;
            return false;
        }

        for (size_t k = 0; k < arguments.size(); ++k)
            pass.argumentSums.add(component, arguments[k], arguments[k]->shape(),
                                  results.values(argumentAt[k]));
        for (size_t k = 0; k < states.size(); ++k)
            nextCarry[component][states[k]] = results.values(stateAt[k]);

        // Route each input's cotangent back along its connection.
        for (size_t k = 0; k < inputs.size(); ++k)
        {
            std::vector<double> cotangent = results.values(inputAt[k]);
            HydroCouple::IOutput *provider = inputs[k]->provider();
            while (auto *adapted =
                       dynamic_cast<HydroCouple::IDifferentiableAdaptedOutput *>(provider))
            {
                HydroCouple::IOutput *adaptee = adapted->adaptee();
                EntryBatch aSeeds, aResults;
                aSeeds.seed(itemOf<HydroCouple::IComponentDataItem>(adapted),
                            DifferentialRole::Output, adapted->shape(), cotangent);
                const size_t adapteeAt = aResults.result(
                    itemOf(adaptee), DifferentialRole::Input, adaptee->shape());
                const auto aArguments = adapted->differentiableArguments();
                std::vector<size_t> aArgumentAt;
                for (auto *argument : aArguments)
                    aArgumentAt.push_back(aResults.result(
                        itemOf(argument), DifferentialRole::Argument,
                        argument->shape()));
                std::string aMessage;
                if (!adapted->vjp(aSeeds.set(), aResults.set(), &aMessage))
                {
                    message = "adapted output '" + adapted->id() +
                              "'.vjp at step " + std::to_string(step) +
                              " refused: " + aMessage;
                    return false;
                }
                for (size_t a = 0; a < aArguments.size(); ++a)
                    pass.argumentSums.add(adapted, aArguments[a], aArguments[a]->shape(),
                                          aResults.values(aArgumentAt[a]));
                cotangent = aResults.values(adapteeAt);
                provider = adaptee;
            }
            if (!provider)
                continue;
            auto *owner = provider->modelComponent();
            if (!owner ||
                std::find(order.begin(), order.end(), owner) == order.end())
                continue;   // unmanaged provider: a constant
            accumulate(outBar[provider], cotangent);
        }
    }
    pass.stateCarry = std::move(nextCarry);
    return true;
}

bool DifferentiableWorkflow::reverseRange(int lo, int hi, Pass &pass,
                                          std::string &message)
{
    // Sweeps steps hi, hi-1, ..., lo+1, given the checkpoint after step lo.
    if (hi <= lo)
        return true;
    if (hi - lo == 1)
        return advance(lo, hi, message) && sweepStep(hi, pass, message);

    // Recursive bisection: recompute forward to the midpoint, keep it as a
    // temporary checkpoint while the upper half is reversed, then drop it.
    const int mid = lo + (hi - lo) / 2;
    if (!advance(lo, mid, message))
        return false;
    std::vector<std::string> tokens;
    if (!saveAll(tokens, message))
        return false;
    m_checkpoints[mid] = std::move(tokens);
    notePeak(1);   // + the final state backward() will return to
    const bool upper = reverseRange(mid, hi, pass, message);
    m_checkpoints.erase(mid);
    return upper && reverseRange(lo, mid, pass, message);
}

bool DifferentiableWorkflow::backward(const SeedFunction &seedFunction,
                                      Gradients &gradients, std::string &message)
{
    gradients = Gradients{};
    m_replays = 0;
    m_peak = 0;
    if (m_records.empty())
    {
        message = "nothing to differentiate: no step has been recorded";
        return false;
    }
    const std::vector<std::string> problems = differentiabilityProblems();
    if (!problems.empty())
    {
        message.clear();
        for (const std::string &p : problems)
            message += (message.empty() ? "" : "; ") + p;
        return false;
    }

    const int steps = static_cast<int>(m_records.size());

    // Where to come back to. Needed whenever a replay will happen.
    const bool willReplay = steps > 1 || m_linearizedAt != steps;
    std::vector<std::string> finalTokens;
    if (willReplay && !saveAll(finalTokens, message))
        return false;
    notePeak(willReplay ? 1 : 0);

    Pass pass;
    pass.seedFunction = &seedFunction;
    pass.order = scheduleOrder();
    pass.nextStep = steps;

    // The rungs held before this pass. reverseRange() drops each temporary
    // it makes before returning, success or failure.
    const std::vector<int> rungs = checkpointedSteps();

    auto finish = [&](bool ok, const std::string &why) {
        if (!ok)
            message = why;
        if (willReplay)
        {
            std::string restoreWhy;
            if (!restoreAll(finalTokens, restoreWhy) && ok)
            {
                ok = false;
                message = restoreWhy;   // the pass succeeded; coming back did not
            }
            m_linearizedAt = -1;
        }
        return ok;
    };

    std::string why;
    // The last step first: it needs no replay if nothing has moved since.
    if (!advance(steps - 1, steps, why) || !sweepStep(steps, pass, why))
        return finish(false, why);

    // Then each gap between rungs, newest first. The newest rung is always
    // the state after step steps-1, so the first gap ends there.
    int hi = steps - 1;
    for (auto rung = rungs.rbegin(); rung != rungs.rend() && hi > 0; ++rung)
    {
        if (!reverseRange(*rung, hi, pass, why))   // empty when *rung >= hi
            return finish(false, why);
        hi = *rung;
    }
    if (pass.nextStep != 0)
        return finish(false, "internal: steps 1.." + std::to_string(pass.nextStep) +
                                 " were never swept (no checkpoint after step 0)");

    for (auto *component : pass.order)
    {
        auto carried = pass.stateCarry.find(component);
        if (carried == pass.stateCarry.end())
            continue;
        for (auto &[state, values] : carried->second)
            gradients.initialStates.push_back(
                StateGradient{component, state, state->shape(), values});
    }
    gradients.arguments = pass.argumentSums.take();

    if (!finish(true, std::string()))
        return false;
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Differentiation
