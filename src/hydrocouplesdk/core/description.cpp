// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/core/description.h"

namespace HydroCouple::SDK {

Description::Description(std::string_view caption)
    : m_caption(caption)
{
}

Description::Description(std::string_view caption, std::string_view description)
    : m_caption(caption), m_description(description)
{
}

const std::string &Description::caption() const
{
    return m_caption;
}

void Description::setCaption(const std::string &caption)
{
    m_caption = caption;
    emitPropertyChanged("Caption");
}

const std::string &Description::description() const
{
    return m_description;
}

void Description::setDescription(const std::string &description)
{
    m_description = description;
    emitPropertyChanged("Description");
}

void Description::connect(const std::shared_ptr<ISlot<std::string>> &slot)
{
    m_propertyChanged.connect(slot);
}

void Description::disconnect(const std::shared_ptr<ISlot<std::string>> &slot)
{
    m_propertyChanged.disconnect(slot);
}

void Description::blockSignals(bool block)
{
    m_propertyChanged.blockSignals(block);
}

void Description::emit(std::string propertyName)
{
    m_propertyChanged.emit(propertyName);
}

void Description::emitPropertyChanged(std::string_view propertyName)
{
    m_propertyChanged.emit(std::string(propertyName));
}

} // namespace HydroCouple::SDK
