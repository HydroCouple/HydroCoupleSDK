#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/linestring.h"
#include "spatial/edge.h"

#include <assert.h>

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

double HCLineString::length() const
{
  double outLength = 0;

  if(m_points.length() > 1)
  {
    for(int i = 1 ; i < m_points.length() ; i++)
    {
      outLength += HCPoint::dist(m_points[i], m_points[i-1]);
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

  return true;
}

bool HCLineString::isRing() const
{
  if(isClosed())
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
  else
  {
    return false;
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

  HCGeometry::setGeometryFlag(GeometryFlag::HasZ,true);
}

void HCLineString::disable3D()
{
  for(HCPoint* p : m_points)
  {
    p->setGeometryFlag(GeometryFlag::HasZ,false);
  }

  HCGeometry::setGeometryFlag(GeometryFlag::HasZ,false);
}

void HCLineString::enableM()
{
  for(HCPoint* p : m_points)
  {
    p->setGeometryFlag(GeometryFlag::HasM,true);
  }

  HCGeometry::setGeometryFlag(GeometryFlag::HasM,true);
}

void HCLineString::disableM()
{
  for(HCPoint* p : m_points)
  {
    p->setGeometryFlag(GeometryFlag::HasM,false);
  }

  HCGeometry::setGeometryFlag(GeometryFlag::HasM,false);
}

QList<HCPoint*> HCLineString::points() const
{
  return m_points;
}

void HCLineString::addPoint(HCPoint *point)
{
  assert(point != nullptr);

  assert(point->geometryFlags().testFlag(GeometryFlag::HasZ) == this->is3D());
  assert(point->geometryFlags().testFlag(GeometryFlag::HasM) == this->isMeasured());

  m_points.append(point);
  setIsEmpty(m_points.length() == 0);
}

bool HCLineString::removePoint(HCPoint *point)
{
  bool removed = m_points.removeOne(point);
  setIsEmpty(m_points.length() == 0);

  return removed;
}

void HCLineString::setGeometryFlag(GeometryFlag flag, bool on)
{
  for(HCPoint *point : m_points)
  {
    if(point)
    {
      point->setGeometryFlag(flag,on);
    }
  }

  HCGeometry::setGeometryFlag(flag,on);
}

HCLine* HCLineString::toLine(QObject *parent) const
{
  assert(m_points.length() == 2);

  HCLine* line = new HCLine(parent);
  line->setP1(m_points[0]->clone(line));
  line->setP2(m_points[1]->clone(line));
  return line;

}

HCLinearRing* HCLineString::toLinearRing(QObject *parent) const
{
  assert(isClosed() && isSimple());

  HCLinearRing* linearRing = new HCLinearRing(parent);

  for(int i = 0 ; i < m_points.length() ; i++)
  {
    HCPoint* point = m_points[i]->clone(linearRing);
    linearRing->addPoint(point);
  }

  return linearRing;
}

double HCLineString::area() const
{
  double outArea = 0;

  if(m_points.length() > 2)
  {
    for(int i=1; i < m_points.length(); i++)
    {
      HCPoint * p1 = m_points[i-1];
      HCPoint * p2 = m_points[i];

      outArea +=  (p1->x()*p2->y()) - (p2->x()*p1->y());
    }
    outArea = outArea * 2.0;
  }

  return outArea;
}

bool HCLineString::isClockWise() const
{
  if(area() < 0)
  {
    return false;
  }

  return true;
}

void HCLineString::flip()
{
  QList<HCPoint*> flipped;

  if(m_points.length() > 1)
  {
    for(int i = m_points.length() - 1 ; i > - 1 ; i--)
    {
      flipped.append(m_points[i]);
    }
  }
}
