#include "stdafx.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/polygon.h"
#include "spatial/point.h"
#include "spatial/edge.h"

#include <math.h>
#include <assert.h>
#include <qdebug.h>

using namespace HydroCouple::Spatial;

HCPolyhedralSurface::HCPolyhedralSurface(QObject *parent):
  HCGeometry(parent)
{
}

HCPolyhedralSurface::~HCPolyhedralSurface()
{
  //fast delete all elements

  QSet<HCEdge*> edges;

  for(HCPolygon *polygon : m_patches)
  {
    if(polygon->edge())
    {
      HCEdge *edge = polygon->edgeInternal();

      do
      {
        if(!edges.contains(edge->symInternal()))
        {
          edges.insert(edge);
        }

        edge = edge->leftNextInternal();

      }while (edge != polygon->edge());
    }

    polygon->m_edge = nullptr;
    polygon->m_polyhedralSurface = nullptr;

    delete polygon;
  }

  m_patches.clear();


  for(QSet<HCEdge*>::iterator  it = edges.begin() ;
      it != edges.end() ; it++)
  {
    delete (*it)->m_quadeEdge;
  }

  for(int i = 0 ; i < m_vertices.length() ; i++)
  {
    HCVertex* vertex = m_vertices[i];
    vertex->m_polyhedralSurface = nullptr;
    delete vertex;
  }

  m_vertices.clear();

}

HydroCouple::Spatial::GeometryType HCPolyhedralSurface::geometryType() const
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

QVector<HCPolygon*> HCPolyhedralSurface::patches() const
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

QVector<HCVertex*> HCPolyhedralSurface::vertices() const
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

  HCEdge *edge  = vertex->edgeInternal();
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

  HCEdge *edgeNew = HCEdge::createEdge(this)->rotInternal();

  // connect the origin (and destination) of the new edge to _vertex_ so that
  // the left face of the edge is _left_
  // this makes a loop on the inside of _left_
  HCEdge::splice(edge2, edgeNew);

  // split the origin and destination of the loop so that the right face of the
  // edge is now _right_
  // this results in a non-loop edge dividing _left_ from _right_
  HCEdge::splice(edge1, edgeNew->symInternal());

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

