// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/temporal/timeseriesexchangeitems.h"

#include "hydrocoupletemporal.h"

namespace HydroCouple::SDK::Temporal {

// Interface types from the standard's Temporal namespace.
using namespace HydroCouple::Temporal;

// Explicit instantiations — keep the common typed exchange items in the library.
template class HYDROCOUPLESDK_EXPORT TimeSeriesInput<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesInput<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesInput<std::string>;

template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesOutput<std::string>;

} // namespace HydroCouple::SDK::Temporal
