// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/argument1d.h"

namespace HydroCouple::SDK {

// The storage base of every exported 1-D item, declared in componentdataitem.h.
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<int>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<double>;
template class HYDROCOUPLESDK_EXPORT ComponentDataItem1D<std::string>;

// Explicit instantiations — keep the common typed arguments in the library.
template class HYDROCOUPLESDK_EXPORT Argument1D<int>;
template class HYDROCOUPLESDK_EXPORT Argument1D<double>;
template class HYDROCOUPLESDK_EXPORT Argument1D<std::string>;

} // namespace HydroCouple::SDK
