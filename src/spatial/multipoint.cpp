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
#include "spatial/point.h"
#include "spatial/geometrycollection.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCMultiPoint::HCMultiPoint(const QString &id,  HCGeometry *parent):
  HCGeometryCollection(id, parent)
{
}

HCMultiPoint::~HCMultiPoint()
{
  m_points.clear();
}

IGeometry::GeometryType HCMultiPoint::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::MultiPointZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return GeometryType::MultiPointZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::MultiPointM;
  }
  else
  {
    return GeometryType::MultiPoint;
  }
}

IPoint* HCMultiPoint::point(int index) const
{
  assert(index < geometries().length());
  return m_points[index];
}

HCPoint* HCMultiPoint::pointInternal(int index) const
{
  assert(index < geometries().length());
  return m_points[index];
}

bool HCMultiPoint::addGeometry(HCGeometry *geometry)
{
  HCPoint *point = dynamic_cast<HCPoint*>(geometry);

  return point != nullptr && addPoint(point);
}

bool HCMultiPoint::addPoint(HCPoint *point)
{
  if(HCGeometryCollection::addGeometry(point))
  {
    m_points.append(point);
    return true;
  }

  return false;
}

bool HCMultiPoint::removeGeometry(HCGeometry *geometry)
{
  HCPoint *point = dynamic_cast<HCPoint*>(geometry);
  return   point != nullptr && removePoint(point);
}

bool HCMultiPoint::removePoint(HCPoint *point)
{
 return HCGeometryCollection::removeGeometry(point) && m_points.removeOne(point);
}

