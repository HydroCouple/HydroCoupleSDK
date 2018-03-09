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
#include "spatial/edge.h"
#include "spatial/point.h"
#include "spatial/triangle.h"
#include "spatial/octree.h"
#include "spatial/spatialreferencesystem.h"
#include "spatial/linestring.h"

#include <assert.h>
#include <iostream>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCTIN::HCTIN(const QString& id):
  HCPolyhedralSurface(id)
{
}

HCTIN::~HCTIN()
{
}

IGeometry::GeometryType HCTIN::geometryType() const
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

ITriangle *HCTIN::triangle(int index) const
{
  assert((size_t)index < m_patches.size());

  return dynamic_cast<ITriangle*>(patchInternal(index));
}

HCTriangle *HCTIN::triangleInternal(int index) const
{
  return dynamic_cast<HCTriangle*>(patchInternal(index));
}

Edge *HCTIN::createFaceEdge(HCPolygon *face, HCVertex *org, HCVertex *dest)
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

  HCPolygon *faceNew = new HCTriangle(QUuid::createUuid().toString(), this);

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

HCTriangle *HCTIN::createTriangle(HCVertex *v1, HCVertex *v2, HCVertex *v3)
{
  QString id = QUuid::createUuid().toString();
  HCTriangle *triangle = new HCTriangle(id, this);

  if(v1->edge() == nullptr && v2->edge() == nullptr && v3->edge() == nullptr)
  {

    Edge* edge1 = Edge::createEdge();
    Edge* edge2 = Edge::createEdge();
    Edge* edge3 = Edge::createEdge();

    Edge::splice(edge1 , edge3->symInternal());
    Edge::splice(edge1->symInternal() , edge2);
    Edge::splice(edge2->symInternal() , edge3);

    setOrbitOrg(edge1,v1);
    setOrbitOrg(edge2,v2);
    setOrbitOrg(edge3,v3);

    setOrbitLeft(edge1, triangle);
  }
  else
  {
    createVertexEdge(v1,v2,triangle);
    createVertexEdge(v2,v3,triangle);
    createVertexEdge(v3,v1,triangle);
  }

  triangle->reCreateOuterLineString();

  return triangle;
}

void HCTIN::nullifyTriangleIOObjectPointers(triangulateio *triangleObject)
{
  triangleObject->pointlist = NULL;
  triangleObject->pointattributelist = NULL;
  triangleObject->pointmarkerlist = NULL;
  triangleObject->trianglelist = NULL;
  triangleObject->triangleattributelist = NULL;
  triangleObject->trianglearealist = NULL;
  triangleObject->neighborlist = NULL;
  triangleObject->segmentlist = NULL;
  triangleObject->segmentmarkerlist = NULL;
  triangleObject->holelist = NULL;
  triangleObject->regionlist = NULL;
  triangleObject->edgelist = NULL;
  triangleObject->edgemarkerlist = NULL;

  triangleObject->numberofpoints = 0;
  triangleObject->numberofpointattributes = 0;
  triangleObject->numberoftriangles = 0;
  triangleObject->numberofcorners = 0;
  triangleObject->numberoftriangleattributes = 0;
  triangleObject->numberofsegments = 0;
  triangleObject->numberofholes = 0;
  triangleObject->numberofregions = 0;
  triangleObject->numberofedges = 0;
}

void HCTIN::deleteTriangleIOObject(triangulateio *triangleObject)
{
  if(triangleObject->pointlist)
    delete[] triangleObject->pointlist;

  if(triangleObject->pointattributelist)
    delete[] triangleObject->pointattributelist;

  if(triangleObject->pointmarkerlist)
    delete[] triangleObject->pointmarkerlist;

  if(triangleObject->trianglelist)
    delete[] triangleObject->trianglelist;

  if(triangleObject->triangleattributelist)
    delete[] triangleObject->triangleattributelist;

  if(triangleObject->trianglearealist)
    delete[] triangleObject->trianglearealist;

  if(triangleObject->neighborlist)
    delete[] triangleObject->neighborlist;

  if(triangleObject->segmentlist)
    delete[] triangleObject->segmentlist;

  if(triangleObject->segmentmarkerlist)
    delete[] triangleObject->segmentmarkerlist;

  if(triangleObject->holelist)
    delete[] triangleObject->holelist;

  if(triangleObject->regionlist)
    delete[] triangleObject->regionlist;

  if(triangleObject->edgelist)
    delete[] triangleObject->edgelist;

  if(triangleObject->edgemarkerlist)
    delete[] triangleObject->edgemarkerlist;

  nullifyTriangleIOObjectPointers(triangleObject);
}

