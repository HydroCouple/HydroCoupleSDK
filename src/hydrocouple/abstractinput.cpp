// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/abstractinput.h"

namespace HydroCouple {

// ── AbstractInput ─────────────────────────────────────────────────────────

AbstractInput::AbstractInput(
    std::string_view id,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractExchangeItem(id, dimensions, valueDefinition, modelComponent)
{
}

AbstractInput::AbstractInput(
    std::string_view id,
    std::string_view caption,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractExchangeItem(id, caption, dimensions, valueDefinition, modelComponent)
{
}

HydroCouple::IOutput *AbstractInput::provider() const
{
    return m_provider;
}

bool AbstractInput::setProvider(HydroCouple::IOutput *provider)
{
    if (provider) {
        std::string msg;
        if (!canConsume(provider, msg))
            return false;
    }
    m_provider = provider;
    emitPropertyChanged("Provider");
    return true;
}


// ── AbstractMultiInput ────────────────────────────────────────────────────

AbstractMultiInput::AbstractMultiInput(
    std::string_view id,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractInput(id, dimensions, valueDefinition, modelComponent)
{
}

AbstractMultiInput::AbstractMultiInput(
    std::string_view id,
    std::string_view caption,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractInput(id, caption, dimensions, valueDefinition, modelComponent)
{
}

std::vector<HydroCouple::IIdentity *> AbstractMultiInput::providerLabels() const
{
    // Default: no named roles — all providers fill the same slot.
    return {};
}

bool AbstractMultiInput::isRequiredProvider(
    const HydroCouple::IIdentity * /*providerLabel*/) const
{
    return false;
}

std::vector<HydroCouple::IOutput *> AbstractMultiInput::providers() const
{
    std::vector<HydroCouple::IOutput *> result;
    result.reserve(m_providers.size());
    for (const auto &[k, v] : m_providers)
        result.push_back(v);
    return result;
}

bool AbstractMultiInput::addProvider(
    HydroCouple::IOutput *provider,
    const HydroCouple::IIdentity *providerRoleIdentifier)
{
    if (!provider) return false;
    std::string key = providerRoleIdentifier ? providerRoleIdentifier->id() : provider->id();
    m_providers[key] = provider;
    emitPropertyChanged("Providers");
    return true;
}

bool AbstractMultiInput::removeProvider(HydroCouple::IOutput *provider)
{
    if (!provider) return false;
    for (auto it = m_providers.begin(); it != m_providers.end(); ++it) {
        if (it->second == provider) {
            m_providers.erase(it);
            emitPropertyChanged("Providers");
            return true;
        }
    }
    return false;
}

} // namespace HydroCouple
