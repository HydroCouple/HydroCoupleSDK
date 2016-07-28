#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/linestring.h"

HCLine::HCLine(QObject *parent)
  :HCLineString(parent)
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

void HCLine::addPoint(HCPoint* point)
{
   throw std::logic_error("Cannot append point. Use set point instead");
}

bool HCLine::removePoint(HCPoint *point)
{
  throw std::logic_error("This is a line and must have two points");
}

void HCLine::setP1(HCPoint* point)
{
   m_points[0] = point;
   
   if(point)
   {
     point->setGeometryFlag(GeometryFlag::HasZ , this->is3D());
     point->setGeometryFlag(GeometryFlag::HasM , this->isMeasured());
   }
}

void HCLine::setP2(HCPoint *point)
{
  m_points[1] = point;
  
  if(point)
  {
    point->setGeometryFlag(GeometryFlag::HasZ , this->is3D());
    point->setGeometryFlag(GeometryFlag::HasM , this->isMeasured());
  }
}
