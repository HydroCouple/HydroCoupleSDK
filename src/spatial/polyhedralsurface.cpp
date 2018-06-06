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
#include "spatial/polyhedralsurface.h"
#include "spatial/polygon.h"
#include "spatial/point.h"
#include "spatial/edge.h"
#include "spatial/envelope.h"
#include "spatial/linestring.h"
//#include "binary_find.h"

#include <math.h>
#include <assert.h>
#include <qdebug.h>

using namespace HydroCouple::Spatial;

HCPolyhedralSurface::HCPolyhedralSurface(const QString &id):
  HCGeometry(id)
{
}

HCPolyhedralSurface::~HCPolyhedralSurface()
{
  //fast delete all elements
  while(m_patches.size())
  {
    HCPolygon *patch = patchInternal(0);
    delete patch;
  }
}

int HCPolyhedralSurface::dimension() const
{
  return 2;
}

IGeometry::GeometryType HCPolyhedralSurface::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::PolyhedralSurfaceZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return GeometryType::PolyhedralSurfaceZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::PolyhedralSurfaceM;
  }
  else
  {
    return GeometryType::PolyhedralSurface;
  }
}

Envelope *HCPolyhedralSurface::envelopeInternal() const
{
  m_envelope->resetExtentsToInfinity();

  for(const auto &vertex : m_vertices)
  {
    m_envelope->merge(vertex->envelope());
  }

  return m_envelope;
}

double HCPolyhedralSurface::area() const
{
  double tarea = 0;

  for(const auto &polygon : m_patches)
  {
    tarea += polygon->area();
  }

  return tarea;
}

IPoint *HCPolyhedralSurface::centroid() const
{
  return nullptr;
}

IPoint *HCPolyhedralSurface::pointOnSurface() const
{
  return nullptr;
}

IMultiCurve *HCPolyhedralSurface::boundaryMultiCurve() const
{
  return nullptr;
}

int HCPolyhedralSurface::patchCount() const
{
  return m_patches.size();
}

IPolygon* HCPolyhedralSurface::patch(int index) const
{
  return patchInternal(index);
}

HCPolygon *HCPolyhedralSurface::patchInternal(int index) const
{
  return m_patches[index];
}

const std::vector<HCPolygon*> &HCPolyhedralSurface::patches() const
{
  return m_patches;
}

int HCPolyhedralSurface::vertexCount() const
{
  return m_vertices.size();
}

IVertex *HCPolyhedralSurface::vertex(int index) const
{
  return vertexInternal(index);
}

HCVertex *HCPolyhedralSurface::vertexInternal(int index) const
{
  return m_vertices[index];
}

const std::vector<HCVertex*> &HCPolyhedralSurface::vertices() const
{
  return m_vertices;
}

IMultiPolygon* HCPolyhedralSurface::boundingPolygons(const IPolygon *polygon) const
{
  return nullptr;
}

bool HCPolyhedralSurface::isClosed() const
{
  for(const auto & polygon : m_patches)
  {

    if(!polygon->exteriorRing()->isClosed())
    {
      return false;
    }
  }

  return true;
}

void HCPolyhedralSurface::enable3D()
{
  for(const auto & polygon : m_patches)
  {
    polygon->enable3D();
  }

  setGeometryFlag(GeometryFlag::HasZ , true);
}

void HCPolyhedralSurface::disable3D()
{
  for(const auto &polygon : m_patches)
  {
    polygon->disable3D();
  }

  setGeometryFlag(GeometryFlag::HasZ , false);
}

void HCPolyhedralSurface::enableM()
{
  for(const auto & polygon : m_patches)
  {
    polygon->enableM();
  }

  setGeometryFlag(GeometryFlag::HasM , true);
}

void HCPolyhedralSurface::disableM()
{
  for(const auto &polygon : m_patches)
  {
    polygon->disableM();
  }

  setGeometryFlag(GeometryFlag::HasM , false);
}

