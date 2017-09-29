#ifndef TIMESERIESIDBASEDEXCHANGEITEM_H
#define TIMESERIESIDBASEDEXCHANGEITEM_H

#include "core/abstractinput.h"
#include "core/abstractoutput.h"
#include "temporal/timeseriesidbasedcomponentdataitem.h"

class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedOutputDouble: public AbstractOutput,
    public TimeSeriesIdBasedComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::Temporal::ITimeIdBasedComponentDataItem)

    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)
    Q_PROPERTY(QList<HydroCouple::Temporal::IDateTime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:

    TimeSeriesIdBasedOutputDouble (const QString& id,
                                   const QStringList& identifiers,
                                   Dimension* identifierDimension,
                                   const QList<SDKTemporal::DateTime*>& times,
                                   Dimension* timeDimension,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesIdBasedOutputDouble();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, int idIndex, void *data) const override;

    void getValues(int timeIndex, int idIndex, int timeStride, int idStride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    virtual void setValue(int timeIndex, int idIndex, const void *data) override;

    virtual void setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data) override;

  private:
    Dimension *m_identifierDimension, *m_timeDimension;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedInputDouble: public AbstractInput,
    public TimeSeriesIdBasedComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeIdBasedComponentDataItem)

    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)
    Q_PROPERTY(QList<HydroCouple::Temporal::IDateTime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:
    TimeSeriesIdBasedInputDouble (const QString& id,
                                   const QStringList& identifiers,
                                   Dimension* identifierDimension,
                                   const QList<SDKTemporal::DateTime*>& times,
                                   Dimension* timeDimension,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesIdBasedInputDouble();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, int idIndex, void *data) const override;

    void getValues(int timeIndex, int idIndex, int timeStride, int idStride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    virtual void setValue(int timeIndex, int idIndex, const void *data) override;

    virtual void setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data) override;

  private:
    Dimension *m_identifierDimension, *m_timeDimension;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedMultiInputDouble: public AbstractMultiInput,
    public TimeSeriesIdBasedComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeIdBasedComponentDataItem)

    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)
    Q_PROPERTY(QList<HydroCouple::Temporal::IDateTime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:

    TimeSeriesIdBasedMultiInputDouble (const QString& id,
                                   const QStringList& identifiers,
                                   Dimension* identifierDimension,
                                   const QList<SDKTemporal::DateTime*>& times,
                                   Dimension* timeDimension,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesIdBasedMultiInputDouble();

    QStringList identifiers() const override;

    HydroCouple::IDimension* identifierDimension() const override;

    QList<HydroCouple::Temporal::IDateTime *> times() const override;

    int timeCount() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, int idIndex, void *data) const override;

    void getValues(int timeIndex, int idIndex, int timeStride, int idStride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    virtual void setValue(int timeIndex, int idIndex, const void *data) override;

    virtual void setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data) override;

  private:
    Dimension *m_identifierDimension, *m_timeDimension;
};


Q_DECLARE_METATYPE(TimeSeriesIdBasedOutputDouble*)
Q_DECLARE_METATYPE(TimeSeriesIdBasedInputDouble*)
Q_DECLARE_METATYPE(TimeSeriesIdBasedMultiInputDouble*)

#endif // TIMESERIESIDBASEDEXCHANGEITEM_H

