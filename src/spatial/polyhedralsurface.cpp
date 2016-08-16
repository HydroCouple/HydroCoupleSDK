#include "stdafx.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/polygon.h"
#include "spatial/point.h"
#include "spatial/edge.h"

#include <assert.h>
#include <qdebug.h>

using namespace HydroCouple::Spatial;

HCPolyhedralSurface::HCPolyhedralSurface(QObject *parent):
  HCGeometry(parent)
{
}

HCPolyhedralSurface::~HCPolyhedralSurface()
{
  while (m_patches.length())
  {
    printAllLeftNext(m_patches[0]->edge());
    delete m_patches[0];
  }

  while (m_vertices.length())
  {
    delete m_vertices[0]  ;
  }

  m_vertices.clear();

}

int HCPolyhedralSurface::dimension() const
{
  return 2;
}

IGeometry* HCPolyhedralSurface::envelope() const
{
  return nullptr;
}

double HCPolyhedralSurface::area() const
{
  double tarea = 0;

  for(HCPolygon *polygon : m_patches)
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
  return m_patches.length();
}

IPolygon* HCPolyhedralSurface::patch(int index) const
{
  return m_patches[index];
}

QList<HCPolygon*> HCPolyhedralSurface::patches() const
{
  return m_patches;
}

int HCPolyhedralSurface::vertexCount() const
{
  return m_vertices.length();
}

IVertex *HCPolyhedralSurface::vertex(int index)
{
  return m_vertices[index];
}

QList<HCVertex*> HCPolyhedralSurface::vertices() const
{
  return m_vertices;
}

IMultiPolygon* HCPolyhedralSurface::boundingPolygons(const IPolygon *polygon) const
{
  return nullptr;
}

bool HCPolyhedralSurface::isClosed() const
{
  for(HCPolygon* polygon : m_patches)
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
  for(HCPolygon *polygon : m_patches)
  {
    polygon->enable3D();
  }

  setGeometryFlag(GeometryFlag::HasZ , true);
}

void HCPolyhedralSurface::disable3D()
{
  for(HCPolygon *polygon : m_patches)
  {
    polygon->disable3D();
  }

  setGeometryFlag(GeometryFlag::HasZ , false);
}

void HCPolyhedralSurface::enableM()
{
  for(HCPolygon *polygon : m_patches)
  {
    polygon->enableM();
  }

  setGeometryFlag(GeometryFlag::HasM , true);
}

void HCPolyhedralSurface::disableM()
{
  for(HCPolygon *polygon : m_patches)
  {
    polygon->disableM();
  }

  setGeometryFlag(GeometryFlag::HasM , false);
}

HCEdge *HCPolyhedralSurface::createVertexEdge(HCVertex *vertex, HCVertex *destination,  HCPolygon *left , HCPolygon *right)
{
  assert(vertex != nullptr);
  assert(left != nullptr);
  assert(right != nullptr);

  // locate the edges to the right of each of the faces in the orbit of the
  // vertex

  HCEdge *edge  = dynamic_cast<HCEdge*>(vertex->edge());
  HCEdge *edge1 = getOrbitLeft(edge, right);
  HCEdge *edge2 = getOrbitLeft(edge, left);

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

  HCEdge *edgeNew = dynamic_cast<HCEdge*>(HCEdge::createEdge(this)->rot());

  // connect the origin (and destination) of the new edge to _vertex_ so that
  // the left face of the edge is _left_
  // this makes a loop on the inside of _left_
  HCEdge::splice(edge2, edgeNew);

  // split the origin and destination of the loop so that the right face of the
  // edge is now _right_
  // this results in a non-loop edge dividing _left_ from _right_
  HCEdge::splice(edge1, dynamic_cast<HCEdge*>(edgeNew->sym()));

  // initialize the secondary attributes of the new edge
  edgeNew->setOrig(dynamic_cast<HCVertex*>(edge1->orig()));

  // all edges leaving the destination orbit of the new edge now have the new
  // vertex as their vertex of origin
  setOrbitOrg(dynamic_cast<HCEdge*>(edgeNew->sym()), destination);

  // initialize the secondary attributes of the new edge
  edgeNew->setLeft(dynamic_cast<HCPolygon*>(edge2->left()));
  edgeNew->setRight(dynamic_cast<HCPolygon*>(edge1->left()));

  return edgeNew;
}

HCEdge *HCPolyhedralSurface::createVertexEdge(HCVertex *origin, HCVertex *destination, HCPolygon *face)
{
  assert(origin != nullptr);
  assert(destination != nullptr);
  assert(face != nullptr);

  HCEdge *originSpliceEdge = nullptr;
  HCEdge *destinationSpliceEdge = nullptr;
  HCEdge *newEdge = nullptr;

  if((newEdge = findEdge(origin,destination)) != nullptr)
  {
    newEdge->setLeft(face);
  }
  else
  {
    newEdge = HCEdge::createEdge(this);

    if(origin->edge())
    {
      originSpliceEdge = getOrbitLeft(dynamic_cast<HCEdge*>(origin->edge()),nullptr);
      HCEdge::splice(originSpliceEdge,newEdge);
    }

    if(destination->edge())
    {
      //check for unclosed polygon
      destinationSpliceEdge = getOrbitLeft(dynamic_cast<HCEdge*>(destination->edge()),nullptr);

      //check for closure
      if(destinationSpliceEdge == nullptr)
        destinationSpliceEdge = getOrbitLeft(dynamic_cast<HCEdge*>(destination->edge()),face);

      HCEdge::splice(destinationSpliceEdge,dynamic_cast<HCEdge*>(newEdge->sym()));
    }

    newEdge->setOrig(origin);
    newEdge->setDest(destination);
    newEdge->setLeft(face);
  }

  return newEdge;

}

