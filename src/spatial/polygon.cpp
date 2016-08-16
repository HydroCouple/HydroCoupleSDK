#include "stdafx.h"
#include "spatial/polygon.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/linestring.h"
#include "spatial/edge.h"
#include "spatial/point.h"

#include <math.h>
#include <assert.h>
#include <qdebug.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

unsigned int HCPolygon::m_nextId(0);

HCPolygon::HCPolygon(HCPolyhedralSurface *parent)
  : HCGeometry(parent),
    m_edge(nullptr),
    m_polyhedralSurface(parent)
{
  m_exteriorRing = new HCLineString(this);
  m_polyhedralSurface->addPatch(this);
  setIndex(HCPolygon::getNextId());
}

HCPolygon::HCPolygon(QObject *parent)
  : HCGeometry(parent),
    m_edge(nullptr),
    m_polyhedralSurface(nullptr)
{
  m_exteriorRing = new HCLineString(this);
  setIndex(HCPolygon::getNextId());
}

HCPolygon::~HCPolygon()
{
  if(m_polyhedralSurface)
  {
    HCEdge* edge = m_edge;

    QList<HCVertex*> vertices;
    QList<HCEdge*> edgesToDelete;

    if(edge != nullptr)
    {
      do
      {
        if(edge->left() == this)
        {
          if(edge->right() == nullptr)
          {
            vertices.append(dynamic_cast<HCVertex*>(edge->orig()));
            edgesToDelete.append(edge);
          }
          else
          {
            edge->setLeft(nullptr);
          }
        }

        edge =  dynamic_cast<HCEdge*>(edge->leftNext());

      }while(edge != m_edge);

      for(HCEdge* temp : edgesToDelete)
      {
        HCPolyhedralSurface::printEdge(temp);
        HCEdge::deleteEdge(temp);
      }

      for(HCVertex *vertex : vertices)
      {
        if(vertex->edge() == nullptr)
        {
          delete vertex;
        }
      }
    }

    m_polyhedralSurface->removePatch(this);
  }
}

int HCPolygon::dimension() const
{
  return 2;
}

GeometryType HCPolygon::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return HydroCouple::Spatial::PolygonZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return HydroCouple::Spatial::PolygonZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return HydroCouple::Spatial::PolygonM;
  }
  else
  {
    return  HydroCouple::Spatial::Polygon;
  }
}

HydroCouple::Spatial::IGeometry* HCPolygon::envelope() const
{
  return nullptr;
}

double HCPolygon::area() const
{
  if(m_exteriorRing->isEmpty() || !m_exteriorRing->isClosed())
    return 0;

  double areaOut = m_exteriorRing->area();

  for(HCLineString *intLineString : m_interiorRings)
  {
    areaOut += intLineString->area();
  }

  return fabs(areaOut);
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
  return m_exteriorRing;
}

HCLineString *HCPolygon::exteriorRingInternal() const
{
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

bool HCPolygon::addInteriorRing(HCLineString* interiorRing)
{
  assert(interiorRing->isClosed());

  if(!m_interiorRings.contains(interiorRing))
  {
    m_interiorRings.append(interiorRing);
    return true;
  }
  return false;
}

bool HCPolygon::removeInteriorRing(HCLineString* interiorRing)
{
  return m_interiorRings.removeAll(interiorRing);
}

HydroCouple::Spatial::IEdge* HCPolygon::edge() const
{
  return m_edge;
}

HydroCouple::Spatial::IPolyhedralSurface* HCPolygon::polyhydralSurface() const
{
  return m_polyhedralSurface;
}

HCPolyhedralSurface* HCPolygon::polyhydralSurfaceInternal() const
{
  return m_polyhedralSurface;
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

  setGeometryFlag(GeometryFlag::HasZ , true);
}

void HCPolygon::disable3D()
{
  m_exteriorRing->disable3D();

  for(HCLineString *lstrng : m_interiorRings)
    lstrng->disable3D();

  setGeometryFlag(GeometryFlag::HasZ , false);
}

void HCPolygon::enableM()
{
  m_exteriorRing->enableM();

  for(HCLineString *lstrng : m_interiorRings)
    lstrng->enableM();

  setGeometryFlag(GeometryFlag::HasM , true);
}

void HCPolygon::disableM()
{
  m_exteriorRing->disableM();

  for(HCLineString *lstrng : m_interiorRings)
    lstrng->disableM();

  setGeometryFlag(GeometryFlag::HasM , false);
}

void HCPolygon::setGeometryFlag(GeometryFlag flag, bool on)
{
  m_exteriorRing->setGeometryFlag(flag,on);

  for(HCLineString *lstrng : m_interiorRings)
    lstrng->setGeometryFlag(flag,on);

  setGeometryFlag(flag, on);
}

unsigned int HCPolygon::getNextId()
{
  unsigned int tid = m_nextId;
  m_nextId++;
  return tid;
}

void HCPolygon::addEdge(HCEdge *edge)
{
  assert(edge != nullptr);
  m_edge = edge;
  reCreateOuterLineString();
}

void HCPolygon::removeEdge(HCEdge *edge)
{
  assert(edge!=nullptr);

  // replace the arbitrary edge with another edge in the orbit
  // use null if this is the only edge
  // assumes that the edge hasn't been actually removed yet

  HCEdge *next = dynamic_cast<HCEdge*>(edge->leftNext());
  m_edge = next != edge ? next : nullptr;

  //  edge->setLeft(nullptr);

  reCreateOuterLineString();
}

void HCPolygon::reCreateOuterLineString()
{
  m_exteriorRing->m_points.clear();

  IEdge* start = m_edge;

  if(start)
  {
    assert(start->orig());

    m_exteriorRing->m_points.append(dynamic_cast<HCVertex*>(start->orig()));
    do
    {
      start = start->leftNext();

      assert(start->orig());

      m_exteriorRing->m_points.append(dynamic_cast<HCVertex*>(start->orig()));

    }while(start != m_edge);

  }
}
