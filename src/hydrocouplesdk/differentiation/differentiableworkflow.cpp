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
    m_linearizedAt = -1;
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
    if (!saveAll(record.tokens, message))
        return false;
    if (!TimeSteppedWorkflow::performStep(message))
        return false;
    m_records.push_back(std::move(record));
    m_linearizedAt = static_cast<int>(m_records.size());
    return true;
}

bool DifferentiableWorkflow::linearizeAt(int step, std::string &message)
{
    if (m_linearizedAt == step)
        return true;
    const Record &record = m_records[static_cast<size_t>(step - 1)];
    if (!restoreAll(record.tokens, message))
        return false;
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

bool DifferentiableWorkflow::backward(const SeedFunction &seedFunction,
                                      Gradients &gradients, std::string &message)
{
    gradients = Gradients{};
    m_replays = 0;
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

    const auto order = scheduleOrder();
    const int steps = static_cast<int>(m_records.size());

    // Where to come back to. Needed whenever a replay will happen.
    const bool willReplay = steps > 1 || m_linearizedAt != steps;
    std::vector<std::string> finalTokens;
    if (willReplay && !saveAll(finalTokens, message))
        return false;

    ArgumentSums argumentSums;
    std::map<HydroCouple::IModelComponent *,
             std::map<HydroCouple::IComponentDataItem *, std::vector<double>>>
        stateCarry;

    auto fail = [&](const std::string &why) {
        message = why;
        if (willReplay)
        {
            std::string ignored;
            restoreAll(finalTokens, ignored);
            m_linearizedAt = -1;
        }
        return false;
    };

    for (int step = steps; step >= 1; --step)
    {
        std::string why;
        if (!linearizeAt(step, why))
            return fail(why);

        Seeds seeds;
        if (seedFunction)
            seedFunction(step, seeds);
        std::map<const HydroCouple::IOutput *, std::vector<double>> outBar =
            std::move(seeds.m_values);

        std::map<HydroCouple::IModelComponent *,
                 std::map<HydroCouple::IComponentDataItem *, std::vector<double>>>
            nextCarry;

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
                auto &carried = stateCarry[component];
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
                return fail("'" + component->id() + "'.vjp at step " +
                            std::to_string(step) + " refused: " + vjpMessage);

            for (size_t k = 0; k < arguments.size(); ++k)
                argumentSums.add(component, arguments[k], arguments[k]->shape(),
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
                        return fail("adapted output '" + adapted->id() +
                                    "'.vjp at step " + std::to_string(step) +
                                    " refused: " + aMessage);
                    for (size_t a = 0; a < aArguments.size(); ++a)
                        argumentSums.add(adapted, aArguments[a], aArguments[a]->shape(),
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
        stateCarry = std::move(nextCarry);
    }

    for (auto *component : order)
    {
        auto carried = stateCarry.find(component);
        if (carried == stateCarry.end())
            continue;
        for (auto &[state, values] : carried->second)
            gradients.initialStates.push_back(
                StateGradient{component, state, state->shape(), values});
    }
    gradients.arguments = argumentSums.take();

    if (willReplay)
    {
        if (!restoreAll(finalTokens, message))
            return false;
        m_linearizedAt = -1;
    }
    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Differentiation
