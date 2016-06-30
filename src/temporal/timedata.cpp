#include "stdafx.h"
#include "temporal/timedata.h"

using namespace SDKTemporal;

Time::Time(QObject *parent)
   :QObject(parent)
{
   setDateTime(0);
}

Time::Time(const QDateTime& dateTime, QObject *parent)
   :QObject(parent)
{
   setDateTime(dateTime);
}

Time::Time(double dateTime, QObject *parent)
   :QObject(parent)
{
   setDateTime(dateTime);
}

Time::~Time()
{

}

double Time::dateTime() const
{
   return m_modifiedJD;
}

QDateTime Time::qDateTime() const
{
   return m_dateTime;
}

void Time::setDateTime(double dateTime)
{
   m_modifiedJD = dateTime;

   int jday = (int) (dateTime + 2400000.5 );
   QDate date = QDate::fromJulianDay(jday);
   m_dateTime.setDate(date);
   QTime time = QTime::fromMSecsSinceStartOfDay((int)((dateTime + 2400000.5 -date.toJulianDay()) * 24 * 60 * 60 *1000));
   m_dateTime.setTime(time);

   emit propertyChanged("DateTime");
}

void Time::setDateTime(const QDateTime &dateTime)
{
   m_dateTime = dateTime;
   m_modifiedJD = (m_dateTime.date().toJulianDay() *1.0 + m_dateTime.time().msecsSinceStartOfDay()*1.0/(24.0*60.0*60.0*1000.0)) -
         2400000.5;
   emit propertyChanged("DateTime");
}

bool Time::compare(Time *time1, Time *time2)
{
   return time1->dateTime() < time2->dateTime();
}

//======================================================================================================================================================================

TimeSpan::TimeSpan(QObject *parent)
   :Time(parent),
     m_duration(0)
{
  setDateTime(0);
}

TimeSpan::TimeSpan(const QDateTime &dateTime, double duration, QObject *parent)
   :Time(dateTime,parent),
     m_duration(duration)
{
}

TimeSpan::TimeSpan(double dateTime, double duration, QObject *parent)
   :Time(dateTime,parent),
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
      m_endTime = qDateTime().addMSecs(duration * 24 * 60 * 60 * 1000);
      propertyChanged("Duration");
   }
}

QDateTime TimeSpan::endTime() const
{
   return m_endTime;
}
