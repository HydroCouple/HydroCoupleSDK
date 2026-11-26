// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin
#include "hydrocouplesdk/spatial/layeredmeshexchangeitems.h"

namespace HydroCouple::SDK::Spatial {
//! Emits LayeredMeshOutput<double>'s vtable and RTTI once, from the library (see the header's extern declaration).
template class HYDROCOUPLESDK_EXPORT LayeredMeshOutput<double>;
//! Emits LayeredMeshInput<double>'s vtable and RTTI once, from the library.
template class HYDROCOUPLESDK_EXPORT LayeredMeshInput<double>;
}
