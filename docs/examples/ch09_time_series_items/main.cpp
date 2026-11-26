// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 9: time-series exchange items. A TimeSeriesOutput records
 *         a step at a time, a TimeSeriesInput collects the latest step, a
 *         TimeIdBasedComponentDataItem keeps a station record, and a
 *         TimeSliceOutput presents one time level of it as a 1-D output.
 */

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"
#include "hydrocouplesdk/data/timesliceoutput.h"
#include "hydrocouplesdk/temporal/timedata.h"
#include "hydrocouplesdk/temporal/timeidbasedcomponentdataitem.h"
#include "hydrocouplesdk/temporal/timeseriesexchangeitems.h"

#include <hydrocouplehelpers.h>

#include <array>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Temporal;

namespace {

int failures = 0;

void check(bool ok, const std::string &what)
{
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok)
        ++failures;
}

} // namespace

int main()
{
    const double t0 = TimeData::gregorianToJulianDay(2026, 5, 1);
    const double hour = 1.0 / 24.0;
    std::unique_ptr<Quantity> flow(Quantity::flowInCMS());
    std::unique_ptr<IUnit> flowUnit(flow->unit());

    //! [output]
    // Two outfalls, recorded hourly. The first dimension is the time axis;
    // the id "time" gives it role Time.
    Dimension time("time", "Time"), outfalls("outfalls", "Outfalls");
    TimeSeriesOutputDouble discharge("discharge", {&time, &outfalls}, flow.get(), nullptr, 2);
    discharge.setTimeKind(TimeKind::IntervalMean, hour); // hourly means ending at each time
    for (int k = 0; k < 3; ++k)
        discharge.addTimeStep(std::make_shared<TimeData>("t", t0 + k * hour),
                              {1.0 + k, 10.0 + k});
    std::printf("discharge: shape {%lld, %lld}, %lld times, span %.4f days, "
                "interpolation None %s, extrapolation Refuse %s\n",
                static_cast<long long>(discharge.shape()[0]),
                static_cast<long long>(discharge.shape()[1]),
                static_cast<long long>(discharge.timeCount()), discharge.timeSpan()->duration(),
                discharge.timeInterpolation() == TimeInterpolation::None ? "yes" : "no",
                discharge.timeExtrapolation() == TimeExtrapolation::Refuse ? "yes" : "no");

    // "Time step 2, all outfalls" is one contiguous row.
    std::array<double, 2> row{};
    const std::array<int64_t, 2> start{2, 0}, count{1, 2};
    const std::array<int64_t, 1> rowShape{2};
    std::string message;
    const bool read = discharge.getValuesInto(
        Helpers::makeContiguous(row.data(), DataKind::Float64, rowShape), start, count, &message);
    std::printf("row at 02:00: %g %g\n", row[0], row[1]);
    //! [output]
    check(discharge.shape() == std::vector<int64_t>{3, 2} && read && row[0] == 3.0 &&
              row[1] == 12.0,
          "time-major storage, one row per step");
    check(time.role() == IDimension::DimensionRole::Time, "the time dimension has role Time");

    //! [input]
    TimeSeriesInputDouble received("discharge", {&time, &outfalls}, flow.get(), nullptr, 2);
    discharge.addConsumer(&received);
    received.retrieveValuesFromProvider(); // appends the provider's latest step
    discharge.addTimeStep(std::make_shared<TimeData>("t", t0 + 3 * hour), {4.0, 13.0});
    received.retrieveValuesFromProvider();
    std::printf("input recorded %lld steps: %.4f -> %g %g, %.4f -> %g %g\n",
                static_cast<long long>(received.timeCount()), received.times()[0] - t0,
                received.getValueAt(0, 0), received.getValueAt(0, 1), received.times()[1] - t0,
                received.getValueAt(1, 0), received.getValueAt(1, 1));
    //! [input]
    check(received.timeCount() == 2 && received.getValueAt(1, 1) == 13.0,
          "the input appends the latest step on each retrieve");
    discharge.removeConsumer(&received);

    //! [time_id]
    TimeIdBasedComponentDataItemDouble gauges("stage", {"G1", "G2", "G3"}, nullptr, nullptr);
    gauges.setTimeKind(TimeKind::Instantaneous);
    gauges.addTimeStep(std::make_shared<TimeData>("t", t0), {1.2, 0.8, 2.1});
    gauges.addTimeStep(std::make_shared<TimeData>("t", t0 + 0.25), {1.5, 0.9, 2.4});
    gauges.addTimeStep(std::make_shared<TimeData>("t", t0 + 0.5), {1.1, 0.7, 2.0});
    std::printf("gauges: shape {%lld, %lld}; dimension 0 '%s' (Time %s), dimension 1 '%s' "
                "(Entity %s)\n",
                static_cast<long long>(gauges.shape()[0]),
                static_cast<long long>(gauges.shape()[1]), gauges.timeDimension()->id().c_str(),
                gauges.timeDimension()->role() == IDimension::DimensionRole::Time ? "yes" : "no",
                gauges.identifierDimension()->id().c_str(),
                gauges.identifierDimension()->role() == IDimension::DimensionRole::Entity
                    ? "yes"
                    : "no");
    //! [time_id]
    check(gauges.shape() == std::vector<int64_t>{3, 3}, "time by identifier");

    //! [slice]
    // One time level of the record as a 1-D output. setTime() seeks the
    // nearest recorded time.
    TimeSliceOutput now("stage_now", &gauges);
    const bool sought = now.setTime(t0 + 0.3); // nearest is t0 + 0.25
    const int64_t soughtIndex = now.timeIndex();
    std::array<double, 3> level{};
    const std::array<int64_t, 1> zero{0}, three{3};
    const bool readLevel = now.getValuesInto(
        Helpers::makeContiguous(level.data(), DataKind::Float64, three), zero, three, &message);
    std::printf("slice: shape {%lld}, time index %lld of %lld, values %g %g %g\n",
                static_cast<long long>(now.shape()[0]), static_cast<long long>(soughtIndex),
                static_cast<long long>(now.timeCount()), level[0], level[1], level[2]);
    now.setTimeIndex(99); // clamped to the last level
    const bool advanced = now.advance();
    std::printf("setTimeIndex(99) -> %lld; advance() at the end: %s\n",
                static_cast<long long>(now.timeIndex()), advanced ? "true" : "false");
    std::printf("slice value definition: '%s'\n", now.valueDefinition()->caption().c_str());
    //! [slice]
    check(sought && readLevel && soughtIndex == 1 && level[1] == 0.9,
          "the slice serves the nearest level");
    check(now.timeIndex() == 2 && !advanced, "setTimeIndex() clamps; advance() stops at the end");

    std::printf("%s\n", failures ? "ch09_time_series_items FAILED" : "ch09_time_series_items passed");
    return failures ? 1 : 0;
}
