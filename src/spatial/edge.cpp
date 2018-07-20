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
#include "spatial/geometry.h"
#include "spatial/edge.h"
#include "spatial/point.h"
#include "spatial/polygon.h"
#include "spatial/polyhedralsurface.h"

#include  <assert.h>
#include  <qdebug.h>


using namespace HydroCouple;
using namespace HydroCouple::Spatial;

unsigned int Edge::m_nextId(0);

class QuadEdge
{
  public:

    QuadEdge()
    {
      m_edges[0].m_index = 0;
      m_edges[1].m_index = 1;
      m_edges[2].m_index = 2;
      m_edges[3].m_index = 3;

      m_edges[0].m_next = m_edges + 0;
      m_edges[1].m_next = m_edges + 3;
      m_edges[2].m_next = m_edges + 2;
      m_edges[3].m_next = m_edges + 1;

      //  actually m_id internally
      m_edges[0].setIndex(Edge::getNextId());
      m_edges[1].setIndex(Edge::getNextId());
      m_edges[2].setIndex(Edge::getNextId());
      m_edges[3].setIndex(Edge::getNextId());

      m_edges[0].m_quadeEdge = this;
      m_edges[1].m_quadeEdge = this;
      m_edges[2].m_quadeEdge = this;
      m_edges[3].m_quadeEdge = this;
    }

    Edge m_edges[4];
};



Edge::Edge():
  m_marker(-1),
  m_dataLength(0),
  m_data(nullptr),
  m_next(nullptr),
  m_vertex(nullptr),
  m_face(nullptr)
{
}

Edge::~Edge()
{
  if(m_data)
    delete[] m_data;
}

Edge *Edge::createEdge()
{
  return (new QuadEdge())->m_edges;
}

void Edge::deleteEdge(Edge *edge)
{
  assert(edge != nullptr);

  HCVertex *orig = edge->origInternal();
  HCVertex *dest = edge->destInternal();

  HCPolygon *left = edge->leftInternal();
  HCPolygon *right = edge->rightInternal();

  if(orig)
  {
    orig->removeEdge(edge);
  }

  if(dest != orig && dest != nullptr)
  {
    dest->removeEdge(edge->symInternal());
  }

  if(left != nullptr)
  {
    left->removeEdge(edge);
  }

  if(right != left && right != nullptr)
  {
    right->removeEdge(edge->symInternal());
  }

  splice(edge, edge->origPrevInternal());
  splice(edge->symInternal(), edge->symInternal()->origPrevInternal());

  // free the quad edge that the edge belongs to
  delete edge->m_quadeEdge;
}

void Edge::splice(Edge *a, Edge *b)
{
  //  assert(a!=0);
  //  assert(b!=0);

  // see Guibas and Stolfi

  Edge* alpha = a->origNextInternal()->rotInternal();
  Edge* beta  = b->origNextInternal()->rotInternal();

  Edge* t1 = b->origNextInternal();
  Edge* t2 = a->origNextInternal();
  Edge* t3 = beta->origNextInternal();
  Edge* t4 = alpha->origNextInternal();

  a->m_next     = t1;
  b->m_next     = t2;
  alpha->m_next = t3;
  beta->m_next  = t4;

}

unsigned int Edge::index() const
{
  return m_id;
}

void Edge::setIndex(unsigned int index)
{
  m_id = index;
}

HCVertex *Edge::point(int index)
{
  if(index == 0)
    return m_vertex;
  else
    return symInternal()->m_vertex;
}

IVertex *Edge::orig()
{
  return m_vertex;
}

HCVertex *Edge::origInternal()
{
  return m_vertex;
}

void Edge::setOrig(HCVertex *origin)
{
  m_vertex = origin;
  //  setP1(m_vertex);

  if(m_vertex)
  {
    m_vertex->addEdge(this);
  }
}

IVertex *Edge::dest()
{
  return destInternal();
}

