#include "stdafx.h"
#include "spatial/geometryfactory.h"
#include "spatial/spatialreferencesystem.h"

#include <gdal/ogrsf_frmts.h>
#include <cstring>
#include <QDebug>
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

OGRGeometry *GeometryFactory::exportToOGRGeometry(const IGeometry *geometry)
{
  assert(geometry != nullptr);

  switch (geometry->geometryType())
  {
    case HydroCouple::Spatial::Point:
    case HydroCouple::Spatial::PointZ:
    case HydroCouple::Spatial::PointM:
    case HydroCouple::Spatial::PointZM:
      {
        const IPoint *point = dynamic_cast<const IPoint*>(geometry);
        OGRPoint *outPoint = exportToOGRPoint(point);
        return outPoint;
      }
      break;
    case HydroCouple::Spatial::LineString:
    case HydroCouple::Spatial::LineStringZ:
    case HydroCouple::Spatial::LineStringM:
    case HydroCouple::Spatial::LineStringZM:
      {
        const ILineString *lineString  = dynamic_cast<const ILineString*>(geometry);
        OGRLineString *outLineString = exportToOGRLineString(lineString);
        return outLineString;
      }
      break;
    case HydroCouple::Spatial::Polygon:
    case HydroCouple::Spatial::PolygonZ:
    case HydroCouple::Spatial::PolygonM:
    case HydroCouple::Spatial::PolygonZM:
      {
        const IPolygon *polygon  = dynamic_cast<const IPolygon*>(geometry);
        OGRPolygon *outPolygon = exportToOGRPolygon(polygon);
        return outPolygon;
      }
      break;
    default:
      break;
  }

  return nullptr;
}

OGRPoint *GeometryFactory::exportToOGRPoint(const IPoint *point)
{

  assert(point != nullptr);

  OGRPoint* oPoint = nullptr;

  if(point->isEmpty())
  {
    oPoint = new OGRPoint();
    oPoint->getSpatialReference()->SetWellKnownGeogCS(point->spatialReferenceSystem()->srText().toStdString().c_str());
  }
  else
  {
    oPoint = new OGRPoint(point->x(),point->y());
    oPoint->getSpatialReference()->SetWellKnownGeogCS(point->spatialReferenceSystem()->srText().toStdString().c_str());

    if(point->geometryType() == GeometryType::PointZ)
    {
      oPoint->setZ(point->z());
    }
    else if(point->geometryType() == GeometryType::PointM)
    {
      oPoint->setM(point->m());
    }
    else if(point->geometryType() == GeometryType::PointZM)
    {
      oPoint->setZ(point->z());
      oPoint->setM(point->m());
    }
    return oPoint;
  }

  return oPoint;
}

OGRLineString *GeometryFactory::exportToOGRLineString(const ILineString *lineString)
{
  assert(lineString != nullptr);

  OGRLineString* outLineString = new OGRLineString();

  if(lineString->geometryType() == HydroCouple::Spatial::LineString)
  {
    for(int i = 0 ; i < lineString->pointCount() ; i++)
    {
      IPoint* point = lineString->point(i);
      outLineString->addPoint(point->x(),point->y());
    }
  }
  else if(lineString->geometryType() == HydroCouple::Spatial::LineStringZ)
  {
    for(int i = 0 ; i < lineString->pointCount() ; i++)
    {
      IPoint* point = lineString->point(i);
      outLineString->addPoint(point->x(),point->y(), point->z());
    }
  }
  else if(lineString->geometryType() == HydroCouple::Spatial::LineStringM)
  {
    for(int i = 0 ; i < lineString->pointCount() ; i++)
    {
      IPoint* point = lineString->point(i);
      outLineString->addPointM(point->x(),point->y(), point->m());
    }
  }
  else if(lineString->geometryType() == HydroCouple::Spatial::LineStringZM)
  {
    for(int i = 0 ; i < lineString->pointCount() ; i++)
    {
      IPoint* point = lineString->point(i);
      outLineString->addPoint(point->x(),point->y(), point->z(), point->m());
    }
  }

  return outLineString;
}

