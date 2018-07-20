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
#include "spatial/linestring.h"
#include "spatial/edge.h"
#include "spatial/envelope.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCLineString::HCLineString(const QString &id, HCGeometry *parent)
  :HCGeometry(id, parent)
{
}

HCLineString::~HCLineString()
{
  for(HCPoint *point : m_points)
  {
    if(point != nullptr)
    {
      delete point;
    }
  }

  m_points.clear();
}

int HCLineString::dimension() const
{
  return 1;
}

bool HCLineString::isEmpty() const
{
  return m_points.length() == 0;
}

IGeometry::GeometryType HCLineString::geometryType() const
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
  return startPointInternal();
}

HCPoint *HCLineString::startPointInternal() const
{
  if(m_points.length())
  {
    return m_points[0];
  }

  return nullptr;
}

IPoint* HCLineString::endPoint() const
{
  return endPointInternal();
}

HCPoint *HCLineString::endPointInternal() const
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
    bool comp = m_points[0]->equals(m_points[m_points.length() - 1]);
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
        if(p1 != p2 && p1->equals(p2))
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

IPoint *HCLineString::point(int index) const
{
  return m_points[index];
}

HCPoint *HCLineString::pointInternal(int index) const
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

bool HCLineString::addPoint(HCPoint *point)
{
  assert(point != nullptr);
  assert(point->geometryFlags().testFlag(GeometryFlag::HasZ) == this->is3D());
  assert(point->geometryFlags().testFlag(GeometryFlag::HasM) == this->isMeasured());

  m_points.append(point);
  return true;
}

bool HCLineString::removePoint(HCPoint *point)
{
  bool removed = m_points.removeOne(point);
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

HCLine* HCLineString::toLine() const
{
  assert(m_points.length() == 2);

  HCLine* line = new HCLine();
  line->setP1(m_points[0]->clone());
  line->setP2(m_points[1]->clone());
  return line;

}

HCLinearRing* HCLineString::toLinearRing() const
{
  assert(isClosed() && isSimple());

  HCLinearRing* linearRing = new HCLinearRing();

  for(int i = 0 ; i < m_points.length() ; i++)
  {
    HCPoint* point = m_points[i]->clone();
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

Envelope *HCLineString::envelopeInternal() const
{
  m_envelope->resetExtentsToInfinity();

  for(HCPoint *point : m_points)
  {
    m_envelope->merge(point->envelopeInternal());
  }

  return m_envelope;
}

bool HCLineString::contains(const IGeometry *geom) const
{
  const IPoint *point = dynamic_cast<const IPoint*>(geom);

  if(point != nullptr && m_points.length() > 0)
  {
    int i, j, nvert = m_points.length();
    bool c = false;

    for(i = 0, j = nvert - 1; i < nvert; j = i++)
    {
      IPoint *pi = m_points[i];
      IPoint *pj = m_points[j];

      if(((pi->y() >= point->y()) != (pj->y() >= point->y())) &&
         (point->x() <= (pj->x() - pi->x()) * (point->y() - pi->y()) / (pj->y() - pi->y()) + pi->x())
         )
      {
        c = !c;
      }
    }

    return c;
  }

  return HCGeometry::contains(point);
}
