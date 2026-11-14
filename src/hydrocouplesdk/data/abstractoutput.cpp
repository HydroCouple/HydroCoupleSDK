// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/abstractoutput.h"
#include "hydrocouplesdk/component/abstractmodelcomponent.h"
#include "hydrocouplesdk/data/abstractadaptedoutput.h"
#include <algorithm>
#include <stdexcept>

namespace HydroCouple::SDK {

AbstractOutput::AbstractOutput(
    std::string_view id,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractExchangeItem(id, dimensions, valueDefinition, modelComponent)
{
}

AbstractOutput::AbstractOutput(
    std::string_view id,
    std::string_view caption,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractExchangeItem(id, caption, dimensions, valueDefinition, modelComponent)
{
}

std::vector<HydroCouple::IInput *> AbstractOutput::consumers() const
{
    return m_consumers;
}

AbstractOutput::~AbstractOutput()
{
    for (auto &[key, adapted] : m_adaptedOutputs)
        if (auto *sdk = dynamic_cast<AbstractAdaptedOutput *>(adapted))
            sdk->detachFromAdaptee();
}

void AbstractOutput::addConsumer(HydroCouple::IInput *consumer,
                                 const HydroCouple::IIdentity *providerRole)
{
    // The single wiring entry point (IOutput::addConsumer contract): check
    // compatibility, record the consumer, and set its provider so both ends
    // agree. Failure is reported through the owning component's error queue
    // and, locally, an exception.
    if (!consumer) return;
    if (std::find(m_consumers.begin(), m_consumers.end(), consumer) != m_consumers.end())
        return; // already connected

    // A multi-input is wired through addProvider() with the role, any other
    // input through setProvider(); a role names a multi-input's slot, so it
    // is refused for a plain input.
    std::string why;
    bool wired = false;
    if (auto *multi = dynamic_cast<HydroCouple::IMultiInput *>(consumer))
        wired = multi->canConsume(this, why, providerRole) &&
                multi->addProvider(this, providerRole);
    else if (providerRole)
        why = "provider role \"" + providerRole->id() +
              "\" given for an input that is not a multi-input";
    else
        wired = consumer->canConsume(this, why) && consumer->setProvider(this);

    if (!wired)
    {
        const std::string message = "Output \"" + id() + "\" cannot be consumed by input \"" +
                                    consumer->id() + "\"" + (why.empty() ? "" : ": " + why);
        if (auto *owner = dynamic_cast<AbstractModelComponent *>(modelComponent()))
            owner->queueError(HydroCouple::ErrorEntry::Severity::Error, 0, message);
        throw std::invalid_argument(message);
    }

    m_consumers.push_back(consumer);
    emitPropertyChanged("Consumers");
}

bool AbstractOutput::removeConsumer(HydroCouple::IInput *consumer)
{
    if (!consumer) return false;
    auto it = std::find(m_consumers.begin(), m_consumers.end(), consumer);
    if (it != m_consumers.end()) {
        m_consumers.erase(it);
        if (auto *multi = dynamic_cast<HydroCouple::IMultiInput *>(consumer))
            (void)multi->removeProvider(this);
        else
            (void)consumer->setProvider(nullptr);
        emitPropertyChanged("Consumers");
        return true;
    }
    return false;
}

std::vector<HydroCouple::IAdaptedOutput *> AbstractOutput::adaptedOutputs() const
{
    std::vector<HydroCouple::IAdaptedOutput *> result;
    result.reserve(m_adaptedOutputs.size());
    for (const auto &[k, v] : m_adaptedOutputs)
        result.push_back(v);
    return result;
}

void AbstractOutput::addAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput)
{
    if (!adaptedOutput) return;
    m_adaptedOutputs[adaptedOutput->id()] = adaptedOutput;
    emitPropertyChanged("AdaptedOutputs");
}

bool AbstractOutput::removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput)
{
    if (!adaptedOutput) return false;
    auto it = m_adaptedOutputs.find(adaptedOutput->id());
    if (it != m_adaptedOutputs.end() && it->second == adaptedOutput) {
        m_adaptedOutputs.erase(it);
        emitPropertyChanged("AdaptedOutputs");
        return true;
    }
    return false;
}

void AbstractOutput::updateValues(const HydroCouple::IInput * /*querySpecifier*/)
{
    updateValues();
    refreshAdaptedOutputs();
}

void AbstractOutput::refreshAdaptedOutputs()
{
    for (auto &[k, ao] : m_adaptedOutputs)
        ao->refresh();
}

void AbstractOutput::initializeAdaptedOutputs()
{
    for (auto &[k, ao] : m_adaptedOutputs)
        ao->initialize();
}

} // namespace HydroCouple::SDK
