// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

// Compatibility include. The interface package owns the loading convention
// and checks the component stamp against the current interface ABI.

/*!
 * \file   componentabi.h
 * \brief  Compatibility include for the component loading convention.
 *
 * Includes the interface package's hydrocouplecomponentabi.h and adds
 * nothing. That header defines the two C entry points a component library
 * exports (hydrocouple_component_abi_v1, which returns the ABI stamp, and
 * hydrocouple_component_info_v1, which returns the library-owned
 * IComponentInfo), the HYDROCOUPLE_DECLARE_COMPONENT macro that emits both,
 * the HYDROCOUPLE_COMPONENT_ABI_STAMP string a host compares before calling
 * any C++ symbol, and the legacy unstamped CreateComponentInfo symbol. The
 * stamp names the loading-convention revision, the interface ABI version,
 * the compiler family, the standard library and its ABI settings, the C
 * runtime on Windows, and the pointer width. Component sources that
 * include hydrocouplesdk/componentabi.h keep building unchanged.
 */

#ifndef HYDROCOUPLESDK_COMPONENTABI_H
#define HYDROCOUPLESDK_COMPONENTABI_H

#include "hydrocouplecomponentabi.h"

#endif // HYDROCOUPLESDK_COMPONENTABI_H
