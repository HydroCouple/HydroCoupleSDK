/*!
 * \file timegeometrycomponentdataitem.h
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

#ifndef TIMEGEOMETRYCOMPONENTDATAITEM_H
#define TIMEGEOMETRYCOMPONENTDATAITEM_H

#include "hydrocouplespatiotemporal.h"
#include "hydrocouplesdk.h"
#include "core/datacursor.h"

#include <vector>

namespace SDKTemporal
{
  class DateTime;
  class TimeSpan;
}

class HCGeometry;
class Envelope;

template<class T>
class HYDROCOUPLESDK_EXPORT TimeGeometryComponentDataItem
{
  public:

    TimeGeometryComponentDataItem(const QString &id,
                                  HydroCouple::Spatial::IGeometry::GeometryType geometryType,
                                  const T &defaultValue);

    virtual ~TimeGeometryComponentDataItem();

    virtual QString getId() const;

    bool addTime(SDKTemporal::DateTime* time);

    bool addTimes(const QList<SDKTemporal::DateTime*> &times);

    bool removeTime(SDKTemporal::DateTime *time);

    void resetTimeSpan();

    void addGeometry(const QSharedPointer<HCGeometry> &geometry);

    void addGeometries(const QList<QSharedPointer<HCGeometry>> &geometries);

    bool removeGeometry(const QSharedPointer<HCGeometry> &geometry);

    void getValuesT(int timeIndex, int geomIndex,
                    int timeStride, int geomStride, void *data) const;

    void setValuesT(int timeIndex, int geomIndex,
                    int timeStride, int geomStride, const void *data);

    bool moveData(int fromTimeIndex, int toTimeIndex);

    void moveDataToPrevTime();

    T &operator()(int timeIndex, int geometryIndex);

    const T &operator ()(int timeIndex, int geometryIndex) const;

    HydroCouple::Spatial::IGeometry::GeometryType getGeometryType() const;

    HCGeometry *getGeometry(int index) const;

    int getGeometryCount() const;

    SDKTemporal::DateTime *getDateTime(int timeIndex) const;

    int getDateTimeCount() const;

    DataCursor *timeCursor() const;

    void resetExtents();

  protected:

    std::vector<SDKTemporal::DateTime*> timesInternal() const;

    SDKTemporal::DateTime* timeInternal(int timeIndex) const;

    SDKTemporal::TimeSpan* timeSpanInternal() const;

    std::vector<QSharedPointer<HCGeometry>> geometriesInternal() const;

    Envelope *envelopeInternal() const;

    void clearGeometries();

    void clearTimes();

  protected:

    std::vector<QSharedPointer<HCGeometry>> m_geometries;

    std::vector<SDKTemporal::DateTime*> m_times;

  private:

    DataCursor *m_timeCursor;
    SDKTemporal::TimeSpan *m_timeSpan;
    std::vector<std::vector<T>> m_data;
    HydroCouple::Spatial::IGeometry::GeometryType m_geometryType;
    Envelope *m_envelope;
    T m_defaultValue;
    QString m_id;
};

#endif // TIMEGEOMETRYCOMPONENTDATAITEM_H
