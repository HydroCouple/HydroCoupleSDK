// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/exchangeitems2d.h"

namespace HydroCouple::SDK {

// Explicit instantiations — keep the common typed exchange items in the library.
template class HYDROCOUPLESDK_EXPORT Input2D<int>;
template class HYDROCOUPLESDK_EXPORT Input2D<double>;
template class HYDROCOUPLESDK_EXPORT Input2D<std::string>;

template class HYDROCOUPLESDK_EXPORT Output2D<int>;
template class HYDROCOUPLESDK_EXPORT Output2D<double>;
template class HYDROCOUPLESDK_EXPORT Output2D<std::string>;

} // namespace HydroCouple::SDK