OGRPolygon *GeometryFactory::exportToOGRPolygon(const IPolygon *polygon)
{
  OGRPolygon* outPolygon = new OGRPolygon();

  outPolygon->set3D(polygon->geometryType() == GeometryType::PolygonZ ||
                    polygon->geometryType() == GeometryType::PolygonZM );

  outPolygon->setMeasured(polygon->geometryType() == GeometryType::PolygonM ||
                          polygon->geometryType() == GeometryType::PolygonZM );



  switch (polygon->geometryType())
  {
    case GeometryType::Polygon:
      {
        OGRLinearRing* externalRing = new OGRLinearRing();
        ILineString *lnString = polygon->exteriorRing();

        for(int i = 0 ; i < lnString->pointCount() ; i++)
        {
          IPoint *p1 = lnString->point(i);
          externalRing->addPoint(p1->x(),p1->y());
        }

        outPolygon->addRing(externalRing);

        for(int i = 0 ; i < polygon->interiorRingCount() ; i++)
        {
          ILineString *interiorRing = polygon->interiorRing(i);
          OGRLinearRing *intRing  = new OGRLinearRing();

          for(int i = 0 ; i < interiorRing->pointCount() ; i++)
          {
            IPoint *p1 = interiorRing->point(i);
            intRing->addPoint(p1->x(),p1->y());
          }

          outPolygon->addRing(intRing);
        }

      }
      break;
    case GeometryType::PolygonZ:
      {
        OGRLinearRing* externalRing = new OGRLinearRing();
        ILineString *lnString = polygon->exteriorRing();

        for(int i = 0 ; i < lnString->pointCount() ; i++)
        {
          IPoint *p1 = lnString->point(i);
          externalRing->addPoint(p1->x(),p1->y(),p1->z());
        }

        outPolygon->addRing(externalRing);

        for(int i = 0 ; i < polygon->interiorRingCount() ; i++)
        {
          ILineString *interiorRing = polygon->interiorRing(i);
          OGRLinearRing *intRing  = new OGRLinearRing();

          for(int i = 0 ; i < interiorRing->pointCount() ; i++)
          {
            IPoint *p1 = interiorRing->point(i);
            intRing->addPoint(p1->x(),p1->y(),p1->z());
          }

          outPolygon->addRing(intRing);
        }
      }
      break;
    case GeometryType::PolygonM:
      {
        OGRLinearRing* externalRing = new OGRLinearRing();
        ILineString *lnString = polygon->exteriorRing();

        for(int i = 0 ; i < lnString->pointCount() ; i++)
        {
          IPoint *p1 = lnString->point(i);
          externalRing->addPointM(p1->x(),p1->y(),p1->m());
        }

        outPolygon->addRing(externalRing);

        for(int i = 0 ; i < polygon->interiorRingCount() ; i++)
        {
          ILineString *interiorRing = polygon->interiorRing(i);
          OGRLinearRing *intRing  = new OGRLinearRing();

          for(int i = 0 ; i < interiorRing->pointCount() ; i++)
          {
            IPoint *p1 = interiorRing->point(i);
            intRing->addPointM(p1->x(),p1->y(),p1->m());
          }

          outPolygon->addRing(intRing);
        }
      }
      break;
    case GeometryType::PolygonZM:
      {
        OGRLinearRing* externalRing = new OGRLinearRing();
        ILineString *lnString = polygon->exteriorRing();

        for(int i = 0 ; i < lnString->pointCount() ; i++)
        {
          IPoint *p1 = lnString->point(i);
          externalRing->addPoint(p1->x(),p1->y(),p1->z(),p1->m());
        }

        outPolygon->addRing(externalRing);

        for(int i = 0 ; i < polygon->interiorRingCount() ; i++)
        {
          ILineString *interiorRing = polygon->interiorRing(i);
          OGRLinearRing *intRing  = new OGRLinearRing();

          for(int i = 0 ; i < interiorRing->pointCount() ; i++)
          {
            IPoint *p1 = interiorRing->point(i);
            intRing->addPoint(p1->x(),p1->y(),p1->z(),p1->m());
          }

          outPolygon->addRing(intRing);
        }
      }
      break;
  }

  return outPolygon;
}

