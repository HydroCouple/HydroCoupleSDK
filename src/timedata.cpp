#include "stdafx.h"
#include "timedata.h"


HTime::HTime(QObject *parent)
   :QObject(parent)
{
   setDateTime(QDateTime::currentDateTime());
}

HTime::HTime(const QDateTime &dateTime, QObject *parent)
   :QObject(parent)
{
   setDateTime(dateTime);
}

HTime::~HTime()
{

}

double HTime::dateTime() const
{
   return m_modifiedJD;
}

QDateTime HTime::qDateTime() const
{
   return m_dateTime;
}

void HTime::setDateTime(double dateTime)
{
   m_modifiedJD = dateTime;

   int jday = (int) (dateTime + 2400000.5 );
   QDate date = QDate::fromJulianDay(jday);
   m_dateTime.setDate(date);
   QTime time = QTime::fromMSecsSinceStartOfDay((int)((dateTime + 2400000.5 -date.toJulianDay()) * 24 * 60 * 60 *1000));
   m_dateTime.setTime(time);

   emit propertyChanged("DateTime");
}

void HTime::setDateTime(const QDateTime &dateTime)
{
   m_dateTime = dateTime;
   m_modifiedJD = (m_dateTime.date().toJulianDay() *1.0 + m_dateTime.time().msecsSinceStartOfDay()*1.0/(24.0*60.0*60.0*1000.0)) -
         2400000.5;
   emit propertyChanged("DateTime");
}

bool HTime::compare(HTime *time1, HTime *time2)
{
   return time1->dateTime() < time2->dateTime();
}

//======================================================================================================================================================================

HTimeSpan::HTimeSpan(QObject *parent)
   :HTime(parent), m_duration(0)
{
}

HTimeSpan::HTimeSpan(const QDateTime &dateTime, double duration, QObject *parent)
   :HTime(dateTime,parent), m_duration(duration)
{
}

HTimeSpan::~HTimeSpan()
{
}

double HTimeSpan::duration() const
{
   return m_duration;
}

void HTimeSpan::setDuration(double duration)
{
   if(duration >= 0)
   {
      m_duration = duration;
      m_endTime = qDateTime().addMSecs(duration * 24 * 60 * 60 * 1000);
      propertyChanged("Duration");
   }
}

QDateTime HTimeSpan::endTime() const
{
   return m_endTime;
}
