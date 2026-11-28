// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/component/pulldrivenworkflow.h"
#include "hydrocouplesdk/data/abstractinput.h"

#include <algorithm>

namespace HydroCouple::SDK {

using ComponentStatus = HydroCouple::IModelComponent::ComponentStatus;

namespace {

/*!
 * \brief All providers connected to an input (single or multi).
 * \param[in] input  The input; must not be null.
 * \return An IMultiInput's providers(), otherwise the single provider, or
 *         an empty vector when nothing is connected.
 */
std::vector<HydroCouple::IOutput *> connectedProviders(
    HydroCouple::IInput *input)
{
    if (auto *multi = dynamic_cast<HydroCouple::IMultiInput *>(input))
        return multi->providers();
    if (HydroCouple::IOutput *provider = input->provider())
        return {provider};
    return {};
}

} // namespace

PullDrivenWorkflow::PullDrivenWorkflow(
    std::string_view id, HydroCouple::IWorkflowComponentInfo *info)
    : AbstractWorkflowComponent(id, info)
{
}

void PullDrivenWorkflow::setTrigger(HydroCouple::IInput *trigger)
{
    m_trigger = trigger;
}

HydroCouple::IInput *PullDrivenWorkflow::trigger() const noexcept
{
    return m_trigger;
}

void PullDrivenWorkflow::validateComposition(
    std::vector<std::string> &problems)
{
    if (!m_trigger)
    {
        problems.emplace_back(
            "no trigger input declared (see setTrigger())");
        return;
    }
    if (connectedProviders(m_trigger).empty())
        problems.emplace_back("trigger input '" + m_trigger->id() +
                              "' has no connected provider");
}

bool PullDrivenWorkflow::performStep(std::string &message)
{
    std::unordered_set<HydroCouple::IModelComponent *> visited;

    // Service every provider chain upstream of the trigger, then refresh
    // the providers and let the trigger retrieve.
    for (auto *provider : connectedProviders(m_trigger))
    {
        if (!serviceComponent(provider->modelComponent(), visited, message))
            return false;
        provider->updateValues(m_trigger);
    }

    if (auto *input = dynamic_cast<AbstractInput *>(m_trigger))
    {
        input->retrieveValuesFromProvider();
        input->applyData();
    }
    return true;
}

bool PullDrivenWorkflow::serviceComponent(
    HydroCouple::IModelComponent *component,
    std::unordered_set<HydroCouple::IModelComponent *> &visited,
    std::string &message)
{
    if (!component || visited.contains(component))
        return true;
    visited.insert(component);

    const ComponentStatus before = component->status();
    if (before == ComponentStatus::Done ||
        before == ComponentStatus::Finished)
        return true;

    // Post-order: satisfy this component's own inputs first.
    for (auto *input : component->inputs())
    {
        for (auto *provider : connectedProviders(input))
        {
            if (!serviceComponent(provider->modelComponent(), visited,
                                  message))
                return false;
            provider->updateValues(input);
        }
        if (auto *abstractInput = dynamic_cast<AbstractInput *>(input))
        {
            abstractInput->retrieveValuesFromProvider();
            abstractInput->applyData();
        }
    }

    component->update({});
    if (component->status() == ComponentStatus::Failed)
    {
        message = "component '" + component->id() + "' failed during update";
        return false;
    }
    return true;
}

} // namespace HydroCouple::SDK
