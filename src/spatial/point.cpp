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
#include "spatial/point.h"
#include "spatial/geometryfactory.h"
#include "spatial/envelope.h"

#include <math.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

unsigned int HCPoint::m_nextId(0);

HCPoint::HCPoint(const QString &id, HCGeometry *parent):
  HCGeometry(id, parent),
  m_x(0),
  m_y(0),
  m_z(0),
  m_m(0)
{
  setGeometryFlag(GeometryFlag::HasZ,false);
  setGeometryFlag(GeometryFlag::HasM,false);
  setIsEmpty(false);
  setIndex(HCPoint::getNextId());
}


HCPoint::HCPoint(double x, double y, const QString &id, HCGeometry *parent):
  HCGeometry(id, parent),
  m_x(x),
  m_y(y),
  m_z(0),
  m_m(0)
{
  setGeometryFlag(GeometryFlag::HasZ,false);
  setGeometryFlag(GeometryFlag::HasM,false);
  setIsEmpty(false);
  setIndex(HCPoint::getNextId());
}

HCPoint::HCPoint(double x, double y, double z, const QString &id, HCGeometry *parent):
  HCGeometry(id, parent),
  m_x(x),
  m_y(y),
  m_z(z),
  m_m(0)
{
  setGeometryFlag(GeometryFlag::HasZ,true);
  setGeometryFlag(GeometryFlag::HasM,false);
  setIsEmpty(false);
  setIndex(HCPoint::getNextId());
}

HCPoint::HCPoint(double x, double y, double z, double m, const QString &id, HCGeometry *parent):
  HCGeometry(id, parent),
  m_x(x),
  m_y(y),
  m_z(z),
  m_m(m)
{
  setGeometryFlag(GeometryFlag::HasZ,true);
  setGeometryFlag(GeometryFlag::HasM,true);
  setIsEmpty(false);
  setIndex(HCPoint::getNextId());
}

HCPoint::~HCPoint()
{
}

double HCPoint::x() const
{
  return m_x;
}

double HCPoint::y() const
{
  return m_y;
}

double HCPoint::z() const
{
  return m_z;
}

double HCPoint::m() const
{
  return m_m;
}

void HCPoint::setX(double x)
{
  m_x = x;
}

void HCPoint::setY(double y)
{
  m_y = y;
}

void HCPoint::setZ(double z)
{
  m_z = z;
}

void HCPoint::setM(double m)
{
  m_m = m;
}

void HCPoint::enable3D()
{
  setGeometryFlag(GeometryFlag::HasZ , true);
}

void HCPoint::disable3D()
{
  m_z = 0;
  setGeometryFlag(GeometryFlag::HasZ , false);
}

void HCPoint::enableM()
{
  setGeometryFlag(GeometryFlag::HasM , true);
}

void HCPoint::disableM()
{
  m_m = 0;
  setGeometryFlag(GeometryFlag::HasM , false);
}

int HCPoint::dimension() const
{
  return 0;
}

IGeometry::GeometryType HCPoint::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return IGeometry::PointZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return IGeometry::PointZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return IGeometry::PointM;
  }
  else
  {
    return IGeometry::Point;
  }
}

Envelope *HCPoint::envelopeInternal() const
{
  double eps = std::numeric_limits<double>::epsilon();
  m_envelope->setMinX(m_x - eps);
  m_envelope->setMaxX(m_x + eps);
  m_envelope->setMinY(m_y - eps);
  m_envelope->setMaxY(m_y + eps);
  m_envelope->setMinZ(m_z - eps);
  m_envelope->setMaxZ(m_z + eps);

  return m_envelope;
}

bool HCPoint::equals(const IPoint *point, double epsilon, bool threeDimensional) const
{
  double dz = 0;
  double dx   = m_x - point->x();
  double dy   = m_y - point->y();

  if(threeDimensional)
  {
    dz = m_z - point->z();
  }

  double size = sqrt(dx * dx + dy*dy + dz*dz);

  return size <= epsilon;
}

HCPoint* HCPoint::clone() const
{
  HCPoint* p1 = new HCPoint(m_x, m_y);

  if(geometryFlags().testFlag(HasZ))
  {
    p1->enable3D();
    p1->setZ(m_z);
  }

  if(geometryFlags().testFlag(HasM))
  {
    p1->enableM();
    p1->setM(m_m);
  }

  return p1;
}

