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
  return m_polygons.removeAll(polygon) && removeGeometry(polygon);
}


void HCMultiPolygon::enable3D()
{
  for(HCPolygon* polygon : m_polygons)
  {
    polygon->enable3D();
  }
  setGeometryFlag(GeometryFlag::HasZ , true);
}

void HCMultiPolygon::disable3D()
{
  for(HCPolygon* polygon : m_polygons)
  {
    polygon->disable3D();
  }
  setGeometryFlag(GeometryFlag::HasZ , false);
}

void HCMultiPolygon::enableM()
{
  for(HCPolygon* polygon : m_polygons)
  {
    polygon->enableM();
  }
  setGeometryFlag(GeometryFlag::HasM , true);
}

void HCMultiPolygon::disableM()
{
  for(HCPolygon* polygon : m_polygons)
  {
    polygon->disableM();
  }
  setGeometryFlag(GeometryFlag::HasM , false);
}
