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
#include "spatial/point.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/edge.h"
#include "spatial/polygon.h"
#include "spatial/network.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCVertex::HCVertex(const QString &id, HCGeometry *parent):
  HCPoint(id, parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_network(nullptr)
{
  setGeometryFlag(GeometryFlag::HasZ, false);
  setGeometryFlag(GeometryFlag::HasM, false);
}

HCVertex::HCVertex(bool hasZ, bool hasM, const QString &id, HCGeometry *parent):
  HCPoint(hasZ, hasM,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_network(nullptr)
{
  setGeometryFlag(GeometryFlag::HasZ, hasZ);
  setGeometryFlag(GeometryFlag::HasM, hasM);
}

HCVertex::HCVertex(const QString &id, HCPolyhedralSurface* parent):
  HCPoint(id, parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent),
  m_network(nullptr)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::HCVertex(const QString &id, Network *parent):
  HCPoint(id, nullptr),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_network(parent)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasZ, false);
  setGeometryFlag(GeometryFlag::HasM, false);

  m_network->addVertex(this);
}


HCVertex::HCVertex(double x, double y, const QString &id, HCGeometry *parent):
  HCPoint(x,y,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_network(nullptr)
{

  setGeometryFlag(GeometryFlag::HasZ, false);
  setGeometryFlag(GeometryFlag::HasM, false);

}

HCVertex::HCVertex(double x, double y, const QString &id, HCPolyhedralSurface* parent):
  HCPoint(x,y,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent),
  m_network(nullptr)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::HCVertex(double x, double y, double z, const QString &id, HCGeometry *parent):
  HCPoint(x,y,z,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_network(nullptr)
{

  setGeometryFlag(GeometryFlag::HasZ, true);
  setGeometryFlag(GeometryFlag::HasM, true);

}


HCVertex::HCVertex(double x, double y, double z, const QString &id, HCPolyhedralSurface* parent):
  HCPoint(x,y,z,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent),
  m_network(nullptr)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::HCVertex(double x, double y, double z,  const QString &id, Network *parent):
  HCPoint(x,y,z,id, nullptr),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_network(parent)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasZ, true);
  setGeometryFlag(GeometryFlag::HasM, false);

  m_network->addVertex(this);
}


HCVertex::HCVertex(double x, double y, double z, double m, const QString &id, HCGeometry *parent):
  HCPoint(x,y,z,m,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_network(nullptr)
{
  setGeometryFlag(GeometryFlag::HasZ, true);
  setGeometryFlag(GeometryFlag::HasM, true);
}

HCVertex::HCVertex(double x, double y, double z, double m, const QString &id, HCPolyhedralSurface* parent):
  HCPoint(x,y,z,m,id,parent),
  m_edge(nullptr),
  m_polyhedralSurface(parent),
  m_network(nullptr)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasM , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasM));
  setGeometryFlag(GeometryFlag::HasZ , m_polyhedralSurface->geometryFlags().testFlag(GeometryFlag::HasZ));

  m_polyhedralSurface->addVertex(this);
}

HCVertex::HCVertex(double x, double y, double z, double m, const QString &id, Network *parent):
  HCPoint(x,y,z,m,id,nullptr),
  m_edge(nullptr),
  m_polyhedralSurface(nullptr),
  m_network(parent)
{
  assert(parent != nullptr);

  setGeometryFlag(GeometryFlag::HasZ, true);
  setGeometryFlag(GeometryFlag::HasM, true);

  m_polyhedralSurface->addVertex(this);
}

HCVertex::~HCVertex()
{
  if(m_polyhedralSurface)
  {
    if(m_edge != nullptr)
    {
      Edge *edge = m_edge;
      std::list<HCPolygon*> polygons;

      do
      {
        if(edge->leftInternal())
          polygons.push_back(edge->leftInternal());

        edge->setOrig(nullptr);
        edge = edge->origNextInternal();

      }while(edge != m_edge);


      for(HCPolygon *dPolygon : polygons)
      {
        m_polyhedralSurface->deletePatch(dPolygon);
      }
    }

    m_polyhedralSurface->removeVertex(this);
  }

  if(m_network)
  {
    if(m_edge != nullptr)
    {
      Edge *edge = m_edge;

      do
      {
        Edge *temp = edge;
        edge = edge->origNextInternal();

        edge->setOrig(nullptr);
        m_network->deleteVertexEdge(temp);

      }while(edge != m_edge);

    }

    m_network->removeVertex(this);
  }
}

IEdge* HCVertex::edge() const
{
  return m_edge;
}

Edge* HCVertex::edgeInternal() const
{
  return m_edge;
}

void HCVertex::addEdge(Edge *edge)
{
  m_edge = edge;
}

void HCVertex::removeEdge(Edge *edge)
{
  assert(edge != nullptr);
  assert(edge->origInternal() == this);

  Edge *next = edge->origNextInternal();
  m_edge = next != edge ? next : nullptr;
}

HCPolyhedralSurface *HCVertex::polyhydralSurfaceInternal() const
{
  return m_polyhedralSurface;
}
