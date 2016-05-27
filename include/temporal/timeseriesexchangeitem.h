#ifndef TIMESERIESEXCHANGEITEM_H
#define TIMESERIESEXCHANGEITEM_H

#include "timeseriescomponentdataitem.h"
#include "core/abstractinput.h"
#include "core/abstractoutput.h"

namespace Temporal
{
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
      TimeSeriesOutputDouble(const QString &id, const QList<Time*> &times,
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
      TimeSeriesInputDouble(const QString &id, const QList<Time*> &times,
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
      TimeSeriesMultiInputDouble(const QString &id, const QList<Time*> &times,
                                 Dimension *timeDimension, ValueDefinition *valueDefinition,
                                 AbstractModelComponent* parentModelComponent);

      virtual ~TimeSeriesMultiInputDouble();
  };
}

Q_DECLARE_METATYPE(Temporal::TimeSeriesOutputDouble*)
Q_DECLARE_METATYPE(Temporal::TimeSeriesInputDouble*)
Q_DECLARE_METATYPE(Temporal::TimeSeriesMultiInputDouble*)

#endif // TIMESERIESEXCHANGEITEM_H

