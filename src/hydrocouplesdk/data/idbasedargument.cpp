// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/idbasedargument.h"

namespace HydroCouple::SDK {

// Explicit instantiations — keep the common typed arguments in the library.
template class HYDROCOUPLESDK_EXPORT IdBasedArgument<int>;
template class HYDROCOUPLESDK_EXPORT IdBasedArgument<double>;
template class HYDROCOUPLESDK_EXPORT IdBasedArgument<std::string>;

} // namespace HydroCouple::SDK
