// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for the temporal module: TimeData Julian-day conversions,
// TimeSeries interpolation and CSV IO, the time-series storage mixin,
// exchange items, and TimeSeriesArgumentDouble.

#include <gtest/gtest.h>

#include "hydrocouplesdk/temporal/timedata.h"
#include "hydrocouplesdk/temporal/timeseries.h"
#include "hydrocouplesdk/temporal/timeseriesexchangeitems.h"
#include "hydrocouplesdk/temporal/timeseriesargument.h"
#include "hydrocouplesdk/core/dimension.h"
#include "hydrocouplesdk/core/valuedefinition.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>

using namespace HydroCouple;
using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Temporal;

#ifndef TEST_ARTIFACTS_DIR
#define TEST_ARTIFACTS_DIR "."
#endif

namespace {

std::filesystem::path artifactPath(const std::string &name)
{
    const std::filesystem::path dir{TEST_ARTIFACTS_DIR};
    std::filesystem::create_directories(dir);
    return dir / name;
}

} // namespace

// ── TimeData ──────────────────────────────────────────────────────────────

TEST(TimeData, GregorianRoundTrip)
{
    const double jd = TimeData::gregorianToJulianDay(2026, 8, 23, 12, 30, 15.0);
    int y, mo, d, h, mi;
    double s;
    TimeData::julianDayToGregorian(jd, y, mo, d, h, mi, s);
    EXPECT_EQ(y, 2026);
    EXPECT_EQ(mo, 8);
    EXPECT_EQ(d, 23);
    EXPECT_EQ(h, 12);
    EXPECT_EQ(mi, 30);
    EXPECT_NEAR(s, 15.0, 1e-3);
}

TEST(TimeData, DerivedCalendars)
{
    TimeData t("t", 2451545.0); // J2000 epoch
    EXPECT_DOUBLE_EQ(t.modifiedJulianDay(), 2451545.0 - 2400000.5);
    EXPECT_DOUBLE_EQ(t.serialDate(), 2451545.0 - 1721425.5);
}

TEST(TimeSpanTest, DurationAndEndpoints)
{
    TimeSpan span("s", 100.0, 2.5);
    EXPECT_DOUBLE_EQ(span.startTime(), 100.0);
    EXPECT_DOUBLE_EQ(span.endTime(), 102.5);
    EXPECT_DOUBLE_EQ(span.duration(), 2.5);
}

// ── TimeSeries ────────────────────────────────────────────────────────────

TEST(TimeSeriesTest, LinearInterpolation)
{
    TimeSeries ts("ts", {0.0, 1.0, 2.0}, {10.0, 20.0, 40.0});
    EXPECT_DOUBLE_EQ(ts.getValue(0.5), 15.0);
    EXPECT_DOUBLE_EQ(ts.getValue(1.5), 30.0);
    EXPECT_DOUBLE_EQ(ts.getValue(2.0), 40.0);
    EXPECT_TRUE(std::isnan(ts.getValue(5.0)));
}

TEST(TimeSeriesTest, CsvRoundTrip)
{
    const auto path = artifactPath("timeseries_roundtrip.csv");
    TimeSeries ts("ts", {0.0, 1.0}, {3.5, 4.5});
    ASSERT_TRUE(ts.saveToCSV(path.string()));

    TimeSeries loaded("loaded");
    ASSERT_TRUE(loaded.loadFromCSV(path.string(), true));
    ASSERT_EQ(loaded.count(), 2);
    EXPECT_DOUBLE_EQ(loaded.values()[1], 4.5);
}

// ── Time-series exchange items ────────────────────────────────────────────

class TimeSeriesItemTest : public ::testing::Test
{
protected:
    Dimension m_timeDim{"time", "Time"};
    std::unique_ptr<Quantity> m_quantity{Quantity::unitLess("TestQuantity")};
};

TEST_F(TimeSeriesItemTest, OutputShapeTracksTimeSteps)
{
    TimeSeriesOutputDouble out("out", {&m_timeDim}, m_quantity.get(), nullptr);
    EXPECT_EQ(out.shape(), (std::vector<int64_t>{0, 1}));
    out.addTimeStep(std::make_shared<TimeData>("t0", 100.0), {1.5});
    out.addTimeStep(std::make_shared<TimeData>("t1", 101.0), {2.5});
    EXPECT_EQ(out.shape(), (std::vector<int64_t>{2, 1}));
    EXPECT_EQ(out.timeCount(), 2);
    EXPECT_DOUBLE_EQ(out.times()[1], 101.0);
    EXPECT_DOUBLE_EQ(out.timeSpan()->duration(), 1.0);
}

