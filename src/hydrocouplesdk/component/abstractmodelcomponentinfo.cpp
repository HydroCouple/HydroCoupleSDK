// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/component/abstractmodelcomponentinfo.h"
// AbstractAdaptedOutputFactory is forward-declared; we only need the
// IAdaptedOutputFactory interface for id() — no inclusion of Qt-based header.

namespace HydroCouple::SDK {

AbstractModelComponentInfo::AbstractModelComponentInfo(std::string_view id)
    : ComponentInfo(id)
{
}

std::vector<HydroCouple::IAdaptedOutputFactory *>
AbstractModelComponentInfo::adaptedOutputFactories() const
{
    std::vector<HydroCouple::IAdaptedOutputFactory *> result;
    result.reserve(m_adaptedOutputFactories.size());
    for (const auto &[key, factory] : m_adaptedOutputFactories)
        result.push_back(factory);
    return result;
}

std::unordered_map<std::string, HydroCouple::IAdaptedOutputFactory *>
AbstractModelComponentInfo::adaptedOutputFactoriesInternal() const
{
    return m_adaptedOutputFactories;
}

void AbstractModelComponentInfo::addAdaptedOutputFactory(
    HydroCouple::IAdaptedOutputFactory *factory)
{
    if (!factory) return;
    m_adaptedOutputFactories[factory->id()] = factory;
    emitPropertyChanged("AdaptedOutputFactories");
}

bool AbstractModelComponentInfo::removeAdaptedOutputFactory(
    HydroCouple::IAdaptedOutputFactory *factory)
{
    if (!factory) return false;
    auto it = m_adaptedOutputFactories.find(factory->id());
    if (it != m_adaptedOutputFactories.end() && it->second == factory) {
        m_adaptedOutputFactories.erase(it);
        emitPropertyChanged("AdaptedOutputFactories");
        return true;
    }
    return false;
}

void AbstractModelComponentInfo::clearAdaptedOutputFactories()
{
    m_adaptedOutputFactories.clear();
    emitPropertyChanged("AdaptedOutputFactories");
}

} // namespace HydroCouple::SDK
