#ifndef TIMESERIESCOMPONENTDATAITEM_H
#define TIMESERIESCOMPONENTDATAITEM_H

class HTime;
class HTimeSpan;


#include "hydrocoupletemporal.h"
#include "componentdataitem1d.h"
#include "abstractcomponentdataitem.h"

template<class T>
class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem : public ComponentDataItem1D<T>
      , public virtual HydroCouple::Temporal::ITimeSeriesComponentDataItem
{
   public:

      using ComponentDataItem1D<T>::getValue;
      using ComponentDataItem1D<T>::getValues;
      using ComponentDataItem1D<T>::setValue;
      using ComponentDataItem1D<T>::setValues;

      TimeSeriesComponentDataItem(const QList<HTime*> &times, Dimension *timeDimension, ValueDefinition *valueDefinition);

      virtual ~TimeSeriesComponentDataItem();

      QList<HydroCouple::Temporal::ITime*> times() const override;

      bool addTime(HTime *time , bool resetDataArray = true);

      void addTimes(const QList<HTime*> &times, bool resetDataArray = true);

      void setTimes(const QList<HTime*> &times);

      bool removeTime(HTime *time, bool resetDataArray = true);

      HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

      HydroCouple::IDimension* timeDimension() const override;

      void getValue(int timeIndex, QVariant &data) const override;

      void getValues(int timeIndex, int stride, QVariant data[]) const override;

      void getValues(int timeIndex, int stride, void *data) const override;

      void setValue(int timeIndex, const QVariant &data) override;

      void setValues(int timeIndex, int stride, const QVariant data[]) override;

      void setValues(int timeIndex, int stride, const void *data) override;

   protected:

      void clearTimes();

      QList<HTime*> hTimes() const;

      HTimeSpan* hTimeSpan() const;

      Dimension* hTimeDimension() const;

   private:
      QList<HTime*> m_times;
      HTimeSpan *m_timeSpan;
      Dimension *m_timeDimension;
};


class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItemDouble : public AbstractComponentDataItem
      , public virtual TimeSeriesComponentDataItem<double>
{
      Q_OBJECT
      Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                   HydroCouple::Temporal::ITimeSeriesComponentDataItem)

      Q_PROPERTY(QList<HydroCouple::IDimension*> Dimensions READ dimensions)
      Q_PROPERTY(HydroCouple::IValueDefinition* ValueDefinition READ valueDefinition)
      Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
      Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
      Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

   public:
      TimeSeriesComponentDataItemDouble(const QString &id, const QList<HTime*> &times,
                                  Dimension *timeDimension, ValueDefinition *valueDefinition,
                                  AbstractModelComponent* parentModelComponent);

      virtual ~TimeSeriesComponentDataItemDouble();

      void readData(QXmlStreamReader &reader) override;

      void writeData(QXmlStreamWriter &xmlWriter) override;
};

Q_DECLARE_METATYPE(TimeSeriesComponentDataItemDouble*)

#endif // TIMESERIESCOMPONENTDATAITEM_H

