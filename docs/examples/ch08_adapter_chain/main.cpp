// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 8: a two-step adapted-output chain built by hand with
 *         SdkAdaptedOutputFactory: temporal interpolation of a flow record
 *         at a query time, then a linear transform from m³/s to ft³/s,
 *         consumed by an ordinary input.
 */

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/exchangeitems1d.h"
#include "hydrocouplesdk/data/sdkadaptedoutputfactory.h"
#include "hydrocouplesdk/temporal/temporalinterpolationadaptedoutput.h"
#include "hydrocouplesdk/temporal/timedata.h"
#include "hydrocouplesdk/temporal/timeseriesexchangeitems.h"

#include <hydrocouplehelpers.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Temporal;
using AT = IArgument::ArgumentInputType;

namespace {

int failures = 0;

void check(bool ok, const std::string &what)
{
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok)
        ++failures;
}

// Julian-day times resolve to about 40 microseconds, so values interpolated
// at a query time carry a relative error near 1e-8.
bool near(double a, double b) { return std::abs(a - b) < 1e-6 * (1.0 + std::abs(b)); }

std::string ids(const std::vector<IIdentity *> &offerings)
{
    std::string text;
    for (IIdentity *offering : offerings)
        text += (text.empty() ? "" : ", ") + offering->id();
    return text.empty() ? "(none)" : text;
}

IIdentity *find(const std::vector<IIdentity *> &offerings, const std::string &id)
{
    for (IIdentity *offering : offerings)
        if (offering->id() == id)
            return offering;
    return nullptr;
}

//! Sets an adapter argument the way the chain builder does: JSON text.
bool setArgument(IAdaptedOutput &adapter, const std::string &id, const std::string &json)
{
    for (IArgument *argument : adapter.arguments())
    {
        std::string message;
        if (argument->id() == id)
            return argument->initialize(json, AT::JSON, message);
    }
    return false;
}

} // namespace

