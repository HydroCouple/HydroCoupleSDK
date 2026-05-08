// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/abstractoutput.h"

namespace HydroCouple {

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
    std::vector<HydroCouple::IInput *> result;
    result.reserve(m_consumers.size());
    for (const auto &[k, v] : m_consumers)
        result.push_back(v);
    return result;
}

void AbstractOutput::addConsumer(HydroCouple::IInput *consumer)
{
    if (!consumer) return;
    m_consumers[consumer->id()] = consumer;
    emitPropertyChanged("Consumers");
}

bool AbstractOutput::removeConsumer(HydroCouple::IInput *consumer)
{
    if (!consumer) return false;
    auto it = m_consumers.find(consumer->id());
    if (it != m_consumers.end() && it->second == consumer) {
        m_consumers.erase(it);
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

} // namespace HydroCouple
