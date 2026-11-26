// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/exchangeitems2d.h"

namespace HydroCouple::SDK {

// Explicit instantiations — keep the common typed exchange items in the library.
//! Explicit instantiation of Input2D for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Input2D<int>;
//! Explicit instantiation of Input2D for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Input2D<double>;
//! Explicit instantiation of Input2D for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Input2D<std::string>;

//! Explicit instantiation of Output2D for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Output2D<int>;
//! Explicit instantiation of Output2D for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Output2D<double>;
//! Explicit instantiation of Output2D for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Output2D<std::string>;

} // namespace HydroCouple::SDK
