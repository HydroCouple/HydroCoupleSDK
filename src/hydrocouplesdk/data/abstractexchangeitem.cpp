// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/abstractexchangeitem.h"

namespace HydroCouple::SDK {

// ── AbstractExchangeItem ──────────────────────────────────────────────────

AbstractExchangeItem::AbstractExchangeItem(
    std::string_view id,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractComponentDataItem(id, dimensions, valueDefinition, modelComponent)
{
}

AbstractExchangeItem::AbstractExchangeItem(
    std::string_view id,
    std::string_view caption,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractComponentDataItem(id, caption, dimensions, valueDefinition, modelComponent)
{
}

void AbstractExchangeItem::emitMessage(const std::string &message)
{
    emitValueChanged({}, {});
    (void)message; // message conveyed via the event args in more specific overrides
}

} // namespace HydroCouple::SDK