TEST_F(TimeSeriesItemTest, HyperslabReadAcrossTime)
{
    TimeSeriesOutputDouble out("out", {&m_timeDim}, m_quantity.get(), nullptr, 2);
    out.addTimeStep(std::make_shared<TimeData>("t0", 100.0), {1.0, 2.0});
    out.addTimeStep(std::make_shared<TimeData>("t1", 101.0), {3.0, 4.0});

    std::array<double, 4> buf{};
    const std::array<int64_t, 2> shape{2, 2}, zero{0, 0};
    const auto d = detail::hostDescriptor(buf.data(), DataKind::Float64, shape);
    std::string msg;
    ASSERT_TRUE(out.getValuesInto(d, zero, shape, &msg)) << msg;
    EXPECT_EQ(buf, (std::array<double, 4>{1, 2, 3, 4}));
}

TEST_F(TimeSeriesItemTest, InputPullsLatestTimeStepFromProvider)
{
    TimeSeriesOutputDouble out("out", {&m_timeDim}, m_quantity.get(), nullptr);
    out.addTimeStep(std::make_shared<TimeData>("t0", 100.0), {1.5});
    out.addTimeStep(std::make_shared<TimeData>("t1", 101.0), {2.5});

    TimeSeriesInputDouble in("in", {&m_timeDim}, m_quantity.get(), nullptr);
    std::string msg;
    ASSERT_TRUE(in.canConsume(&out, msg)) << msg;
    ASSERT_TRUE(in.setProvider(&out));

    in.retrieveValuesFromProvider();
    ASSERT_EQ(in.timeCount(), 1);
    EXPECT_DOUBLE_EQ(in.times()[0], 101.0);
    EXPECT_DOUBLE_EQ(in.getValueAt(0, 0), 2.5);
}

// ── TimeSeriesArgumentDouble ──────────────────────────────────────────────

class TimeSeriesArgumentTest : public ::testing::Test
{
protected:
    Dimension m_timeDim{"time", "Time"};
};

TEST_F(TimeSeriesArgumentTest, InitializeFromJsonAndInterpolate)
{
    TimeSeriesArgumentDouble arg("arg", "Series", nullptr, &m_timeDim);
    std::string msg;
    ASSERT_TRUE(arg.initialize(R"({"times": [0, 1], "values": [10, 20]})",
                               HydroCouple::IArgument::ArgumentInputType::String,
                               msg)) << msg;
    EXPECT_EQ(arg.shape(), (std::vector<int64_t>{2}));
    EXPECT_DOUBLE_EQ(arg.getInterpolatedValue(0.5), 15.0);
}

TEST_F(TimeSeriesArgumentTest, DataPlaneIsReadOnly)
{
    TimeSeriesArgumentDouble arg("arg", "Series", nullptr, &m_timeDim);
    double v = 0.0;
    const std::array<int64_t, 1> shape{1}, zero{0};
    const auto d = detail::hostDescriptor(&v, DataKind::Float64, shape);
    std::string msg;
    EXPECT_FALSE(arg.setValuesFrom(d, zero, shape, &msg));
    EXPECT_FALSE(msg.empty());
}

TEST_F(TimeSeriesArgumentTest, CsvFileInitialize)
{
    const auto path = artifactPath("timeseries_argument.csv");
    {
        std::ofstream f(path);
        f << "time,value\n0.0,5.0\n1.0,7.0\n";
    }
    TimeSeriesArgumentDouble arg("arg", "Series", nullptr, &m_timeDim);
    std::string msg;
    ASSERT_TRUE(arg.initialize(path.string(),
                               HydroCouple::IArgument::ArgumentInputType::File,
                               msg)) << msg;
    EXPECT_DOUBLE_EQ(arg.getInterpolatedValue(0.5), 6.0);
}

TEST_F(TimeSeriesArgumentTest, HyperslabReadsValues)
{
    TimeSeriesArgumentDouble arg("arg", "Series", nullptr, &m_timeDim);
    std::string msg;
    ASSERT_TRUE(arg.initialize(R"({"times": [0, 1, 2], "values": [1, 2, 3]})",
                               HydroCouple::IArgument::ArgumentInputType::String,
                               msg)) << msg;

    std::array<double, 2> out{};
    const std::array<int64_t, 1> outShape{2}, start{1}, count{2};
    const auto d = detail::hostDescriptor(out.data(), DataKind::Float64, outShape);
    ASSERT_TRUE(arg.getValuesInto(d, start, count, &msg)) << msg;
    EXPECT_EQ(out, (std::array<double, 2>{2, 3}));
}
