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
#include "spatial/geometrycollection.h"
#include "spatial/polygon.h"

#include "hydrocoupleexceptions.h"

#include <assert.h>

using namespace HydroCouple::Spatial;

HCMultiPolygon::HCMultiPolygon(const QString &id, HCGeometry *parent)
  :HCGeometryCollection(id, parent)
{
}

HCMultiPolygon::~HCMultiPolygon()
{
  m_polygons.clear();
}

IGeometry::GeometryType HCMultiPolygon::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::MultiPolygonZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return GeometryType::MultiPolygonZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::MultiPolygonM;
  }
  else
  {
    return GeometryType::MultiPolygon;
  }
}

double HCMultiPolygon::area() const
{
  double area = 0;

  for(HCPolygon *polygon : m_polygons)
  {
    area += polygon->area();
  }

  return area;
}

IPoint *HCMultiPolygon::centroid() const
{
  throw NotImplementedException("HCMultiPolygon","centroid");

  return nullptr;
}

IPoint *HCMultiPolygon::pointOnSurface() const
{
  throw NotImplementedException("HCMultiPolygon","pointOnSurface");

  return nullptr;
}

IPolygon* HCMultiPolygon::polygon(int index) const
{
  assert(index < m_polygons.length());
  return m_polygons[index];
}

HCPolygon* HCMultiPolygon::polygonInternal(int index) const
{
  assert(index < m_polygons.length());
  return m_polygons[index];
}


bool HCMultiPolygon::addGeometry(HCGeometry *geometry)
{
  HCPolygon *polygon = dynamic_cast<HCPolygon*>(geometry);

  return polygon != nullptr && addPolygon(polygon);
}


bool HCMultiPolygon::addPolygon(HCPolygon *polygon)
{
  if(HCGeometryCollection::addGeometry(polygon))
  {
    m_polygons.append(polygon);
    return true;
  }

  return false;
}

bool HCMultiPolygon::removeGeometry(HCGeometry *geometry)
{
  HCPolygon *polygon = dynamic_cast<HCPolygon*>(geometry);
  return   polygon != nullptr && removePolygon(polygon);
}

bool HCMultiPolygon::removePolygon(HCPolygon *polygon)
{
  return HCGeometryCollection::removeGeometry(polygon) && m_polygons.removeOne(polygon);
}

