// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/idbasedexchangeitems.h"

namespace HydroCouple::SDK {

//! Explicit instantiation of IdBasedInput for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT IdBasedInput<int>;
//! Explicit instantiation of IdBasedInput for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT IdBasedInput<double>;
//! Explicit instantiation of IdBasedInput for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT IdBasedInput<std::string>;
//! Explicit instantiation of IdBasedOutput for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT IdBasedOutput<int>;
//! Explicit instantiation of IdBasedOutput for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT IdBasedOutput<double>;
//! Explicit instantiation of IdBasedOutput for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT IdBasedOutput<std::string>;

} // namespace HydroCouple::SDK
