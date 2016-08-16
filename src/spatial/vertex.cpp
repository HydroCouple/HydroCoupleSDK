#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/polyhedralsurface.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCVertex::HCVertex(QObject* parent):
  HCPoint(parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr)
{
}

HCVertex::HCVertex(HCPolyhedralSurface* parent):
  HCPoint(parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}


HCVertex::HCVertex(double x, double y, QObject* parent):
  HCPoint(x,y,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr)
{
}

HCVertex::HCVertex(double x, double y, HCPolyhedralSurface* parent):
  HCPoint(x,y,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::HCVertex(double x, double y, double z, QObject* parent):
  HCPoint(x,y,z,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr)
{
}

HCVertex::HCVertex(double x, double y, double z, HCPolyhedralSurface* parent):
  HCPoint(x,y,z,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::HCVertex(double x, double y, double z, double m, QObject* parent):
  HCPoint(x,y,z,m,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr)
{
}

HCVertex::HCVertex(double x, double y, double z, double m, HCPolyhedralSurface* parent):
  HCPoint(x,y,z,m,parent),
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
    m_polyhedralSurface->removeVertex(this);
  }
}

IEdge* HCVertex::edge() const
{
  return m_edge;
}

void HCVertex::addEdge(IEdge *edge)
{
  m_edge = edge;
}

void HCVertex::removeEdge(IEdge *edge)
{
  IEdge *next = edge->origNext();
  m_edge = next!= edge ? next : nullptr;
}


