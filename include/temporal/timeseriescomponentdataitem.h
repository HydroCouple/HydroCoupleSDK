#ifndef TIMESERIESCOMPONENTDATAITEM_H
#define TIMESERIESCOMPONENTDATAITEM_H

#include "hydrocoupletemporal.h"
#include "core/exchangeitems1d.h"
#include "core/abstractcomponentdataitem.h"

namespace SDKTemporal
{
  class Time;
  class TimeSpan;
}

template<class T>
class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem :
    public ComponentDataItem1D<T>
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


#endif // TIMESERIESCOMPONENTDATAITEM_H

