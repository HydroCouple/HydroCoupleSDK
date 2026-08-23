// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/data/idbasedexchangeitems.h"

namespace HydroCouple::SDK {

template class HYDROCOUPLESDK_EXPORT IdBasedInput<int>;
template class HYDROCOUPLESDK_EXPORT IdBasedInput<double>;
template class HYDROCOUPLESDK_EXPORT IdBasedInput<std::string>;
template class HYDROCOUPLESDK_EXPORT IdBasedOutput<int>;
template class HYDROCOUPLESDK_EXPORT IdBasedOutput<double>;
template class HYDROCOUPLESDK_EXPORT IdBasedOutput<std::string>;

} // namespace HydroCouple::SDK
