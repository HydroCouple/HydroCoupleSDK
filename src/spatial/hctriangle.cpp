/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
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
#include "spatial/polygon.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/linestring.h"
#include "spatial/point.h"

#include <assert.h>
#include <QDebug>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCTriangle::HCTriangle(const QString &id, HCGeometry *parent)
  :HCPolygon(id, parent)
{
}

HCTriangle::HCTriangle(const QString &id, HCTIN *tin )
  : HCPolygon(id,tin)
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
  if(m_exteriorRing->m_points.length() > 0)
  {
    return dynamic_cast<HCVertex*>(m_exteriorRing->m_points[0]);
  }

  return nullptr;
}

IGeometry::GeometryType HCTriangle::geometryType() const
{

  //change to triangle later
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return IGeometry::TriangleZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return IGeometry::TriangleZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return IGeometry::TriangleM;
  }
  else
  {
    return  IGeometry::Triangle;
  }
}

IVertex *HCTriangle::p2() const
{
  return p2Internal();
}

HCVertex *HCTriangle::p2Internal() const
{
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
  if(m_exteriorRing->m_points.length() > 2)
  {
    return dynamic_cast<HCVertex*>(m_exteriorRing->m_points[2]);
  }

  return nullptr;
}

double HCTriangle::area() const
{
  if(m_exteriorRing->m_points.size() > 2)
  {
    Vect v22(*m_exteriorRing->m_points[1]);
    Vect v1 = Vect(*m_exteriorRing->m_points[0]) - v22;
    Vect v2 = Vect(*m_exteriorRing->m_points[2]) - v22;

    v1.v[2] = 0;
    v2.v[2] = 0;

    Vect v3 = Vect::crossProduct(v2,v1);

    double a = v3.length() / 2.0;

    return a;
  }
  else
  {
    return 0.0;
  }
}

double HCTriangle::area3D() const
{
  double area = normal().length();
  area /= 2.0;
  return area;
}

Vect HCTriangle::normal() const
{
  HCPoint *p2 = m_exteriorRing->m_points[1];
  Vect v1 = Vect(*m_exteriorRing->m_points[0]) - Vect(*p2);
  Vect v2 = Vect(*m_exteriorRing->m_points[2]) - Vect(*p2);
  Vect v3 = Vect::crossProduct(v2,v1);
  return v3;
}

double HCTriangle::interpolateZ(double x, double y)
{
  HCPoint *v1 = m_exteriorRing->m_points[0];


  Vect normal = this->normal();
  normal.normalize();

  double d = -normal.v[0] * v1->x()
             -normal.v[1] * v1->y()
             -normal.v[2] * v1->z();

  double z = (normal.v[0] * x + normal.v[1] * y + d) / (-normal.v[2]);

  return z;
}

bool HCTriangle::contains(const IGeometry *geom) const
{
  const IPoint* point = dynamic_cast<const IPoint*>(geom);

  if(point != nullptr)
  {
    HCPoint *v1 = m_exteriorRing->m_points[0];
    HCPoint *v2 = m_exteriorRing->m_points[1];
    HCPoint *v3 = m_exteriorRing->m_points[2];

    bool b1, b2, b3;

    b1 = sign(point,v1,v2) < 0.0f;
    b2 = sign(point,v2,v3) < 0.0f;
    b3 = sign(point,v3,v1) < 0.0f;

    return ((b1 == b2) && (b2 == b3));
  }

  return HCPolygon::contains(geom);
}

double HCTriangle::sign(const IPoint *p1, const IPoint *p2, const IPoint *p3)
{
  return (p1->x() - p3->x()) * (p2->y() - p3->y()) - (p2->x() - p3->x()) * (p1->y() - p3->y());
}

double HCTriangle::sign(const Vect &p1, const Vect &p2, const Vect &p3)
{
  return (p1.v[0] - p3.v[0]) * (p2.v[1] - p3.v[1]) - (p2.v[0] - p3.v[0]) * (p1.v[1] - p3.v[1]);
}

double HCTriangle::triArea3D(const IPoint *p1, const IPoint *p2, const IPoint *p3)
{
  Vect v1 = Vect(*p1) - Vect(*p2);
  Vect v2 = Vect(*p3) - Vect(*p2);
  Vect v3 = Vect::crossProduct(v2,v1);
  return v3.length() / 2.0;
}

double HCTriangle::triArea3D(const Vect &p1, const Vect &p2, const Vect &p3)
{
  Vect v1 = p1 - p2;
  Vect v2 = p3 - p2;
  Vect v3 = Vect::crossProduct(v2,v1);
  return v3.length() / 2.0;
}

double HCTriangle::signedVolume(const IPoint *p1, const IPoint *p2, const IPoint *p3)
{
  double v321 = p3->x()*p2->y()*p1->z();
  double v231 = p2->x()*p3->y()*p1->z();
  double v312 = p3->x()*p1->y()*p2->z();
  double v132 = p1->x()*p3->y()*p2->z();
  double v213 = p2->x()*p1->y()*p3->z();
  double v123 = p1->x()*p2->y()*p3->z();

  return (1.0/6.0)*(-v321 + v231 + v312 - v132 - v213 + v123);
}

double HCTriangle::signedVolume(const Vect &p1, const Vect &p2, const Vect &p3)
{
  double v321 = p3.v[0] * p2.v[1]  *p1.v[2];
  double v231 = p2.v[0] * p3.v[1] * p1.v[2];
  double v312 = p3.v[0] * p1.v[1] * p2.v[2];
  double v132 = p1.v[0] * p3.v[1] * p2.v[2];
  double v213 = p2.v[0] * p1.v[1] * p3.v[2];
  double v123 = p1.v[0] * p2.v[1] * p3.v[2];

  return (1.0/6.0)*(-v321 + v231 + v312 - v132 - v213 + v123);
}
