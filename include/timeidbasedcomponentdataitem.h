#ifndef IDBASEDTIMECOMPONENTDATAITEM_H
#define IDBASEDTIMECOMPONENTDATAITEM_H

class HTime;
class HTimeSpan;

#include "abstractcomponentdataitem.h"
#include "componentdataitem2d.h"
#include "hydrocoupletemporal.h"

template<class T>
class HYDROCOUPLESDK_EXPORT IdBasedTimeComponentDataItem : public ComponentDataItem2D<T>
      , public virtual HydroCouple::Temporal::IIdBasedTimeComponentDataItem
{

   public:
      using ComponentDataItem2D<T>::getValue;
      using ComponentDataItem2D<T>::getValues;
      using ComponentDataItem2D<T>::setValue;
      using ComponentDataItem2D<T>::setValues;

      IdBasedTimeComponentDataItem(const QStringList &identifiers, const QList<HTime*> &times,
                                   Dimension* identifierDimension, Dimension *timeDimension,
                                   ValueDefinition *valueDefinition);
      
      virtual ~IdBasedTimeComponentDataItem();

      QStringList identifiers() const override;

      bool addIdentifier(const QString &identifier, bool resetDataArray = true);

      void addIdentifiers(const QList<QString> &identifiers, bool resetDataArray = true);

      bool removeIdentifier(const QString &identifier, bool resetDataArray = true);

      HydroCouple::IDimension* identifierDimension() const override;

      QList<HydroCouple::Temporal::ITime*> times() const override;

      bool addTime(HTime *time , bool resetDataArray = true);

      void addTimes(const QList<HTime*> &times, bool resetDataArray = true);

      bool removeTime(HTime *time, bool resetDataArray = true);

      HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

      HydroCouple::IDimension* timeDimension() const override;

      void getValue(int idIndex, int timeIndex, QVariant &data) const override;

      void getValues(int idIndex, int timeIndex, int idStride, int timeStride, QVariant data[]) const override;

      void getValues(int idIndex, int timeIndex, int idStride, int timeStride, void *data) const override;

      void setValue(int idIndex, int timeIndex, const QVariant &data) override;

      void setValues(int idIndex, int timeIndex, int idStride, int timeStride, const QVariant data[]) override;

      void setValues(int idIndex, int timeIndex, int idStride, int timeStride, const void *data) override;

   protected:

      void clearIdentifiers();

      Dimension* hIdentifierDimension() const;

      void clearTimes();

      QList<HTime*> hTimes() const;

      HTimeSpan* hTimeSpan() const;

      Dimension* hTimeDimension() const;

   private:

      QStringList  m_identifiers;
      Dimension *m_identifierDimension;
      QList<HTime*> m_times;
      HTimeSpan *m_timeSpan;
      Dimension *m_timeDimension;
};


class HYDROCOUPLESDK_EXPORT IdBasedTimeComponentDataItemDouble : public AbstractComponentDataItem, 
      public virtual IdBasedTimeComponentDataItem<double>
{
      Q_OBJECT

      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::IIdBasedTimeComponentDataItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)
      Q_PROPERTY(QStringList Identifiers READ identifiers)
      
   public:
     IdBasedTimeComponentDataItemDouble (const QString &id, const QStringList &identifiers, const QList<HTime*> &times,
                                         Dimension* identifierDimension, Dimension *timeDimension,
                                         ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent);
     
     virtual ~IdBasedTimeComponentDataItemDouble();
     
};

Q_DECLARE_METATYPE(IdBasedTimeComponentDataItemDouble*)
#endif // IDBASEDTIMECOMPONENTDATAITEM_H

