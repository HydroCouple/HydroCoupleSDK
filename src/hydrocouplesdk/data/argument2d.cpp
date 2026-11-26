// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/argument2d.h"

namespace HydroCouple::SDK {

// The storage base of every exported 2-D item, declared in componentdataitem.h.
//! Explicit instantiation of ComponentDataItem2D for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<int>;
//! Explicit instantiation of ComponentDataItem2D for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<double>;
//! Explicit instantiation of ComponentDataItem2D for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<std::string>;

// Explicit instantiations — keep the common typed arguments in the library.
//! Explicit instantiation of Argument2D for `int`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Argument2D<int>;
//! Explicit instantiation of Argument2D for `double`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Argument2D<double>;
//! Explicit instantiation of Argument2D for `std::string`, compiled into and exported from the library.
template class HYDROCOUPLESDK_EXPORT Argument2D<std::string>;

} // namespace HydroCouple::SDK
