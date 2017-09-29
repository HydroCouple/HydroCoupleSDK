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

