#include "stdafx.h"
#include "spatial/geometry.h"
#include "spatial/spatialreferencesystem.h"
#include "spatial/geometryfactory.h"
#include <QFlags>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCGeometry::HCGeometry(QObject *parent)
  :Identity(QUuid::createUuid().toString(),parent),
    m_index(-1),
    m_coordinateDimension(0)
{

  m_srs = new SpatialReferenceSystem("Default SRS" , "EPSG" , 4326 , this);
}

HCGeometry::HCGeometry(const QString &id, QObject *parent)
  :Identity(id,parent),
    m_index(-1),
    m_coordinateDimension(0)
{
  m_srs = new SpatialReferenceSystem("Default SRS" , "EPSG" , 4326 , this);
}

HCGeometry::~HCGeometry()
{
}

unsigned int HCGeometry::index() const
{
  return m_index;
}

void HCGeometry::setIndex(unsigned int index)
{
  m_index = index;
}

int HCGeometry::coordinateDimension() const
{
  return m_coordinateDimension;
}

HydroCouple::Spatial::GeometryType HCGeometry::geometryType() const
{
  if(m_geomFlags.testFlag(GeometryFlag::HasZ) &&
     m_geomFlags.testFlag(GeometryFlag::HasM))
  {
    return GeometryType::GeometryZM;
  }
  else if(m_geomFlags.testFlag(GeometryFlag::HasZ))
  {
    return GeometryType::GeometryZ;
  }
  else if(m_geomFlags.testFlag(GeometryFlag::HasM))
  {
    return GeometryType::GeometryM;
  }
  else
  {
    return GeometryType::Geometry;
  }
}

ISpatialReferenceSystem* HCGeometry::spatialReferenceSystem() const
{
  return m_srs;
}

QString HCGeometry::wkt() const
{
  char* wktCharText = nullptr;
  OGRGeometry* geometry = GeometryFactory::exportToOGRGeometry(this);
  geometry->exportToWkt(&wktCharText);
  QString wktString(wktCharText);

  delete[] wktCharText;

  return wktString;
}

bool HCGeometry::isEmpty() const
{
  return m_geomFlags.testFlag(GeometryFlag::IsEmpty);
  //throw std::logic_error("Missing isEmpty() method for a class");
}

bool HCGeometry::isSimple() const
{
  OGRGeometry* geom = GeometryFactory::exportToOGRGeometry(this);
  bool simple = geom->IsSimple();
  delete geom;

  return simple;
}

bool HCGeometry::is3D() const
{
  return m_geomFlags.testFlag(GeometryFlag::HasZ);
}

bool HCGeometry::isMeasured() const
{
  return m_geomFlags.testFlag(GeometryFlag::HasM);
}

IGeometry* HCGeometry::boundary() const
{
  OGRGeometry* geom = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* bound = geom->Boundary();

  IGeometry* outBoundGeom = GeometryFactory::exportFromOGRGeometry(bound);

  delete geom;
  delete bound;
  return outBoundGeom;
}

bool HCGeometry::equals(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);
  
  bool outEquals = gthis->Equals(inpGeom);
  
  delete gthis;
  delete inpGeom;
  
  return outEquals;
}

bool HCGeometry::disjoint(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);

  bool outDisjoint = gthis->Disjoint(inpGeom);

  delete gthis;
  delete inpGeom;

  return outDisjoint;
}

bool HCGeometry::intersects(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);

  bool outIntersects = gthis->Intersects(inpGeom);

  delete gthis;
  delete inpGeom;

  return outIntersects;
}

bool HCGeometry::touches(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);

  bool outTouches = gthis->Touches(inpGeom);

  delete gthis;
  delete inpGeom;

  return outTouches;
}

bool HCGeometry::crosses(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);

  bool outCrosses = gthis->Crosses(inpGeom);

  delete gthis;
  delete inpGeom;

  return outCrosses;
}

bool HCGeometry::within(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);

  bool outWithin = gthis->Within(inpGeom);

  delete gthis;
  delete inpGeom;

  return outWithin;
}

bool HCGeometry::contains(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);

  bool outContains = gthis->Contains(inpGeom);

  delete gthis;
  delete inpGeom;

  return outContains;
}

bool HCGeometry::overlaps(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);

  bool outOverlaps = gthis->Overlaps(inpGeom);

  delete gthis;
  delete inpGeom;

  return outOverlaps;
}

double HCGeometry::distance(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);

  double outDistance = gthis->Distance(inpGeom);

  delete gthis;
  delete inpGeom;

  return outDistance;
}

bool HCGeometry::relate(const IGeometry *geom) const
{
  return false;
}

IGeometry* HCGeometry::locateAlong(double value) const
{
  return nullptr;
}

IGeometry* HCGeometry::locateBetween(double mStart, double mEnd) const
{
  return nullptr;
}

IGeometry* HCGeometry::buffer(double bufferDistance) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* outBuffer = gthis->Buffer(bufferDistance,30);

  IGeometry* outBufferG = GeometryFactory::exportFromOGRGeometry(outBuffer);

  delete gthis;
  delete outBuffer;

  return outBufferG;
}

IGeometry* HCGeometry::convexHull() const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* ogrConvexHull = gthis->ConvexHull();
  IGeometry* outConvexHull = GeometryFactory::exportFromOGRGeometry(ogrConvexHull);

  return outConvexHull ;
}

IGeometry* HCGeometry::intersection(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);
  OGRGeometry* ogrIntersection = gthis->Intersection(inpGeom);

  delete gthis;
  delete inpGeom;

  IGeometry* outIntersection =GeometryFactory::exportFromOGRGeometry(ogrIntersection);

  delete ogrIntersection;

  return outIntersection;
}

IGeometry* HCGeometry::unionG(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);
  OGRGeometry* ogrUnion = gthis->Union(inpGeom);

  delete gthis;
  delete inpGeom;

  IGeometry* outUnion =GeometryFactory::exportFromOGRGeometry(ogrUnion);

  delete ogrUnion;

  return outUnion;
}

IGeometry* HCGeometry::difference(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);
  OGRGeometry* ogrDiff = gthis->Difference(inpGeom);

  delete gthis;
  delete inpGeom;

  IGeometry* outDiff =GeometryFactory::exportFromOGRGeometry(ogrDiff);

  delete ogrDiff;

  return outDiff;
}

IGeometry* HCGeometry::symmetricDifference(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);
  OGRGeometry* ogrSymDiff = gthis->Difference(inpGeom);

  delete gthis;
  delete inpGeom;

  IGeometry* outSymDiff =GeometryFactory::exportFromOGRGeometry(ogrSymDiff);

  delete ogrSymDiff;

  return outSymDiff;
}

void HCGeometry::setCoordinateDimension(int dimension)
{
  m_coordinateDimension = dimension;
}

void HCGeometry::setGeometryFlag(GeometryFlag flag, bool on)
{
  m_geomFlags = on ? (m_geomFlags |= flag) : (m_geomFlags &= ~flag);
}

HCGeometry::GeometryFlags HCGeometry::geometryFlags() const
{
  return m_geomFlags;
}