HCTIN *HCTIN::triangulateSFS(const QString &command, const std::vector<HCPoint*> &points, const std::vector<HCLineString *> &plsg, const std::vector<HCPoint *> &holes)
{
  triangulateio inTriangle, outTriangle, vorout;
  nullifyTriangleIOObjectPointers(&inTriangle);
  nullifyTriangleIOObjectPointers(&outTriangle);
  nullifyTriangleIOObjectPointers(&vorout);

  Octree *PLSGOctree = new Octree(Octree::Octree2D, Octree::AlongEnvelopes,8,5000);
  PLSGOctree->clearObjects();

  std::unordered_map<HCPoint*,int> uniquePoints;

  //index begin end marker
  std::unordered_map<int,std::tuple<int,int,int>> edgePoints;

  for(size_t i = 0 ; i < points.size() ; i++)
  {
    HCPoint *p1 = points[i];
    p1->setMarker(0);
    PLSGOctree->addGeometry(p1);
    uniquePoints[p1] = i;
  }

  int edgeCount = 0;
  int lineStringIndex = 1;

  for(size_t i = 0 ; i < plsg.size() ; i++)
  {
    HCLineString* lineString = plsg[i];

    for(int j = 0; j < lineString->pointCount() - 1 ; j++)
    {
      std::tuple<int,int,int> edgeData = std::make_tuple(0,0,0);

      std::get<2>(edgeData) = lineStringIndex;

      HCPoint *begin = lineString->pointInternal(j);
      HCPoint *end = lineString->pointInternal(j+1);

      begin->setMarker(lineStringIndex);
      end->setMarker(lineStringIndex);

      HCPoint *ptest;

      if((ptest = findPointInOctree(begin, PLSGOctree, 0.000000001)) != nullptr)
      {
        std::get<0>(edgeData) = uniquePoints[ptest];
        ptest->setMarker(lineStringIndex);
      }
      else
      {
        int index = uniquePoints.size();
        std::get<0>(edgeData) = index;
        uniquePoints[begin] = index;
        PLSGOctree->addGeometry(begin);
      }


      if((ptest = findPointInOctree(end, PLSGOctree, 0.000000001)) != nullptr)
      {
        std::get<1>(edgeData) = uniquePoints[ptest];
        ptest->setMarker(lineStringIndex);
      }
      else
      {
        int index = uniquePoints.size();
        std::get<1>(edgeData) = index;
        uniquePoints[end] = index;
        PLSGOctree->addGeometry(end);
      }

      edgePoints[edgeCount] = edgeData;

      edgeCount++;
    }

    lineStringIndex++;
  }

  delete PLSGOctree;

  inTriangle.numberofpoints = uniquePoints.size();
  inTriangle.numberofpointattributes = 1;
  inTriangle.pointlist = new REAL[uniquePoints.size() * 2 ];
  inTriangle.pointmarkerlist = new int[uniquePoints.size() ];
  inTriangle.pointattributelist =   new REAL[uniquePoints.size() ];
  inTriangle.numberofholes = holes.size();
  inTriangle.holelist = new REAL[holes.size() * 2 ];
  inTriangle.numberofsegments = edgePoints.size();
  inTriangle.segmentmarkerlist = new int[edgePoints.size() ];
  inTriangle.segmentlist = new int[edgePoints.size() * 2 ];

  for(std::unordered_map<HCPoint*,int>::iterator it = uniquePoints.begin() ; it!= uniquePoints.end(); it++)
  {
    int index = it->second;
    int cindex = index * 2;
    HCPoint *p = it->first;
    inTriangle.pointlist[cindex] = p->x();
    inTriangle.pointlist[cindex+1] = p->y();
    inTriangle.pointattributelist[index] = p->z();
    inTriangle.pointmarkerlist[index] = p->marker();
  }

  for(std::unordered_map<int,std::tuple<int,int,int>>::iterator it = edgePoints.begin() ; it != edgePoints.end() ; it++)
  {
    int eIndex = it->first ;
    int ePindex = eIndex * 2;
    inTriangle.segmentlist[ePindex] = std::get<0>(it->second);
    inTriangle.segmentlist[ePindex + 1] = std::get<1>(it->second);
    inTriangle.segmentmarkerlist[eIndex] = std::get<2>(it->second);
  }

  for(size_t i = 0; i < holes.size() ; i++)
  {
    HCPoint *point = holes[i];
    inTriangle.holelist[i*2] = point->x();
    inTriangle.holelist[i*2 + 1] = point->y();
  }

  char *commandChar = const_cast<char *>(command.toStdString().c_str());
  triangulate(commandChar,&inTriangle,&outTriangle,&vorout);

  HCTIN *output = readTriangleIOObject(&outTriangle);

  if(points.size())
  {
    output->spatialReferenceSystemInternal()->setSrText(points[0]->spatialReferenceSystem()->srText());
  }
  else if(plsg.size())
  {
    output->spatialReferenceSystemInternal()->setSrText(plsg[0]->spatialReferenceSystem()->srText());
  }
  else if(holes.size())
  {
    output->spatialReferenceSystemInternal()->setSrText(holes[0]->spatialReferenceSystem()->srText());
  }

  deleteTriangleIOObject(&inTriangle);
  outTriangle.holelist = NULL;
  outTriangle.regionlist = NULL;
  deleteTriangleIOObject(&outTriangle);
  deleteTriangleIOObject(&vorout);

  return output;
}