HCVertex *Edge::destInternal()
{
  HCVertex *vertex = symInternal()->m_vertex;
  return vertex;
}

void Edge::setDest(HCVertex *destination)
{
  symInternal()->m_vertex = destination;

  if(destination)
  {
    destination->addEdge(symInternal());
  }
}

IPolygon *Edge::left()
{
  return leftInternal();
}

HCPolygon *Edge::leftInternal()
{
  return rotInternal()->m_face;
}

void Edge::setLeft(HCPolygon *left)
{
  rotInternal()->m_face = left;

  if(left)
  {
    left->addEdge(this);
  }
}

IPolygon *Edge::right()
{
  return rightInternal();
}

HCPolygon *Edge::rightInternal()
{
  return invRotInternal()->m_face;
}

IPolygon *Edge::face()
{
  return m_face;
}

HCPolygon *Edge::faceInternal()
{
  return m_face;
}

void Edge::setRight(HCPolygon *right)
{
  invRotInternal()->m_face = right;

  if(right)
  {
    right->addEdge(symInternal());
  }
}

IEdge *Edge::rot()
{
  return rotInternal();
}

Edge *Edge::rotInternal()
{
  return m_index < 3 ? this+1 : this-3;
}

IEdge *Edge::invRot()
{
  return invRotInternal();
}

Edge *Edge::invRotInternal()
{
  return m_index > 0 ? this-1 : this+3;
}

IEdge *Edge::sym()
{
  return symInternal();
}

Edge *Edge::symInternal()
{
  return m_index < 2 ? this+2 : this-2;
}

IEdge *Edge::origNext()
{
  return m_next;
}

Edge *Edge::origNextInternal()
{
  return m_next;
}

IEdge *Edge::origPrev()
{
  return origPrevInternal();
}

Edge *Edge::origPrevInternal()
{
  return rotInternal()->origNextInternal()->rotInternal();
}

IEdge *Edge::destNext()
{
  return destNextInternal();
}

Edge *Edge::destNextInternal()
{
  return symInternal()->origNextInternal()->symInternal();
}

IEdge *Edge::destPrev()
{
  return destPrevInternal();
}

Edge *Edge::destPrevInternal()
{
  return invRotInternal()->origNextInternal()->invRotInternal();
}

IEdge *Edge::leftNext()
{
  return leftNextInternal();
}

Edge *Edge::leftNextInternal()
{
  return invRotInternal()->origNextInternal()->rotInternal();
}

IEdge *Edge::leftPrev()
{
  return leftPrevInternal();
}

Edge *Edge::leftPrevInternal()
{
  return origNextInternal()->symInternal();
}

IEdge *Edge::rightNext()
{
  return rightNextInternal();
}

Edge *Edge::rightNextInternal()
{
  return rotInternal()->origNextInternal()->invRotInternal();
}

IEdge *Edge::rightPrev()
{
  return rightPrevInternal();
}

Edge *Edge::rightPrevInternal()
{
  return symInternal()->origNextInternal();
}

double  Edge::length()
{
  if(m_vertex && destInternal())
  {
    HCVertex* dest = destInternal();
    double dx = dest->x() - m_vertex->x();
    double dy = dest->y() - m_vertex->y();
    double dz = dest->z() - m_vertex->z();
    return sqrt(dx*dx + dy*dy + dz*dz);
  }

  return 0.0;
}

int Edge::marker() const
{
  return m_marker;
}

void Edge::setMarker(int index)
{
  m_marker = index;
}

unsigned int Edge::getNextId()
{
  unsigned int tid = m_nextId;
  m_nextId++;
  return tid;
}

void Edge::initializeData(int dataLength)
{
  if(m_data)
  {
    delete[] m_data;
    m_data = nullptr;
  }

  if(dataLength > 0)
  {
    m_data =  new double[dataLength];
    m_dataLength = dataLength;
  }

}

int Edge::dataLength() const
{
  return m_dataLength;
}

double &Edge::data(int index)
{
  return m_data[index];
}



