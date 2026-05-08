// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   timedata.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Concrete implementations of Temporal interfaces for date/time handling.
 */

#ifndef HYDROCOUPLESDK_TIMEDATA_H
#define HYDROCOUPLESDK_TIMEDATA_H

#include "identity.h"
#include "hydrocoupletemporal.h"

#include <chrono>
#include <cmath>

namespace HydroCouple::Temporal {

/*!
 * \brief Concrete implementation of IDateTime using Julian Day Number.
 *
 * Stores time as a double Julian Day value (days since noon, January 1, 4713 BCE).
 * Provides conversions to Modified Julian Day (JD - 2400000.5) and
 * MATLAB Serial Date (days since 0000-01-01).
 *
 * \threadsafety Immutable after construction; safe to share across threads.
 */
class HYDROCOUPLESDK_EXPORT TimeData : public HydroCouple::Identity,
                                       public virtual IDateTime
{
public:
    /*!
     * \brief Construct from a Julian Day value.
     * \param id       Identifier (typically "Time", "CurrentTime", etc.)
     * \param jd       Julian Day value (default: current time as JD)
     */
    TimeData(std::string_view id, double jd = julianDayNow());

    /*!
     * \brief Construct from year, month, day, hour, minute, second (Gregorian calendar).
     * \param id       Identifier
     * \param year     Gregorian year (e.g., 2025)
     * \param month    Month (1-12)
     * \param day      Day (1-31)
     * \param hour     Hour (0-23, default 0)
     * \param minute   Minute (0-59, default 0)
     * \param second   Second (0-59.9999, default 0)
     */
    TimeData(std::string_view id, int year, int month, int day,
             int hour = 0, int minute = 0, double second = 0.0);

    ~TimeData() override = default;

    // IDateTime
    [[nodiscard]] double julianDay() const override { return m_julianDay; }
    [[nodiscard]] double modifiedJulianDay() const override { return m_julianDay - 2400000.5; }
    [[nodiscard]] double serialDate() const override { return m_julianDay - 1721425.5; }

    // Setters
    void setJulianDay(double jd);
    void setModifiedJulianDay(double mjd) { setJulianDay(mjd + 2400000.5); }
    void setSerialDate(double sd) { setJulianDay(sd + 1721425.5); }

    // Calendar conversion methods (static, no object needed)
    /*!
     * \brief Convert Gregorian date/time to Julian Day.
     */
    static double gregorianToJulianDay(int year, int month, int day,
                                       int hour = 0, int minute = 0, double second = 0.0);

    /*!
     * \brief Convert Julian Day to Gregorian calendar.
     * \param jd       Julian Day value
     * \param[out] y, m, d, h, min, s   Gregorian components
     */
    static void julianDayToGregorian(double jd,
                                     int &year, int &month, int &day,
                                     int &hour, int &minute, double &second);

    /*!
     * \brief Get current time as Julian Day (UTC).
     */
    static double julianDayNow();

private:
    double m_julianDay;
};

/*!
 * \brief Concrete implementation of ITimeSpan for time intervals.
 *
 * Represents a time duration as a double (in days).
 * Inherits DateTime interface from a reference time plus duration.
 *
 * \threadsafety Immutable after construction; safe to share across threads.
 */
class HYDROCOUPLESDK_EXPORT TimeSpan : public TimeData,
                                       public virtual ITimeSpan
{
public:
    /*!
     * \brief Construct a time span from a reference time and duration.
     * \param id          Identifier
     * \param startTime   Start time (as Julian Day)
     * \param durationDays Duration in days (can be negative)
     */
    TimeSpan(std::string_view id, double startTime, double durationDays);

    /*!
     * \brief Construct from two time points.
     * \param id    Identifier
     * \param start Start time (as Julian Day)
     * \param end   End time (as Julian Day)
     */
    static std::shared_ptr<TimeSpan> fromTimeRange(std::string_view id,
                                                     double start, double end);

    ~TimeSpan() override = default;

    // ITimeSpan
    [[nodiscard]] double duration() const override { return m_duration; }

    // Accessors
    [[nodiscard]] double startTime() const { return julianDay(); }
    [[nodiscard]] double endTime() const { return julianDay() + m_duration; }

    // Setters
    void setDuration(double days) { m_duration = days; emitPropertyChanged("Duration"); }

private:
    double m_duration;
};

} // namespace HydroCouple::Temporal

#endif // HYDROCOUPLESDK_TIMEDATA_H
