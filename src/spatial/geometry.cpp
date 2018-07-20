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
#include "spatial/envelope.h"
#include "spatial/geometry.h"
#include "spatial/spatialreferencesystem.h"
#include "spatial/geometryfactory.h"

#include "hydrocoupleexceptions.h"
#include <ogrsf_frmts.h>

#include <QFlags>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCGeometry::HCGeometry(const QString &id, HCGeometry *parent):
  m_envelope(nullptr),
  m_id(id),
  m_marker(-1),
  m_index(-1),
  m_isEmpty(true),
  m_srs(nullptr),
  m_geomFlags(GeometryFlag::None),
  m_parent(parent)
{
  if(m_parent != nullptr)
  {
    m_parent->m_children.insert(this);
  }
  else
  {
    m_srs = new SpatialReferenceSystem();
  }

  m_envelope = new Envelope();
}

HCGeometry::~HCGeometry()
{

  if(m_envelope)
  {
    delete m_envelope;
    m_envelope = nullptr;
  }

  if(m_srs)
  {
    delete m_srs;
    m_srs = nullptr;
  }

  if(m_parent && m_parent->m_children.size())
  {
    std::set<HCGeometry*>::iterator it =  m_parent->m_children.find(this);

    if(it != m_parent->m_children.end())
    {
      m_parent->m_children.erase(it);
    }
  }

  while(m_children.size())
  {
    HCGeometry *geometry = *m_children.begin();
    m_children.erase(m_children.begin());
    delete geometry;
  }
}

QString HCGeometry::id() const
{
  return m_id;
}

void HCGeometry::setId(const QString &id)
{
  m_id = id;
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

HydroCouple::Spatial::IGeometry::GeometryType HCGeometry::geometryType() const
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
  return spatialReferenceSystemInternal();
}

SpatialReferenceSystem *HCGeometry::spatialReferenceSystemInternal() const
{
  return m_parent != nullptr ? m_parent->m_srs : m_srs;
}

QString HCGeometry::getWKT() const
{
  char* wktCharText = nullptr;
  OGRGeometry* geometry = GeometryFactory::exportToOGRGeometry(this);
  geometry->exportToWkt(&wktCharText,wkbVariantIso);
  QString wktString(wktCharText);

  delete[] wktCharText;

  return wktString;
}

unsigned char* HCGeometry::getWKB(int &size) const
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

