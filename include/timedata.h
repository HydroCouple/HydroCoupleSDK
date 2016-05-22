#ifndef TIMEDATA_H
#define TIMEDATA_H

#include "hydrocouple.h"
#include "hydrocoupletemporal.h"
#include "hydrocouplesdk.h"

#include <QObject>
#include <QDateTime>

class HYDROCOUPLESDK_EXPORT HTime : public QObject , public virtual HydroCouple::Temporal::ITime
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::IPropertyChanged HydroCouple::Temporal::ITime)
      Q_PROPERTY(QDateTime DateTime READ qDateTime WRITE setDateTime NOTIFY propertyChanged)

   public:
      HTime(QObject *parent = nullptr);

      HTime(const QDateTime &dateTime, QObject *parent = nullptr);

      virtual ~HTime();

      double dateTime() const override;

      QDateTime qDateTime() const;

      void setDateTime(double dateTime) override;

      void setDateTime(const QDateTime &dateTime);

      static bool compare(HTime *time1, HTime *time2);

   signals:
      void propertyChanged(const QString &propertyName) override;

   private:
      QDateTime m_dateTime;
      double m_modifiedJD;
};

class HYDROCOUPLESDK_EXPORT HTimeSpan : public HTime, public virtual HydroCouple::Temporal::ITimeSpan
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeSpan)
      Q_PROPERTY(double Duration READ duration WRITE setDuration NOTIFY propertyChanged)
      Q_PROPERTY(QDateTime EndTime READ endTime NOTIFY propertyChanged)

   public:
      HTimeSpan(QObject* parent = nullptr);

      HTimeSpan(const QDateTime &dateTime, double duration = 0, QObject *parent = nullptr);

      ~HTimeSpan();

      double duration() const override;

      void setDuration(double duration);

      QDateTime endTime() const;

   signals:
      void propertyChanged(const QString& propertyName) override;

   private:
      double m_duration;
      QDateTime m_endTime;
};

Q_DECLARE_METATYPE(HTime*)
Q_DECLARE_METATYPE(HTimeSpan*)

#endif // TIMEDATA_H

