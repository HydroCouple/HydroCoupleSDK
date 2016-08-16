#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/geometrycollection.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCMultiPoint::HCMultiPoint(QObject *parent):
  HCGeometryCollection(parent)
{
}

HCMultiPoint::~HCMultiPoint()
{
}

HydroCouple::Spatial::GeometryType HCMultiPoint::geometryType() const
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
  return dynamic_cast<IPoint*>(geometry(index));
}

void HCMultiPoint::addPoint(HCPoint *point)
{
  addGeometry(point);
}

bool HCMultiPoint::removePoint(HCPoint *point)
{
  return removeGeometry(point);
}

void HCMultiPoint::enable3D()
{
  for(HCGeometry* geom : geometries())
  {
    HCPoint* point = dynamic_cast<HCPoint*>(geom);
    point->setGeometryFlag(GeometryFlag::HasZ , true);
  }
  setGeometryFlag(GeometryFlag::HasZ , true);
}

void HCMultiPoint::disable3D()
{
  for(HCGeometry* geom : geometries())
  {
    HCPoint* point = dynamic_cast<HCPoint*>(geom);
    point->setGeometryFlag(GeometryFlag::HasZ , false);
  }
  setGeometryFlag(GeometryFlag::HasZ , false);
}

void HCMultiPoint::enableM()
{
  for(HCGeometry* geom : geometries())
  {
    HCPoint* point = dynamic_cast<HCPoint*>(geom);
    point->setGeometryFlag(GeometryFlag::HasM , true);
  }
  setGeometryFlag(GeometryFlag::HasM , true);
}

void HCMultiPoint::disableM()
{
  for(HCGeometry* geom : geometries())
  {
    HCPoint* point = dynamic_cast<HCPoint*>(geom);
    point->setGeometryFlag(GeometryFlag::HasM , false);
  }
  setGeometryFlag(GeometryFlag::HasM , false);
}