HCVertex* HCPoint::cloneToVertex() const
{
  HCVertex* vertex = new HCVertex(m_x,m_y);

  if(geometryFlags().testFlag(HasZ))
  {
    vertex->enable3D();
    vertex->setZ(m_z);
  }

  if(geometryFlags().testFlag(HasM))
  {
    vertex->enableM();
    vertex->setM(m_m);
  }

  return vertex;
}

void HCPoint::setGeometryFlag(HCGeometry::GeometryFlag flag, bool on)
{
  if(!on)
  {
    switch (flag)
    {
      case HCGeometry::HasZ:
        {
          m_z = 0.0 ;
        }
        break;
      case HCGeometry::HasM:
        {
          m_m = 0.0;
        }
        break;
      case HCGeometry::None:
      default:
        break;
    }
  }

  HCGeometry::setGeometryFlag(flag,on);
}

double HCPoint::dist(const HCPoint *p1 , const HCPoint* p2)
{
  double xd = p1->m_x - p2->m_x;
  double yd = p1->m_y - p2->m_y;
  double zd = p1->m_z - p2->m_z;
  return sqrt(xd*xd + yd*yd + zd*zd);
}

HCPoint* HCPoint::crossProduct(const HCPoint *p1 , const HCPoint* p2)
{
  double x = (p1->m_y * p2->m_z) - (p1->m_z * p2->m_y);
  double y = (p1->m_z * p2->m_x) - (p1->m_x * p2->m_z);
  double z = (p1->m_x * p2->m_y) - (p1->m_y * p2->m_x);

  return new HCPoint(x,y,z);
}

double HCPoint::dotProduct(const HCPoint *p1 , const HCPoint* p2)
{
  return (p1->m_x * p2->m_x) + (p1->m_y * p2->m_y) + (p1->m_z * p2->m_z);
}

unsigned int HCPoint::getNextId()
{
  unsigned int tid = m_nextId;
  m_nextId++;
  return tid;
}

void HCPoint::resetId()
{
  m_nextId = 0;
}

//==========================================================================================================================================

Vect::Vect(const Vect &point)
{
  v = new double[3]{point.v[0], point.v[1], point.v[2]};

}


Vect::Vect(const IPoint &point)
{
  v = new double[3]{point.x(), point.y(), point.z()};
}

Vect::Vect(const IVertex &point)
{
  v = new double[3]{point.x(), point.y(), point.z()};

}

Vect::Vect(double x, double y, double z)
{
  v = new double[3]{x, y, z};
}

Vect &Vect::operator =(const Vect &vect)
{
//  v = new double[3]();

  for(int i = 0; i < 3; i++)
  {
    v[i] = vect.v[i];
  }

  return *this;
}


Vect::~Vect()\
{
  delete[] v;
}

double Vect::x() const
{
  return v[0];
}

double Vect::y() const
{
  return v[1];
}

double Vect::z() const
{
  return v[2];
}

double Vect::length() const
{
  return sqrt(lengthSquared());
}

double Vect::lengthSquared() const
{
  double l = 0;

  for(int i = 0; i < 3; i++)
    l += v[i] * v[i];

  return l;
}

void Vect::normalize()
{
  double l = length();

  for(int i = 0; i < 3; i++)
    v[i] = v[i] / l;
}

Vect Vect::normalized() const
{
  Vect ov;

  double l = length();

  for(int i = 0; i < 3; i++)
    ov.v[i] = v[i] / l;

  return ov;
}

Vect Vect::normal2dToVector() const
{
  Vect ov;

  ov.v[0] = v[1];
  ov.v[1] = -v[0];

  return ov;
}

Vect Vect::unitNormal2dToVector() const
{
  Vect ov;
  double l = length();
  ov.v[0] = v[1] / l ;
  ov.v[1] = -v[0] / l;
  return ov;
}

Vect &Vect::operator *=(double factor)
{
  for(int i = 0; i < 3; i++)
    v[i] *= factor;

  return *this;
}

Vect &Vect::operator *=(const Vect &vector)
{
  for(int i = 0; i < 3; i++)
    v[i] *= vector.v[i];

  return *this;
}

Vect &Vect::operator +=(const Vect &vector)
{
  for(int i = 0; i < 3; i++)
    v[i] += vector.v[i];

  return *this;
}

Vect &Vect::operator -=(const Vect &vector)
{
  for(int i = 0; i < 3; i++)
    v[i] -= vector.v[i];

  return *this;
}

