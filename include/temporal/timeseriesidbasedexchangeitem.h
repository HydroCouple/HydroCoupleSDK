#ifndef TIMESERIESIDBASEDEXCHANGEITEM_H
#define TIMESERIESIDBASEDEXCHANGEITEM_H

#include "core/abstractinput.h"
#include "core/abstractoutput.h"
#include "temporal/timeseriesidbasedcomponentdataitem.h"

class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedOutputDouble: public AbstractOutput,
    public TimeSeriesIdBasedComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeSeriesIdBasedExchangeItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeSeriesIdBasedExchangeItem)

    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)
    Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:
    TimeSeriesIdBasedOutputDouble (const QString& id,
                                   const QStringList& identifiers,
                                   Dimension* identifierDimension,
                                   const QList<SDKTemporal::Time*>& times,
                                   Dimension* timeDimension,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesIdBasedOutputDouble();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    QList<HydroCouple::Temporal::ITime*> times() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int timeIndex, int idIndex, QVariant &data) const override;

    void getValues(int timeIndex, int idIndex, int timeStride, int idStride, QVariant data[]) const override;

    void getValues(int timeIndex, int idIndex, int timeStride, int idStride, void *data) const override;

    void setValue(int timeIndex, int idIndex, const QVariant &data) override;

    void setValues(int timeIndex, int idIndex, int timeStride, int idStride, const QVariant data[]) override;

    void setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedInputDouble: public AbstractInput,
    public TimeSeriesIdBasedComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeSeriesIdBasedExchangeItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeSeriesIdBasedExchangeItem)

    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)
    Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:
    TimeSeriesIdBasedInputDouble (const QString& id,
                                   const QStringList& identifiers,
                                   Dimension* identifierDimension,
                                   const QList<SDKTemporal::Time*>& times,
                                   Dimension* timeDimension,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesIdBasedInputDouble();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    QList<HydroCouple::Temporal::ITime*> times() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int timeIndex, int idIndex, QVariant &data) const override;

    void getValues(int timeIndex, int idIndex, int timeStride, int idStride, QVariant data[]) const override;

    void getValues(int timeIndex, int idIndex, int timeStride, int idStride, void *data) const override;

    void setValue(int timeIndex, int idIndex, const QVariant &data) override;

    void setValues(int timeIndex, int idIndex, int timeStride, int idStride, const QVariant data[]) override;

    void setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data) override;

};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedMultiInputDouble: public AbstractMultiInput,
    public TimeSeriesIdBasedComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeSeriesIdBasedExchangeItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeSeriesIdBasedExchangeItem)

    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)
    Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:

    TimeSeriesIdBasedMultiInputDouble (const QString& id,
                                   const QStringList& identifiers,
                                   Dimension* identifierDimension,
                                   const QList<SDKTemporal::Time*>& times,
                                   Dimension* timeDimension,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesIdBasedMultiInputDouble();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    QList<HydroCouple::Temporal::ITime*> times() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int timeIndex, int idIndex, QVariant &data) const override;

    void getValues(int timeIndex, int idIndex, int timeStride, int idStride, QVariant data[]) const override;

    void getValues(int timeIndex, int idIndex, int timeStride, int idStride, void *data) const override;

    void setValue(int timeIndex, int idIndex, const QVariant &data) override;

    void setValues(int timeIndex, int idIndex, int timeStride, int idStride, const QVariant data[]) override;

    void setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data) override;

};


Q_DECLARE_METATYPE(TimeSeriesIdBasedOutputDouble*)
Q_DECLARE_METATYPE(TimeSeriesIdBasedInputDouble*)
Q_DECLARE_METATYPE(TimeSeriesIdBasedMultiInputDouble*)

#endif // TIMESERIESIDBASEDEXCHANGEITEM_H

