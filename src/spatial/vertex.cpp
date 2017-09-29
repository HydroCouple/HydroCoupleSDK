#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/edge.h"
#include "spatial/polygon.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCVertex::HCVertex(const QString &id, HCGeometry *parent):
  HCPoint(id, parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr)
{
}

HCVertex::HCVertex(bool hasZ, bool hasM, const QString &id, HCGeometry *parent):
  HCPoint(hasZ, hasM,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr)
{
}

HCVertex::HCVertex(const QString &id, HCPolyhedralSurface* parent):
  HCPoint(id, parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}


HCVertex::HCVertex(double x, double y, const QString &id, HCGeometry *parent):
  HCPoint(x,y,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr)
{
}

HCVertex::HCVertex(double x, double y, const QString &id, HCPolyhedralSurface* parent):
  HCPoint(x,y,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::HCVertex(double x, double y, double z, const QString &id, HCGeometry *parent):
  HCPoint(x,y,z,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr)
{
}

HCVertex::HCVertex(double x, double y, double z, const QString &id, HCPolyhedralSurface* parent):
  HCPoint(x,y,z,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::HCVertex(double x, double y, double z, double m, const QString &id, HCGeometry *parent):
  HCPoint(x,y,z,m,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr)
{
}

HCVertex::HCVertex(double x, double y, double z, double m, const QString &id, HCPolyhedralSurface* parent):
  HCPoint(x,y,z,m,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::~HCVertex()
{
  if(m_polyhedralSurface)
  {
    if(m_edge != nullptr)
    {
      Edge *edge = m_edge;
      std::list<HCPolygon*> polygons;

      do
      {
        if(edge->leftInternal())
          polygons.push_back(edge->leftInternal());

        edge->setOrig(nullptr);

        edge = edge->origNextInternal();

      }while(edge != m_edge);


      for(HCPolygon *dPolygon : polygons)
      {
        m_polyhedralSurface->deletePatch(dPolygon);
      }
    }

    m_polyhedralSurface->removeVertex(this);
  }
}

IEdge* HCVertex::edge() const
{
  return m_edge;
}

Edge* HCVertex::edgeInternal() const
{
  return m_edge;
}

void HCVertex::addEdge(Edge *edge)
{
  m_edge = edge;
}

void HCVertex::removeEdge(Edge *edge)
{
  assert(edge != nullptr);
  assert(edge->origInternal() == this);

  Edge *next = edge->origNextInternal();
  m_edge = next != edge ? next : nullptr;
}

HCPolyhedralSurface *HCVertex::polyhydralSurfaceInternal() const
{
  return m_polyhedralSurface;
}
