#ifndef TIMEDATA_H
#define TIMEDATA_H

#include "hydrocouple.h"
#include "hydrocoupletemporal.h"
#include "hydrocouplesdk.h"

#include <QObject>
#include <QDateTime>

namespace  SDKTemporal
{
  class HYDROCOUPLESDK_EXPORT Time : public QObject,
      public virtual HydroCouple::Temporal::ITime
  {
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IPropertyChanged)
      Q_INTERFACES(HydroCouple::Temporal::ITime)
      Q_PROPERTY(QDateTime DateTime READ qDateTime WRITE setDateTime NOTIFY propertyChanged)

    public:

      Time(QObject *parent = nullptr);

      explicit Time(const QDateTime &dateTime, QObject *parent = nullptr);

      explicit Time(double dateTime, QObject *parent = nullptr);

      virtual ~Time();

      double dateTime() const override;

      QDateTime qDateTime() const;

      void setDateTime(double dateTime) override;

      void setDateTime(const QDateTime &dateTime);

      static bool compare(Time *time1, Time *time2);

    signals:
      void propertyChanged(const QString &propertyName) override;

    private:
      QDateTime m_dateTime;
      double m_modifiedJD;
  };

  class HYDROCOUPLESDK_EXPORT TimeSpan : public Time,
      public virtual HydroCouple::Temporal::ITimeSpan
  {
      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeSpan)
      Q_PROPERTY(double Duration READ duration WRITE setDuration NOTIFY propertyChanged)
      Q_PROPERTY(QDateTime EndTime READ endTime NOTIFY propertyChanged)

    public:

      TimeSpan(QObject* parent = nullptr);

      explicit TimeSpan(const QDateTime &dateTime, double duration = 0, QObject *parent = nullptr);

      explicit TimeSpan(double dateTime, double duration = 0, QObject *parent = nullptr);

      virtual ~TimeSpan();

      double duration() const override;

      void setDuration(double duration);

      QDateTime endTime() const;

    signals:
      void propertyChanged(const QString& propertyName) override;

    private:
      double m_duration;
      QDateTime m_endTime;
  };
}

Q_DECLARE_METATYPE(SDKTemporal::Time*)
Q_DECLARE_METATYPE(SDKTemporal::TimeSpan*)

#endif // TIMEDATA_H

