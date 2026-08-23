// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/abstractexchangeitem.h"

namespace HydroCouple::SDK {

// ── ExchangeItemChangeEventArgs ───────────────────────────────────────────

ExchangeItemChangeEventArgs::ExchangeItemChangeEventArgs(
    HydroCouple::IExchangeItem *item, std::string message)
    : m_item(item), m_message(std::move(message))
{
}

HydroCouple::IExchangeItem *ExchangeItemChangeEventArgs::exchangeItem() const
{
    return m_item;
}

std::string ExchangeItemChangeEventArgs::message() const
{
    return m_message;
}


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
