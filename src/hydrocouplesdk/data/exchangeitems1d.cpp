// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"

namespace HydroCouple::SDK {

// Explicit instantiations — keep the common typed exchange items in the library.
//! Explicit instantiation of Input1D for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Input1D<int>;
//! Explicit instantiation of Input1D for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Input1D<double>;
//! Explicit instantiation of Input1D for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Input1D<std::string>;

//! Explicit instantiation of Output1D for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Output1D<int>;
//! Explicit instantiation of Output1D for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Output1D<double>;
//! Explicit instantiation of Output1D for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Output1D<std::string>;

} // namespace HydroCouple::SDK
