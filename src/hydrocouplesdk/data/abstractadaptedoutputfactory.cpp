// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/abstractadaptedoutputfactory.h"
#include "hydrocouplesdk/data/abstractadaptedoutputfactorycomponentinfo.h"

namespace HydroCouple::SDK {

// ── AbstractAdaptedOutputFactory ─────────────────────────────────────────

AbstractAdaptedOutputFactory::AbstractAdaptedOutputFactory(std::string_view id)
    : Identity(id)
{
}


// ── AbstractAdaptedOutputFactoryComponent ────────────────────────────────

AbstractAdaptedOutputFactoryComponent::AbstractAdaptedOutputFactoryComponent(
    std::string_view id, AbstractAdaptedOutputFactoryComponentInfo *info)
    : AbstractAdaptedOutputFactory(id)
    , m_componentInfo(info)
{
}

HydroCouple::IAdaptedOutputFactoryComponentInfo *
AbstractAdaptedOutputFactoryComponent::componentInfo() const
{
    return m_componentInfo;
}

void AbstractAdaptedOutputFactoryComponent::setComponentInfo(
    AbstractAdaptedOutputFactoryComponentInfo *info)
{
    m_componentInfo = info;
}

} // namespace HydroCouple::SDK
