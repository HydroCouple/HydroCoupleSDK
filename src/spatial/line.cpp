#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/linestring.h"
#include <assert.h>


HCLine::HCLine(const QString &id, HCGeometry *parent)
  :HCLineString(id, parent)
{
  m_points.append(nullptr);
  m_points.append(nullptr);
}

HCLine::~HCLine()
{
}

bool HCLine::isValid() const
{
  if(m_points.length() == 2)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool HCLine::addPoint(HCPoint* point)
{
  return false;
 // throw std::logic_error("Cannot append point " + point->id().toStdString() + ". Use set point instead");
}

bool HCLine::removePoint(HCPoint *point)
{
  throw std::logic_error("Cannot remove point " + point->id().toStdString() + ". Use set point instead");
}

void HCLine::setP1(HCPoint* point)
{

  m_points[0] = point;

  if(point)
  {
    point->setGeometryFlag(GeometryFlag::HasZ , this->is3D());
    point->setGeometryFlag(GeometryFlag::HasM , this->isMeasured());

    setIsEmpty(false);
  }
}

void HCLine::setP2(HCPoint *point)
{
  m_points[1] = point;
  
  if(point)
  {
    point->setGeometryFlag(GeometryFlag::HasZ , this->is3D());
    point->setGeometryFlag(GeometryFlag::HasM , this->isMeasured());
    setIsEmpty(false);
  }

}
