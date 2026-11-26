// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 9: Julian days and calendar dates (TimeData, TimeSpan),
 *         TimeSeries interpolation at and between samples, CSV text, and a
 *         TimeSeriesArgumentDouble read from JSON and from a CSV file.
 */

#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/temporal/timedata.h"
#include "hydrocouplesdk/temporal/timeseries.h"
#include "hydrocouplesdk/temporal/timeseriesargument.h"

#include <cmath>
#include <cstdio>
#include <filesystem>
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

bool same(double a, double b)
{
    return (std::isnan(a) && std::isnan(b)) || std::abs(a - b) < 1e-9;
}

} // namespace

int main()
{
    //! [julian]
    // The standard's convention: Julian days, proleptic Gregorian, UTC.
    const double j2000 = TimeData::gregorianToJulianDay(2000, 1, 1, 12, 0, 0.0);
    TimeData epoch("J2000", j2000);
    std::printf("2000-01-01 12:00 UTC: JD %.1f, MJD %.1f, MATLAB datenum %.1f\n",
                epoch.julianDay(), epoch.modifiedJulianDay(), epoch.serialDate());

    TimeData start("start", 2026, 5, 1, 6, 30, 0.0);
    int year, month, day, hour, minute;
    double second;
    TimeData::julianDayToGregorian(start.julianDay(), year, month, day, hour, minute, second);
    std::printf("2026-05-01 06:30 is JD %.8f; back: %04d-%02d-%02d %02d:%02d:%.6f s\n",
                start.julianDay(), year, month, day, hour, minute, second);
    //! [julian]
    check(j2000 == 2451545.0 && epoch.modifiedJulianDay() == 51544.5 &&
              epoch.serialDate() == 730486.5,
          "J2000 in the three calendars");
    check(year == 2026 && month == 5 && day == 1 && hour == 6 && minute == 30 &&
              std::abs(second) < 1e-3,
          "calendar round trip to the millisecond");

    //! [span]
    const auto storm = TimeSpan::fromTimeRange("storm", start.julianDay(),
                                               start.julianDay() + 0.75);
    std::printf("storm: starts JD %.4f, lasts %.2f days (%.0f h), ends JD %.4f\n",
                storm->julianDay(), storm->duration(), storm->duration() * 24.0,
                storm->endJulianDay());
    //! [span]
    check(std::abs(storm->duration() - 0.75) < 1e-9 &&
              std::abs(storm->endJulianDay() - (start.julianDay() + 0.75)) < 1e-9,
          "a span is a start and a duration");

    //! [methods]
    // Samples at t = 0, 1, 2 holding 10, 20, 30, queried at and between them.
    TimeSeries series("series", {0.0, 1.0, 2.0}, {10.0, 20.0, 30.0});
    const std::vector<double> queries{-0.5, 0.0, 0.5, 1.0, 1.5, 2.0, 2.5};
    const TimeSeries::InterpolationMethod methods[] = {
        TimeSeries::InterpolationMethod::Linear, TimeSeries::InterpolationMethod::Nearest,
        TimeSeries::InterpolationMethod::Constant};
    const char *names[] = {"Linear  ", "Nearest ", "Constant"};
    std::vector<std::vector<double>> table;
    std::printf("t        ");
    for (double t : queries)
        std::printf("%6.1f", t);
    std::printf("\n");
    for (int m = 0; m < 3; ++m)
    {
        series.setInterpolationMethod(methods[m]);
        table.push_back(series.getValues(queries));
        std::printf("%s ", names[m]);
        for (double v : table.back())
            std::printf("%6.4g", v);
        std::printf("\n");
    }
    //! [methods]
    const double nan = std::nan("");
    const std::vector<std::vector<double>> expected{
        {nan, 10, 15, 20, 25, 30, nan},  // Linear: NaN outside the samples
        {nan, 10, 10, 20, 20, 30, 30},   // Nearest: earlier on a tie, holds the last
        {nan, 10, 10, 10, 20, 20, 30}};  // Constant: the value before an exact sample
    bool allSame = true;
    for (int m = 0; m < 3; ++m)
        for (size_t i = 0; i < queries.size(); ++i)
            allSame = allSame && same(table[m][i], expected[m][i]);
    check(allSame, "interpolation table as documented");

    //! [csv_text]
    TimeSeries rainfall("rainfall");
    const bool parsed = rainfall.loadFromCSVText(
        "time,intensity_mm_per_h\n"
        "2461161.50,0.0\n"
        "2461161.75,4.2\n"
        "2461162.00,1.1\n");
    std::printf("CSV text: %d samples from JD %.2f to %.2f; at 06:00 %g mm/h\n",
                rainfall.count(), rainfall.minTime(), rainfall.maxTime(),
                rainfall.getValue(2461161.75));
    TimeSeries bad("bad");
    const bool outOfOrder = bad.loadFromCSVText("t,v\n2.0,1\n1.0,2\n");
    std::printf("times out of order: load %s, %d sample kept\n", outOfOrder ? "true" : "false",
                bad.count());
    //! [csv_text]
    check(parsed && rainfall.count() == 3 && rainfall.getValue(2461161.75) == 4.2,
          "CSV text parsed");
    check(!outOfOrder && bad.count() == 1, "a decreasing time stops the load");

    //! [argument_json]
    Dimension time("time", "Time");
    TimeSeriesArgumentDouble boundary("boundary_inflow", "Boundary inflow", nullptr, &time);
    std::string message;
    const bool fromJson = boundary.initialize(
        R"({"times": [2461161.5, 2461161.75, 2461162.0], "values": [2.0, 2.5, 4.0],
            "timeKind": 1})",
        AT::JSON, message);
    auto *asSeries = dynamic_cast<ITimeSeriesComponentDataItem *>(&boundary);
    std::printf("argument: %lld times, span %.2f days, Instantaneous %s, "
                "value at 03:00 %g, after the end %g\n",
                static_cast<long long>(asSeries->timeCount()), asSeries->timeSpan()->duration(),
                asSeries->timeKind() == TimeKind::Instantaneous ? "yes" : "no",
                boundary.getInterpolatedValue(2461161.625),
                boundary.getInterpolatedValue(2461163.0));
    //! [argument_json]
    check(fromJson && asSeries->timeCount() == 3 && asSeries->timeSpan()->duration() == 0.5 &&
              same(boundary.getInterpolatedValue(2461161.625), 2.25) &&
              std::isnan(boundary.getInterpolatedValue(2461163.0)),
          "TimeSeriesArgumentDouble from JSON");

    //! [argument_csv]
    const std::filesystem::path csv =
        std::filesystem::path(DOCS_EXAMPLE_SOURCE_DIR) / "inflow.csv";
    const bool fromCsv = boundary.initialize(csv.string(), AT::File, message);
    std::printf("from inflow.csv: %lld times; value at 12:00 %g; isValidArgType(File) %s\n",
                static_cast<long long>(boundary.timeCount()),
                boundary.getInterpolatedValue(2461162.0),
                boundary.isValidArgType(AT::File) ? "true" : "false");
    std::printf("declared: interpolation Linear %s, extrapolation HoldLast %s\n",
                boundary.timeInterpolation() == TimeInterpolation::Linear ? "yes" : "no",
                boundary.timeExtrapolation() == TimeExtrapolation::HoldLast ? "yes" : "no");
    //! [argument_csv]
    check(fromCsv && boundary.timeCount() == 4 && boundary.getInterpolatedValue(2461162.0) == 4.0,
          "TimeSeriesArgumentDouble from a CSV file");

    //! [argument_errors]
    std::string unequal, decreasing;
    const bool e1 = boundary.initialize(R"({"times": [1, 2], "values": [1]})", AT::JSON, unequal);
    const bool e2 =
        boundary.initialize(R"({"times": [2, 1], "values": [1, 2]})", AT::JSON, decreasing);
    std::printf("unequal arrays: %s\ndecreasing times: %s\n", unequal.c_str(), decreasing.c_str());
    //! [argument_errors]
    check(!e1 && unequal == "times/values must have equal length", "unequal arrays refused");
    check(!e2 && decreasing == "TimeSeries: times must be strictly increasing",
          "decreasing times refused");

    std::printf("%s\n", failures ? "ch09_time_series FAILED" : "ch09_time_series passed");
    return failures ? 1 : 0;
}
