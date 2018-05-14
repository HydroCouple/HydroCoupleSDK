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
  setModifiedJulianDay(0);
}

DateTime::DateTime(const QDateTime& dateTime, QObject *parent)
  :QObject(parent)
{
  setDateTime(dateTime);
}

DateTime::DateTime(double dateTime, QObject *parent)
  :QObject(parent)
{
  setModifiedJulianDay(dateTime);
}

DateTime::~DateTime()
{

}

double DateTime::modifiedJulianDay() const
{
  return m_modifiedJD;
}

QDateTime DateTime::dateTime() const
{
  return fromModifiedJulianDays(m_modifiedJD);
}

void DateTime::setModifiedJulianDay(double modifiedJD)
{
  m_modifiedJD = modifiedJD;
  emit propertyChanged("DateTime");
}

void DateTime::setDateTime(const QDateTime &dateTime)
{
  m_modifiedJD = toModifiedJulianDays(dateTime);
  emit propertyChanged("DateTime");
}

bool DateTime::compare(DateTime *time1, DateTime *time2)
{
  return time1->modifiedJulianDay() < time2->modifiedJulianDay();
}

double DateTime::toJulianDays(const QDateTime &dateTime)
{
  double jd = dateTime.date().toJulianDay() * 1.0 - 0.5 +
  dateTime.time().msecsSinceStartOfDay() * 1.0 / (24.0 * 60.0 * 60.0 * 1000.0);
  
  return jd;
}

double DateTime::toModifiedJulianDays(const QDateTime &dateTime)
{
  double mjd = toJulianDays(dateTime) - 2400000.5;
  return mjd;
}

QDateTime DateTime::fromJulianDays(double julianDays)
{
  QDateTime jd;
  long long int jday = floor(julianDays);
  QDate date = QDate::fromJulianDay(jday);
  jd.setDate(date);

  double stime = julianDays - date.toJulianDay();
  QTime time = QTime::fromMSecsSinceStartOfDay(std::ceil(stime * 24.0 * 60.0 * 60.0 * 1000.0));
  jd.setTime(time);

  return jd;
}

QDateTime DateTime::fromModifiedJulianDays(double modifiedJulianDay)
{
  QDateTime mjd;

  long long int jday = floor(modifiedJulianDay + 2400000.5);
  QDate date = QDate::fromJulianDay(jday);
  mjd.setDate(date);

  double stime = modifiedJulianDay + 2400000.5 - date.toJulianDay();

  QTime time = QTime::fromMSecsSinceStartOfDay(std::ceil(stime * 24.0 * 60.0 * 60.0 * 1000.0));
  mjd.setTime(time);

  return mjd;
}

bool DateTime::tryParse(const QString &dateTimeString, QDateTime &dateTime)
{
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

//======================================================================================================================================================================

TimeSpan::TimeSpan(QObject *parent)
  :DateTime(parent),
    m_duration(0)
{
  setModifiedJulianDay(0);
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
    m_endDateTime = modifiedJulianDay() + duration;
    propertyChanged("Duration");
  }
}

QDateTime TimeSpan::endDateTime() const
{
  return fromModifiedJulianDays(m_endDateTime);
}
