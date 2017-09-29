#ifndef TIMESERIESCOMPONENTDATAITEM_H
#define TIMESERIESCOMPONENTDATAITEM_H

#include "hydrocoupletemporal.h"
#include "core/exchangeitems1d.h"
#include "core/abstractcomponentdataitem.h"
#include "core/datacursor.h"

namespace SDKTemporal
{
  class DateTime;
  class TimeSpan;
}

template<class T>
class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem
{
  public:

    TimeSeriesComponentDataItem(const QString &id, const T& defaultValue, int initialSize);

    TimeSeriesComponentDataItem(const QString &id, const std::vector<SDKTemporal::DateTime*>& times,
                                const T& defaultValue);

    TimeSeriesComponentDataItem(const QString &id, const std::list<SDKTemporal::DateTime*>& times,
                                const T& defaultValue);

    virtual ~TimeSeriesComponentDataItem();

    virtual QString getId() const;

    bool addTime(SDKTemporal::DateTime* time);

    bool addTimes(const QList<SDKTemporal::DateTime*>& times);

    bool removeTime(SDKTemporal::DateTime *time);

    void getValueT(const std::vector<int> &dimensionIndexes, void *data) const;

    void getValueT(int timeIndex, void *data) const;

    void getValuesT(int timeIndex, int stride, void *data) const;

    void setValueT(const std::vector<int> &dimensionIndexes, const void *data);

    void setValueT(int timeIndex, const void *data);

    void setValuesT(int timeIndex, int stride, const void *data);

    int length() const;

    void moveDataToPrevTime();

    T& operator[](int index);

    const T& operator [](int index) const;

    DataCursor *timeCursor() const;

  protected:

    std::vector<SDKTemporal::DateTime*> timesInternal() const;

    SDKTemporal::DateTime *timeInternal(int timeIndex) const;

    int timeCountInternal() const;

    SDKTemporal::TimeSpan* timeSpanInternal() const;

    void clearTimes();

  private:

    DataCursor *m_timeCursor;
    std::vector<SDKTemporal::DateTime*> m_times;
    std::vector<T> m_data;
    SDKTemporal::TimeSpan *m_timeSpan;
    T m_defaultValue;
    QString m_id;
};


#endif // TIMESERIESCOMPONENTDATAITEM_H