IEnvelope *HCGeometry::envelope() const
{
  return envelopeInternal();
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

  IGeometry* outIntersection = GeometryFactory::importFromOGRGeometry(ogrIntersection);

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

void HCGeometry::setIsEmpty(bool isEmpty)
{
  m_isEmpty = isEmpty;
}

int HCGeometry::marker() const
{
  return m_marker;
}

void HCGeometry::setMarker(int index)
{
  m_marker = index;
}

std::set<HCGeometry*> HCGeometry::children() const
{
  return m_children;
}

void HCGeometry::setParent(HCGeometry *parent)
{
  if(m_parent)
  {
    m_parent->m_children.erase(this);
  }

  m_parent = parent;
  m_parent->m_children.insert(this);
}

QString HCGeometry::geometryTypeToString(GeometryType type)
{
  switch (type)
  {
    case GeometryType::Geometry:
      return "HCGeometry";
      break;
    case GeometryType::Point:
      return "Point";

      break;
    case GeometryType::LineString:
      return "LineString";

      break;
    case GeometryType::Polygon:
      return "Polygon";

      break;
    case GeometryType::Triangle:
      return "Triangle";

      break;
    case GeometryType::MultiPoint:
      return "MultiPoint";

      break;
    case GeometryType::MultiLineString:
      return "MultiLineString";

      break;
    case GeometryType::MultiPolygon:
      return "MultiPolygon";

      break;
    case GeometryType::GeometryCollection:
      return "GeometryCollection";

      break;
    case GeometryType::CircularString:
      return "CircularString";

      break;
    case GeometryType::CompoundCurve:
      return "CompoundCurve";

      break;
    case GeometryType::CurvePolygon:
      return "CurvePolygon";

      break;
    case GeometryType::MultiCurve:
      return "MultiCurve";

      break;
    case GeometryType::MultiSurface:
      return "MultiSurface";

      break;
    case GeometryType::Curve:
      return "Curve";

      break;
    case GeometryType::Surface:
      return "Surface";

      break;
    case GeometryType::PolyhedralSurface:
      return "PolyhedralSurface";

      break;
    case GeometryType::TIN:
      return "TIN";

      break;
    case GeometryType::GeometryZ:
      return "GeometryZ";

      break;
    case GeometryType::PointZ:
      return "PointZ";

      break;
    case GeometryType::LineStringZ:
      return "LineStringZ";

      break;
    case GeometryType::PolygonZ:
      return "PolygonZ";

      break;
    case GeometryType::TriangleZ:
      return "TriangleZ";

      break;
    case GeometryType::MultiPointZ:
      return "MultiPointZ";

      break;
    case GeometryType::MultiLineStringZ:
      return "MultiLineStringZ";

      break;
    case GeometryType::MultiPolygonZ:
      return "MultiPolygonZ";

      break;
    case GeometryType::GeometryCollectionZ:
      return "GeometryCollectionZ";

      break;
    case GeometryType::CircularStringZ:
      return "CircularStringZ";

      break;
    case GeometryType::CompoundCurveZ:
      return "CompoundCurveZ";

      break;
    case GeometryType::CurvePolygonZ:
      return "CurvePolygonZ";

      break;
    case GeometryType::MultiCurveZ:
      return "MultiCurveZ";

      break;
    case GeometryType::MultiSurfaceZ:
      return "MultiSurfaceZ";

      break;
    case GeometryType::CurveZ:
      return "CurveZ";

      break;
    case GeometryType::SurfaceZ:
      return "SurfaceZ";

      break;
    case GeometryType::PolyhedralSurfaceZ:
      return "PolyhedralSurfaceZ";

      break;
    case GeometryType::TINZ:
      return "TINZ";

      break;
    case GeometryType::GeometryM :
      return "GeometryM";

      break;
    case GeometryType::PointM:
      return "PointM";

      break;
    case GeometryType::LineStringM:
      return "LineStringM";

      break;
    case GeometryType::PolygonM:
      return "PolygonM";

      break;
    case GeometryType::TriangleM:
      return "TriangleM";

      break;
    case GeometryType::MultiPointM :
      return "MultiPointM";

      break;
    case GeometryType::MultiLineStringM:
      return "MultiLineStringM";

      break;
    case GeometryType::MultiPolygonM :
      return "MultiPolygonM";

      break;
    case GeometryType::GeometryCollectionM :
      return "GeometryCollectionM";

      break;
    case GeometryType::CircularStringM:
      return "CircularStringM";

      break;
    case GeometryType::CompoundCurveM:
      return "CompoundCurveM";

      break;
    case GeometryType::CurvePolygonM:
      return "CurvePolygonM";

      break;
    case GeometryType::MultiCurveM :
      return "MultiCurveM";

      break;
    case GeometryType::MultiSurfaceM :
      return "MultiSurfaceM";

      break;
    case GeometryType::CurveM :
      return "CurveM";

      break;
    case GeometryType::SurfaceM :
      return "SurfaceM";

      break;
    case GeometryType::PolyhedralSurfaceM:
      return "PolyhedralSurfaceM";

      break;
    case GeometryType::TINM :
      return "TINM";

      break;
    case GeometryType::GeometryZM :
      return "GeometryZM";

      break;
    case GeometryType::PointZM :
      return "PointZM";

      break;
    case GeometryType::LineStringZM:
      return "LineStringZM";

      break;
    case GeometryType::PolygonZM :
      return "PolygonZM";

      break;
    case GeometryType::TriangleZM:
      return "TriangleZM";

      break;
    case GeometryType::MultiPointZM :
      return "MultiPointZM";

      break;
    case GeometryType::MultiLineStringZM :
      return "MultiLineStringZM";

      break;
    case GeometryType::MultiPolygonZM:
      return "MultiPolygonZM";

      break;
    case GeometryType::GeometryCollectionZM:
      return "GeometryCollectionZM";

      break;
    case GeometryType::CircularStringZM:
      return "CircularStringZM";

      break;
    case GeometryType::CompoundCurveZM:
      return "CompoundCurveZM";

      break;
    case GeometryType::CurvePolygonZM:
      return "CurvePolygonZM";

      break;
    case GeometryType::MultiCurveZM:
      return "MultiCurveZM";

      break;
    case GeometryType::MultiSurfaceZM:
      return "MultiSurfaceZM";

      break;
    case GeometryType::CurveZM:
      return "CurveZM";

      break;
    case GeometryType::SurfaceZM:
      return "SurfaceZM";

      break;
    case GeometryType::PolyhedralSurfaceZM:
      return "PolyhedralSurfaceZM";

      break;
    case GeometryType::TINZM:
      return "TINZM";

      break;
  }
}

OGRwkbGeometryType HCGeometry::toOGRDataType(GeometryType type)
{
  switch (type)
  {
    case GeometryType::Geometry:
      return  wkbUnknown;
      break;
    case GeometryType::Point:
      return wkbPoint;
      break;
    case GeometryType::LineString:
      return wkbLineString;

      break;
    case GeometryType::Polygon:
      return wkbPolygon;

      break;
    case GeometryType::Triangle:
      return wkbTriangle;

      break;
    case GeometryType::MultiPoint:
      return wkbMultiPoint;
      break;
    case GeometryType::MultiLineString:
      return wkbMultiLineString;
      break;
    case GeometryType::MultiPolygon:
      return wkbMultiPolygon;

      break;
    case GeometryType::GeometryCollection:
      return wkbGeometryCollection;

      break;
    case GeometryType::CircularString:
      return wkbCircularString;

      break;
    case GeometryType::CompoundCurve:
      return wkbCompoundCurve;

      break;
    case GeometryType::CurvePolygon:
      return wkbCurvePolygon;

      break;
    case GeometryType::MultiCurve:
      return wkbMultiCurve;

      break;
    case GeometryType::MultiSurface:
      return wkbMultiSurface;

      break;
    case GeometryType::Curve:
      return wkbCurve;

      break;
    case GeometryType::Surface:
      return wkbSurface;

      break;
    case GeometryType::PolyhedralSurface:
      return wkbPolyhedralSurface;

      break;
    case GeometryType::TIN:
      return wkbTIN;

      break;
    case GeometryType::GeometryZ:
      return wkbUnknown;

      break;
    case GeometryType::PointZ:
      return wkbPoint25D;

      break;
    case GeometryType::LineStringZ:
      return wkbLineString25D;

      break;
    case GeometryType::PolygonZ:
      return wkbPolygon25D;

      break;
    case GeometryType::TriangleZ:
      return wkbTriangleZ;

      break;
    case GeometryType::MultiPointZ:
      return wkbMultiPoint25D;

      break;
    case GeometryType::MultiLineStringZ:
      return wkbMultiLineString25D;

      break;
    case GeometryType::MultiPolygonZ:
      return wkbMultiPolygon25D;

      break;
    case GeometryType::GeometryCollectionZ:
      return wkbGeometryCollection25D;

      break;
    case GeometryType::CircularStringZ:
      return wkbCircularStringZ;

      break;
    case GeometryType::CompoundCurveZ:
      return wkbCompoundCurveZ;

      break;
    case GeometryType::CurvePolygonZ:
      return wkbCurvePolygonZ;

      break;
    case GeometryType::MultiCurveZ:
      return wkbMultiCurveZ;

      break;
    case GeometryType::MultiSurfaceZ:
      return wkbMultiSurfaceZ;

      break;
    case GeometryType::CurveZ:
      return wkbCurveZ;

      break;
    case GeometryType::SurfaceZ:
      return wkbSurfaceZ;

      break;
    case GeometryType::PolyhedralSurfaceZ:
      return wkbPolyhedralSurfaceZ;

      break;
    case GeometryType::TINZ:
      return wkbTINZ;

      break;
    case GeometryType::GeometryM :
      return wkbUnknown;

      break;
    case GeometryType::PointM:
      return wkbPointM;

      break;
    case GeometryType::LineStringM:
      return wkbLineStringM;

      break;
    case GeometryType::PolygonM:
      return wkbPolygonM;

      break;
    case GeometryType::TriangleM:
      return wkbTriangleM;

      break;
    case GeometryType::MultiPointM :
      return wkbMultiPointM;

      break;
    case GeometryType::MultiLineStringM:
      return wkbMultiLineStringM;

      break;
    case GeometryType::MultiPolygonM :
      return wkbMultiPolygonM;

      break;
    case GeometryType::GeometryCollectionM :
      return wkbGeometryCollectionM;

      break;
    case GeometryType::CircularStringM:
      return wkbCircularStringM;

      break;
    case GeometryType::CompoundCurveM:
      return wkbCompoundCurveM;

      break;
    case GeometryType::CurvePolygonM:
      return wkbCurvePolygonM;

      break;
    case GeometryType::MultiCurveM :
      return wkbMultiCurveM;

      break;
    case GeometryType::MultiSurfaceM :
      return wkbMultiSurfaceM;

      break;
    case GeometryType::CurveM :
      return wkbCurveM;

      break;
    case GeometryType::SurfaceM :
      return wkbSurfaceM;

      break;
    case GeometryType::PolyhedralSurfaceM:
      return wkbPolyhedralSurfaceM;

      break;
    case GeometryType::TINM :
      return wkbTINM;

      break;
    case GeometryType::GeometryZM :
      return wkbUnknown;

      break;
    case GeometryType::PointZM :
      return wkbPointZM;

      break;
    case GeometryType::LineStringZM:
      return wkbLineStringZM;

      break;
    case GeometryType::PolygonZM :
      return wkbPolygonZM;

      break;
    case GeometryType::TriangleZM:
      return wkbTriangleZM;

      break;
    case GeometryType::MultiPointZM :
      return wkbMultiPointZM;

      break;
    case GeometryType::MultiLineStringZM :
      return wkbMultiLineStringZM;

      break;
    case GeometryType::MultiPolygonZM:
      return wkbMultiPolygonZM;

      break;
    case GeometryType::GeometryCollectionZM:
      return wkbGeometryCollectionZM;

      break;
    case GeometryType::CircularStringZM:
      return wkbCircularStringZM;

      break;
    case GeometryType::CompoundCurveZM:
      return wkbCompoundCurveZM;

      break;
    case GeometryType::CurvePolygonZM:
      return wkbCurvePolygonZM;

      break;
    case GeometryType::MultiCurveZM:
      return wkbMultiCurveZM;

      break;
    case GeometryType::MultiSurfaceZM:
      return wkbMultiSurfaceZM;

      break;
    case GeometryType::CurveZM:
      return wkbCurveZM;

      break;
    case GeometryType::SurfaceZM:
      return wkbSurfaceZM;

      break;
    case GeometryType::PolyhedralSurfaceZM:
      return wkbPolyhedralSurfaceZM;

      break;
    case GeometryType::TINZM:
      return wkbTINZM;

      break;
  }
}
