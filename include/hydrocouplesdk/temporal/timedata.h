// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   timedata.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete implementations of Temporal interfaces for date/time handling.
 */

#ifndef HYDROCOUPLESDK_TIMEDATA_H
#define HYDROCOUPLESDK_TIMEDATA_H

#include "hydrocouplesdk/core/identity.h"
#include "hydrocoupletemporal.h"

#include <chrono>
#include <cmath>

namespace HydroCouple::SDK::Temporal {

// Interface types from the standard's Temporal namespace.
using namespace HydroCouple::Temporal;

/*!
 * \brief Concrete implementation of IDateTime using Julian Day Number.
 *
 * Stores time as a double Julian Day value (days since noon, January 1, 4713 BCE).
 * Provides conversions to Modified Julian Day (JD - 2400000.5) and
 * MATLAB datenum (julianDay() - 1721058.5).
 *
 * \details The convention is the standard's: the astronomical Julian day in
 * the proleptic Gregorian calendar, UTC, with no time zone and no leap
 * seconds. 2000-01-01 12:00 UTC is 2451545.0; 2000-01-01 00:00 is 2451544.5.
 * A double resolves a present-day Julian day to about 40 microseconds, so
 * conversions through the calendar functions are exact only to that
 * resolution (HydroCouple::Temporal::TIME_EQUALITY_TOLERANCE_DAYS is the
 * tolerance the standard uses to compare two instants).
 *
 * TimeData is also an Identity, so it has an id and a caption, and its
 * setters fire propertyChanged("JulianDay").
 *
 * \threadsafety Immutable after construction; safe to share across threads.
 * (Only while no setter is called: setJulianDay(), setModifiedJulianDay()
 * and setSerialDate() change the value and are not synchronized.)
 */
class HYDROCOUPLESDK_EXPORT TimeData : public HydroCouple::SDK::Identity,
                                       public virtual IDateTime
{
public:
    /*!
     * \brief Construct from a Julian Day value.
     * \param id       Identifier (typically "Time", "CurrentTime", etc.)
     * \param jd       Julian Day value (default: current time as JD)
     * \details The value is not range-checked.
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
     * \details Converted with gregorianToJulianDay(); the fields are not
     *          range-checked, so out-of-range values roll over arithmetically
     *          (hour 25 is 01:00 the next day).
     */
    TimeData(std::string_view id, int year, int month, int day,
             int hour = 0, int minute = 0, double second = 0.0);

    //! Destroys the instant.
    ~TimeData() override = default;

    // IDateTime
    //! The instant as a Julian day.
    //! \return Days since 4713 BC January 1, 12:00 UTC (proleptic Gregorian).
    [[nodiscard]] double julianDay() const override { return m_julianDay; }
    //! The instant as a modified Julian day.
    //! \return julianDay() - 2400000.5: days since 1858-11-17 00:00 UTC.
    [[nodiscard]] double modifiedJulianDay() const override { return m_julianDay - 2400000.5; }
    //! The instant as a MATLAB serial date number (datenum).
    //! \return julianDay() - 1721058.5: days since 0000-01-00 00:00.
    [[nodiscard]] double serialDate() const override { return m_julianDay - 1721058.5; }

    // Setters
    /*!
     * \brief Replaces the instant.
     * \details Fires propertyChanged("JulianDay").
     * \param[in] jd New value as a Julian day.
     */
    void setJulianDay(double jd);
    //! Replaces the instant from a modified Julian day.
    //! \param[in] mjd Days since 1858-11-17 00:00 UTC.
    void setModifiedJulianDay(double mjd) { setJulianDay(mjd + 2400000.5); }
    //! Replaces the instant from a MATLAB serial date number.
    //! \param[in] sd MATLAB datenum.
    void setSerialDate(double sd) { setJulianDay(sd + 1721058.5); }

    // Calendar conversion methods (static, no object needed)
    /*!
     * \brief Convert Gregorian date/time to Julian Day.
     * \details Uses the Fliegel and Van Flandern integer algorithm for the
     *          day number, then adds the time of day; valid for the
     *          proleptic Gregorian calendar after 4800 BC. Fields are not
     *          range-checked.
     * \param[in] year   Gregorian year (astronomical numbering: 0 is 1 BC).
     * \param[in] month  Month, 1 to 12.
     * \param[in] day    Day of the month, 1 to 31.
     * \param[in] hour   Hour, 0 to 23.
     * \param[in] minute Minute, 0 to 59.
     * \param[in] second Seconds, 0 up to 60, fractional.
     * \return The Julian day of that UTC instant.
     */
    static double gregorianToJulianDay(int year, int month, int day,
                                       int hour = 0, int minute = 0, double second = 0.0);

    /*!
     * \brief Convert Julian Day to Gregorian calendar.
     * \details The inverse of gregorianToJulianDay() (Richards' algorithm).
     *          Hour and minute are truncated and the seconds carry the
     *          floating-point residue: 2025-01-01 06:30 comes back as
     *          06:30:00.0000134. Round the seconds when printing; a residue
     *          just below a whole minute shows as 59.99999 seconds of the
     *          previous minute.
     * \param[in]  jd     Julian Day value
     * \param[out] year   Gregorian year.
     * \param[out] month  Month, 1 to 12.
     * \param[out] day    Day of the month.
     * \param[out] hour   Hour, 0 to 23.
     * \param[out] minute Minute, 0 to 59.
     * \param[out] second Seconds with fraction.
     */
    static void julianDayToGregorian(double jd,
                                     int &year, int &month, int &day,
                                     int &hour, int &minute, double &second);

    /*!
     * \brief Get current time as Julian Day (UTC).
     * \details Whole seconds come from \c std::gmtime of the system clock.
     *          The fraction added to them is the clock's tick count modulo
     *          10^9 divided by 10^9, which is the sub-second part only when the
     *          clock counts nanoseconds; with the microsecond clock of
     *          libc++ (macOS) or the 100 ns clock of MSVC it is an unrelated
     *          number below one second. Do not rely on sub-second accuracy.
     * \return The current instant as a Julian day.
     */
    static double julianDayNow();

private:
    double m_julianDay; //!< The instant as a Julian day.
};

/*!
 * \brief Concrete implementation of ITimeSpan for time intervals.
 *
 * Represents a time duration as a double (in days).
 * Inherits DateTime interface from a reference time plus duration.
 *
 * \details The inherited instant (julianDay()) is the start of the span and
 * the end is start + duration(), as ITimeSpan defines. The standard says a
 * duration is non-negative; this class does not check, so a negative
 * duration gives an end before the start.
 *
 * \threadsafety Immutable after construction; safe to share across threads.
 * (Only while setDuration() and the inherited setters are not called.)
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
     * \return A new span with duration end - start (not checked for order).
     */
    static std::shared_ptr<TimeSpan> fromTimeRange(std::string_view id,
                                                     double start, double end);

    //! Destroys the span.
    ~TimeSpan() override = default;

    // ITimeSpan
    //! Length of the span.
    //! \return The duration in days.
    [[nodiscard]] double duration() const override { return m_duration; }

    //! End of the span.
    //! \return julianDay() + duration(), as a Julian day.
    [[nodiscard]] double endJulianDay() const override { return julianDay() + m_duration; }

    // Accessors
    //! Start of the span (the inherited instant).
    //! \return julianDay().
    [[nodiscard]] double startTime() const { return julianDay(); }
    //! End of the span; the same as endJulianDay().
    //! \return julianDay() + duration().
    [[nodiscard]] double endTime() const { return julianDay() + m_duration; }

    // Setters
    //! Replaces the duration and fires propertyChanged("Duration").
    //! \param[in] days New duration in days.
    void setDuration(double days) { m_duration = days; emitPropertyChanged("Duration"); }

private:
    double m_duration; //!< Duration in days.
};

} // namespace HydroCouple::SDK::Temporal

#endif // HYDROCOUPLESDK_TIMEDATA_H