HCEdge *HCPolyhedralSurface::createVertexEdge(HCVertex *origin, HCVertex *destination, HCPolygon *face)
{

  HCEdge *newEdge = nullptr;

  if((newEdge = findEdge(origin,destination)) != nullptr)
  {
    newEdge->setLeft(face);
  }
  else
  {
    newEdge = HCEdge::createEdge(this);

    if(origin->edgeInternal())
    {
      HCEdge* originSpliceEge = getClosestOrbitLeftNull(origin, destination);
      HCEdge::splice(originSpliceEge,newEdge);
    }

    HCEdge *destinationSpliceEdge = nullptr;

    if(destination->edgeInternal())
    {
      //check for closure
       destinationSpliceEdge = getOrbitLeft(destination->edgeInternal(),face);

      if(destinationSpliceEdge == nullptr)
      {
        //check for unclosed polygon
        destinationSpliceEdge = getClosestOrbitLeftNull(destination, origin);
      }

      HCEdge::splice(destinationSpliceEdge,newEdge->symInternal());
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
  HCEdge *edge1 = edge->origPrevInternal();
  HCEdge *edge2 = edge->leftNextInternal();

  // use _edge1_ for _edge2_ if the destination vertex is isolated
  if (edge2 == edge->sym())
    edge2 = edge1;

  // inverse of _makeVertexEdge_
  HCEdge::splice(edge1, edge->symInternal());
  HCEdge::splice(edge2, edge);

  // all edges leaving the destination orbit of the deleted edge now have its
  // origin vertex as their vertex of origin
  setOrbitOrg(edge2, edge1->origInternal());

  // don't use the deleted edge as a reference edge any more
  edge1->origInternal()->addEdge(edge1);
  edge1->leftInternal()->addEdge(edge1);
  edge2->leftInternal()->addEdge(edge2);

  // reclaim the vertex and the edge

  HCVertex *dest = edge->destInternal();
  HCEdge::deleteEdge(edge);
  delete dest;

}

HCEdge *HCPolyhedralSurface::createFaceEdge(HCPolygon *face, HCVertex *org, HCVertex *dest)
{
  assert(face!=0);
  assert(org!=0);
  assert(dest!=0);

  // locate the edges leaving each of the vertices in the orbit of the face

  HCEdge *edge  = face->edgeInternal();
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

  HCEdge::splice(edge2, edgeNew->symInternal());

  // connect the origin of the new edge to _edge1_
  // _face_ is split in half along the new edge, with the new face introduced
  // on the right

  HCEdge::splice(edge1, edgeNew);

  // initialize the secondary attributes of the new edge

  edgeNew->setOrig(edge1->origInternal());
  edgeNew->setDest(edge2->origInternal());
  edgeNew->setLeft(edge2->leftInternal());

  // all edges in the right orbit of the new edge (i.e. the left orbit of its
  // Sym) now have the new face as their left face

  setOrbitLeft(edgeNew->symInternal(), faceNew);

  return edgeNew;
}

void HCPolyhedralSurface::deleteFaceEdge(HCEdge *edge)
{
  assert(edge!=0);

  // locate _edge1_ and _edge2_ as in _makeFaceEdge_
  if(edge->leftInternal() == edge->rightInternal())
  {
    HCEdge::deleteEdge(edge);
    return  ;
  }

  HCEdge *edge1 = edge->origPrevInternal();
  HCEdge *edge2 = edge->leftNextInternal();

  // use _edge2_ for _edge1_ if the right face is inside a loop

  if (edge1 == edge->sym())
    edge1 = edge2;

  // inverse of _makeFaceEdge_

  HCEdge::splice(edge2, edge->symInternal());
  HCEdge::splice(edge1, edge);

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

  deletePatch(edge->rightInternal());

  //avoid trying to remove and wreaking havoc
  edge->setOrig(nullptr);
  edge->setDest(nullptr);

  HCEdge::deleteEdge(edge);
}

void HCPolyhedralSurface::deletePatch(HCPolygon *polygon)
{
  if(polygon->m_edge != nullptr)
  {
    HCEdge* edge = polygon->m_edge;
    QList<HCVertex*> vertices;
    QList<HCEdge*> edgesToDelete;

    do
    {
      if(edge->right() == nullptr)
      {
        vertices.append(edge->origInternal());
        edgesToDelete.append(edge);
      }
      else
      {
        edge->setLeft(nullptr);
      }

      edge =  edge->leftNextInternal();

    }while(edge != polygon->m_edge);

    for(HCEdge* temp : edgesToDelete)
    {
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

  delete polygon;
}

HCEdge *HCPolyhedralSurface::findDuplicateEdge(HCEdge *edge) const
{
  HCEdge* currentEdge = edge;

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

HCEdge *HCPolyhedralSurface::findEdge(HCVertex *origin, HCVertex *destination) const
{
  HCEdge *currentEdge = origin->edgeInternal();
  HCEdge *oEdge = currentEdge;

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

void HCPolyhedralSurface::addPatch(HCPolygon *patch)
{
  m_patches.append(patch);

  assert(patch->geometryFlags().testFlag(GeometryFlag::HasZ) == is3D());
  assert(patch->geometryFlags().testFlag(GeometryFlag::HasM) == isMeasured());

  patch->setIndex(m_patches.length() - 1);
}

bool HCPolyhedralSurface::removePatch(HCPolygon *patch)
{
  return m_patches.removeOne(patch);
}

void HCPolyhedralSurface::addVertex(HCVertex *vertex)
{
  m_vertices.append(vertex);

  assert(vertex->geometryFlags().testFlag(GeometryFlag::HasZ) == is3D());
  assert(vertex->geometryFlags().testFlag(GeometryFlag::HasM) == isMeasured());

  vertex->setIndex(m_vertices.length() - 1);
}

bool HCPolyhedralSurface::removeVertex(HCVertex *vertex)
{
  return m_vertices.removeOne(vertex);
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

    scan = scan->leftNextInternal();
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
    scan = scan->origNextInternal();
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

    scan = scan->origNextInternal();
  }
  while (scan!=edge);

  return nullptr;
}

HCEdge* HCPolyhedralSurface::getClosestOrbitLeftNull(HCVertex *origin, HCVertex *destination)
{
  //  assert(origin!=0);
  //  assert(destination!=0);

  HCEdge *closestEdge = nullptr;

  double o_x = origin->x();
  double o_y = origin->y();

  double x1 = destination->x() - o_x;
  double y1 = destination->y() - o_y;

  HCEdge *scan = origin->edgeInternal();
  HCEdge *oEdge = scan;

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

    scan = scan->origNextInternal();
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

    scan = scan->leftNextInternal();
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
    HCEdge *edge = polygon->edgeInternal();

    if(edge)
    {
      do
      {
        if(edge->dataLength != length)
        {
          edge->initializeData(length);
        }
        edge = edge->leftNextInternal();

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
