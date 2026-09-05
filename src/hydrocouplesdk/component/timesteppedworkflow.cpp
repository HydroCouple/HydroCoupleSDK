// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/component/timesteppedworkflow.h"

#include <algorithm>
#include <functional>
#include <unordered_map>

namespace HydroCouple::SDK {

using ComponentStatus = HydroCouple::IModelComponent::ComponentStatus;

TimeSteppedWorkflow::TimeSteppedWorkflow(
    std::string_view id, HydroCouple::IWorkflowComponentInfo *info)
    : AbstractWorkflowComponent(id, info)
{
}

int TimeSteppedWorkflow::iterationsPerGroup() const noexcept
{
    return m_iterationsPerGroup;
}

void TimeSteppedWorkflow::setIterationsPerGroup(int iterations)
{
    if (iterations >= 1)
        m_iterationsPerGroup = iterations;
}

const std::vector<std::vector<HydroCouple::IModelComponent *>> &
TimeSteppedWorkflow::schedule() const noexcept
{
    return m_schedule;
}

bool TimeSteppedWorkflow::onPrepare(std::string &message)
{
    m_schedule.clear();

    const std::vector<HydroCouple::IModelComponent *> components =
        modelComponents();
    const size_t count = components.size();

    std::unordered_map<HydroCouple::IModelComponent *, size_t> indexOf;
    for (size_t i = 0; i < count; ++i)
        indexOf[components[i]] = i;

    // Edges: provider component → consumer component for every connected
    // exchange link between managed components.
    std::vector<std::vector<size_t>> successors(count);
    for (size_t consumer = 0; consumer < count; ++consumer)
    {
        for (auto *input : components[consumer]->inputs())
        {
            std::vector<HydroCouple::IOutput *> providers;
            if (auto *multi = dynamic_cast<HydroCouple::IMultiInput *>(input))
                providers = multi->providers();
            else if (HydroCouple::IOutput *provider = input->provider())
                providers.push_back(provider);

            for (auto *provider : providers)
            {
                auto found = indexOf.find(provider->modelComponent());
                if (found != indexOf.end() && found->second != consumer)
                    successors[found->second].push_back(consumer);
            }
        }
    }

    // Tarjan's strongly-connected-components algorithm (iterative form is
    // unnecessary at composition scale; recursion depth = component count).
    std::vector<int>    order(count, -1), low(count, 0);
    std::vector<bool>   onStack(count, false);
    std::vector<size_t> stack;
    std::vector<std::vector<size_t>> sccs;
    int nextOrder = 0;

    std::function<void(size_t)> strongConnect = [&](size_t v) {
        order[v] = low[v] = nextOrder++;
        stack.push_back(v);
        onStack[v] = true;
        for (size_t w : successors[v])
        {
            if (order[w] < 0)
            {
                strongConnect(w);
                low[v] = std::min(low[v], low[w]);
            }
            else if (onStack[w])
            {
                low[v] = std::min(low[v], order[w]);
            }
        }
        if (low[v] == order[v])
        {
            std::vector<size_t> scc;
            size_t w;
            do
            {
                w = stack.back();
                stack.pop_back();
                onStack[w] = false;
                scc.push_back(w);
            } while (w != v);
            sccs.push_back(std::move(scc));
        }
    };

    for (size_t v = 0; v < count; ++v)
    {
        if (order[v] < 0)
            strongConnect(v);
    }

    // Tarjan emits SCCs in reverse topological order of the condensation;
    // reverse to schedule providers before their consumers, and keep each
    // group's members in composition insertion order for determinism.
    std::reverse(sccs.begin(), sccs.end());
    m_schedule.reserve(sccs.size());
    for (auto &scc : sccs)
    {
        std::sort(scc.begin(), scc.end());
        std::vector<HydroCouple::IModelComponent *> group;
        group.reserve(scc.size());
        for (size_t v : scc)
            group.push_back(components[v]);
        m_schedule.push_back(std::move(group));
    }

    message.clear();
    return true;
}

bool TimeSteppedWorkflow::performStep(std::string &message)
{
    for (const auto &group : m_schedule)
    {
        const int sweeps =
            group.size() > 1 ? m_iterationsPerGroup : 1;
        for (int sweep = 0; sweep < sweeps; ++sweep)
        {
            for (auto *component : group)
            {
                const ComponentStatus status = component->status();
                if (status == ComponentStatus::Done ||
                    status == ComponentStatus::Finished)
                    continue;

                component->update({});
                if (component->status() == ComponentStatus::Failed)
                {
                    message = "component '" + component->id() +
                              "' failed during update";
                    return false;
                }

                // The push-driven schedule bypasses
                // IOutput::updateValues(query), which is where adapted
                // outputs normally refresh (pull-driven rides it). First
                // tier only: the IAdaptedOutput contract makes each
                // adapted output refresh its own children. Inside the
                // sweep loop, so cyclic groups iterate on refreshed
                // values.
                for (HydroCouple::IOutput *output : component->outputs())
                {
                    if (!output)
                        continue;
                    for (HydroCouple::IAdaptedOutput *adapted :
                         output->adaptedOutputs())
                        if (adapted)
                            adapted->refresh();
                }
            }
        }
    }
    return true;
}

} // namespace HydroCouple::SDK
