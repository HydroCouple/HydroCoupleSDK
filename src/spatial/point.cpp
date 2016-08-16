#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/geometryfactory.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

unsigned int HCPoint::m_nextId(0);

HCPoint::HCPoint(QObject* parent):
  HCGeometry(parent),
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

HCPoint::HCPoint(double x, double y, QObject* parent):
  HCGeometry(parent),
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

HCPoint::HCPoint(double x, double y, double z, QObject* parent):
  HCGeometry(parent),
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

HCPoint::HCPoint(double x, double y, double z, double m, QObject* parent):
  HCGeometry(parent),
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
  setGeometryFlag(GeometryFlag::HasZ , true);
}

void HCPoint::setM(double m)
{
  m_m = m;
  setGeometryFlag(GeometryFlag::HasM , true);
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

GeometryType HCPoint::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return HydroCouple::Spatial::PointZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return HydroCouple::Spatial::PointZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return HydroCouple::Spatial::PointM;
  }
  else
  {
    return  HydroCouple::Spatial::Point;
  }
}

IGeometry* HCPoint::envelope() const
{
  return nullptr;
}

bool HCPoint::compare(const IPoint *point) const
{
  if(m_x == point->x() &&
     m_y == point->y() &&
     m_z == point->z() &&
     m_m == point->m())
  {
    return true;
  }

  return false;
}

HCPoint* HCPoint::clone(QObject* parent) const
{
  HCPoint* p1 = new HCPoint(m_x, m_y,parent);

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

HCVertex* HCPoint::cloneToVertex(QObject *parent) const
{
  HCVertex* vertex = new HCVertex(m_x,m_y,parent);

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
