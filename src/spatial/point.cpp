#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/geometryfactory.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCPoint::HCPoint(QObject* parent):
  HCGeometry(parent),
  m_x(0),
  m_y(0),
  m_z(0),
  m_m(0)
{
  setGeometryFlag(GeometryFlag::HasZ,false);
  setGeometryFlag(GeometryFlag::HasM,false);
  setGeometryFlag(GeometryFlag::IsEmpty,true);
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
  setGeometryFlag(GeometryFlag::IsEmpty,false);
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
  setGeometryFlag(GeometryFlag::IsEmpty,false);
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
  setGeometryFlag(GeometryFlag::IsEmpty,false);
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

unsigned char* HCPoint::wkb(int &size) const
{
  OGRGeometry* geom = GeometryFactory::exportToOGRGeometry(this);
  size = geom->WkbSize();

  unsigned char* wkbData = new unsigned char[geom->WkbSize()];
  geom->exportToWkb(wkbXDR,wkbData,wkbVariantIso);

  delete geom;

  return wkbData;
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

double HCPoint::dist(const IPoint *point) const
{
  double xd = m_x - point->x();
  double yd = m_y - point->y();
  double zd = m_z - point->z();
  return sqrt(xd*xd + yd*yd + zd*zd);
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
      default:
        break;
    }
  }
}

