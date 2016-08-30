#include "stdafx.h"
#include "spatial/geometrycollection.h"
#include "spatial/polygon.h"

#include "hydrocoupleexceptions.h"

#include <assert.h>

using namespace HydroCouple::Spatial;

HCMultiPolygon::HCMultiPolygon(QObject *parent)
  :HCGeometryCollection(parent)
{
}

HCMultiPolygon::~HCMultiPolygon()
{
}

HydroCouple::Spatial::GeometryType HCMultiPolygon::geometryType() const
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

IPolygon* HCMultiPolygon::element(int index) const
{
  assert(index < m_polygons.length());
  return m_polygons[index];
}

void HCMultiPolygon::addPolygon(HCPolygon *polygon)
{
  m_polygons.append(polygon);
  addGeometry(polygon);
}

bool HCMultiPolygon::removePolygon(HCPolygon *polygon)
{
  return m_polygons.removeOne(polygon) && removeGeometry(polygon);
}

