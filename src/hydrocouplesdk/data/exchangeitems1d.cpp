// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"

namespace HydroCouple::SDK {

// Explicit instantiations — keep the common typed exchange items in the library.
template class HYDROCOUPLESDK_EXPORT Input1D<int>;
template class HYDROCOUPLESDK_EXPORT Input1D<double>;
template class HYDROCOUPLESDK_EXPORT Input1D<std::string>;

template class HYDROCOUPLESDK_EXPORT Output1D<int>;
template class HYDROCOUPLESDK_EXPORT Output1D<double>;
template class HYDROCOUPLESDK_EXPORT Output1D<std::string>;

} // namespace HydroCouple::SDK
