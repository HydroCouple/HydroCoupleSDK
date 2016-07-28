#ifndef TIMESERIESCOMPONENTDATAITEM_H
#define TIMESERIESCOMPONENTDATAITEM_H

#include "hydrocoupletemporal.h"
#include "core/componentdataitem1d.h"
#include "core/abstractcomponentdataitem.h"

namespace SDKTemporal
{
  class Time;
  class TimeSpan;
}

template<class T>
class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem : public ComponentDataItem1D<T>
{

  public:

    TimeSeriesComponentDataItem(const QList<SDKTemporal::Time*>& times,
                                const T& defaultValue);

    virtual ~TimeSeriesComponentDataItem();

    bool addTime(SDKTemporal::Time* time , bool resetDataArray = true);

    void addTimes(const QList<SDKTemporal::Time*>& times, bool resetDataArray = true);

    bool removeTime(SDKTemporal::Time *time, bool resetDataArray = true);

    void setTimes(const QList<SDKTemporal::Time*>& times);

    void getValueT(int timeIndex, QVariant& data) const;

    void getValuesT(int timeIndex, int stride, QVariant data[]) const;

    void getValuesT(int timeIndex, int stride, void *data) const;

    void setValueT(int timeIndex, const QVariant& data);

    void setValuesT(int timeIndex, int stride, const QVariant data[]);

    void setValuesT(int timeIndex, int stride, const void *data);

  protected:

    QList<SDKTemporal::Time*> timesInternal() const;

    SDKTemporal::TimeSpan* timeSpanInternal() const;

    void clearTimes();

  private:

    QList<SDKTemporal::Time*> m_times;
    SDKTemporal::TimeSpan *m_timeSpan;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItemDouble : public AbstractComponentDataItem,
    public TimeSeriesComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeComponentDataItem
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem)

    Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:

    TimeSeriesComponentDataItemDouble(const QString& id,
                                      const QList<SDKTemporal::Time*>& times,
                                      Dimension* timeDimension,
                                      ValueDefinition* valueDefinition,
                                      AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesComponentDataItemDouble();

    QList<HydroCouple::Temporal::ITime*> times() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int timeIndex, QVariant &data) const override;

    void getValues(int timeIndex, int stride, QVariant data[]) const override;

    void getValues(int timeIndex, int stride, void *data) const override;

    void setValue(int timeIndex, const QVariant &data) override;

    void setValues(int timeIndex, int stride, const QVariant data[]) override;

    void setValues(int timeIndex, int stride, const void *data) override;

    void readData(QXmlStreamReader &reader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

  private:
    Dimension *m_timeDimension;

};

Q_DECLARE_METATYPE(TimeSeriesComponentDataItemDouble*)

#endif // TIMESERIESCOMPONENTDATAITEM_H

