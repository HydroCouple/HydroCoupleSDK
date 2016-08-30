#include "stdafx.h"
#include "spatial/geometry.h"
#include "spatial/edge.h"
#include "spatial/point.h"
#include "spatial/polygon.h"
#include "spatial/polyhedralsurface.h"

#include  <assert.h>
#include  <qdebug.h>


using namespace HydroCouple;
using namespace HydroCouple::Spatial;

unsigned int HCEdge::m_nextId(0);

QuadEdge::QuadEdge(HCPolyhedralSurface *parent)
{
//  m_edges[0].setParent(parent);
//  m_edges[1].setParent(parent);
//  m_edges[2].setParent(parent);
//  m_edges[3].setParent(parent);

  m_edges[0].setGeometryFlag(HCGeometry::HasZ , parent->is3D());
  m_edges[1].setGeometryFlag(HCGeometry::HasZ , parent->is3D());
  m_edges[2].setGeometryFlag(HCGeometry::HasZ , parent->is3D());
  m_edges[3].setGeometryFlag(HCGeometry::HasZ , parent->is3D());

  m_edges[0].setGeometryFlag(HCGeometry::HasM , parent->isMeasured());
  m_edges[1].setGeometryFlag(HCGeometry::HasM , parent->isMeasured());
  m_edges[2].setGeometryFlag(HCGeometry::HasM , parent->isMeasured());
  m_edges[3].setGeometryFlag(HCGeometry::HasM , parent->isMeasured());

  m_edges[0].m_index = 0;
  m_edges[1].m_index = 1;
  m_edges[2].m_index = 2;
  m_edges[3].m_index = 3;

  m_edges[0].m_next = m_edges + 0;
  m_edges[1].m_next = m_edges + 3;
  m_edges[2].m_next = m_edges + 2;
  m_edges[3].m_next = m_edges + 1;

//  actually m_id internally
  m_edges[0].setIndex(HCEdge::getNextId());
  m_edges[1].setIndex(HCEdge::getNextId());
  m_edges[2].setIndex(HCEdge::getNextId());
  m_edges[3].setIndex(HCEdge::getNextId());

  m_edges[0].m_quadeEdge = this;
  m_edges[1].m_quadeEdge = this;
  m_edges[2].m_quadeEdge = this;
  m_edges[3].m_quadeEdge = this;
}

HCEdge::HCEdge()
  :Identity(QUuid::createUuid().toString(),nullptr),
    data(nullptr),
    dataLength(0),
    m_next(nullptr),
    m_vertex(nullptr),
    m_face(nullptr),
    m_geomFlags(HCGeometry::None)
{
}

HCEdge::~HCEdge()
{

}

HCEdge *HCEdge::createEdge(HCPolyhedralSurface *parent)
{
  return (new QuadEdge(parent))->m_edges;
}

void HCEdge::deleteEdge(HCEdge *edge)
{
  assert(edge != nullptr);

  if(edge->origInternal())
    edge->origInternal()->removeEdge(edge);

  if(edge->destInternal())
    edge->destInternal()->removeEdge(edge->symInternal());

  splice(edge, edge->origPrevInternal());
  splice(edge->symInternal(), edge->symInternal()->origPrevInternal());


  // free the quad edge that the edge belongs to
  delete edge->m_quadeEdge;
}

void HCEdge::splice(HCEdge *a, HCEdge *b)
{
//  assert(a!=0);
//  assert(b!=0);

  // see Guibas and Stolfi

  HCEdge* alpha = a->origNextInternal()->rotInternal();
  HCEdge* beta  = b->origNextInternal()->rotInternal();

  HCEdge* t1 = b->origNextInternal();
  HCEdge* t2 = a->origNextInternal();
  HCEdge* t3 = beta->origNextInternal();
  HCEdge* t4 = alpha->origNextInternal();

  a->m_next     = t1;
  b->m_next     = t2;
  alpha->m_next = t3;
  beta->m_next  = t4;

}

unsigned int HCEdge::index() const
{
  return m_id;
}

void HCEdge::setIndex(unsigned int index)
{
  m_id = index;
}

HCGeometry::GeometryFlags HCEdge::geometryFlags() const
{
  return m_geomFlags;
}

bool HCEdge::is3D() const
{
  return m_geomFlags.testFlag(HCGeometry::HasZ);
}

bool HCEdge::isMeasured() const
{
  return m_geomFlags.testFlag(HCGeometry::HasM);
}

HCVertex *HCEdge::point(int index)
{
  if(index == 0)
    return m_vertex;
  else
    return symInternal()->m_vertex;
}

