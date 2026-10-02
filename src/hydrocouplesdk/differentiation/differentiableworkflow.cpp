// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/differentiation/differentiableworkflow.h"
#include "hydrocouplesdk/differentiation/differentials.h"

#include <algorithm>
#include <deque>
#include <functional>
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

HydroCouple::IDifferentiableAdaptedOutput *
differentiable(HydroCouple::IAdaptedOutput *adapted)
{
    return dynamic_cast<HydroCouple::IDifferentiableAdaptedOutput *>(adapted);
}

HydroCouple::ICheckpointableAdaptedOutput *
checkpointable(HydroCouple::IAdaptedOutput *adapted)
{
    return dynamic_cast<HydroCouple::ICheckpointableAdaptedOutput *>(adapted);
}

//! Whether an adapter's values depend on its previous refreshes.
bool carriesState(HydroCouple::IAdaptedOutput *adapted)
{
    if (!adapted->states().empty())
        return true;
    auto *d = differentiable(adapted);
    return d && !d->differentiableStates().empty();
}

const HydroCouple::IIdentity *ownerOf(const HydroCouple::IModelComponent *component)
{
    return component;
}

const HydroCouple::IIdentity *ownerOf(const HydroCouple::IAdaptedOutput *adapted)
{
    return adapted;
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
Gradients::findState(const HydroCouple::IIdentity *owner,
                     const HydroCouple::IComponentDataItem *state) const
{
    for (const StateGradient &g : initialStates)
        if (g.owner == owner && g.state == state)
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
    for (auto &entry : m_checkpoints)
        releaseAll(entry.second);
    m_records.clear();
    m_checkpoints.clear();
    m_linearizedAt = -1;
}

bool DifferentiableWorkflow::rewind(std::string &message)
{
    auto first = m_checkpoints.find(0);
    if (m_records.empty() || first == m_checkpoints.end())
    {
        message = "nothing to rewind: no step has been recorded";
        return false;
    }
    if (!restoreAll(first->second, message))
        return false;
    clearRecording();
    message.clear();
    return true;
}

void DifferentiableWorkflow::finish()
{
    clearRecording();   // while the components are still unfinished
    TimeSteppedWorkflow::finish();
}

int DifferentiableWorkflow::releasedTokens() const noexcept
{
    return m_released;
}

int DifferentiableWorkflow::refusedReleases() const noexcept
{
    return m_refusedReleases;
}

void DifferentiableWorkflow::releaseAll(const Checkpoint &checkpoint)
{
    const auto order = scheduleOrder();
    for (size_t i = 0; i < order.size() && i < checkpoint.components.size(); ++i)
    {
        if (checkpoint.components[i].empty())
            continue;   // a component without Capability::Checkpointing
        auto *cp = checkpointable(order[i]);
        if (!cp)
            continue;
        std::string why;
        if (cp->releaseState(checkpoint.components[i], why))
            ++m_released;
        else
            ++m_refusedReleases;
    }
    // Only adapters still in the composition are asked: one that has left it
    // may be gone, and its token is dropped unreleased.
    const auto adapters = managedAdapters();
    const std::set<HydroCouple::IAdaptedOutput *> present(adapters.begin(), adapters.end());
    for (const AdapterToken &saved : checkpoint.adapters)
    {
        if (!present.count(saved.adapter) || saved.adapter->id() != saved.id)
            continue;
        auto *cp = checkpointable(saved.adapter);
        if (!cp)
            continue;
        std::string why;
        if (cp->releaseState(saved.token, why))
            ++m_released;
        else
            ++m_refusedReleases;
    }
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
        if ((k % power) == 0)
        {
            ++it;
            continue;
        }
        releaseAll(it->second);
        it = m_checkpoints.erase(it);
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

std::vector<HydroCouple::IAdaptedOutput *> DifferentiableWorkflow::managedAdapters() const
{
    std::vector<HydroCouple::IAdaptedOutput *> found;
    std::set<HydroCouple::IAdaptedOutput *> seen;
    std::function<void(HydroCouple::IOutput *)> walk = [&](HydroCouple::IOutput *output) {
        for (HydroCouple::IAdaptedOutput *adapted : output->adaptedOutputs())
        {
            if (!adapted || !seen.insert(adapted).second)
                continue;
            found.push_back(adapted);
            walk(adapted);
        }
    };
    for (auto *component : scheduleOrder())
        for (HydroCouple::IOutput *output : component->outputs())
            if (output)
                walk(output);
    return found;
}

bool DifferentiableWorkflow::onPrepare(std::string &message)
{
    clearRecording();
    return TimeSteppedWorkflow::onPrepare(message);
}

bool DifferentiableWorkflow::saveAll(Checkpoint &checkpoint, std::string &message)
{
    checkpoint = Checkpoint{};
    const auto order = scheduleOrder();
    checkpoint.components.assign(order.size(), std::string());
    for (size_t i = 0; i < order.size(); ++i)
    {
        auto *cp = checkpointable(order[i]);
        if (!cp)
            continue;
        std::string why;
        if (!cp->saveState(checkpoint.components[i], why))
        {
            message = "checkpoint of '" + order[i]->id() + "' failed: " + why;
            checkpoint.components[i].clear();
            releaseAll(checkpoint);   // nothing will restore a partial checkpoint
            checkpoint = Checkpoint{};
            return false;
        }
    }
    for (HydroCouple::IAdaptedOutput *adapted : managedAdapters())
    {
        auto *cp = checkpointable(adapted);
        if (!cp)
            continue;
        std::string token, why;
        if (!cp->saveState(token, why))
        {
            message = "checkpoint of adapted output '" + adapted->id() + "' failed: " + why;
            releaseAll(checkpoint);
            checkpoint = Checkpoint{};
            return false;
        }
        checkpoint.adapters.push_back(AdapterToken{adapted, adapted->id(), std::move(token)});
    }
    return true;
}

bool DifferentiableWorkflow::restoreAll(const Checkpoint &checkpoint,
                                        std::string &message)
{
    // Refuse before restoring anything: a composition restored in part is
    // worse than one left where it was.
    const auto order = scheduleOrder();
    for (size_t i = 0; i < order.size(); ++i)
        if (!checkpointable(order[i]))
        {
            message = "component '" + order[i]->id() +
                      "' lacks Capability::Checkpointing, so an earlier step "
                      "cannot be replayed";
            return false;
        }
    const auto adapters = managedAdapters();
    const std::set<HydroCouple::IAdaptedOutput *> present(adapters.begin(), adapters.end());
    std::set<HydroCouple::IAdaptedOutput *> saved;
    for (const AdapterToken &entry : checkpoint.adapters)
    {
        if (!present.count(entry.adapter) || entry.adapter->id() != entry.id)
        {
            message = "adapted output '" + entry.id + "', checkpointed earlier, "
                      "has left the composition, so an earlier step cannot be replayed";
            return false;
        }
        saved.insert(entry.adapter);
    }
    for (HydroCouple::IAdaptedOutput *adapted : adapters)
        if (carriesState(adapted) && !saved.count(adapted))
        {
            message = "adapted output '" + adapted->id() +
                      "' carries state but " +
                      (checkpointable(adapted)
                           ? std::string("joined the composition after the checkpoint")
                           : std::string("is not an ICheckpointableAdaptedOutput")) +
                      ", so an earlier step cannot be replayed";
            return false;
        }

    for (size_t i = 0; i < order.size(); ++i)
    {
        std::string why;
        if (!checkpointable(order[i])->restoreState(checkpoint.components[i], why))
        {
            message = "restore of '" + order[i]->id() + "' failed: " + why;
            return false;
        }
    }
    // A stateless adapter's values follow from its adaptee's: refresh the
    // trees on the restored outputs (each adapter refreshes its children)...
    for (auto *component : order)
        for (HydroCouple::IOutput *output : component->outputs())
            if (output)
                for (HydroCouple::IAdaptedOutput *adapted : output->adaptedOutputs())
                    if (adapted)
                        adapted->refresh();
    // ... then return every stateful one to its checkpoint, which that
    // refresh moved on.
    for (const AdapterToken &entry : checkpoint.adapters)
    {
        std::string why;
        if (!checkpointable(entry.adapter)->restoreState(entry.token, why))
        {
            message = "restore of adapted output '" + entry.id + "' failed: " + why;
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
    Checkpoint tokens;
    if (!saveAll(tokens, message))
        return false;
    if (!TimeSteppedWorkflow::performStep(message))
    {
        // The step never happened, so nothing will restore to before it.
        releaseAll(tokens);
        return false;
    }
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
            std::vector<HydroCouple::IAdaptedOutput *> chain;
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
                chain.push_back(adapted);
                provider = adapted->adaptee();
            }
            if (broken || !provider)
                continue;
            auto *owner = provider->modelComponent();
            if (owner == component)
            {
                // The schedule has no self-edges, so this is no loop to it;
                // but the input reads the previous step's output, a lag the
                // sweep does not model.
                problems.push_back("'" + component->id() + "." + input->id() +
                                   "' reads '" + component->id() + "''s own output '" +
                                   provider->id() +
                                   "': a lagged self-coupling is not differentiable yet");
                continue;
            }
            if (!owner || !managed.count(owner))
            {
                // An unmanaged provider is a constant, and so is a stateless
                // adapter on its connection; a stateful one would have to be
                // replayed, which nothing here does.
                for (HydroCouple::IAdaptedOutput *adapted : chain)
                    if (carriesState(adapted))
                        problems.push_back("adapted output '" + adapted->id() +
                                           "' carries state but adapts '" +
                                           provider->id() +
                                           "', whose component is outside the "
                                           "workflow, so its refreshes cannot be replayed");
                continue;
            }
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

    // Stateful adapters (G2.4): replaying an earlier step must return them
    // too, whether or not a derivative crosses them -- an adapter whose state
    // ran on would feed the replay the wrong values.
    if (m_records.size() > 1)
        for (HydroCouple::IAdaptedOutput *adapted : managedAdapters())
            if (carriesState(adapted) && !checkpointable(adapted))
                problems.push_back("adapted output '" + adapted->id() +
                                   "' carries state but is not an "
                                   "ICheckpointableAdaptedOutput, so only a "
                                   "single recorded step can be differentiated");

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

//! State cotangents, by the component or adapter that carries the state.
using StateCarry =
    std::map<const HydroCouple::IIdentity *,
             std::map<HydroCouple::IComponentDataItem *, std::vector<double>>>;

//! The working state of one reverse pass.
struct DifferentiableWorkflow::Pass
{
    const SeedFunction *seedFunction = nullptr;
    std::vector<HydroCouple::IModelComponent *> order;
    std::vector<HydroCouple::IAdaptedOutput *> adapters;
    ArgumentSums argumentSums;
    //! State cotangents carried from the step after the one being swept.
    StateCarry stateCarry;
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
    // Cotangents of every output this step, root outputs and adapted outputs
    // alike: whatever reads an output adds to its entry.
    std::map<const HydroCouple::IOutput *, std::vector<double>> outBar =
        std::move(seeds.m_values);

    StateCarry nextCarry;
    const std::set<HydroCouple::IModelComponent *> managed(pass.order.begin(),
                                                           pass.order.end());
    std::set<HydroCouple::IAdaptedOutput *> reversed;
    //! Root outputs of unmanaged providers whose adapters a managed input reads.
    std::set<HydroCouple::IOutput *> externalRoots;

    // One adapter's vjp for this step: seeded with what its consumers added
    // and with its state carried back from step+1; it adds its adaptee's
    // cotangent to the adaptee.
    auto reverseAdapter = [&](HydroCouple::IAdaptedOutput *adapted) -> bool {
        auto *d = differentiable(adapted);
        if (!d)
        {
            // No differentiable input reads through it (checked up front), so
            // a cotangent here came from a seed -- on it, or on a child.
            if (outBar.count(adapted))
            {
                message = "a cotangent reached adapted output '" + adapted->id() +
                          "' at step " + std::to_string(step) +
                          " (seeded on it or on an adapter it feeds), but it is "
                          "not differentiable";
                return false;
            }
            return true;
        }
        const auto *self = itemOf<HydroCouple::IComponentDataItem>(adapted);
        EntryBatch seedBatch;
        auto found = outBar.find(adapted);
        if (found != outBar.end())
            seedBatch.seed(self, DifferentialRole::Output, adapted->shape(), found->second);
        const auto states = d->differentiableStates();
        auto &carried = pass.stateCarry[ownerOf(adapted)];
        for (HydroCouple::IComponentDataItem *state : states)
        {
            auto c = carried.find(state);
            if (c != carried.end())
                seedBatch.seed(state, DifferentialRole::StateAfter, state->shape(), c->second);
        }
        if (seedBatch.empty())
            return true;   // nothing downstream depends on this adapter

        HydroCouple::IOutput *adaptee = adapted->adaptee();
        EntryBatch results;
        size_t adapteeAt = 0;
        if (adaptee)
            adapteeAt = results.result(itemOf(adaptee), DifferentialRole::Input,
                                       adaptee->shape());
        const auto arguments = d->differentiableArguments();
        std::vector<size_t> argumentAt, stateAt;
        for (auto *argument : arguments)
            argumentAt.push_back(results.result(itemOf(argument), DifferentialRole::Argument,
                                                argument->shape()));
        for (auto *state : states)
            stateAt.push_back(results.result(state, DifferentialRole::StateBefore,
                                             state->shape()));

        std::string why;
        if (!d->vjp(seedBatch.set(), results.set(), &why))
        {
            message = "adapted output '" + adapted->id() + "'.vjp at step " +
                      std::to_string(step) + " refused: " + why;
            return false;
        }
        for (size_t a = 0; a < arguments.size(); ++a)
            pass.argumentSums.add(adapted, arguments[a], arguments[a]->shape(),
                                  results.values(argumentAt[a]));
        for (size_t k = 0; k < states.size(); ++k)
            nextCarry[ownerOf(adapted)][states[k]] = results.values(stateAt[k]);
        if (adaptee)
            accumulate(outBar[adaptee], results.values(adapteeAt));
        return true;
    };

    // An adapter tree, leaves first: every child has added its share to its
    // adaptee before the adaptee's own vjp runs.
    std::function<bool(HydroCouple::IAdaptedOutput *)> reverseTree =
        [&](HydroCouple::IAdaptedOutput *adapted) -> bool {
        if (!reversed.insert(adapted).second)
            return true;
        for (HydroCouple::IAdaptedOutput *child : adapted->adaptedOutputs())
            if (child && !reverseTree(child))
                return false;
        return reverseAdapter(adapted);
    };

    const auto &order = pass.order;
    for (auto it = order.rbegin(); it != order.rend(); ++it)
    {
        HydroCouple::IModelComponent *component = *it;
        auto *d = differentiable(component);

        // Everything downstream of this component has run: reverse the
        // adapters hanging off its outputs, so their cotangents reach the
        // outputs themselves.
        for (HydroCouple::IOutput *output : component->outputs())
            if (output)
                for (HydroCouple::IAdaptedOutput *adapted : output->adaptedOutputs())
                    if (adapted && !reverseTree(adapted))
                        return false;

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
        auto &carried = pass.stateCarry[ownerOf(component)];
        for (HydroCouple::IComponentDataItem *state : states)
        {
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
            nextCarry[ownerOf(component)][states[k]] = results.values(stateAt[k]);

        // Each input's cotangent goes to the output it reads -- a root output
        // or an adapted output, which passes it on when its tree is reversed.
        for (size_t k = 0; k < inputs.size(); ++k)
        {
            HydroCouple::IOutput *provider = inputs[k]->provider();
            HydroCouple::IOutput *root = provider;
            while (auto *adapted = dynamic_cast<HydroCouple::IAdaptedOutput *>(root))
                root = adapted->adaptee();
            if (!root)
                continue;
            auto *owner = root->modelComponent();
            if (owner && managed.count(owner))
            {
                accumulate(outBar[provider], results.values(inputAt[k]));
                continue;
            }
            // An unmanaged provider is a constant, but the adapters on its
            // connection still have arguments: reverse them after the loop,
            // and drop what reaches the root.
            if (provider != root)
            {
                accumulate(outBar[provider], results.values(inputAt[k]));
                externalRoots.insert(root);
            }
        }
    }
    for (HydroCouple::IOutput *root : externalRoots)
        for (HydroCouple::IAdaptedOutput *adapted : root->adaptedOutputs())
            if (adapted && !reverseTree(adapted))
                return false;
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
    Checkpoint tokens;
    if (!saveAll(tokens, message))
        return false;
    m_checkpoints[mid] = std::move(tokens);
    notePeak(1);   // + the final state backward() will return to
    const bool upper = reverseRange(mid, hi, pass, message);
    releaseAll(m_checkpoints[mid]);
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
    Checkpoint finalTokens;
    if (willReplay && !saveAll(finalTokens, message))
        return false;
    notePeak(willReplay ? 1 : 0);

    Pass pass;
    pass.seedFunction = &seedFunction;
    pass.order = scheduleOrder();
    pass.adapters = managedAdapters();
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
            releaseAll(finalTokens);    // back (or not): it cannot be used again
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
        auto carried = pass.stateCarry.find(ownerOf(component));
        if (carried == pass.stateCarry.end())
            continue;
        for (auto &[state, values] : carried->second)
            gradients.initialStates.push_back(StateGradient{
                ownerOf(component), component, nullptr, state, state->shape(), values});
    }
    for (auto *adapted : pass.adapters)
    {
        auto carried = pass.stateCarry.find(ownerOf(adapted));
        if (carried == pass.stateCarry.end())
            continue;
        for (auto &[state, values] : carried->second)
            gradients.initialStates.push_back(StateGradient{
                ownerOf(adapted), nullptr, adapted, state, state->shape(), values});
    }
    gradients.arguments = pass.argumentSums.take();

    if (!finish(true, std::string()))
        return false;
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Differentiation
