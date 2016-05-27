#ifndef TIMEIDBASEDEXCHANGEITEM_H
#define TIMEIDBASEDEXCHANGEITEM_H

#include "core/abstractinput.h"
#include "core/abstractoutput.h"
#include "temporal/timeidbasedcomponentdataitem.h"

namespace Temporal
{
  class HYDROCOUPLESDK_EXPORT TimeIdBasedOutputDouble: public AbstractOutput,
      public TimeIdBasedComponentDataItem<double> , public virtual HydroCouple::Temporal::ITimeIdBasedExchangeItem
  {
      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::ITimeIdBasedComponentDataItem
                   HydroCouple::Temporal::ITimeIdBasedExchangeItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)
      Q_PROPERTY(QStringList Identifiers READ identifiers)

    public:
      TimeIdBasedOutputDouble (const QString &id, const QStringList &identifiers, const QList<Time*> &times,
                               Dimension* identifierDimension, Dimension *timeDimension,
                               ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent);

      virtual ~TimeIdBasedOutputDouble();

  };

  class HYDROCOUPLESDK_EXPORT TimeIdBasedInputDouble: public AbstractInput,
      public TimeIdBasedComponentDataItem<double>, public virtual HydroCouple::Temporal::ITimeIdBasedExchangeItem
  {
      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::ITimeIdBasedComponentDataItem
                   HydroCouple::Temporal::ITimeIdBasedExchangeItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)
      Q_PROPERTY(QStringList Identifiers READ identifiers)

    public:
      TimeIdBasedInputDouble (const QString &id, const QStringList &identifiers, const QList<Time*> &times,
                              Dimension* identifierDimension, Dimension *timeDimension,
                              ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent);

      virtual ~TimeIdBasedInputDouble();

  };


  class HYDROCOUPLESDK_EXPORT TimeIdBasedMultiInputDouble: public AbstractMultiInput,
      public TimeIdBasedComponentDataItem<double>, public virtual HydroCouple::Temporal::ITimeIdBasedExchangeItem
  {
      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::ITimeIdBasedComponentDataItem
                   HydroCouple::Temporal::ITimeIdBasedExchangeItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)
      Q_PROPERTY(QStringList Identifiers READ identifiers)

    public:
      TimeIdBasedMultiInputDouble(const QString &id, const QStringList &identifiers, const QList<Time*> &times,
                                  Dimension* identifierDimension, Dimension *timeDimension,
                                  ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent);

      virtual ~TimeIdBasedMultiInputDouble();

  };
}

Q_DECLARE_METATYPE(Temporal::TimeIdBasedOutputDouble*)
Q_DECLARE_METATYPE(Temporal::TimeIdBasedInputDouble*)
Q_DECLARE_METATYPE(Temporal::TimeIdBasedMultiInputDouble*)

#endif // TIMEIDBASEDEXCHANGEITEM_H

