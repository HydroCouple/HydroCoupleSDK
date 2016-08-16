#include "stdafx.h"
#include "spatial/edge.h"
#include "spatial/point.h"
#include "spatial/polygon.h"
#include "spatial/polyhedralsurface.h"

#include  <assert.h>
#include  <qdebug.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

unsigned int HCEdge::m_nextId(0);

QuadEdge::QuadEdge(QObject *parent)
{

  m_edges[0].setParent(parent);
  m_edges[1].setParent(parent);
  m_edges[2].setParent(parent);
  m_edges[3].setParent(parent);

  m_edges[0].m_index = 0;
  m_edges[1].m_index = 1;
  m_edges[2].m_index = 2;
  m_edges[3].m_index = 3;

  m_edges[0].m_next = m_edges + 0;
  m_edges[1].m_next = m_edges + 3;
  m_edges[2].m_next = m_edges + 2;
  m_edges[3].m_next = m_edges + 1;

  //actually m_id internally
  m_edges[0].setIndex(HCEdge::getNextId());
  m_edges[1].setIndex(HCEdge::getNextId());
  m_edges[2].setIndex(HCEdge::getNextId());
  m_edges[3].setIndex(HCEdge::getNextId());

}

HCEdge::HCEdge()
  :HCLine(nullptr),
    m_next(nullptr),
    m_vertex(nullptr),
    m_face(nullptr)
{
}

HCEdge::~HCEdge()
{

}

HCEdge *HCEdge::createEdge(QObject *parent)
{
  return (new QuadEdge(parent))->m_edges;
}

void HCEdge::deleteEdge(HCEdge *edge)
{
  assert(edge != nullptr);

//  qDebug() << "deleting edge ...";
//  HCPolyhedralSurface::printEdge(edge);

  if(edge->orig())
  dynamic_cast<HCVertex*>(edge->orig())->removeEdge(edge);

  if(edge->dest())
  dynamic_cast<HCVertex*>(edge->dest())->removeEdge(edge->sym());

  // detach the edge from its cell
  splice(edge, dynamic_cast<HCEdge*>(edge->origPrev()));
  splice(dynamic_cast<HCEdge*>(edge->sym()),dynamic_cast<HCEdge*>(edge->sym()->origPrev()));

  // free the quad edge that the edge belongs to
  delete (QuadEdge*)(edge-edge->m_index);
}

void HCEdge::splice(HCEdge *a, HCEdge *b)
{
  assert(a!=0);
  assert(b!=0);

  // see Guibas and Stolfi

  HCEdge* alpha = dynamic_cast<HCEdge*>(a->origNext()->rot());
  HCEdge* beta  = dynamic_cast<HCEdge*>(b->origNext()->rot());

  HCEdge* t1 = dynamic_cast<HCEdge*>(b->origNext());
  HCEdge* t2 = dynamic_cast<HCEdge*>(a->origNext());
  HCEdge* t3 = dynamic_cast<HCEdge*>(beta->origNext());
  HCEdge* t4 = dynamic_cast<HCEdge*>(alpha->origNext());

  a->m_next     = t1;
  b->m_next     = t2;
  alpha->m_next = t3;
  beta->m_next  = t4;

}

void HCEdge::setIndex(unsigned int index)
{
  m_id = index;
  HCGeometry::setIndex(index);
}

IVertex *HCEdge::orig()
{
  return m_vertex;
}

void HCEdge::setOrig(HCVertex *origin)
{
  m_vertex = origin;
  setP1(m_vertex);

  if(m_vertex)
  {
    m_vertex->addEdge(this);

    if(left())
    {
      dynamic_cast<HCPolygon*>(left())->reCreateOuterLineString();
    }

    if(right())
    {
      dynamic_cast<HCPolygon*>(left())->reCreateOuterLineString();
    }
  }
}

IVertex *HCEdge::dest()
{
  HCVertex *vertex = dynamic_cast<HCEdge*>(sym())->m_vertex;
  return vertex;
}

void HCEdge::setDest(HCVertex *destination)
{
  dynamic_cast<HCEdge*>(sym())->m_vertex = destination;
  setP2(destination);

  if(destination)
  {
    destination->addEdge(this->sym());

    if(left())
    {
      dynamic_cast<HCPolygon*>(left())->reCreateOuterLineString();
    }

    if(right())
    {
      dynamic_cast<HCPolygon*>(left())->reCreateOuterLineString();
    }
  }
}

IPolygon *HCEdge::left()
{
  return dynamic_cast<HCEdge*>(rot())->m_face;
}

void HCEdge::setLeft(HCPolygon *left)
{
  dynamic_cast<HCEdge*>(rot())->m_face = left;

  if(left)
  {
    left->addEdge(this);
  }
}

IPolygon *HCEdge::right()
{
  return dynamic_cast<HCEdge*>(invRot())->m_face;
}

IPolygon *HCEdge::face()
{
  return m_face;
}

void HCEdge::setRight(HCPolygon *right)
{
  dynamic_cast<HCEdge*>(invRot())->m_face = right;

  if(right)
  {
    right->addEdge(dynamic_cast<HCEdge*>(sym()));
  }
}

IEdge *HCEdge::rot()
{
  return m_index < 3 ? this+1 : this-3;
}

IEdge *HCEdge::invRot()
{
  return m_index > 0 ? this-1 : this+3;
}

IEdge *HCEdge::sym()
{
  return m_index < 2 ? this+2 : this-2;
}

IEdge *HCEdge::origNext()
{
  return m_next;
}

IEdge *HCEdge::origPrev()
{
  return rot()->origNext()->rot();
}

IEdge *HCEdge::destNext()
{
  return sym()->origNext()->sym();
}

IEdge *HCEdge::destPrev()
{
  return invRot()->origNext()->invRot();
}

IEdge *HCEdge::leftNext()
{
  return invRot()->origNext()->rot();
}

IEdge *HCEdge::leftPrev()
{
  return origNext()->sym();
}

IEdge *HCEdge::rightNext()
{
  return rot()->origNext()->invRot();
}

IEdge *HCEdge::rightPrev()
{
  return sym()->origNext();
}

unsigned int HCEdge::getNextId()
{
  unsigned int tid = m_nextId;
  m_nextId++;
  return tid;
}


