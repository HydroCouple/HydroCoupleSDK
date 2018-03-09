/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
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
#include "spatial/polygon.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/linestring.h"
#include "spatial/edge.h"
#include "spatial/point.h"
#include "spatial/envelope.h"

#include <math.h>
#include <set>
#include <assert.h>
#include <qdebug.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

unsigned int HCPolygon::m_nextId(0);


HCPolygon::HCPolygon(const QString &id, HCGeometry *parent)
  : HCGeometry(id, parent),
    m_polyhedralSurface(nullptr),
    m_edge(nullptr)
{
  m_exteriorRing = new HCLineString(QUuid::createUuid().toString(), this);
  setIndex(HCPolygon::getNextId());
}

HCPolygon::HCPolygon(const QString &id, HCPolyhedralSurface *parent)
  : HCGeometry(id, parent),
    m_polyhedralSurface(parent),
    m_edge(nullptr)
{
  m_exteriorRing = new HCLineString(QUuid::createUuid().toString(), this);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addPatch(this);
  setIndex(HCPolygon::getNextId());
}

HCPolygon::~HCPolygon()
{
  if(m_polyhedralSurface)
  {

    if(m_edge != nullptr)
    {
      std::list<Edge*> edgesToDelete;

      Edge *edge = m_edge;

      do
      {
        if(edge->rightInternal() == nullptr)
          edgesToDelete.push_back(edge);

        edge->setLeft(nullptr);

        edge = edge->leftNextInternal();

      }while(edge != m_edge);

      for(Edge *dEdge : edgesToDelete)
      {
        HCVertex  *origin = dEdge->origInternal();
        HCVertex  *dest = dEdge->destInternal();

        Edge::deleteEdge(dEdge);

        if(origin != nullptr && origin->edgeInternal() == nullptr)
        {
          delete origin;
        }

        if(dest != nullptr && dest != origin && dest->edgeInternal() == nullptr)
        {
          delete dest;
        }
      }
    }

    m_polyhedralSurface->removePatch(this);
    m_exteriorRing->m_points.clear();
  }

  delete m_exteriorRing;

  qDeleteAll(m_interiorRings);
  m_interiorRings.clear();

}

int HCPolygon::dimension() const
{
  return 2;
}

IGeometry::GeometryType HCPolygon::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return IGeometry::PolygonZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return IGeometry::PolygonZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return IGeometry::PolygonM;
  }
  else
  {
    return IGeometry::Polygon;
  }
}

Envelope *HCPolygon::envelopeInternal() const
{
  return exteriorRingInternal()->envelopeInternal();
}

double HCPolygon::area() const
{
  //  if(m_exteriorRing->isEmpty() || !m_exteriorRing->isClosed())
  //    return 0;

  double areaOut = m_exteriorRing->area();

  for(HCLineString *intLineString : m_interiorRings)
  {
    areaOut += intLineString->area();
  }

  return fabs(areaOut);
}

double HCPolygon::area3D() const
{
  //triangularte and use the 3d area calculation
  return 0;
}

IPoint *HCPolygon::centroid() const
{
  if(m_exteriorRing->m_points.length() > 1)
  {
    double x = 0;
    double y = 0;

    for(int i = 1; i < m_exteriorRing->m_points.length() ; i++)
    {
      HCPoint *p1 = m_exteriorRing->m_points[i-1];
      HCPoint *p2 = m_exteriorRing->m_points[i];

      double multip = (p1->x()* p2->y() - p2->x()*p1->y());

      x += (p1->x()+ p2->x()) * multip;
      y += (p1->y()+ p2->y()) * multip;

    }

    double tarea = fabs(m_exteriorRing->area());

    x = x/(6.0*tarea);
    y = y/(6.0*tarea);

    HCPoint *outPoint = new HCPoint(x , y);
    return outPoint;

  }
  else if(m_exteriorRing->m_points.length() == 1)
  {
    HCPoint *p = m_exteriorRing->m_points[0];
    HCPoint *outPoint = new HCPoint(p->x() , p->y());
    return outPoint;
  }

  return nullptr;
}

IPoint *HCPolygon::pointOnSurface() const
{
  return nullptr;
}

IMultiCurve *HCPolygon::boundaryMultiCurve() const
{
  return nullptr;
}

ILineString *HCPolygon::exteriorRing() const
{
  reCreateOuterLineString();
  return m_exteriorRing;
}

HCLineString *HCPolygon::exteriorRingInternal() const
{
  reCreateOuterLineString();
  return m_exteriorRing;
}

int HCPolygon::interiorRingCount() const
{
  return m_interiorRings.length();
}

ILineString *HCPolygon::interiorRing(int index) const
{
  return m_interiorRings[index];
}

HCLineString *HCPolygon::interiorRingInternal(int index) const
{
  return m_interiorRings[index];
}