int main()
{
    //! [provider]
    // A flow record: one value (m³/s) per hour, times as Julian days.
    Dimension time("time", "Time");
    std::unique_ptr<Quantity> cms(Quantity::flowInCMS());
    std::unique_ptr<IUnit> cmsUnit(cms->unit());
    TimeSeriesOutputDouble flow("flow", {&time}, cms.get(), nullptr);
    const double t0 = TimeData::gregorianToJulianDay(2026, 5, 1, 0, 0, 0.0);
    const double hour = 1.0 / 24.0;
    flow.addTimeStep(std::make_shared<TimeData>("t", t0), {10.0});
    flow.addTimeStep(std::make_shared<TimeData>("t", t0 + hour), {14.0});
    flow.addTimeStep(std::make_shared<TimeData>("t", t0 + 2 * hour), {12.0});
    //! [provider]

    //! [offerings]
    SdkAdaptedOutputFactory *factory = SdkAdaptedOutputFactory::instance();
    const std::vector<IIdentity *> onSeries = factory->getAvailableAdaptedOutputIds(&flow);
    std::printf("offered on the {3, 1} time series: %s\n", ids(onSeries).c_str());
    //! [offerings]
    check(ids(onSeries) == "temporal_interpolation", "only temporal interpolation on a series");

    //! [interpolate]
    // createAdaptedOutput() registers the adapter with its provider; the
    // unique_ptr owns it, and its destructor deregisters it.
    std::unique_ptr<IAdaptedOutput> interpolated =
        factory->createAdaptedOutput(find(onSeries, "temporal_interpolation"), &flow);
    auto *temporal = dynamic_cast<TemporalInterpolationAdaptedOutput *>(interpolated.get());
    setArgument(*interpolated, "method", R"(["linear"])");
    interpolated->initialize(); // seeds the three recorded samples
    temporal->setQueryTime(t0 + 1.5 * hour);
    std::printf("adapter id %s, %d samples, value at 01:30 = %g m3/s\n",
                interpolated->id().c_str(), temporal->sampleCount(),
                [&] {
                    double v = 0.0;
                    const std::array<int64_t, 1> i{0};
                    (void)Helpers::getValue(*interpolated, v, i);
                    return v;
                }());
    //! [interpolate]
    check(flow.adaptedOutputs().size() == 1 && interpolated->adaptee() == &flow,
          "the factory registered the adapter on its provider");

    //! [transform]
    // The interpolated value is a rank-1 Float64 item, so the element-wise
    // adapters are offered on it.
    const std::vector<IIdentity *> onValue =
        factory->getAvailableAdaptedOutputIds(interpolated.get());
    std::printf("offered on the interpolated value: %s\n", ids(onValue).c_str());
    std::unique_ptr<IAdaptedOutput> converted = factory->createAdaptedOutput(
        find(onValue, "linear_transform"), interpolated.get());
    setArgument(*converted, "multiplier", R"({"values": [35.314666721]})"); // m³/s to ft³/s
    converted->initialize();
    //! [transform]
    check(ids(onValue) == "linear_transform, relaxation", "element-wise adapters on a rank-1 value");

    //! [consume]
    // The consumer is wired to the tail of the chain.
    Dimension single("element", "Element");
    std::unique_ptr<Quantity> cfs(Quantity::flowInCFS());
    std::unique_ptr<IUnit> cfsUnit(cfs->unit());
    Input1DDouble inflow("inflow", &single, 1, cfs.get(), nullptr);
    converted->addConsumer(&inflow);
    inflow.retrieveValuesFromProvider();
    std::printf("consumer reads %.4f ft3/s (13 m3/s)\n", inflow[0]);
    //! [consume]
    check(near(inflow[0], 13.0 * 35.314666721), "consumer reads the converted interpolated value");
    check(inflow.provider() == converted.get() && converted->adaptee() == interpolated.get(),
          "consumer -> linear transform -> temporal interpolation -> flow");

    //! [refresh]
    // The provider records a new hour and pushes: the first adapter appends
    // the sample and refreshes the transform below it.
    flow.addTimeStep(std::make_shared<TimeData>("t", t0 + 3 * hour), {20.0});
    flow.refreshAdaptedOutputs();
    temporal->setQueryTime(t0 + 2.5 * hour);
    // The transform computed its value at the refresh, before the query time
    // moved: it still serves the old one until it refreshes itself.
    inflow.retrieveValuesFromProvider();
    const double stale = inflow[0];
    converted->refresh();
    inflow.retrieveValuesFromProvider();
    std::printf("after the new hour: %d samples; before transform refresh %.4f, after %.4f "
                "ft3/s (16 m3/s)\n",
                temporal->sampleCount(), stale, inflow[0]);
    //! [refresh]
    check(temporal->sampleCount() == 4, "refresh appended the new sample");
    check(near(stale, 13.0 * 35.314666721) && near(inflow[0], 16.0 * 35.314666721),
          "a query-time change reaches the consumer after the transform refreshes");

    //! [outside]
    // Linear interpolation is not defined past the last sample: the value is
    // NaN, and the read still succeeds.
    temporal->setQueryTime(t0 + 5 * hour);
    converted->refresh();
    inflow.retrieveValuesFromProvider();
    std::printf("query past the record: %g\n", inflow[0]);
    //! [outside]
    check(std::isnan(inflow[0]), "linear interpolation past the record is NaN");

    //! [teardown]
    // Disconnect consumers, then destroy the tail first; each adapter
    // deregisters itself from its adaptee.
    converted->removeConsumer(&inflow);
    converted.reset();
    const size_t onInterpolated = interpolated->adaptedOutputs().size();
    interpolated.reset();
    std::printf("after teardown: %zu adapters on the interpolation, %zu on flow\n",
                onInterpolated, flow.adaptedOutputs().size());
    //! [teardown]
    check(onInterpolated == 0 && flow.adaptedOutputs().empty(), "adapters deregister on destruction");

    std::printf("%s\n", failures ? "ch08_adapter_chain FAILED" : "ch08_adapter_chain passed");
    return failures ? 1 : 0;
}