Vect &Vect::operator /=(double divisor)
{
  for(int i = 0; i < 3; i++)
    v[i] /= divisor;

  return *this;
}

Vect &Vect::operator /= (const Vect &vector)
{
  for(int i = 0; i < 3; i++)
    v[i] /= vector.v[i];

  return *this;
}

double &Vect::operator [](const int index)
{
  return v[index];
}

const double &Vect::operator [](const int index) const
{
  return v[index];
}

Vect operator *(const Vect &v, double factor)
{
  Vect ov;

  for(int i = 0; i < 3; i++)
    ov.v[i] = v.v[i] * factor;

  return ov;
}

Vect operator *(double factor, const Vect &v)
{
  Vect ov;

  for(int i = 0; i < 3; i++)
    ov.v[i] = v.v[i] * factor;

  return ov;
}

Vect operator *(const Vect &v1 , const Vect &v2)
{
  Vect ov;

  for(int i = 0; i < 3; i++)
    ov.v[i] = v1.v[i] * v2.v[i];

  return ov;
}

Vect operator +(const Vect &v1 , const Vect &v2)
{
  Vect ov;

  for(int i = 0; i < 3; i++)
    ov.v[i] = v1.v[i] + v2.v[i];

  return ov;
}

Vect operator -(const Vect &v1 , const Vect &v2)
{

  Vect ov;

  for(int i = 0; i < 3; i++)
    ov.v[i] = v1.v[i] - v2.v[i];

  return ov;
}

Vect operator /(const Vect &v, double divisor)
{
  Vect vv;

  for(int i = 0; i < 3; i++)
    vv.v[i] = v.v[i] / divisor;

  return vv;
}

Vect operator /(const Vect &v1 , const Vect &v2)
{
  Vect vv;

  for(int i = 0; i < 3; i++)
    vv.v[i] = v1.v[i] / v2.v[i];

  return vv;
}

double Vect::dotProduct(const Vect& u, const Vect &v)
{
  double prod = 0.0;

  for(int i = 0; i < 3; i++)
    prod += u.v[i] * v.v[i];

  return prod;
}

double Vect::dotProduct(double x, double y, double z, const Vect &v)
{
  return x * v.v[0] + y*v.v[1] + z * v.v[2];
}

double Vect::dotProduct(double x1, double y1, double z1, double x2, double y2, double z2)
{
  return x1 * x2 + y1 * y2 + z1 * z2;
}

Vect Vect::crossProduct(const Vect& u, const Vect &v)
{
  Vect ov;

  ov.v[0] = u.v[1] * v.v[2] - u.v[2] * v.v[1];
  ov.v[1] = u.v[2] * v.v[0] - u.v[0] * v.v[2];
  ov.v[2] = u.v[0] * v.v[1] - u.v[1] * v.v[0];

  return ov;
}

bool Vect::linesIntersect2d(const Vect &l1p1, const Vect &l1p2, const Vect &l2p1, const Vect &l2p2, Vect &outVect)
{
  double denom = ((l2p2.v[1] - l2p1.v[1])*(l1p2.v[0] - l1p1.v[0]) - (l2p2.v[0]-l2p1.v[0])*(l1p2.v[1]-l1p1.v[1]));

  if(denom)
  {
    double l1u = (l2p2.v[0] - l2p1.v[0])*(l1p1.v[1] - l2p1.v[1]) - (l2p2.v[1] - l2p1.v[1])*(l1p1.v[0] - l2p1.v[0]);

    //    double l2u = (l1p2.x - l1p1.x)*(l1p1.y - l2p1.y) - (l1p2.y - l1p1.y)*(l1p1.x - l2p1.x); other line

    if(l1u <= 0.0 && l1u <= 1.0 )
    {
      outVect = l1p1 + (l1p2 - l1p1)*l1u;
      return true;
    }
  }

  return false;
}

Vect Vect::unitX()
{
  Vect v(1.0);
  return v;
}

Vect Vect::unitY()
{
  Vect v(0.0, 1.0);
  return v;
}

Vect Vect::unitZ()
{
  Vect v(0.0,0.0,1.0);
  return v;
}

void Vect::print()
{
  printf("x:%f\ty:%f\tz:%f\n",v[0],v[1],v[2]);
}

void Vect::zero()
{
   v[0] = v[1] = v[2] = 0.0;
}
