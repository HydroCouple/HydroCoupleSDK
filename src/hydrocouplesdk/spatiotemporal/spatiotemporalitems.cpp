// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/spatiotemporal/timegeometrycomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timenetworkcomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timepolyhedralsurfacecomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timeregulargridcomponentdataitems.h"
#include "hydrocouplesdk/temporal/timeidbasedcomponentdataitem.h"

namespace HydroCouple::SDK::SpatioTemporal {

template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeNetworkComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeNetworkComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT
    TimeSeriesPolyhedralSurfaceComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT
    TimeSeriesPolyhedralSurfaceComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesTINComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesTINComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeRegularGrid2DComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT
    TimeRegularGrid2DComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeRegularGrid3DComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT
    TimeRegularGrid3DComponentDataItem<double>;

} // namespace HydroCouple::SDK::SpatioTemporal

namespace HydroCouple::SDK::Temporal {

template class HYDROCOUPLESDK_EXPORT TimeIdBasedComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeIdBasedComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT
    TimeIdBasedComponentDataItem<std::string>;

} // namespace HydroCouple::SDK::Temporal
