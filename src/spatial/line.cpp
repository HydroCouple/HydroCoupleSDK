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
