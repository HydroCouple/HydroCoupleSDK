// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/abstractargument.h"

namespace HydroCouple {

AbstractArgument::AbstractArgument(
    std::string_view id,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractComponentDataItem(id, dimensions, valueDefinition, modelComponent)
{
}

AbstractArgument::AbstractArgument(
    std::string_view id,
    std::string_view caption,
    const std::vector<Dimension *> &dimensions,
    ValueDefinition *valueDefinition,
    HydroCouple::IModelComponent *modelComponent)
    : AbstractComponentDataItem(id, caption, dimensions, valueDefinition, modelComponent)
{
}

bool AbstractArgument::isOptional() const
{
    return m_isOptional;
}

bool AbstractArgument::isReadOnly() const
{
    return m_isReadOnly;
}

void AbstractArgument::saveData()
{
    // Default: no external file to write.
}

std::list<std::string> AbstractArgument::fileFilters() const
{
    return m_fileFilters;
}

std::list<std::reference_wrapper<const std::type_info>>
AbstractArgument::validComponentDataItemTypes() const
{
    return m_validTypes;
}

bool AbstractArgument::isValidArgType(
    HydroCouple::IArgument::ArgumentInputType argType) const
{
    switch (argType) {
    case HydroCouple::IArgument::ArgumentInputType::String:
    case HydroCouple::IArgument::ArgumentInputType::JSON:
        return true;
    case HydroCouple::IArgument::ArgumentInputType::File:
    case HydroCouple::IArgument::ArgumentInputType::XML:
    case HydroCouple::IArgument::ArgumentInputType::URL:
        return !m_fileFilters.empty();
    case HydroCouple::IArgument::ArgumentInputType::MEMORY_OBJECT:
        return !m_validTypes.empty();
    default:
        return false;
    }
}

HydroCouple::IArgument::ArgumentInputType
AbstractArgument::currentArgumentInputType() const
{
    return m_currentArgType;
}

void AbstractArgument::setIsOptional(bool isOptional)
{
    m_isOptional = isOptional;
    emitPropertyChanged("IsOptional");
}

void AbstractArgument::setIsReadOnly(bool isReadOnly)
{
    m_isReadOnly = isReadOnly;
    emitPropertyChanged("IsReadOnly");
}

void AbstractArgument::addFileFilter(const std::string &filter)
{
    m_fileFilters.push_back(filter);
    emitPropertyChanged("FileFilters");
}

void AbstractArgument::setCurrentArgumentInputType(
    HydroCouple::IArgument::ArgumentInputType argType)
{
    m_currentArgType = argType;
}

} // namespace HydroCouple