QList<HCLineString*> HCPolygon::interiorRingsInternal() const
{
  return m_interiorRings;
}

bool HCPolygon::addInteriorRing(HCLineString* interiorRing)
{
  assert(interiorRing->isClosed());

  assert(interiorRing->geometryFlags().testFlag(GeometryFlag::HasZ) == this->is3D());
  assert(interiorRing->geometryFlags().testFlag(GeometryFlag::HasM) == this->isMeasured());

  if(!m_interiorRings.contains(interiorRing))
  {
    m_interiorRings.append(interiorRing);
    return true;
  }

  return false;
}

bool HCPolygon::removeInteriorRing(HCLineString* interiorRing)
{
  return m_interiorRings.removeOne(interiorRing);
}

HydroCouple::Spatial::IEdge* HCPolygon::edge() const
{
  return m_edge;
}

Edge *HCPolygon::edgeInternal() const
{
  return m_edge;
}

Edge *HCPolygon::getEdge(int index) const
{
  Edge* edge = m_edge;

  int i = 0;

  do
  {
    if(i == index)
    {
      return edge;
    }

    edge = edge->leftNextInternal();
    i++;

  }while(edge != m_edge);

  return nullptr;
}

void HCPolygon::moveCurrentEdgeToNext()
{
  if(m_edge != nullptr)
  {
    m_edge = m_edge->leftNextInternal();
  }
}

HydroCouple::Spatial::IPolyhedralSurface* HCPolygon::polyhydralSurface() const
{
  return m_polyhedralSurface;
}

HCPolyhedralSurface* HCPolygon::polyhydralSurfaceInternal() const
{
  return m_polyhedralSurface;
}

bool HCPolygon::contains(const HydroCouple::Spatial::IGeometry *geom) const
{
  const IPoint *point = dynamic_cast<const IPoint*>(geom);

  if(point != nullptr)
  {
    bool c = m_exteriorRing->contains(point);

    if(c && m_interiorRings.length())
    {
      for(HCLineString *lineString : m_interiorRings)
      {
        if(lineString->contains(point))
          return false;
      }
    }

    return c;
  }

  return HCGeometry::contains(point);

}

bool HCPolygon::isConvex() const
{
  return false;
}

void HCPolygon::enable3D()
{
  m_exteriorRing->enable3D();

  for(HCLineString *lstrng : m_interiorRings)
    lstrng->enable3D();

  HCGeometry::setGeometryFlag(GeometryFlag::HasZ , true);
}

void HCPolygon::disable3D()
{
  m_exteriorRing->disable3D();

  for(HCLineString *lstrng : m_interiorRings)
    lstrng->disable3D();

  HCGeometry::setGeometryFlag(GeometryFlag::HasZ , false);
}

void HCPolygon::enableM()
{
  m_exteriorRing->enableM();

  for(HCLineString *lstrng : m_interiorRings)
    lstrng->enableM();

  HCGeometry::setGeometryFlag(GeometryFlag::HasM , true);
}

void HCPolygon::disableM()
{
  m_exteriorRing->disableM();

  for(HCLineString *lstrng : m_interiorRings)
    lstrng->disableM();

  HCGeometry::setGeometryFlag(GeometryFlag::HasM , false);
}

void HCPolygon::setGeometryFlag(GeometryFlag flag, bool on)
{
  m_exteriorRing->setGeometryFlag(flag,on);

  for(HCLineString *lstrng : m_interiorRings)
    lstrng->setGeometryFlag(flag,on);

  HCGeometry::setGeometryFlag(flag, on);
}

unsigned int HCPolygon::getNextId()
{
  unsigned int tid = m_nextId;
  m_nextId++;
  return tid;
}

void HCPolygon::addEdge(Edge *edge)
{
  assert(edge != nullptr);
  m_edge = edge;
}

void HCPolygon::removeEdge(Edge *edge)
{
  assert(edge!=nullptr);

  // replace the arbitrary edge with another edge in the orbit
  // use null if this is the only edge
  // assumes that the edge hasn't been actually removed yet

  Edge *next = edge->leftNextInternal();
  m_edge = next != edge ? next : nullptr;

  //  edge->setLeft(nullptr);

}

void HCPolygon::reCreateOuterLineString() const
{
  if(m_polyhedralSurface && m_edge)
  {
    m_exteriorRing->m_points.clear();

    Edge* start = m_edge;

    if(start)
    {
      assert(start->origInternal());

      m_exteriorRing->setIsEmpty(false);
      m_exteriorRing->m_points.append(start->origInternal());

      do
      {
        start = start->leftNextInternal();
        m_exteriorRing->m_points.append(start->origInternal());
        assert(start->origInternal());

      }while(start != m_edge);
    }
  }
}