HCGeometry *GeometryFactory::importFromOGRGeometry(const OGRGeometry *geometry)
{
  assert(geometry != nullptr);

  switch (geometry->getGeometryType())
  {
    case wkbPoint:
    case wkbPointM:
    case wkbPointZM:
    case wkbPoint25D:
      {
        const OGRPoint* inPoint = static_cast<const OGRPoint*>(geometry);
        HCPoint* outPoint = importFromOGRPoint(inPoint);
        return outPoint;
      }
      break;
    case wkbLineString:
    case wkbLineStringM:
    case wkbLineStringZM:
    case wkbLineString25D:
      {
        const OGRLineString* inLineString = static_cast<const OGRLineString*>(geometry);
        HCLineString* outLineString = importFromOGRLineString(inLineString);
        return outLineString;
      }
    default:
      break;
  }

  return nullptr;
}

HCPoint *GeometryFactory::importFromOGRPoint(const OGRPoint *point)
{
  if(point->IsEmpty())
  {
    HCPoint* oPoint = new HCPoint();
    SpatialReferenceSystem* srs = dynamic_cast<SpatialReferenceSystem*>(oPoint->spatialReferenceSystem());

    if(srs && point->getSpatialReference())
    {
      char* wkt = nullptr;
      point->getSpatialReference()->exportToWkt(&wkt);
      srs->setSrText(QString(wkt));
      if(wkt)
        delete[] wkt;
    }

    return oPoint;
  }
  else
  {
    HCPoint* oPoint = new HCPoint(point->getX() , point->getY());

    SpatialReferenceSystem* srs = dynamic_cast<SpatialReferenceSystem*>(oPoint->spatialReferenceSystem());

    if(srs && point->getSpatialReference())
    {
      char* wkt = nullptr;
      point->getSpatialReference()->exportToWkt(&wkt);
      srs->setSrText(QString(wkt));
      if(wkt)
        delete[] wkt;
    }

    if(point->Is3D())
    {
      oPoint->setZ(point->getZ());
    }

    if(point->IsMeasured())
    {
      oPoint->setM(point->getM());
    }

    return oPoint;
  }
}

HCLineString *GeometryFactory::importFromOGRLineString(const OGRLineString *lineString)
{
  HCLineString* outLineString = new HCLineString();

  for(int i = 0 ; i < lineString->getNumPoints() ; i++)
  {
    double x = lineString->getX(i);
    double y = lineString->getY(i);

    HCPoint* point = new HCPoint(x,y, outLineString);
    outLineString->addPoint(point);
  }

  if(lineString->getGeometryType() == wkbLineStringM)
  {
    outLineString->enableM();
    for(int i = 0 ; i < lineString->getNumPoints() ; i++)
    {
      outLineString->points()[i]->setM(lineString->getM(i));
    }
  }
  else if(lineString->getGeometryType() == wkbLineStringZM)
  {
    outLineString->enableM();
    outLineString->enable3D();

    for(int i = 0 ; i < lineString->getNumPoints() ; i++)
    {
      HCPoint* p = outLineString->points()[i];
      p->setM(lineString->getM(i));
      p->setZ(lineString->getZ(i));
    }
  }

  return outLineString;
}

HCPolygon *GeometryFactory::importFromOGRPolygon(const OGRPolygon *polygon)
{
  return nullptr;
}

IGeometry* GeometryFactory::importFromWkt(const QString &wktData)
{
  OGRGeometry* geometry = nullptr;
  char* wktDataCpy =  new char[wktData.length() + 1 ];
  strcpy(wktDataCpy , wktData.toStdString().c_str());
  char* wktDataCpyCpy = wktDataCpy;

  OGRSpatialReference* srs  = new OGRSpatialReference();

  OGRErr error;

  if( (error = OGRGeometryFactory::createFromWkt(&wktDataCpy,srs,&geometry)) == OGRERR_NONE)
  {
    IGeometry* oGeom = importFromOGRGeometry(geometry);

    delete[] wktDataCpyCpy;
    delete geometry;
    return  oGeom;
  }
  else
  {
    delete[] wktDataCpyCpy;
    delete srs;
    return nullptr;
  }

}

IGeometry* GeometryFactory::importFromWkb(unsigned char *wkbData, int nBytes)
{
  OGRGeometry* geometry = nullptr;
  OGRSpatialReference* srs  = new OGRSpatialReference();

  if(OGRGeometryFactory::createFromWkb(wkbData,srs,&geometry,nBytes,wkbVariantIso) == OGRERR_NONE)
  {
    IGeometry* oGeom = importFromOGRGeometry(geometry);
    delete geometry;
    return  oGeom;
  }
  else
  {
    delete srs;
    return nullptr;
  }
}
