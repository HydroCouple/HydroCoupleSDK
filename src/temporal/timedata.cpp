/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#include "stdafx.h"
#include "temporal/timedata.h"

#include <cmath>

using namespace SDKTemporal;

DateTime::DateTime(QObject *parent)
  :QObject(parent)
{
  setJulianDay(0);
}

DateTime::DateTime(const QDateTime& dateTime, QObject *parent)
  :QObject(parent)
{
  setDateTime(dateTime);
}

DateTime::DateTime(double dateTime, QObject *parent)
  :QObject(parent)
{
  setJulianDay(dateTime);
}

DateTime::~DateTime()
{

}

double DateTime::julianDay() const
{
  return m_JD;
}

QDateTime DateTime::dateTime() const
{
  return fromJulianDays(m_JD);
}

void DateTime::setJulianDay(double julianDay)
{
  m_JD = julianDay;
  emit propertyChanged("DateTime");
}

void DateTime::setDateTime(const QDateTime &dateTime)
{
  m_JD = toJulianDays(dateTime);
  emit propertyChanged("DateTime");
}

bool DateTime::compare(DateTime *time1, DateTime *time2)
{
  return time1->julianDay() < time2->julianDay();
}

double DateTime::toJulianDays(const QDateTime &dateTime)
{
  double fracDayTime = (dateTime.time().msecsSinceStartOfDay() * 1.0 )/ (86400000.0);
  double jd = 1.0 * dateTime.date().toJulianDay() - 0.5 + fracDayTime;

  return jd;
}

QDateTime DateTime::fromJulianDays(double julianDays)
{
  long long int jday = floor(julianDays);
  QDate date = QDate::fromJulianDay(jday);

  double stime = julianDays - date.toJulianDay();
  QTime time(0,0);
  time.addMSecs(stime * 86400000.0);

  QDateTime jd = QDateTime(date,time);

  return jd;
}

bool DateTime::tryParse(const QString &dateTimeString, QDateTime &dateTime)
{

  QStringList dateCols = dateTimeString.split(m_dateTimeDelim, QString::SkipEmptyParts);

  if (dateCols.size() == 6)
  {

    bool monthOk;
    int month = dateCols[0].toInt(&monthOk);

    bool dayOk;
    int day = dateCols[1].toInt(&dayOk);

    bool yearOk;
    int year = dateCols[2].toInt(&yearOk);

    bool hourOk;
    int hour = dateCols[3].toInt(&hourOk);

    bool minuteOk;
    int minute = dateCols[4].toInt(&minuteOk);

    bool secondOk;
    int second = dateCols[5].toInt(&secondOk);

    if (monthOk && dayOk && yearOk && hourOk && minuteOk && secondOk)
    {
      QDate date(year, month, day);
      QTime time = QTime(hour, minute, second);

      dateTime = QDateTime(date, time);

      if (dateTime.isValid())
      {
        return true;
      }
    }
  }


  dateTime = QDateTime::fromString(dateTimeString,Qt::ISODate);
  if(dateTime.isValid()) return true;

  dateTime = QDateTime::fromString(dateTimeString,Qt::TextDate);
  if(dateTime.isValid()) return true;

  dateTime = QDateTime::fromString(dateTimeString,Qt::SystemLocaleShortDate);
  if(dateTime.isValid()) return true;

  dateTime = QDateTime::fromString(dateTimeString,Qt::SystemLocaleLongDate);
  if(dateTime.isValid()) return true;

  dateTime = QDateTime::fromString(dateTimeString,Qt::DefaultLocaleShortDate);
  if(dateTime.isValid()) return true;

  dateTime = QDateTime::fromString(dateTimeString,Qt::DefaultLocaleLongDate);
  if(dateTime.isValid()) return true;

  dateTime = QDateTime::fromString(dateTimeString,Qt::SystemLocaleDate);
  if(dateTime.isValid()) return true;

  dateTime = QDateTime::fromString(dateTimeString,Qt::LocaleDate);
  if(dateTime.isValid()) return true;

  dateTime = QDateTime::fromString(dateTimeString,Qt::RFC2822Date);
  if(dateTime.isValid()) return true;

  for(QString format :  m_dateTimeFormats)
  {
    dateTime = QDateTime::fromString(dateTimeString,format);
    if(dateTime.isValid())
      return true;
  }


  return false;
}

