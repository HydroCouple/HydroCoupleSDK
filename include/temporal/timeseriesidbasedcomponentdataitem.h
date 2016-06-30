#ifndef TIMESERIESIDBASEDCOMPONENTDATAITEM_H
#define TIMESERIESIDBASEDCOMPONENTDATAITEM_H

namespace SDKTemporal
{
  class Time;
  class TimeSpan;
}

#include "hydrocoupletemporal.h"
#include "core/componentdataitem2d.h"
#include "core/abstractcomponentdataitem.h"


template<class T>
class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem : public ComponentDataItem2D<T>
{

  public:

    TimeSeriesIdBasedComponentDataItem(const QStringList& identifiers,
                                 Dimension* identifierDimension,
                                 const QList<SDKTemporal::Time*>& times,
                                 Dimension *timeDimension,
                                 const T& defaultValue);

    virtual ~TimeSeriesIdBasedComponentDataItem();

    bool addIdentifier(const QString& identifier, bool resetDataArray = true);

    void addIdentifiers(const QList<QString>& identifiers, bool resetDataArray = true);

    bool removeIdentifier(const QString& identifier, bool resetDataArray = true);

    bool addTime(SDKTemporal::Time* time , bool resetDataArray = true);

    void addTimes(const QList<SDKTemporal::Time*>& times, bool resetDataArray = true);

    bool removeTime(SDKTemporal::Time* time, bool resetDataArray = true);

    void setTimes(const QList<SDKTemporal::Time*>& times);

    void getValueT(int idIndex, int timeIndex, QVariant& data) const;

    void getValuesT(int idIndex, int timeIndex, int idStride, int timeStride, QVariant data[]) const;

    void getValuesT(int idIndex, int timeIndex, int idStride, int timeStride, void* data) const;

    void setValueT(int idIndex, int timeIndex, const QVariant& data) ;

    void setValuesT(int idIndex, int timeIndex, int idStride, int timeStride, const QVariant data[]) ;

    void setValuesT(int idIndex, int timeIndex, int idStride, int timeStride, const void* data);

  protected:

    QStringList identifiersInternal() const;

    Dimension* identifierDimensionInternal() const;

    void clearIdentifiers();

    QList<SDKTemporal::Time*> timesInternal() const;

    Dimension* timeDimensionInternal() const;

    void clearTimes();

    SDKTemporal::TimeSpan* timeSpanInternal() const;

  private:
    QStringList  m_identifiers;
    Dimension* m_identifierDimension;
    QList<SDKTemporal::Time*> m_times;
    SDKTemporal::TimeSpan* m_timeSpan;
    Dimension* m_timeDimension;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItemDouble : public AbstractComponentDataItem,
    public TimeSeriesIdBasedComponentDataItem<double>,
    public virtual HydroCouple::Temporal::ITimeSeriesIdBasedComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeSeriesIdBasedComponentDataItem)

    Q_PROPERTY(QStringList Identifiers READ identifiers)
    Q_PROPERTY(HydroCouple::IDimension* IdentifierDimension READ identifierDimension)
    Q_PROPERTY(QList<HydroCouple::Temporal::ITime*> Times READ times)
    Q_PROPERTY(HydroCouple::Temporal::ITimeSpan* TimeSpan READ timeSpan)
    Q_PROPERTY(HydroCouple::IDimension* TimeDimension READ timeDimension)

  public:

    TimeSeriesIdBasedComponentDataItemDouble (const QString& id,
                                        const QStringList& identifiers,
                                        Dimension* identifierDimension,
                                        const QList<SDKTemporal::Time*>& times,
                                        Dimension* timeDimension,
                                        ValueDefinition* valueDefinition,
                                        AbstractModelComponent* modelComponent);

    virtual ~TimeSeriesIdBasedComponentDataItemDouble();

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

Q_DECLARE_METATYPE(TimeSeriesIdBasedComponentDataItemDouble*)

#endif // TIMESERIESIDBASEDCOMPONENTDATAITEM_H

