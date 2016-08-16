#include "stdafx.h"
#include "spatial/geometry.h"
#include "spatial/spatialreferencesystem.h"
#include "spatial/geometryfactory.h"

#include "hydrocoupleexceptions.h"

#include <QFlags>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCGeometry::HCGeometry(QObject *parent)
  :Identity(QUuid::createUuid().toString(),parent),
    m_index(-1),
    m_isEmpty(true),
    m_dataLength(0),
    m_data(nullptr)
{
  m_srs = new SpatialReferenceSystem("Default SRS" , "EPSG" , 4326 , this);
}

HCGeometry::HCGeometry(const QString &id, QObject *parent)
  :Identity(id,parent),
    m_index(-1),
    m_isEmpty(true),
    m_dataLength(0),
    m_data(nullptr)
{
  m_srs = new SpatialReferenceSystem("Default SRS" , "EPSG" , 4326 , this);
}

HCGeometry::~HCGeometry()
{
  if(m_data)
  {
    delete[] m_data;
  }
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
  if(isEmpty())
    return 0;
  else if(m_geomFlags.testFlag(GeometryFlag::HasZ))
    return 3;
  else
    return 2;
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
  geometry->exportToWkt(&wktCharText,wkbVariantIso);
  QString wktString(wktCharText);

  delete[] wktCharText;

  return wktString;
}

unsigned char* HCGeometry::wkb(int &size) const
{
  OGRGeometry* geom = GeometryFactory::exportToOGRGeometry(this);
  size = geom->WkbSize();

  unsigned char* wkbData = new unsigned char[geom->WkbSize()];
  geom->exportToWkb(wkbXDR,wkbData,wkbVariantIso);

  delete geom;

  return wkbData;
}

bool HCGeometry::isEmpty() const
{
  return m_isEmpty;
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

  IGeometry* outBoundGeom = GeometryFactory::importFromOGRGeometry(bound);

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
  throw NotImplementedException("HCGeometry", "relate");
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

  HCGeometry* outBufferG = GeometryFactory::importFromOGRGeometry(outBuffer);

  delete gthis;
  delete outBuffer;

  return outBufferG;
}

IGeometry* HCGeometry::convexHull() const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* ogrConvexHull = gthis->ConvexHull();
  IGeometry* outConvexHull = GeometryFactory::importFromOGRGeometry(ogrConvexHull);

  return outConvexHull ;
}

IGeometry* HCGeometry::intersection(const IGeometry *geom) const
{
  OGRGeometry* gthis = GeometryFactory::exportToOGRGeometry(this);
  OGRGeometry* inpGeom = GeometryFactory::exportToOGRGeometry(geom);
  OGRGeometry* ogrIntersection = gthis->Intersection(inpGeom);

  delete gthis;
  delete inpGeom;

  IGeometry* outIntersection =GeometryFactory::importFromOGRGeometry(ogrIntersection);

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

  IGeometry* outUnion = GeometryFactory::importFromOGRGeometry(ogrUnion);

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

  IGeometry* outDiff =GeometryFactory::importFromOGRGeometry(ogrDiff);

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

  IGeometry* outSymDiff =GeometryFactory::importFromOGRGeometry(ogrSymDiff);

  delete ogrSymDiff;

  return outSymDiff;
}

HCGeometry::GeometryFlags HCGeometry::geometryFlags() const
{
  return m_geomFlags;
}

void HCGeometry::setGeometryFlag(GeometryFlag flag, bool on)
{
  m_geomFlags = on ? m_geomFlags |= flag : m_geomFlags &= ~flag;
}

void HCGeometry::initializeData(int length)
{
  if(m_data)
  {
    delete[] m_data;
    m_data = nullptr;
  }

  if(length > 0)
  {
    m_data = new double[length];
  }
}

int HCGeometry::dataLength() const
{
  return m_dataLength;
}

double* HCGeometry::data() const
{
  return m_data;
}

void HCGeometry::setData(double value, int index)
{
  m_data[index] = value;
}

void HCGeometry::setIsEmpty(bool isEmpty)
{
  m_isEmpty = isEmpty;
}
