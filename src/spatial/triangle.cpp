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
  reCreateOuterLineString();

  return m_exteriorRing->isClosed()  && m_exteriorRing->m_points.length() == 3;
}

IVertex *HCTriangle::p1() const
{
  return p1Internal();
}

HCVertex *HCTriangle::p1Internal() const
{
  reCreateOuterLineString();

  if(m_exteriorRing->m_points.length() > 0)
  {
    return dynamic_cast<HCVertex*>(m_exteriorRing->m_points[0]);
  }

  return nullptr;
}

IVertex *HCTriangle::p2() const
{
  return p2Internal();
}

HCVertex *HCTriangle::p2Internal() const
{
  reCreateOuterLineString();

  if(m_exteriorRing->m_points.length() > 1)
  {
    return dynamic_cast<HCVertex*>(m_exteriorRing->m_points[1]);
  }

  return nullptr;
}

IVertex *HCTriangle::p3() const
{
  return p3Internal();
}

HCVertex *HCTriangle::p3Internal() const
{
  reCreateOuterLineString();

  if(m_exteriorRing->m_points.length() > 2)
  {
    return dynamic_cast<HCVertex*>(m_exteriorRing->m_points[2]);
  }

  return nullptr;
}