IVertex *HCEdge::orig()
{
  return m_vertex;
}

HCVertex *HCEdge::origInternal()
{
  return m_vertex;
}

void HCEdge::setOrig(HCVertex *origin)
{
  m_vertex = origin;
//  setP1(m_vertex);

  if(m_vertex)
  {
    origin->setGeometryFlag(HCGeometry::HasZ , this->is3D());
    origin->setGeometryFlag(HCGeometry::HasM , this->isMeasured());

    m_vertex->addEdge(this);
  }
}

IVertex *HCEdge::dest()
{
  return destInternal();
}

HCVertex *HCEdge::destInternal()
{
  HCVertex *vertex = symInternal()->m_vertex;
  return vertex;
}

void HCEdge::setDest(HCVertex *destination)
{
  symInternal()->m_vertex = destination;

  if(destination)
  {
    destination->setGeometryFlag(HCGeometry::HasZ , this->is3D());
    destination->setGeometryFlag(HCGeometry::HasM , this->isMeasured());

    destination->addEdge(symInternal());
  }
}

IPolygon *HCEdge::left()
{
  return leftInternal();
}

HCPolygon *HCEdge::leftInternal()
{
  return rotInternal()->m_face;
}

void HCEdge::setLeft(HCPolygon *left)
{
  rotInternal()->m_face = left;

  if(left)
  {
    left->addEdge(this);
  }
}

IPolygon *HCEdge::right()
{
  return rightInternal();
}

HCPolygon *HCEdge::rightInternal()
{
  return invRotInternal()->m_face;
}

IPolygon *HCEdge::face()
{
  return m_face;
}

HCPolygon *HCEdge::faceInternal()
{
  return m_face;
}

void HCEdge::setRight(HCPolygon *right)
{
  invRotInternal()->m_face = right;

  if(right)
  {
    right->addEdge(symInternal());
  }
}

IEdge *HCEdge::rot()
{
  return rotInternal();
}

HCEdge *HCEdge::rotInternal()
{
  return m_index < 3 ? this+1 : this-3;
}

IEdge *HCEdge::invRot()
{
  return invRotInternal();
}

HCEdge *HCEdge::invRotInternal()
{
  return m_index > 0 ? this-1 : this+3;
}

IEdge *HCEdge::sym()
{
  return symInternal();
}

HCEdge *HCEdge::symInternal()
{
  return m_index < 2 ? this+2 : this-2;
}

IEdge *HCEdge::origNext()
{
  return m_next;
}

HCEdge *HCEdge::origNextInternal()
{
  return m_next;
}

IEdge *HCEdge::origPrev()
{
  return origPrevInternal();
}

HCEdge *HCEdge::origPrevInternal()
{
  return rotInternal()->origNextInternal()->rotInternal();
}

IEdge *HCEdge::destNext()
{
  return destNextInternal();
}

HCEdge *HCEdge::destNextInternal()
{
  return symInternal()->origNextInternal()->symInternal();
}

IEdge *HCEdge::destPrev()
{
  return destPrevInternal();
}

HCEdge *HCEdge::destPrevInternal()
{
  return invRotInternal()->origNextInternal()->invRotInternal();
}

IEdge *HCEdge::leftNext()
{
  return leftNextInternal();
}

HCEdge *HCEdge::leftNextInternal()
{
  return invRotInternal()->origNextInternal()->rotInternal();
}

IEdge *HCEdge::leftPrev()
{
  return leftPrevInternal();
}

HCEdge *HCEdge::leftPrevInternal()
{
  return origNextInternal()->symInternal();
}

IEdge *HCEdge::rightNext()
{
  return rightNextInternal();
}

HCEdge *HCEdge::rightNextInternal()
{
  return rotInternal()->origNextInternal()->invRotInternal();
}

IEdge *HCEdge::rightPrev()
{
  return rightPrevInternal();
}

HCEdge *HCEdge::rightPrevInternal()
{
  return symInternal()->origNextInternal();
}

void HCEdge::setGeometryFlag(HCGeometry::GeometryFlag flag, bool on)
{
  m_geomFlags = on ? m_geomFlags |= flag : m_geomFlags &= ~flag;
}

void HCEdge::initializeData(int length, double defaultValue)
{
  dataLength = length;

  if(data)
  {
    delete[] data;
    data = nullptr;
  }

  if(length > 0)
  {
    data = new double[length];

    for(int i = 0; i< length; i++)
    {
      data[i] = defaultValue;
    }
  }
}

unsigned int HCEdge::getNextId()
{
  unsigned int tid = m_nextId;
  m_nextId++;
  return tid;
}


