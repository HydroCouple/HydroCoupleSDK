// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/distributed/partitioneddataitem.h"

namespace HydroCouple::SDK::Distributed {

// Explicit instantiations — keep the common typed items in the library.
template class HYDROCOUPLESDK_EXPORT PartitionedDataItem<double>;
template class HYDROCOUPLESDK_EXPORT PartitionedDataItem<int32_t>;

} // namespace HydroCouple::SDK::Distributed
