#include "stdafx.h"
#include "spatial/polygon.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/linestring.h"
#include "spatial/point.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCTriangle::HCTriangle(HCTIN *tin)
  : HCPolygon(tin)
{
}

HCTriangle::HCTriangle(QObject *parent)
  :HCPolygon(parent)
{
}

HCTriangle::~HCTriangle()
{
}

bool HCTriangle::isValid() const
{
  return m_exteriorRing->isClosed()  && m_exteriorRing->m_points.length() == 3;
}

IVertex *HCTriangle::p1() const
{
  if(m_exteriorRing->m_points.length() > 0)
  {
    return dynamic_cast<IVertex*>(m_exteriorRing->m_points[0]);
  }

  return nullptr;
}

IVertex *HCTriangle::p2() const
{
  if(m_exteriorRing->m_points.length() > 1)
  {
    return dynamic_cast<IVertex*>(m_exteriorRing->m_points[1]);
  }

  return nullptr;
}

IVertex *HCTriangle::p3() const
{
  if(m_exteriorRing->m_points.length() > 2)
  {
    return dynamic_cast<IVertex*>(m_exteriorRing->m_points[2]);
  }

  return nullptr;
}
