// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/temporal/timeseriesexchangeitems.h"

#include "hydrocoupletemporal.h"

namespace HydroCouple::SDK::Temporal {

// Interface types from the standard's Temporal namespace.
using namespace HydroCouple::Temporal;

// Explicit instantiations — keep the common typed exchange items in the library.
//! Explicit instantiation of TimeSeriesInput for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT TimeSeriesInput<int>;
//! Explicit instantiation of TimeSeriesInput for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT TimeSeriesInput<double>;
//! Explicit instantiation of TimeSeriesInput for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT TimeSeriesInput<std::string>;

//! Explicit instantiation of TimeSeriesOutput for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<int>;
//! Explicit instantiation of TimeSeriesOutput for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<double>;
//! Explicit instantiation of TimeSeriesOutput for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<std::string>;

} // namespace HydroCouple::SDK::Temporal