void HCPolyhedralSurface::deleteVertexEdge(HCEdge *edge)
{
  assert(edge != nullptr);

  // locate _edge1_ and _edge2_ as in _makeVertexEdge_
  HCEdge *edge1 = dynamic_cast<HCEdge*>(edge->origPrev());
  HCEdge *edge2 = dynamic_cast<HCEdge*>(edge->leftNext());

  // use _edge1_ for _edge2_ if the destination vertex is isolated
  if (edge2 == edge->sym())
    edge2 = edge1;

  // inverse of _makeVertexEdge_
  HCEdge::splice(edge1, dynamic_cast<HCEdge*>(edge->sym()));
  HCEdge::splice(edge2, edge);

  // all edges leaving the destination orbit of the deleted edge now have its
  // origin vertex as their vertex of origin
  setOrbitOrg(edge2, dynamic_cast<HCVertex*>(edge1->orig()));

  // don't use the deleted edge as a reference edge any more
  dynamic_cast<HCVertex*>(edge1->orig())->addEdge(edge1);
  dynamic_cast<HCPolygon*>(edge1->left())->addEdge(edge1);
  dynamic_cast<HCPolygon*>(edge2->left())->addEdge(edge2);

  // reclaim the vertex and the edge

  IVertex *dest = edge->dest();
  HCEdge::deleteEdge(edge);
  delete dest;

}

HCEdge *HCPolyhedralSurface::createFaceEdge(HCPolygon *face, HCVertex *org, HCVertex *dest)
{
  assert(face!=0);
  assert(org!=0);
  assert(dest!=0);

  // locate the edges leaving each of the vertices in the orbit of the face

  HCEdge *edge  = dynamic_cast<HCEdge*>(face->edge());
  HCEdge *edge1 = getOrbitOrg(edge, org);
  HCEdge *edge2 = getOrbitOrg(edge, dest);

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

  HCPolygon *faceNew = new HCPolygon(this);

  // create a new (non-loop) edge

  HCEdge *edgeNew = HCEdge::createEdge(this);

  // connect the destination of the new edge to the origin of _edge2_
  // both faces of the edge are now _face_

  HCEdge::splice(edge2, dynamic_cast<HCEdge*>(edgeNew->sym()));

  // connect the origin of the new edge to _edge1_
  // _face_ is split in half along the new edge, with the new face introduced
  // on the right

  HCEdge::splice(edge1, edgeNew);

  // initialize the secondary attributes of the new edge

  edgeNew->setOrig(dynamic_cast<HCVertex*>(edge1->orig()));
  edgeNew->setDest(dynamic_cast<HCVertex*>(edge2->orig()));
  edgeNew->setLeft(dynamic_cast<HCPolygon*>(edge2->left()));

  // all edges in the right orbit of the new edge (i.e. the left orbit of its
  // Sym) now have the new face as their left face

  setOrbitLeft(dynamic_cast<HCEdge*>(edgeNew->sym()), faceNew);

  return edgeNew;
}

void HCPolyhedralSurface::deleteFaceEdge(HCEdge *edge)
{
  assert(edge!=0);

  // locate _edge1_ and _edge2_ as in _makeFaceEdge_

  HCEdge *edge1 = dynamic_cast<HCEdge*>(edge->origPrev());
  HCEdge *edge2 = dynamic_cast<HCEdge*>(edge->leftNext());

  // use _edge2_ for _edge1_ if the right face is inside a loop

  if (edge1 == edge->sym())
    edge1 = edge2;

  // inverse of _makeFaceEdge_

  HCEdge::splice(edge2, dynamic_cast<HCEdge*>(edge->sym()));
  HCEdge::splice(edge1, edge);

  // all edges in the right orbit of the deleted edge now have its left face
  // as their left face

  setOrbitLeft(edge1, dynamic_cast<HCPolygon*>(edge2->left()));

  // don't use the deleted edge as a reference edge any more

  dynamic_cast<HCVertex*>(edge1->orig())->addEdge(edge1);
  dynamic_cast<HCVertex*>(edge2->orig())->addEdge(edge2);
  dynamic_cast<HCPolygon*>(edge2->left())->addEdge(edge2);

  // reclaim the face and the edge
  HCPolygon *polygon = dynamic_cast<HCPolygon*>(edge->right());
  polygon->m_edge = nullptr;
  delete edge->right();
  HCEdge::deleteEdge(edge);
}

