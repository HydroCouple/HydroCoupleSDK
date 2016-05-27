#ifndef IDBASEDTIMEEXCHANGEITEM_H
#define IDBASEDTIMEEXCHANGEITEM_H

#include "idbasedtimecomponentdataitem.h"
#include "abstractinput.h"
#include "abstractoutput.h"

class HYDROCOUPLESDK_EXPORT IdBasedTimeOutputDouble: public AbstractOutput,
      public IdBasedTimeComponentDataItem<double> , public virtual HydroCouple::Temporal::IIdBasedTimeExchangeItem
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::IIdBasedTimeComponentDataItem
                   HydroCouple::Temporal::IIdBasedTimeExchangeItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)
      Q_PROPERTY(QStringList Identifiers READ identifiers)

   public:
     IdBasedTimeOutputDouble (const QString &id, const QStringList &identifiers, const QList<HTime*> &times,
                                         Dimension* identifierDimension, Dimension *timeDimension,
                                         ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent);

     virtual ~IdBasedTimeOutputDouble();

};

class HYDROCOUPLESDK_EXPORT IdBasedTimeInputDouble: public AbstractInput,
      public IdBasedTimeComponentDataItem<double>, public virtual HydroCouple::Temporal::IIdBasedTimeExchangeItem
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::IIdBasedTimeComponentDataItem
                   HydroCouple::Temporal::IIdBasedTimeExchangeItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)
      Q_PROPERTY(QStringList Identifiers READ identifiers)

   public:
     IdBasedTimeInputDouble (const QString &id, const QStringList &identifiers, const QList<HTime*> &times,
                                         Dimension* identifierDimension, Dimension *timeDimension,
                                         ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent);

     virtual ~IdBasedTimeInputDouble();

};


class HYDROCOUPLESDK_EXPORT IdBasedTimeMultiInputDouble: public AbstractMultiInput,
      public IdBasedTimeComponentDataItem<double>, public virtual HydroCouple::Temporal::IIdBasedTimeExchangeItem
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::IIdBasedTimeComponentDataItem
                   HydroCouple::Temporal::IIdBasedTimeExchangeItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)
      Q_PROPERTY(QStringList Identifiers READ identifiers)

   public:
     IdBasedTimeMultiInputDouble(const QString &id, const QStringList &identifiers, const QList<HTime*> &times,
                                         Dimension* identifierDimension, Dimension *timeDimension,
                                         ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent);

     virtual ~IdBasedTimeMultiInputDouble();

};


Q_DECLARE_METATYPE(IdBasedTimeOutputDouble*)
Q_DECLARE_METATYPE(IdBasedTimeInputDouble*)
Q_DECLARE_METATYPE(IdBasedTimeMultiInputDouble*)

#endif // IDBASEDTIMEEXCHANGEITEM_H

