/*!
 * \file geometrycomponentdataitem.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
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

#ifndef GEOMETRYCOMPONENTDATAITEM_H
#define GEOMETRYCOMPONENTDATAITEM_H

#include "hydrocouplespatial.h"
#include "core/componentdataitem.h"

class HCGeometry;
class Envelope;

template<class T>
class HYDROCOUPLESDK_EXPORT GeometryComponentDataItem:
    public ComponentDataItem1D<T>
{
  public:

    GeometryComponentDataItem(const QString &id,
                              HydroCouple::Spatial::IGeometry::GeometryType geometryType,
                              const T &defaultValue);

    virtual ~GeometryComponentDataItem();

    void addGeometry(const QSharedPointer<HCGeometry> &geometry);

    void addGeometries(const QList<QSharedPointer<HCGeometry>> &geometries);

    bool removeGeometry(const QSharedPointer<HCGeometry> &geometry);

    HydroCouple::Spatial::IGeometry::GeometryType getGeometryType() const;

    HCGeometry *getGeometry(int index) const;

    int getGeometryCount() const;

  protected:

    QList<QSharedPointer<HCGeometry>> geometriesInternal() const;

    Envelope *envelopeInternal() const;

  private:

    void resetExtents();

  protected:

    QList<QSharedPointer<HCGeometry>> m_geometries;

  private:

    HydroCouple::Spatial::IGeometry::GeometryType m_geometryType;
    std::vector<T> m_data;
    Envelope *m_envelope;
};


#endif // GEOMETRYCOMPONENTDATAITEM_H
