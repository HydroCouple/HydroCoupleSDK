// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/argument1d.h"

namespace HydroCouple::SDK {

// The storage base of every exported 1-D item, declared in componentdataitem.h.
//! Explicit instantiation of ComponentDataItem1D for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<int>;
//! Explicit instantiation of ComponentDataItem1D for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<double>;
//! Explicit instantiation of ComponentDataItem1D for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<std::string>;

// Explicit instantiations — keep the common typed arguments in the library.
//! Explicit instantiation of Argument1D for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Argument1D<int>;
//! Explicit instantiation of Argument1D for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Argument1D<double>;
//! Explicit instantiation of Argument1D for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Argument1D<std::string>;

} // namespace HydroCouple::SDK
