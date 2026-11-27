// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   iolibrarylock.cpp
 * \brief  The one instance of the IO library lock, owned by the SDK library.
 */

#include "stdafx.h"
#include "hydrocouplesdk/io/iolibrarylock.h"

namespace HydroCouple::SDK::IO {

std::recursive_mutex &ioLibraryMutex()
{
    // Constructed on first use, before the first library call takes it, so
    // it outlives the libraries' own exit-time teardown.
    static std::recursive_mutex mutex;
    return mutex;
}

} // namespace HydroCouple::SDK::IO
