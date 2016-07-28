#ifndef TIMESERIESEXCHANGEITEM_H
#define TIMESERIESEXCHANGEITEM_H

#include "timeseriescomponentdataitem.h"
#include "core/abstractinput.h"
#include "core/abstractoutput.h"


class HYDROCOUPLESDK_EXPORT TimeSeriesOutputDouble: public AbstractOutput,
    public TimeSeriesComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeExchangeItem
{

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeExchangeItem)

    Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:
    TimeSeriesOutputDouble(const QString& id,
                           const QList<SDKTemporal::Time*>& times,
                           Dimension* timeDimension,
                           ValueDefinition* valueDefinition,
                           AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesOutputDouble();

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

  private:
    Dimension *m_timeDimension;
};

//==============================================================================================================================


class HYDROCOUPLESDK_EXPORT TimeSeriesInputDouble: public AbstractInput,
    public TimeSeriesComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeExchangeItem
{

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeExchangeItem)

    Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:
    TimeSeriesInputDouble(const QString& id,
                          const QList<SDKTemporal::Time*>& times,
                          Dimension* timeDimension,
                          ValueDefinition* valueDefinition,
                          AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesInputDouble();

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

  private:
    Dimension *m_timeDimension;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT TimeSeriesMultiInputDouble: public AbstractMultiInput,
    public TimeSeriesComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeExchangeItem
{

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeExchangeItem)

    Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:

    TimeSeriesMultiInputDouble(const QString& id,
                               const QList<SDKTemporal::Time*>& times,
                               Dimension* timeDimension,
                               ValueDefinition* valueDefinition,
                               AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesMultiInputDouble();

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

  private:
    Dimension *m_timeDimension;
};


Q_DECLARE_METATYPE(TimeSeriesOutputDouble*)
Q_DECLARE_METATYPE(TimeSeriesInputDouble*)
Q_DECLARE_METATYPE(TimeSeriesMultiInputDouble*)

#endif // TIMESERIESEXCHANGEITEM_H

