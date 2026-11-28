// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/abstractadaptedoutputfactorycomponentinfo.h"

namespace HydroCouple::SDK {

// The class is abstract — createComponentInstance() is pure virtual — so this
// constructor only ever runs as a subclass's base initializer. It still needs
// a definition: the class is exported, and __declspec(dllexport) requires one
// for every declared member, where GCC and Clang emit only what is used.
AbstractAdaptedOutputFactoryComponentInfo::AbstractAdaptedOutputFactoryComponentInfo(
    std::string_view id)
    : ComponentInfo(id)
{
}

} // namespace HydroCouple::SDK
