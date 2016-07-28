#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/linestring.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCLineString::HCLineString(QObject *parent)
  :HCGeometry(parent)
{
}

HCLineString::~HCLineString()
{
}

int HCLineString::dimension() const
{
  return 1;
}

HydroCouple::Spatial::GeometryType HCLineString::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::LineStringZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return GeometryType::LineStringZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::LineStringM;
  }
  else
  {
    return GeometryType::LineString;
  }
}

IGeometry* HCLineString::envelope() const
{
  return nullptr;
}

unsigned char* HCLineString::wkb(int &size) const
{
  return 0;
}

double HCLineString::length() const
{
  double outLength = 0;

  if(m_points.length() > 1)
  {
    for(int i = 1 ; i < m_points.length() ; i++)
    {
      outLength += m_points[i]->dist(m_points[i-1]);
    }
  }
  return outLength ;
}

IPoint* HCLineString::startPoint() const
{
  if(m_points.length())
  {
    return m_points[0];
  }

  return nullptr;
}

IPoint* HCLineString::endPoint() const
{
  if(m_points.length())
  {
    return m_points[m_points.length() - 1];
  }

  return nullptr;
}

bool HCLineString::isClosed() const
{
  if(m_points.length())
  {
    bool comp = m_points[0]->compare(m_points[m_points.length() - 1]);
    return comp;
  }

  return false;
}

bool HCLineString::isRing() const
{
  int count = 0;

  for(HCPoint* p1 : m_points)
  {
    for(HCPoint* p2 : m_points)
    {
      if(p1 != p2 && p1->compare(p2))
      {
        count++;
      }
    }
  }

  if(count > 1)
  {
    return false;
  }
  else
  {
    return true;
  }
}

int HCLineString::pointCount() const
{
  return m_points.length();
}

IPoint* HCLineString::point(int index) const
{
  return m_points[index];
}

void HCLineString::enable3D()
{
  for(HCPoint* p : m_points)
  {
    p->setGeometryFlag(GeometryFlag::HasZ,true);
  }
}

void HCLineString::disable3D()
{
  for(HCPoint* p : m_points)
  {
    p->setGeometryFlag(GeometryFlag::HasZ,false);
  }
}

void HCLineString::enableM()
{
  for(HCPoint* p : m_points)
  {
    p->setGeometryFlag(GeometryFlag::HasM,true);
  }
}

void HCLineString::disableM()
{
  for(HCPoint* p : m_points)
  {
    p->setGeometryFlag(GeometryFlag::HasM,false);
  }
}

QList<HCPoint*> HCLineString::points() const
{
  return m_points;
}

void HCLineString::addPoint(HCPoint *point)
{
  if(point)
  {
    point->setGeometryFlag(GeometryFlag::HasZ , this->is3D());
    point->setGeometryFlag(GeometryFlag::HasM , this->isMeasured());
  }

  m_points.append(point);
}

bool HCLineString::removePoint(HCPoint *point)
{
  return m_points.removeAll(point);
}