const QList<QString> DateTime::m_dateTimeFormats({
                                                   "M/d/yy h:mm:ss",
                                                   "MM/dd/yyyy h:mm:ss",
                                                   "MM/dd/yy hh:mm:ss",
                                                   "MM/dd/yyyy hh:mm:ss",
                                                   "M/d/yy h:mm:ss A",
                                                   "M/d/yyyy h:mm:ss A",
                                                   "MM/dd/yyyy hh:mm:ss",
                                                   "M-d-yy h:mm:ss",
                                                   "MM-dd-yyyy h:mm:ss",
                                                   "MM-dd-yy hh:mm:ss",
                                                   "MM-dd-yyyy hh:mm:ss",
                                                   "M-d-yy h:mm:ss A",
                                                   "M-d-yyyy h:mm:ss A",
                                                   "MM-dd-yyyy hh:mm:ss",

                                                   "M/d/yy h/mm/ss",
                                                   "MM/dd/yyyy h/mm/ss",
                                                   "MM/dd/yy hh/mm/ss",
                                                   "MM/dd/yyyy hh/mm/ss",
                                                   "M/d/yy h/mm/ss A",
                                                   "M/d/yyyy h/mm/ss A",
                                                   "MM/dd/yyyy hh/mm/ss",
                                                   "M-d-yy h/mm/ss",
                                                   "MM-dd-yyyy h/mm/ss",
                                                   "MM-dd-yy hh/mm/ss",
                                                   "MM-dd-yyyy hh/mm/ss",
                                                   "M-d-yy h/mm/ss A",
                                                   "M-d-yyyy h/mm/ss A",
                                                   "MM-dd-yyyy hh/mm/ss",

                                                   "M/d/yy h:mm",
                                                   "MM/dd/yyyy h:mm",
                                                   "MM/dd/yy hh:mm",
                                                   "MM/dd/yyyy hh:mm",
                                                   "M/d/yy h:mm A",
                                                   "M/d/yyyy h:mm A",
                                                   "MM/dd/yyyy hh:mm",
                                                   "M-d-yy h:mm",
                                                   "MM-dd-yyyy h:mm",
                                                   "MM-dd-yy hh:mm",
                                                   "MM-dd-yyyy hh:mm",
                                                   "M-d-yy h:mm A",
                                                   "M-d-yyyy h:mm A",
                                                   "MM-dd-yyyy hh:mm",

                                                   "M/d/yy",
                                                   "MM/dd/yyyy",
                                                   "MM/dd/yy",
                                                   "MM/dd/yyyy",
                                                   "M/d/yy",
                                                   "M/d/yyyy",
                                                   "MM/dd/yyyy",
                                                   "M-d-yy",
                                                   "MM-dd-yyyy",
                                                   "MM-dd-yy",
                                                   "MM-dd-yyyy",
                                                   "M-d-yy",
                                                   "M-d-yyyy",
                                                   "MM-dd-yyyy"
                                                 });

const QRegExp DateTime::m_dateTimeDelim("(\\,|\\t|\\\n|\\/|\\s+|\\:)");


//======================================================================================================================================================================

TimeSpan::TimeSpan(QObject *parent)
  :DateTime(parent),
    m_duration(0)
{
  setJulianDay(0);
}

TimeSpan::TimeSpan(const QDateTime &dateDateTime, double duration, QObject *parent)
  :DateTime(dateDateTime,parent),
    m_duration(duration)
{
}

TimeSpan::TimeSpan(double dateDateTime, double duration, QObject *parent)
  :DateTime(dateDateTime,parent),
    m_duration(duration)
{
}

TimeSpan::~TimeSpan()
{
}

double TimeSpan::duration() const
{
  return m_duration;
}

void TimeSpan::setDuration(double duration)
{
  if(duration >= 0)
  {
    m_duration = duration;
    m_endDateTime = julianDay() + duration;
    propertyChanged("Duration");
  }
}

QDateTime TimeSpan::endDateTime() const
{
  return fromJulianDays(m_endDateTime);
}
