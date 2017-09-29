#ifndef TIMESERIESIDBASEDCOMPONENTDATAITEM_H
#define TIMESERIESIDBASEDCOMPONENTDATAITEM_H

namespace SDKTemporal
{
  class DateTime;
  class TimeSpan;
}

#include "hydrocoupletemporal.h"
#include "core/exchangeitems2d.h"
#include "core/abstractcomponentdataitem.h"


template<class T>
class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem: public ComponentDataItem2D<T>
{

  public:

    TimeSeriesIdBasedComponentDataItem(const QString &id, const QStringList& identifiers,
                                       const QList<SDKTemporal::DateTime*> &times,
                                       const T& defaultValue);

    virtual ~TimeSeriesIdBasedComponentDataItem();

    bool addIdentifier(const QString& identifier);

    bool addIdentifiers(const QList<QString>& identifiers);

    bool removeIdentifier(const QString& identifier);

    bool addTime(SDKTemporal::DateTime* time);

    bool addTimes(const QList<SDKTemporal::DateTime*>& times);

    bool removeTime(SDKTemporal::DateTime* time);

    void resetTimeSpan();

    void getValueT(const std::vector<int> &dimensionIndexes, void* data) const;

    void getValueT(int idIndex, int timeIndex, void* data) const;

    void getValuesT(int idIndex, int timeIndex, int idStride, int timeStride, void* data) const;

    void setValueT(const std::vector<int> &dimensionIndexes, const void* data);

    void setValueT(int idIndex, int timeIndex, const void* data);

    void setValuesT(int idIndex, int timeIndex, int idStride, int timeStride, const void* data);

  protected:

    QStringList identifiersInternal() const;

    void clearIdentifiers();

    QList<SDKTemporal::DateTime*> timesInternal() const;

    SDKTemporal::DateTime* timeInternal(int timeIndex) const;

    int timeCountInternal() const;

    SDKTemporal::TimeSpan* timeSpanInternal() const;

    void clearTimes();


  private:
    QStringList  m_identifiers;
    QList<SDKTemporal::DateTime*> m_times;
    SDKTemporal::TimeSpan* m_timeSpan;
    QString m_id;
};


#endif // TIMESERIESIDBASEDCOMPONENTDATAITEM_H

