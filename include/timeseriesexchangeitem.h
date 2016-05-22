#ifndef TIMESERIESEXCHANGEITEM_H
#define TIMESERIESEXCHANGEITEM_H

#include "timeseriescomponentdataitem.h"
#include "abstractinput.h"
#include "abstractoutput.h"

class HYDROCOUPLESDK_EXPORT TimeSeriesOutputDouble: public AbstractOutput,
      public TimeSeriesComponentDataItem<double>,
      public virtual HydroCouple::Temporal::ITimeSeriesExchangeItem
{

      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::ITimeSeriesComponentDataItem
                   HydroCouple::Temporal::ITimeExchangeItem
                   HydroCouple::Temporal::ITimeSeriesExchangeItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

   public:
      TimeSeriesOutputDouble(const QString &id, const QList<HTime*> &times,
                             Dimension *timeDimension, ValueDefinition *valueDefinition,
                             AbstractModelComponent* parentModelComponent);

      virtual ~TimeSeriesOutputDouble();
};



class HYDROCOUPLESDK_EXPORT TimeSeriesInputDouble: public AbstractInput,
      public TimeSeriesComponentDataItem<double> ,
      public virtual HydroCouple::Temporal::ITimeSeriesExchangeItem
{

      Q_OBJECT

      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::ITimeSeriesComponentDataItem
                   HydroCouple::Temporal::ITimeExchangeItem
                   HydroCouple::Temporal::ITimeSeriesExchangeItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

   public:
      TimeSeriesInputDouble(const QString &id, const QList<HTime*> &times,
                             Dimension *timeDimension, ValueDefinition *valueDefinition,
                             AbstractModelComponent* parentModelComponent);

      virtual ~TimeSeriesInputDouble();
};


class HYDROCOUPLESDK_EXPORT TimeSeriesMultiInputDouble: public AbstractMultiInput,
      public TimeSeriesComponentDataItem<double>,
      public virtual HydroCouple::Temporal::ITimeSeriesExchangeItem
{

      Q_OBJECT

      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::ITimeSeriesComponentDataItem
                   HydroCouple::Temporal::ITimeExchangeItem
                   HydroCouple::Temporal::ITimeSeriesExchangeItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

   public:
      TimeSeriesMultiInputDouble(const QString &id, const QList<HTime*> &times,
                             Dimension *timeDimension, ValueDefinition *valueDefinition,
                             AbstractModelComponent* parentModelComponent);

      virtual ~TimeSeriesMultiInputDouble();
};

Q_DECLARE_METATYPE(TimeSeriesOutputDouble*)
Q_DECLARE_METATYPE(TimeSeriesInputDouble*)
Q_DECLARE_METATYPE(TimeSeriesMultiInputDouble*)

#endif // TIMESERIESEXCHANGEITEM_H

