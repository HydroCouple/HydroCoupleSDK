#ifndef TIMESERIESIDBASEDCOMPONENTDATAITEM_H
#define TIMESERIESIDBASEDCOMPONENTDATAITEM_H

namespace SDKTemporal
{
  class Time;
  class TimeSpan;
}

#include "hydrocoupletemporal.h"
#include "core/exchangeitems2d.h"
#include "core/abstractcomponentdataitem.h"


template<class T>
class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem: public ComponentDataItem2D<T>
{

  public:

    TimeSeriesIdBasedComponentDataItem(const QStringList& identifiers,
                                       const QList<SDKTemporal::Time*>& times,
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

    void clearIdentifiers();

    QList<SDKTemporal::Time*> timesInternal() const;

    SDKTemporal::TimeSpan* timeSpanInternal() const;

    void clearTimes();

  private:
    QStringList  m_identifiers;
    QList<SDKTemporal::Time*> m_times;
    SDKTemporal::TimeSpan* m_timeSpan;
};


#endif // TIMESERIESIDBASEDCOMPONENTDATAITEM_H

