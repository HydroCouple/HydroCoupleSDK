#include "stdafx.h"
#include "spatial/edge.h"
#include "spatial/point.h"
#include "spatial/polygon.h"
#include  <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

unsigned int HCEdge::m_nextId(0);

QuadEdge::QuadEdge(QObject *parent):
  m_edges(nullptr)
{
  std::vector<HCEdge*> tempEdge;
  tempEdge.push_back(new HCEdge(parent));
  tempEdge.push_back(new HCEdge(parent));
  tempEdge.push_back(new HCEdge(parent));
  tempEdge.push_back(new HCEdge(parent));


  tempEdge[0]->m_index = 0;
  tempEdge[1]->m_index = 1;
  tempEdge[2]->m_index = 2;
  tempEdge[3]->m_index = 3;

  tempEdge[0]->m_next = tempEdge[0];
  tempEdge[1]->m_next = tempEdge[3];
  tempEdge[2]->m_next = tempEdge[2];
  tempEdge[3]->m_next = tempEdge[1];

  //actually m_id internally
  tempEdge[0]->setIndex(HCEdge::getNextId());
  tempEdge[1]->setIndex(HCEdge::getNextId());
  tempEdge[2]->setIndex(HCEdge::getNextId());
  tempEdge[3]->setIndex(HCEdge::getNextId());

  m_edges = tempEdge[0];

}

HCEdge::HCEdge(QObject *parent)
  :HCLine(parent),
    m_next(nullptr),
    m_vertex(nullptr),
    m_face(nullptr)
{

}

HCEdge::~HCEdge()
{
}


HCEdge* HCEdge::createEdge(QObject *parent)
{
  return (new QuadEdge(parent))->m_edges;
}

void HCEdge::deleteEdge(HCEdge *edge)
{
  assert(edge != nullptr);

  // detach the edge from its cell

  splice(edge, dynamic_cast<HCEdge*>(edge->origPrev()));
  splice(dynamic_cast<HCEdge*>(edge->sym()),dynamic_cast<HCEdge*>(edge->sym()->origPrev()));

  // free the quad edge that the edge belongs to

  delete (QuadEdge*)(edge-edge->m_index);
}

void HCEdge::splice(HCEdge *a, HCEdge *b)
{

}

void HCEdge::setIndex(unsigned int index)
{
  m_id = index;
  HCGeometry::setIndex(index);
}

IVertex* HCEdge::orig() const
{
  return m_vertex;
}

void HCEdge::setOrig(HCVertex *origin)
{
  m_vertex = origin;
  setP1(m_vertex);
  origin->addEdge(this);
}

IVertex* HCEdge::dest() const
{
  const HCEdge* sym = m_index < 2 ? this+2 : this-2;
  return sym->m_vertex;
}

void HCEdge::setDest(HCVertex *destination)
{
  dynamic_cast<HCEdge*>(sym())->m_vertex = destination;
  setP2(destination);
  destination->addEdge(this);
}


IPolygon* HCEdge::left() const
{
  const HCEdge* rot = m_index < 3 ? this+1 : this-3;
  return rot->m_face;
}

void HCEdge::setLeft(HCPolygon *left)
{
  dynamic_cast<HCEdge*>(rot())->m_face = left;
  left->addEdge(this);
}

IPolygon* HCEdge::right() const
{
  const HCEdge* invRot = m_index < 3 ? this+1 : this-3;

  return invRot->m_face;
}

void HCEdge::setRight(HCPolygon *right)
{
  dynamic_cast<HCEdge*>(invRot())->m_face = right;
  right->addEdge(dynamic_cast<HCEdge*>(sym()));
}

IEdge* HCEdge::rot()
{
  return m_index < 3 ? this+1 : this-3;
}

IEdge* HCEdge::invRot()
{
  return m_index > 0 ? this-1 : this+3;
}

IEdge* HCEdge::sym()
{
  return m_index < 2 ? this+2 : this-2;
}

IEdge* HCEdge::origNext() const
{
  return m_next;
}

IEdge* HCEdge::origPrev() const
{
  const HCEdge* rot = m_index < 3 ? this+1 : this-3;
  return rot->origNext()->rot();
}

IEdge* HCEdge::destNext() const
{
  const HCEdge* sym = m_index < 2 ? this+2 : this-2;
  return sym->origNext()->sym();
}

IEdge* HCEdge::destPrev() const
{
  const HCEdge* invRot = m_index > 0 ? this-1 : this+3;
  return invRot->origNext()->invRot();
}

IEdge* HCEdge::leftNext() const
{
  const HCEdge* invRot = m_index > 0 ? this-1 : this+3;
  return invRot->origNext()->rot();
}

IEdge* HCEdge::leftPrev() const
{
  return origNext()->sym();
}

IEdge* HCEdge::rightNext() const
{
  const HCEdge* rot = m_index < 3 ? this+1 : this-3;
  return rot->origNext()->invRot();
}

IEdge* HCEdge::rightPrev() const
{
  const HCEdge* sym = m_index < 2 ? this+2 : this-2;
  return sym->origNext();
}

unsigned int HCEdge::getNextId()
{
  unsigned int tid = m_nextId;
  m_nextId++;
  return tid;
}


