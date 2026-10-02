// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Every class template the SDK ships, instantiated once.
//
// A class template is only checked against the interfaces it implements when
// something instantiates it. LayeredMeshInput/LayeredMeshOutput went through
// an interface revision (entityDimension() replacing the per-location
// accessors, vectorBasis() added) with nothing in the SDK instantiating them:
// they still compiled, and were abstract, and the first component author to
// use one would have found out. Completing each type here makes a stale
// override or an unimplemented pure virtual a build failure of the SDK's own
// tests instead.

#include <gtest/gtest.h>

#include "hydrocouplesdk/data/argument1d.h"
#include "hydrocouplesdk/data/argument2d.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/data/exchangeitems2d.h"
#include "hydrocouplesdk/data/idbasedargument.h"
#include "hydrocouplesdk/data/idbasedexchangeitems.h"
#include "hydrocouplesdk/spatial/layeredmeshcomponentdataitem.h"
#include "hydrocouplesdk/spatial/layeredmeshexchangeitems.h"
#include "hydrocouplesdk/spatiotemporal/timegeometrycomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timenetworkcomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timepolyhedralsurfacecomponentdataitem.h"
#include "hydrocouplesdk/spatiotemporal/timeregulargridcomponentdataitems.h"
#include "hydrocouplesdk/temporal/timeidbasedcomponentdataitem.h"
#include "hydrocouplesdk/temporal/timeseriesexchangeitems.h"

#include <type_traits>

namespace {

namespace SDK = HydroCouple::SDK;

// std::is_abstract requires a complete type, so each assertion instantiates
// the class: its member declarations are checked against the bases (an
// `override` of nothing is an error) and its pure virtuals counted.
#define HC_ASSERT_CONCRETE(...)                                                \
    static_assert(!std::is_abstract_v<__VA_ARGS__>, #__VA_ARGS__ " is abstract")

HC_ASSERT_CONCRETE(SDK::Argument1D<double>);
HC_ASSERT_CONCRETE(SDK::Argument2D<double>);
HC_ASSERT_CONCRETE(SDK::IdBasedArgument<double>);
HC_ASSERT_CONCRETE(SDK::Input1D<double>);
HC_ASSERT_CONCRETE(SDK::Output1D<double>);
HC_ASSERT_CONCRETE(SDK::Input2D<double>);
HC_ASSERT_CONCRETE(SDK::Output2D<double>);
HC_ASSERT_CONCRETE(SDK::IdBasedInput<double>);
HC_ASSERT_CONCRETE(SDK::IdBasedOutput<double>);
HC_ASSERT_CONCRETE(SDK::Spatial::LayeredMeshComponentDataItem<double>);
HC_ASSERT_CONCRETE(SDK::Spatial::LayeredMeshInput<double>);
HC_ASSERT_CONCRETE(SDK::Spatial::LayeredMeshOutput<double>);
HC_ASSERT_CONCRETE(SDK::SpatioTemporal::TimeGeometryComponentDataItem<double>);
HC_ASSERT_CONCRETE(SDK::SpatioTemporal::TimeNetworkComponentDataItem<double>);
HC_ASSERT_CONCRETE(SDK::SpatioTemporal::TimeSeriesPolyhedralSurfaceComponentDataItem<double>);
HC_ASSERT_CONCRETE(SDK::SpatioTemporal::TimeSeriesTINComponentDataItem<double>);
HC_ASSERT_CONCRETE(SDK::SpatioTemporal::TimeRegularGrid2DComponentDataItem<double>);
HC_ASSERT_CONCRETE(SDK::SpatioTemporal::TimeRegularGrid3DComponentDataItem<double>);
HC_ASSERT_CONCRETE(SDK::Temporal::TimeIdBasedComponentDataItem<double>);
HC_ASSERT_CONCRETE(SDK::Temporal::TimeSeriesInput<double>);
HC_ASSERT_CONCRETE(SDK::Temporal::TimeSeriesOutput<double>);

#undef HC_ASSERT_CONCRETE

} // namespace

// The assertions above are the test; this keeps the file visible in a run.
TEST(TemplateCompletenessTest, EveryShippedTemplateIsConcrete)
{
    SUCCEED();
}
