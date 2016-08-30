#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/edge.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCVertex::HCVertex(QObject* parent):
  HCPoint(parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_marker(-1)
{
}

HCVertex::HCVertex(HCPolyhedralSurface* parent):
  HCPoint(parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent),
  m_marker(-1)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}


HCVertex::HCVertex(double x, double y, QObject* parent):
  HCPoint(x,y,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_marker(-1)
{
}

HCVertex::HCVertex(double x, double y, HCPolyhedralSurface* parent):
  HCPoint(x,y,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent),
  m_marker(-1)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::HCVertex(double x, double y, double z, QObject* parent):
  HCPoint(x,y,z,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_marker(-1)
{
}

HCVertex::HCVertex(double x, double y, double z, HCPolyhedralSurface* parent):
  HCPoint(x,y,z,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent),
  m_marker(-1)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::HCVertex(double x, double y, double z, double m, QObject* parent):
  HCPoint(x,y,z,m,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_marker(-1)
{
}

HCVertex::HCVertex(double x, double y, double z, double m, HCPolyhedralSurface* parent):
  HCPoint(x,y,z,m,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent),
  m_marker(-1)
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
    m_polyhedralSurface->removeVertex(this);
  }
}

IEdge* HCVertex::edge() const
{
  return m_edge;
}

HCEdge* HCVertex::edgeInternal() const
{
  return m_edge;
}

void HCVertex::addEdge(HCEdge *edge)
{
  m_edge = edge;
}

void HCVertex::removeEdge(HCEdge *edge)
{
  HCEdge *next = edge->origNextInternal();
  m_edge = next != edge ? next : nullptr;
}

int HCVertex::getMarker() const
{
  return m_marker;
}

void HCVertex::setMarker(int marker)
{
  m_marker = marker;
}
