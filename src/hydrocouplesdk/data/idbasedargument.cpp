// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/idbasedargument.h"

namespace HydroCouple::SDK {

// Explicit instantiations — keep the common typed arguments in the library.
//! Explicit instantiation of IdBasedArgument for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT IdBasedArgument<int>;
//! Explicit instantiation of IdBasedArgument for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT IdBasedArgument<double>;
//! Explicit instantiation of IdBasedArgument for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT IdBasedArgument<std::string>;

} // namespace HydroCouple::SDK
