// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/abstractadaptedoutput.h"
#include "hydrocouplesdk/data/abstractargument.h"

namespace HydroCouple::SDK {

AbstractAdaptedOutput::AbstractAdaptedOutput(
    std::string_view id,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IOutput *adaptee,
    HydroCouple::IAdaptedOutputFactory *adaptedOutputFactory)
    : AbstractComponentDataItem(id, dimensions, valueDefinition,
                                adaptee ? adaptee->modelComponent() : nullptr)
    , m_adaptee(adaptee)
    , m_adaptedOutputFactory(adaptedOutputFactory)
{
}

// ── IOutput ───────────────────────────────────────────────────────────────

std::vector<HydroCouple::IInput *> AbstractAdaptedOutput::consumers() const
{
    std::vector<HydroCouple::IInput *> result;
    result.reserve(m_consumers.size());
    for (const auto &[k, v] : m_consumers)
        result.push_back(v);
    return result;
}

void AbstractAdaptedOutput::addConsumer(HydroCouple::IInput *consumer)
{
    if (!consumer) return;
    m_consumers[consumer->id()] = consumer;
    emitPropertyChanged("Consumers");
}

bool AbstractAdaptedOutput::removeConsumer(HydroCouple::IInput *consumer)
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

std::vector<HydroCouple::IAdaptedOutput *> AbstractAdaptedOutput::adaptedOutputs() const
{
    std::vector<HydroCouple::IAdaptedOutput *> result;
    result.reserve(m_adaptedOutputs.size());
    for (const auto &[k, v] : m_adaptedOutputs)
        result.push_back(v);
    return result;
}

void AbstractAdaptedOutput::addAdaptedOutput(HydroCouple::IAdaptedOutput *ao)
{
    if (!ao) return;
    m_adaptedOutputs[ao->id()] = ao;
    emitPropertyChanged("AdaptedOutputs");
}

bool AbstractAdaptedOutput::removeAdaptedOutput(HydroCouple::IAdaptedOutput *ao)
{
    if (!ao) return false;
    auto it = m_adaptedOutputs.find(ao->id());
    if (it != m_adaptedOutputs.end() && it->second == ao) {
        m_adaptedOutputs.erase(it);
        emitPropertyChanged("AdaptedOutputs");
        return true;
    }
    return false;
}

void AbstractAdaptedOutput::updateValues(const HydroCouple::IInput *querySpecifier)
{
    m_input = const_cast<HydroCouple::IInput *>(querySpecifier);
    updateValues();
    refreshAdaptedOutputs();
}

// ── IAdaptedOutput ────────────────────────────────────────────────────────

HydroCouple::IAdaptedOutputFactory *AbstractAdaptedOutput::adaptedOutputFactory() const
{
    return m_adaptedOutputFactory;
}

std::vector<HydroCouple::IArgument *> AbstractAdaptedOutput::arguments() const
{
    std::vector<HydroCouple::IArgument *> result;
    result.reserve(m_arguments.size());
    for (const auto &[k, v] : m_arguments)
        result.push_back(v);
    return result;
}

HydroCouple::IOutput *AbstractAdaptedOutput::adaptee() const
{
    return m_adaptee;
}

HydroCouple::IInput *AbstractAdaptedOutput::input() const
{
    return m_input;
}

void AbstractAdaptedOutput::setInput(HydroCouple::IInput *input)
{
    m_input = input;
}

void AbstractAdaptedOutput::refreshAdaptedOutputs()
{
    for (auto &[k, ao] : m_adaptedOutputs)
        ao->refresh();
}

void AbstractAdaptedOutput::initializeAdaptedOutputs()
{
    for (auto &[k, ao] : m_adaptedOutputs)
        ao->initialize();
}

// ── Protected ─────────────────────────────────────────────────────────────

void AbstractAdaptedOutput::addArgument(AbstractArgument *argument)
{
    if (!argument) return;
    m_arguments[argument->id()] = argument;
    emitPropertyChanged("Arguments");
}

bool AbstractAdaptedOutput::removeArgument(AbstractArgument *argument)
{
    if (!argument) return false;
    auto it = m_arguments.find(argument->id());
    if (it != m_arguments.end() && it->second == argument) {
        m_arguments.erase(it);
        emitPropertyChanged("Arguments");
        return true;
    }
    return false;
}

void AbstractAdaptedOutput::clearArguments()
{
    m_arguments.clear();
    emitPropertyChanged("Arguments");
}

std::unordered_map<std::string, AbstractArgument *>
AbstractAdaptedOutput::argumentsInternal() const
{
    return m_arguments;
}

} // namespace HydroCouple::SDK
