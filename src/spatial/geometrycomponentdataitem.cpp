/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
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


#include "stdafx.h"
#include "spatial/geometrycomponentdataitem.h"
#include "spatial/geometry.h"
#include "spatial/envelope.h"

#include <QDateTime>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

template<class T>
GeometryComponentDataItem<T>::GeometryComponentDataItem(const QString &id,
                                                        IGeometry::GeometryType geometryType,
                                                        const T &defaultValue)
  : ComponentDataItem1D<T>(id, 0,defaultValue),
    m_geometryType(geometryType)
{
  m_envelope = new Envelope();
}

template<class T>
GeometryComponentDataItem<T>::~GeometryComponentDataItem()
{
  delete m_envelope;
  m_geometries.clear();
}

template<class T>
void GeometryComponentDataItem<T>::addGeometry(const QSharedPointer<HCGeometry> &geometry)
{
  if(geometry->geometryType() == m_geometryType)
  {
    m_geometries.append(geometry);
    ComponentDataItem1D<T>::resizeDataArray(m_geometries.length());
    resetExtents();
  }
}

template<class T>
void GeometryComponentDataItem<T>::addGeometries(const QList<QSharedPointer<HCGeometry> > &geometries)
{
  for(QSharedPointer<HCGeometry> geometry :geometries)
  {
    if(geometry->geometryType() == m_geometryType)
      m_geometries.append(geometry);
  }

  ComponentDataItem1D<T>::resizeDataArray(m_geometries.length());
  resetExtents();
}

template<class T>
bool GeometryComponentDataItem<T>::removeGeometry(const QSharedPointer<HCGeometry> &geometry)
{
  int index  = m_geometries.indexOf(geometry);

  if(index >= 0 )
  {
    m_geometries.removeAt(index);
    ComponentDataItem1D<T>::removeItemAt(index);
    resetExtents();

    return true;
  }

  return false;
}

template<class T>
IGeometry::GeometryType GeometryComponentDataItem<T>::getGeometryType() const
{
  return m_geometryType;
}

template<class T>
HCGeometry *GeometryComponentDataItem<T>::getGeometry(int index) const
{
  return m_geometries[index].data();
}

template<class T>
int GeometryComponentDataItem<T>::getGeometryCount() const
{
  return m_geometries.length();
}


template<class T>
QList<QSharedPointer<HCGeometry>> GeometryComponentDataItem<T>::geometriesInternal() const
{
  return m_geometries;
}

template<class T>
Envelope *GeometryComponentDataItem<T>::envelopeInternal() const
{
  return m_envelope;
}

template<class T>
void GeometryComponentDataItem<T>::resetExtents()
{
  m_envelope->resetExtentsToInfinity();

  for(QSharedPointer<HCGeometry> &geometry : m_geometries)
  {
    m_envelope->merge(geometry->envelope());
  }
}

template class HYDROCOUPLESDK_EXPORT GeometryComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT GeometryComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT GeometryComponentDataItem<QString>;
template class HYDROCOUPLESDK_EXPORT GeometryComponentDataItem<QDateTime>;