Edge *HCPolyhedralSurface::createVertexEdge(HCVertex *vertex, HCVertex *destination,  HCPolygon *left , HCPolygon *right)
{
  assert(vertex != nullptr);
  assert(left != nullptr);
  assert(right != nullptr);

  // locate the edges to the right of each of the faces in the orbit of the
  // vertex

  Edge *edge  = vertex->edgeInternal();
  Edge *edge1 = getOrbitLeft(edge, right);
  Edge *edge2 = getOrbitLeft(edge, left);

  if (edge1 == nullptr)
  {
    throw std::logic_error("HCPolyhedralSurface::createVertexEdge: unable to locate right face " +
                           std::to_string(right->index()) + " on vertex " +
                           std::to_string(vertex->index()));
  }

  if (edge2 == nullptr)
  {
    throw std::logic_error("HCPolyhedralSurface::createVertexEdge: unable to locate left face " +
                           std::to_string(left->index()) + " on vertex " +
                           std::to_string(vertex->index()));
  }

  // create a new edge and rotate it to make a clockwise loop
  Edge *edgeNew = Edge::createEdge()->rotInternal();

  // connect the origin (and destination) of the new edge to _vertex_ so that
  // the left face of the edge is _left_
  // this makes a loop on the inside of _left_
  Edge::splice(edge2, edgeNew);

  // split the origin and destination of the loop so that the right face of the
  // edge is now _right_
  // this results in a non-loop edge dividing _left_ from _right_
  Edge::splice(edge1, edgeNew->symInternal());

  // initialize the secondary attributes of the new edge
  edgeNew->setOrig(edge1->origInternal());

  // all edges leaving the destination orbit of the new edge now have the new
  // vertex as their vertex of origin
  setOrbitOrg(edgeNew->symInternal(), destination);

  // initialize the secondary attributes of the new edge
  edgeNew->setLeft(edge2->leftInternal());
  edgeNew->setRight(edge1->leftInternal());

  return edgeNew;
}

Edge *HCPolyhedralSurface::createVertexEdge(HCVertex *origin, HCVertex *destination, HCPolygon *face)
{

  Edge *newEdge = nullptr;

  if((newEdge = findEdge(origin,destination)) != nullptr)
  {
    newEdge->setLeft(face);
  }
  else
  {
    newEdge = Edge::createEdge();

    if(origin->edgeInternal())
    {
      Edge* originSpliceEge = getClosestOrbitLeftNull(origin, destination);
      Edge::splice(originSpliceEge,newEdge);
    }

    Edge *destinationSpliceEdge = nullptr;

    if(destination->edgeInternal())
    {
      //check for closure
      destinationSpliceEdge = getOrbitLeft(destination->edgeInternal(),face);

      if(destinationSpliceEdge == nullptr)
      {
        //check for unclosed polygon
        destinationSpliceEdge = getClosestOrbitLeftNull(destination, origin);
      }

      Edge::splice(destinationSpliceEdge,newEdge->symInternal());
    }

    newEdge->setOrig(origin);
    newEdge->setDest(destination);
    newEdge->setLeft(face);
  }

  return newEdge;

}

void HCPolyhedralSurface::deleteVertexEdge(Edge *edge)
{
  assert(edge != nullptr);

  // locate _edge1_ and _edge2_ as in _makeVertexEdge_
  Edge *edge1 = edge->origPrevInternal();
  Edge *edge2 = edge->leftNextInternal();

  // use _edge1_ for _edge2_ if the destination vertex is isolated
  if (edge2 == edge->sym())
    edge2 = edge1;

  // inverse of _makeVertexEdge_
  Edge::splice(edge1, edge->symInternal());
  Edge::splice(edge2, edge);

  // all edges leaving the destination orbit of the deleted edge now have its
  // origin vertex as their vertex of origin
  setOrbitOrg(edge2, edge1->origInternal());

  // don't use the deleted edge as a reference edge any more
  edge1->origInternal()->addEdge(edge1);
  edge1->leftInternal()->addEdge(edge1);
  edge2->leftInternal()->addEdge(edge2);

  // reclaim the vertex and the edge
  HCVertex *dest = edge->destInternal();
  dest->addEdge(nullptr);
  delete dest;

  edge->setOrig(nullptr);
  edge->setDest(nullptr);
  edge->setLeft(nullptr);
  edge->setRight(nullptr);
  Edge::deleteEdge(edge);

}

Edge *HCPolyhedralSurface::createFaceEdge(HCPolygon *face, HCVertex *org, HCVertex *dest)
{
  assert(face!=0);
  assert(org!=0);
  assert(dest!=0);

  // locate the edges leaving each of the vertices in the orbit of the face

  Edge *edge  = face->edgeInternal();
  Edge *edge1 = getOrbitOrg(edge, org);
  Edge *edge2 = getOrbitOrg(edge, dest);

  if (edge1==0)
  {
    throw std::logic_error("Cell::makeFaceEdge: unable to locate origin vertex " +
                           std::to_string(org->index()) + " on face " +
                           std::to_string(face->index()));
  }

  if (edge2==0)
  {
    throw std::logic_error("Cell::makeFaceEdge: unable to locate origin vertex " +
                           std::to_string(dest->index()) + " on face " +
                           std::to_string(face->index()));
  }

  // create a new face

  HCPolygon *faceNew = new HCPolygon(QUuid::createUuid().toString(), this);

  // create a new (non-loop) edge

  Edge *edgeNew = Edge::createEdge();

  // connect the destination of the new edge to the origin of _edge2_
  // both faces of the edge are now _face_

  Edge::splice(edge2, edgeNew->symInternal());

  // connect the origin of the new edge to _edge1_
  // _face_ is split in half along the new edge, with the new face introduced
  // on the right

  Edge::splice(edge1, edgeNew);

  // initialize the secondary attributes of the new edge

  edgeNew->setOrig(edge1->origInternal());
  edgeNew->setDest(edge2->origInternal());
  edgeNew->setLeft(edge2->leftInternal());

  // all edges in the right orbit of the new edge (i.e. the left orbit of its
  // Sym) now have the new face as their left face

  setOrbitLeft(edgeNew->symInternal(), faceNew);

  return edgeNew;
}

