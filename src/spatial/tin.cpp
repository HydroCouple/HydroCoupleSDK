#include "stdafx.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/polygon.h"
#include "spatial/edge.h"
#include "spatial/point.h"

#include <assert.h>
#include <iostream>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCTIN::HCTIN(QObject *parent):
  HCPolyhedralSurface(parent)
{
}

HCTIN::~HCTIN()
{
}

HydroCouple::Spatial::GeometryType HCTIN::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::TINZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return GeometryType::TINZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::TINM;
  }
  else
  {
    return GeometryType::TIN;
  }
}

ITriangle *HCTIN::patch(int index) const
{
  assert(index < m_patches.length());

  return dynamic_cast<ITriangle*>(m_patches[index]);
}

HCEdge *HCTIN::createFaceEdge(HCPolygon *face, HCVertex *org, HCVertex *dest)
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

  HCPolygon *faceNew = new HCTriangle(this);

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

HCTriangle *HCTIN::createTriangle(HCVertex *v1, HCVertex *v2, HCVertex *v3)
{
  HCTriangle *triangle = new HCTriangle(this);

  if(v1->edge() == nullptr && v2->edge() == nullptr && v3->edge() == nullptr)
  {

    HCEdge* edge1 = HCEdge::createEdge(this);
    HCEdge* edge2 = HCEdge::createEdge(this);
    HCEdge* edge3 = HCEdge::createEdge(this);

    HCEdge::splice(edge1 , edge3->symInternal());
    HCEdge::splice(edge1->symInternal() , edge2);
    HCEdge::splice(edge2->symInternal() , edge3);

    setOrbitOrg(edge1,v1);
    setOrbitOrg(edge2,v2);
    setOrbitOrg(edge3,v3);

//    std::cout << "edge type size" << sizeof(HCEdge) << std::endl;
//    std::cout << "edge actual size" << sizeof(edge1) << std::endl;

    setOrbitLeft(edge1, triangle);

  }
  else
  {
    createVertexEdge(v1,v2,triangle);
    createVertexEdge(v2,v3,triangle);
    createVertexEdge(v3,v1,triangle);
  }

  return triangle;
}