HCTIN *HCTIN::readTriangleIOObject(triangulateio *triangleObject)
{
  HCTIN *tin = new HCTIN();
  tin->enable3D();

  for(int i = 0 ; i < triangleObject->numberofpoints ; i++)
  {
    int index = i * 2;
    double x = triangleObject->pointlist[index];
    double y = triangleObject->pointlist[index + 1];
    double z = triangleObject->pointattributelist[i];
    HCVertex* vertex = new HCVertex(x,y,z,QUuid::createUuid().toString(),tin);
    vertex->setMarker(triangleObject->pointmarkerlist[i]);
  }

  assert(triangleObject->numberofcorners == 3);

  //  ProgressChecker progressChecker(0, triangulation->numberoftriangles, 100, nullptr);

  for(int i = 0 ; i < triangleObject->numberoftriangles ; i++)
  {
    int index  = i * 3;
    HCVertex *v1 = tin->vertexInternal(triangleObject->trianglelist[index]);
    HCVertex *v2 = tin->vertexInternal(triangleObject->trianglelist[index+1]);
    HCVertex *v3 = tin->vertexInternal(triangleObject->trianglelist[index+2]);
    HCTriangle *tri = tin->createTriangle(v1,v2,v3);
    tri->reCreateOuterLineString();

    //    if(progressChecker.performStep(i))
    //    {
    //      setStatus(HydroCouple::Updating , "Copying triangulation...", progressChecker.progress());
    //    }
  }

  return tin;
}

HCPoint *HCTIN::findPointInOctree(HCPoint *point, Octree *octree, double tolerance)
{
  std::vector<IGeometry*> eligibleGeoms = octree->findCollidingGeometries(point);

  for(IGeometry *geom : eligibleGeoms)
  {
    HCPoint *p1 = dynamic_cast<HCPoint*>(geom);

    if(p1->equals(point,tolerance))
    {
      return p1;
    }
  }

  return nullptr;
}
