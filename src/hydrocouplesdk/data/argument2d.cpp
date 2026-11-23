// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/argument2d.h"

namespace HydroCouple::SDK {

// The storage base of every exported 2-D item, declared in componentdataitem.h.
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem2D<std::string>;

// Explicit instantiations — keep the common typed arguments in the library.
template class HYDROCOUPLESDK_EXPORT Argument2D<int>;
template class HYDROCOUPLESDK_EXPORT Argument2D<double>;
template class HYDROCOUPLESDK_EXPORT Argument2D<std::string>;

} // namespace HydroCouple::SDK
