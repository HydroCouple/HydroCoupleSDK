/*!
 * \file timedata.h
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

#ifndef TIMEDATA_H
#define TIMEDATA_H

#include "hydrocouple.h"
#include "hydrocoupletemporal.h"
#include "hydrocouplesdk.h"

#include <QObject>
#include <QDateTime>

namespace  SDKTemporal
{
  class HYDROCOUPLESDK_EXPORT DateTime : public QObject,
      public virtual HydroCouple::Temporal::IDateTime
  {
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IPropertyChanged)
      Q_INTERFACES(HydroCouple::Temporal::IDateTime)
      Q_PROPERTY(QDateTime DateTime READ dateTime WRITE setDateTime NOTIFY propertyChanged)

    public:

      DateTime(QObject *parent = nullptr);

      explicit DateTime(const QDateTime &dateTime, QObject *parent = nullptr);

      explicit DateTime(double dateTime, QObject *parent = nullptr);

      virtual ~DateTime();

      double modifiedJulianDay() const override;

      QDateTime dateTime() const;

      void setModifiedJulianDay(double modifiedJD);

      void setDateTime(const QDateTime &dateTime);

      static bool compare(DateTime *time1, DateTime *time2);

      static double toJulianDays(const QDateTime &dateTime);

      static double toModifiedJulianDays(const QDateTime &dateTime);

      static QDateTime fromJulianDays(double julianDays);

      static QDateTime fromModifiedJulianDays(double modifiedJulianDays);

      static bool tryParse(const QString &dateTimeString, QDateTime &dateTime);

    signals:

      void propertyChanged(const QString &propertyName) override;

    private:

      double m_modifiedJD;
      static const QList<QString> m_dateTimeFormats;

  };

  class HYDROCOUPLESDK_EXPORT TimeSpan : public DateTime,
      public virtual HydroCouple::Temporal::ITimeSpan
  {
      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeSpan)
      Q_PROPERTY(double Duration READ duration WRITE setDuration NOTIFY propertyChanged)
      Q_PROPERTY(QDateTime EndDateTime READ endDateTime NOTIFY propertyChanged)

    public:

      TimeSpan(QObject* parent = nullptr);

      explicit TimeSpan(const QDateTime &dateTime, double duration = 0, QObject *parent = nullptr);

      explicit TimeSpan(double dateTime, double duration = 0, QObject *parent = nullptr);

      virtual ~TimeSpan();

      double duration() const override;

      void setDuration(double duration);

      QDateTime endDateTime() const;

    signals:

      void propertyChanged(const QString& propertyName) override;

    private:
      double m_endDateTime;
      double m_duration;
  };
}

Q_DECLARE_METATYPE(SDKTemporal::DateTime*)
Q_DECLARE_METATYPE(SDKTemporal::TimeSpan*)

#endif // TIMEDATA_H

