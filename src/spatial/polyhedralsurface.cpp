#include "stdafx.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/polygon.h"

using namespace HydroCouple::Spatial;

HCPolyhedralSurface::HCPolyhedralSurface(QObject *parent):
  HCGeometry(parent)
{
}

HCPolyhedralSurface::~HCPolyhedralSurface()
{
}

int HCPolyhedralSurface::patchCount() const
{
  return m_patches.length();
}

IPolygon* HCPolyhedralSurface::patch(int index) const
{
  return m_patches[index];
}

bool HCPolyhedralSurface::addPatch(HCPolygon *patch)
{
  if(!m_patches.contains(patch))
  {
    m_patches.append(patch);
    return true;
  }
  else
  {
    return false;
  }
}

bool HCPolyhedralSurface::deletePatch(HCPolygon *patch)
{
  bool removed = m_patches.removeAll(patch);

  if(removed)
  {
    delete patch;
  }

  return removed;
}

QList<HCVertex*> HCPolyhedralSurface::vertices() const
{
  return m_vertices;
}

bool HCPolyhedralSurface::addVertex(HCVertex *vertex)
{
  if(!m_vertices.contains(vertex))
  {
    m_vertices.append(vertex);
    return true;
  }
  else
  {
    return false;
  }
}

bool HCPolyhedralSurface::deleteVertex(HCVertex *vertex)
{
  bool removed = m_vertices.removeAll(vertex);

  if(removed)
  {
    delete vertex;
  }

  return vertex;
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
