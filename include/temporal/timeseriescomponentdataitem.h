/*!
 * \file timeseriescomponentdataitem.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

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

