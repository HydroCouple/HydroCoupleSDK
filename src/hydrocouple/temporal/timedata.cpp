// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

#include "stdafx.h"
#include "hydrocouple/temporal/timedata.h"

#include <ctime>
#include <iomanip>
#include <sstream>

namespace HydroCouple::Temporal {

TimeData::TimeData(std::string_view id, double jd)
    : Identity(id), m_julianDay(jd) {}

TimeData::TimeData(std::string_view id, int year, int month, int day,
                   int hour, int minute, double second)
    : Identity(id),
      m_julianDay(gregorianToJulianDay(year, month, day, hour, minute, second)) {}

void TimeData::setJulianDay(double jd) {
    m_julianDay = jd;
    emitPropertyChanged("JulianDay");
}

double TimeData::gregorianToJulianDay(int year, int month, int day,
                                      int hour, int minute, double second) {
    // Fliegel & Van Flandern algorithm
    int a = (14 - month) / 12;
    int y = year + 4800 - a;
    int m = month + 12 * a - 3;

    long jdn = day + (153 * m + 2) / 5 + 365 * y + y / 4 - y / 100 + y / 400 - 32045;
    double fraction = (hour + minute / 60.0 + second / 3600.0) / 24.0 - 0.5;

    return static_cast<double>(jdn) + fraction;
}

void TimeData::julianDayToGregorian(double jd,
                                    int &year, int &month, int &day,
                                    int &hour, int &minute, double &second) {
    long jdn = static_cast<long>(std::floor(jd + 0.5));
    double fraction = jd + 0.5 - jdn;

    long a = jdn + 32044;
    long b = (4 * a + 3) / 146097;
    long c = a - (146097 * b) / 4;
    a = (4 * c + 3) / 1461;
    b = c - (1461 * a) / 4;
    long m = (5 * b + 2) / 153;

    day = static_cast<int>(b - (153 * m + 2) / 5 + 1);
    month = static_cast<int>(m + 3 - 12 * (m / 10));
    year = static_cast<int>(100 * b - 4800 + m / 10);

    double dayFraction = fraction * 24.0;
    hour = static_cast<int>(std::floor(dayFraction));
    double minFraction = (dayFraction - hour) * 60.0;
    minute = static_cast<int>(std::floor(minFraction));
    second = (minFraction - minute) * 60.0;
}

double TimeData::julianDayNow() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm *gmt = std::gmtime(&time_t_now);

    double millis = (now.time_since_epoch().count() % 1000000000) / 1e9;

    return gregorianToJulianDay(gmt->tm_year + 1900, gmt->tm_mon + 1, gmt->tm_mday,
                               gmt->tm_hour, gmt->tm_min, gmt->tm_sec + millis);
}

// ── TimeSpan ──────────────────────────────────────────────────────────────

TimeSpan::TimeSpan(std::string_view id, double startTime, double durationDays)
    : TimeData(id, startTime), m_duration(durationDays) {}

std::shared_ptr<TimeSpan> TimeSpan::fromTimeRange(std::string_view id,
                                                   double start, double end) {
    return std::make_shared<TimeSpan>(id, start, end - start);
}

} // namespace HydroCouple::Temporal