HCEdge *HCPolyhedralSurface::findDuplicateEdge(HCEdge *edge)
{
  HCEdge* currentEdge = edge;

  do
  {
    if(currentEdge->dest() == edge->dest() && currentEdge != edge)
    {
      return currentEdge;
    }

    currentEdge = dynamic_cast<HCEdge*>(currentEdge->origNext());

  }while(currentEdge != edge);

  return nullptr;
}

HCEdge *HCPolyhedralSurface::findEdge(HCVertex *origin, HCVertex *destination)
{
  HCEdge* currentEdge = dynamic_cast<HCEdge*>(origin->edge());

  if(currentEdge)
  {
    do
    {
      if(currentEdge->dest() == destination)
      {
        return currentEdge;
      }

      currentEdge = dynamic_cast<HCEdge*>(currentEdge->origNext());

    }while(currentEdge != origin->edge());
  }

  return nullptr;
}

void HCPolyhedralSurface::addPatch(HCPolygon *patch)
{
  m_patches.append(patch);
  patch->setIndex(m_patches.length() - 1);
}

bool HCPolyhedralSurface::removePatch(HCPolygon *patch)
{
  for(int i = patch->index()+1 ; i < m_patches.length() ; i++)
  {
    HCPolygon *poly = m_patches[i];
    poly->setIndex(poly->index() -1);
  }

  return m_patches.removeAll(patch);
}

void HCPolyhedralSurface::addVertex(HCVertex *vertex)
{
  m_vertices.append(vertex);
  vertex->setIndex(m_vertices.length() - 1);
}

bool HCPolyhedralSurface::removeVertex(HCVertex *vertex)
{
  for(int i = vertex->index()+1 ; i < m_vertices.length() ; i++)
  {
    HCVertex *vert = m_vertices[i];
    vert->setIndex(vert->index() -1);
  }

  return m_vertices.removeAll(vertex);
}

HCEdge *HCPolyhedralSurface::getOrbitOrg(HCEdge *edge, HCVertex *org)
{
  assert(edge != nullptr);
  assert(org != nullptr);

  // traverse the leftNext orbit of _edge_ looking for an edge whose origin is
  // _org_

  HCEdge *scan = edge;

  do
  {
    if (scan->orig() == org)
      return scan;

    scan = dynamic_cast<HCEdge*>(scan->leftNext());
  }
  while (scan != edge);

  return nullptr;
}

void HCPolyhedralSurface::setOrbitOrg(HCEdge *edge, HCVertex *org)
{
  assert(edge != nullptr);
  assert(org != nullptr);

  // traverse the Onext orbit of _edge_, setting the origin of each edge to
  // _org_

  HCEdge *scan = edge;

  do
  {
    scan->setOrig(org);
    scan = dynamic_cast<HCEdge*>(scan->origNext());
  }
  while (scan != edge);
}

HCEdge *HCPolyhedralSurface::getOrbitLeft(HCEdge *edge, HCPolygon *left)
{
  assert(edge!=0);
  //  assert(left!=0);

  // traverse the Onext orbit of _edge_ looking for an edge whose left face is
  // _left

  HCEdge *scan = edge;

  do
  {
    if (scan->left()==left)
      return scan;

    scan = dynamic_cast<HCEdge*>(scan->origNext());
  }
  while (scan!=edge);

  return nullptr;
}

HCEdge *HCPolyhedralSurface::getOrbitRight(HCEdge *edge, HCPolygon *right)
{
  assert(edge!=0);
  //  assert(left!=0);

  // traverse the Onext orbit of _edge_ looking for an edge whose left face is
  // _left

  HCEdge *scan = edge;

  do
  {
    if (scan->right() == right)
      return scan;

    scan = dynamic_cast<HCEdge*>(scan->origNext());
  }
  while (scan!=edge);

  return nullptr;
}

void HCPolyhedralSurface::setOrbitLeft(HCEdge *edge, HCPolygon *left)
{
  assert(edge!=0);
  assert(left!=0);

  // traverse the leftNext orbit of _edge_, setting the left face of each edge to
  // _left_

  HCEdge *scan = edge;

  do
  {
    scan->setLeft(left);

    scan = dynamic_cast<HCEdge*>(scan->leftNext());
  }
  while (scan != edge);
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

void HCPolyhedralSurface::initializePatchData(int length)
{
  for(HCPolygon *polygon : m_patches)
  {
    polygon->initializeData(length);
  }
}

void HCPolyhedralSurface::initializeEdgeData(int length)
{
  for(HCPolygon *polygon : m_patches)
  {
    HCEdge *edge = dynamic_cast<HCEdge*>(polygon->edge());

    if(edge)
    {
      do
      {
        if(edge->dataLength() != length)
        {
          edge->initializeData(length);
        }
        edge = dynamic_cast<HCEdge*>(edge->leftNext());

      }while (edge != polygon->edge());
    }
  }
}

void HCPolyhedralSurface::initializeNodeData(int length)
{
  for(HCVertex *vertex : m_vertices)
  {
    vertex->initializeData(length);
  }

}