void HCPolyhedralSurface::deleteFaceEdge(Edge *edge)
{
  assert(edge!=0);

  // locate _edge1_ and _edge2_ as in _makeFaceEdge_
  if(edge->leftInternal() == edge->rightInternal())
  {

    edge->setLeft(nullptr);
    edge->setRight(nullptr);
    edge->setDest(nullptr);
    Edge::deleteEdge(edge);
    return;
  }

  Edge *edge1 = edge->origPrevInternal();
  Edge *edge2 = edge->leftNextInternal();

  // use _edge2_ for _edge1_ if the right face is inside a loop

  if (edge1 == edge->sym())
    edge1 = edge2;

  // inverse of _makeFaceEdge_

  Edge::splice(edge2, edge->symInternal());
  Edge::splice(edge1, edge);

  // all edges in the right orbit of the deleted edge now have its left face
  // as their left face

  setOrbitLeft(edge1, edge2->leftInternal());

  // don't use the deleted edge as a reference edge any more

  edge1->origInternal()->addEdge(edge1);
  edge2->origInternal()->addEdge(edge2);
  edge2->leftInternal()->addEdge(edge2);

  // reclaim the face and the edge
  HCPolygon *polygon = edge->rightInternal();
  polygon->m_edge = nullptr;
  delete polygon;


  //avoid trying to remove and wreaking havoc
  edge->setOrig(nullptr);
  edge->setDest(nullptr);
  edge->setRight(nullptr);
  edge->setLeft(nullptr);

  Edge::deleteEdge(edge);
}

void HCPolyhedralSurface::deletePatch(HCPolygon *polygon)
{
  delete polygon;
}

Edge *HCPolyhedralSurface::findDuplicateEdge(Edge *edge) const
{
  Edge* currentEdge = edge;

  do
  {
    if(currentEdge->dest() == edge->dest() && currentEdge != edge)
    {
      return currentEdge;
    }

    currentEdge = currentEdge->origNextInternal();

  }while(currentEdge != edge);

  return nullptr;
}

Edge *HCPolyhedralSurface::findEdge(HCVertex *origin, HCVertex *destination) const
{
  Edge *currentEdge = origin->edgeInternal();
  Edge *oEdge = currentEdge;

  if(currentEdge)
  {
    do
    {
      if(currentEdge->dest() == destination)
      {
        return currentEdge;
      }

      currentEdge = currentEdge->origNextInternal();

    }while(currentEdge != oEdge);
  }

  return nullptr;
}

void HCPolyhedralSurface::reIndexVerticesAndPatches()
{
  unsigned int i = 0;

  for(HCPolygon *patch : m_patches)
  {
    patch->setIndex(i);
    i++;
  }

  i = 0;

  for(HCVertex *vertex : m_vertices)
  {
    vertex->setIndex(i);
    i++;
  }
}

void HCPolyhedralSurface::printEdge(IEdge *edge)
{
  qDebug() << " edge: " << edge->index()
           << " sym: "<< edge->sym()->index()
           << " org: "<< QString::number( edge->orig() != nullptr ? edge->orig()->index() : -1)
           << " dest: " << QString::number( edge->dest() != nullptr ? edge->dest()->index() : -1)
           << " left face: " << QString::number(edge->left() != nullptr ? edge->left()->index() : -1)
           << " right face: " << QString::number(edge->right() != nullptr ? edge->right()->index() : -1);
}

void HCPolyhedralSurface::printAllOrigNext(IEdge *edge)
{
  IEdge *temp = edge;

  qDebug() << "Orig Next" ;

  do
  {
    printEdge(temp);
    temp = temp->origNext();

  }while (temp !=  edge);

  qDebug() << "";

}

void HCPolyhedralSurface::printAllLeftNext(IEdge *edge)
{
  IEdge *temp = edge;

  qDebug() << "Left Next" ;

  do
  {
    printEdge(temp);
    temp = temp->leftNext();
  }while (temp !=  edge);

  qDebug() << "";
}

