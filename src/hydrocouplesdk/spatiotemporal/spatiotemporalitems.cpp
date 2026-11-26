// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/spatiotemporal/timegeometrycomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timenetworkcomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timepolyhedralsurfacecomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timeregulargridcomponentdataitems.h"
#include "hydrocouplesdk/temporal/timeidbasedcomponentdataitem.h"

// The explicit instantiations matching the headers' extern template
// declarations: each emits the class's vtable and RTTI once, from this
// library, so a plugin built with hidden visibility casts across the library
// boundary against one type_info.

namespace HydroCouple::SDK::SpatioTemporal {

//! Exports TimeGeometryComponentDataItem<int> from the library.
template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<int>;
//! Exports TimeGeometryComponentDataItem<double> from the library.
template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<double>;
//! Exports TimeNetworkComponentDataItem<int> from the library.
template class HYDROCOUPLESDK_EXPORT TimeNetworkComponentDataItem<int>;
//! Exports TimeNetworkComponentDataItem<double> from the library.
template class HYDROCOUPLESDK_EXPORT TimeNetworkComponentDataItem<double>;
//! Exports TimeSeriesPolyhedralSurfaceComponentDataItem<int> from the library.
template class HYDROCOUPLESDK_EXPORT
    TimeSeriesPolyhedralSurfaceComponentDataItem<int>;
//! Exports TimeSeriesPolyhedralSurfaceComponentDataItem<double> from the library.
template class HYDROCOUPLESDK_EXPORT
    TimeSeriesPolyhedralSurfaceComponentDataItem<double>;
//! Exports TimeSeriesTINComponentDataItem<int> from the library.
template class HYDROCOUPLESDK_EXPORT TimeSeriesTINComponentDataItem<int>;
//! Exports TimeSeriesTINComponentDataItem<double> from the library.
template class HYDROCOUPLESDK_EXPORT TimeSeriesTINComponentDataItem<double>;
//! Exports TimeRegularGrid2DComponentDataItem<int> from the library.
template class HYDROCOUPLESDK_EXPORT TimeRegularGrid2DComponentDataItem<int>;
//! Exports TimeRegularGrid2DComponentDataItem<double> from the library.
template class HYDROCOUPLESDK_EXPORT
    TimeRegularGrid2DComponentDataItem<double>;
//! Exports TimeRegularGrid3DComponentDataItem<int> from the library.
template class HYDROCOUPLESDK_EXPORT TimeRegularGrid3DComponentDataItem<int>;
//! Exports TimeRegularGrid3DComponentDataItem<double> from the library.
template class HYDROCOUPLESDK_EXPORT
    TimeRegularGrid3DComponentDataItem<double>;

} // namespace HydroCouple::SDK::SpatioTemporal

namespace HydroCouple::SDK::Temporal {

//! Exports TimeIdBasedComponentDataItem<int> (temporal module) from the library.
template class HYDROCOUPLESDK_EXPORT TimeIdBasedComponentDataItem<int>;
//! Exports TimeIdBasedComponentDataItem<double> from the library.
template class HYDROCOUPLESDK_EXPORT TimeIdBasedComponentDataItem<double>;
//! Exports TimeIdBasedComponentDataItem<std::string> from the library.
template class HYDROCOUPLESDK_EXPORT
    TimeIdBasedComponentDataItem<std::string>;

} // namespace HydroCouple::SDK::Temporal