void HCPolyhedralSurface::addPatch(HCPolygon *patch)
{
  m_patches.push_back(patch);

  assert(patch->geometryFlags().testFlag(GeometryFlag::HasZ) == is3D());
  assert(patch->geometryFlags().testFlag(GeometryFlag::HasM) == isMeasured());

  patch->setIndex(m_patches.size() - 1);

  m_patchCntr++;
}

bool HCPolyhedralSurface::removePatch(HCPolygon *patch)
{
  std::vector<HCPolygon*>::iterator it = std::find(m_patches.begin(), m_patches.end(), patch);

  if(it != m_patches.end())
  {
    m_patches.erase(it);
    return true;
  }

  return false;
}

void HCPolyhedralSurface::addVertex(HCVertex *vertex)
{
  m_vertices.push_back(vertex);

  assert(vertex->geometryFlags().testFlag(GeometryFlag::HasZ) == is3D());
  assert(vertex->geometryFlags().testFlag(GeometryFlag::HasM) == isMeasured());

  vertex->setIndex(m_vertices.size() - 1);
  m_vertexCntr++;
}

bool HCPolyhedralSurface::removeVertex(HCVertex *vertex)
{
  std::vector<HCVertex*>::iterator it = std::find(m_vertices.begin(), m_vertices.end(), vertex);

  if(it != m_vertices.end())
  {
    m_vertices.erase(it);
    return true;
  }

  return false;
}

Edge *HCPolyhedralSurface::getOrbitOrg(Edge *edge, HCVertex *org)
{
  assert(edge != nullptr);
  assert(org != nullptr);

  // traverse the leftNext orbit of _edge_ looking for an edge whose origin is
  // _org_

  Edge *scan = edge;

  do
  {
    if (scan->orig() == org)
      return scan;

    scan = scan->leftNextInternal();
  }
  while (scan != edge);

  return nullptr;
}

void HCPolyhedralSurface::setOrbitOrg(Edge *edge, HCVertex *org)
{
  assert(edge != nullptr);
  assert(org != nullptr);

  // traverse the Onext orbit of _edge_, setting the origin of each edge to
  // _org_

  Edge *scan = edge;

  do
  {
    scan->setOrig(org);
    scan = scan->origNextInternal();
  }
  while (scan != edge);
}

Edge *HCPolyhedralSurface::getOrbitLeft(Edge *edge, HCPolygon *left)
{
  assert(edge!=0);
  //  assert(left!=0);

  // traverse the Onext orbit of _edge_ looking for an edge whose left face is
  // _left

  Edge *scan = edge;

  do
  {
    if (scan->left()==left)
      return scan;

    scan = scan->origNextInternal();
  }
  while (scan!=edge);

  return nullptr;
}

Edge* HCPolyhedralSurface::getClosestOrbitLeftNull(HCVertex *origin, HCVertex *destination)
{

  Edge *closestEdge = nullptr;

  double o_x = origin->x();
  double o_y = origin->y();

  double x1 = destination->x() - o_x;
  double y1 = destination->y() - o_y;

  Edge *scan = origin->edgeInternal();
  Edge *oEdge = scan;

  //start with large number
  double small = 1000.0;

  do
  {
    if (scan->left() == nullptr)
    {
      double x2 = scan->dest()->x() - o_x;
      double y2 = scan->dest()->y() - o_y;

      double angle2 =  atan2(x2*y1-y2*x1,x2*x1+y2*y1);

      if(angle2 < 0 )
        angle2 = angle2 + 2* M_PI;

      if(angle2 < small)
      {
        small = angle2;
        closestEdge = scan;
      }
    }

    scan = scan->origNextInternal();
  }
  while (scan != oEdge);

  return closestEdge;
}

Edge *HCPolyhedralSurface::getOrbitRight(Edge *edge, HCPolygon *right)
{
  assert(edge!=0);
  //  assert(left!=0);

  // traverse the Onext orbit of _edge_ looking for an edge whose left face is
  // _left

  Edge *scan = edge;

  do
  {
    if (scan->right() == right)
      return scan;

    scan = scan->origNextInternal();
  }
  while (scan!=edge);

  return nullptr;
}

void HCPolyhedralSurface::setOrbitLeft(Edge *edge, HCPolygon *left)
{
  assert(edge!=0);
  assert(left!=0);

  // traverse the leftNext orbit of _edge_, setting the left face of each edge to
  // _left_

  Edge *scan = edge;

  do
  {
    scan->setLeft(left);

    scan = scan->leftNextInternal();
  }
  while (scan != edge);
}

