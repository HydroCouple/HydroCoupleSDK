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
#include "spatial/geometryfactory.h"
#include "spatial/spatialreferencesystem.h"
#include "spatial/geometry.h"
#include "spatial/point.h"
#include "spatial/linestring.h"
#include "spatial/polygon.h"
#include "spatial/geometry.h"
#include "spatial/geometryargument.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/geometrycollection.h"
#include "spatial/envelope.h"
#include "spatial/edge.h"
#include "spatiotemporal/timegeometrycomponentdataitem.h"

#include <cstring>
#include <QDebug>
#include <assert.h>
#include <QVariant>
#include <iostream>
#include <QFile>

#include <netcdf>
#include "ogrsf_frmts.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace netCDF;
using namespace netCDF::exceptions;

bool GeometryFactory::m_GDALRegistered = false;

OGRGeometry *GeometryFactory::exportToOGRGeometry(const IGeometry *geometry)
{
  assert(geometry != nullptr);

  switch (geometry->geometryType())
  {
    case IGeometry::Point:
    case IGeometry::PointZ:
    case IGeometry::PointM:
    case IGeometry::PointZM:
      {
        const IPoint *point = dynamic_cast<const IPoint*>(geometry);
        OGRPoint *outPoint = exportToOGRPoint(point);
        return outPoint;
      }
      break;
    case IGeometry::LineString:
    case IGeometry::LineStringZ:
    case IGeometry::LineStringM:
    case IGeometry::LineStringZM:
      {
        const ILineString *lineString  = dynamic_cast<const ILineString*>(geometry);
        OGRLineString *outLineString = exportToOGRLineString(lineString);
        return outLineString;
      }
      break;
    case IGeometry::Polygon:
    case IGeometry::PolygonZ:
    case IGeometry::PolygonM:
    case IGeometry::PolygonZM:
    case IGeometry::Triangle:
    case IGeometry::TriangleZ:
    case IGeometry::TriangleM:
    case IGeometry::TriangleZM:
      {
        const IPolygon *polygon  = dynamic_cast<const IPolygon*>(geometry);
        OGRPolygon *outPolygon = exportToOGRPolygon(polygon);
        return outPolygon;
      }
      break;
    case IGeometry::MultiPoint:
    case IGeometry::MultiPointZ:
    case IGeometry::MultiPointM:
    case IGeometry::MultiPointZM:
      {
        const IMultiPoint *mpoint  = dynamic_cast<const IMultiPoint*>(geometry);
        OGRMultiPoint *outMPoint = exportToOGRMultiPoint(mpoint);
        return outMPoint;
      }
      break;
    case IGeometry::MultiLineString:
    case IGeometry::MultiLineStringZ:
    case IGeometry::MultiLineStringM:
    case IGeometry::MultiLineStringZM:
      {
        const IMultiLineString *mlineString  = dynamic_cast<const IMultiLineString*>(geometry);
        OGRMultiLineString *outMLineString = exportToOGRMultiLineString(mlineString);
        return outMLineString;
      }
      break;
    case IGeometry::MultiPolygon:
    case IGeometry::MultiPolygonZ:
    case IGeometry::MultiPolygonM:
    case IGeometry::MultiPolygonZM:
      {
        const IMultiPolygon *mpolygon  = dynamic_cast<const IMultiPolygon*>(geometry);
        OGRMultiPolygon *outMPolygon = exportToOGRMultiPolygon(mpolygon);
        return outMPolygon;
      }
      break;
    default:
      break;
  }

  //throw I am lazy exception
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

    if(point->geometryType() == IGeometry::PointZ)
    {
      oPoint->setZ(point->z());
    }
    else if(point->geometryType() == IGeometry::PointM)
    {
      oPoint->setM(point->m());
    }
    else if(point->geometryType() == IGeometry::PointZM)
    {
      oPoint->setZ(point->z());
      oPoint->setM(point->m());
    }
  }

  return oPoint;
}

OGRLineString *GeometryFactory::exportToOGRLineString(const ILineString *lineString)
{
  assert(lineString != nullptr);

  OGRLineString* outLineString = new OGRLineString();
  outLineString->getSpatialReference()->SetWellKnownGeogCS(lineString->spatialReferenceSystem()->srText().toStdString().c_str());

  if(lineString->geometryType() == IGeometry::LineString)
  {
    for(int i = 0 ; i < lineString->pointCount() ; i++)
    {
      IPoint* point = lineString->point(i);
      outLineString->addPoint(point->x(),point->y());
    }
  }
  else if(lineString->geometryType() == IGeometry::LineStringZ)
  {
    for(int i = 0 ; i < lineString->pointCount() ; i++)
    {
      IPoint* point = lineString->point(i);
      outLineString->addPoint(point->x(),point->y(), point->z());
    }
  }
  else if(lineString->geometryType() == IGeometry::LineStringM)
  {
    for(int i = 0 ; i < lineString->pointCount() ; i++)
    {
      IPoint* point = lineString->point(i);
      outLineString->addPointM(point->x(),point->y(), point->m());
    }
  }
  else if(lineString->geometryType() == IGeometry::LineStringZM)
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
  outPolygon->getSpatialReference()->SetWellKnownGeogCS(polygon->spatialReferenceSystem()->srText().toStdString().c_str());

  outPolygon->set3D(polygon->is3D());
  outPolygon->setMeasured(polygon->isMeasured());

  switch (polygon->geometryType())
  {
    case IGeometry::Polygon:
    case IGeometry::Triangle:
      {
        OGRLinearRing* externalRing = new OGRLinearRing();
        ILineString *lnString = polygon->exteriorRing();

        for(int i = 0 ; i < lnString->pointCount() ; i++)
        {
          IPoint *p1 = lnString->point(i);
          externalRing->addPoint(p1->x(),p1->y());
        }

        outPolygon->addRingDirectly(externalRing);

        for(int i = 0 ; i < polygon->interiorRingCount() ; i++)
        {
          ILineString *interiorRing = polygon->interiorRing(i);
          OGRLinearRing *intRing  = new OGRLinearRing();

          for(int i = 0 ; i < interiorRing->pointCount() ; i++)
          {
            IPoint *p1 = interiorRing->point(i);
            intRing->addPoint(p1->x(),p1->y());
          }

          outPolygon->addRingDirectly(intRing);
        }

      }
      break;
    case IGeometry::PolygonZ:
    case IGeometry::TriangleZ:
      {
        OGRLinearRing* externalRing = new OGRLinearRing();
        ILineString *lnString = polygon->exteriorRing();

        for(int i = 0 ; i < lnString->pointCount() ; i++)
        {
          IPoint *p1 = lnString->point(i);
          externalRing->addPoint(p1->x(),p1->y(),p1->z());
        }

        outPolygon->addRingDirectly(externalRing);

        for(int i = 0 ; i < polygon->interiorRingCount() ; i++)
        {
          ILineString *interiorRing = polygon->interiorRing(i);
          OGRLinearRing *intRing  = new OGRLinearRing();

          for(int i = 0 ; i < interiorRing->pointCount() ; i++)
          {
            IPoint *p1 = interiorRing->point(i);
            intRing->addPoint(p1->x(),p1->y(),p1->z());
          }

          outPolygon->addRingDirectly(intRing);
        }
      }
      break;
    case IGeometry::PolygonM:
    case IGeometry::TriangleM:
      {
        OGRLinearRing* externalRing = new OGRLinearRing();
        ILineString *lnString = polygon->exteriorRing();

        for(int i = 0 ; i < lnString->pointCount() ; i++)
        {
          IPoint *p1 = lnString->point(i);
          externalRing->addPointM(p1->x(),p1->y(),p1->m());
        }

        outPolygon->addRingDirectly(externalRing);

        for(int i = 0 ; i < polygon->interiorRingCount() ; i++)
        {
          ILineString *interiorRing = polygon->interiorRing(i);
          OGRLinearRing *intRing  = new OGRLinearRing();

          for(int i = 0 ; i < interiorRing->pointCount() ; i++)
          {
            IPoint *p1 = interiorRing->point(i);
            intRing->addPointM(p1->x(),p1->y(),p1->m());
          }

          outPolygon->addRingDirectly(intRing);
        }
      }
      break;
    case IGeometry::PolygonZM:
    case IGeometry::TriangleZM:
      {
        OGRLinearRing* externalRing = new OGRLinearRing();
        ILineString *lnString = polygon->exteriorRing();

        for(int i = 0 ; i < lnString->pointCount() ; i++)
        {
          IPoint *p1 = lnString->point(i);
          externalRing->addPoint(p1->x(),p1->y(),p1->z(),p1->m());
        }

        outPolygon->addRingDirectly(externalRing);

        for(int i = 0 ; i < polygon->interiorRingCount() ; i++)
        {
          ILineString *interiorRing = polygon->interiorRing(i);
          OGRLinearRing *intRing  = new OGRLinearRing();

          for(int i = 0 ; i < interiorRing->pointCount() ; i++)
          {
            IPoint *p1 = interiorRing->point(i);
            intRing->addPoint(p1->x(),p1->y(),p1->z(),p1->m());
          }

          outPolygon->addRingDirectly(intRing);
        }
      }
      break;
  }

  return outPolygon;
}

OGRMultiPoint *GeometryFactory::exportToOGRMultiPoint(const IMultiPoint *multiPoint)
{
  OGRMultiPoint *outMultiPoint = new OGRMultiPoint();
  outMultiPoint->getSpatialReference()->SetWellKnownGeogCS(multiPoint->spatialReferenceSystem()->srText().toStdString().c_str());

  outMultiPoint->set3D(multiPoint->is3D());
  outMultiPoint->setMeasured(multiPoint->isMeasured());

  for(int i = 0 ; i < multiPoint->geometryCount() ; i++)
  {
    outMultiPoint->addGeometryDirectly(exportToOGRPoint(multiPoint->point(i)));
  }

  return outMultiPoint;
}

OGRMultiLineString *GeometryFactory::exportToOGRMultiLineString(const IMultiLineString *multiLineString)
{
  OGRMultiLineString *outMultiLineString = new OGRMultiLineString();
  outMultiLineString->getSpatialReference()->SetWellKnownGeogCS(multiLineString->spatialReferenceSystem()->srText().toStdString().c_str());

  outMultiLineString->set3D(multiLineString->is3D());
  outMultiLineString->setMeasured(multiLineString->isMeasured());

  for(int i = 0 ; i < multiLineString->geometryCount() ; i++)
  {
    outMultiLineString->addGeometryDirectly(exportToOGRLineString(multiLineString->lineString(i)));
  }

  return outMultiLineString;
}

OGRMultiPolygon *GeometryFactory::exportToOGRMultiPolygon(const IMultiPolygon *multiPolygon)
{
  OGRMultiPolygon *outMultiPolygon = new OGRMultiPolygon();
  outMultiPolygon->getSpatialReference()->SetWellKnownGeogCS(multiPolygon->spatialReferenceSystem()->srText().toStdString().c_str());

  outMultiPolygon->set3D(multiPolygon->is3D());
  outMultiPolygon->setMeasured(multiPolygon->isMeasured());

  for(int i = 0 ; i < multiPolygon->geometryCount() ; i++)
  {
    outMultiPolygon->addGeometryDirectly(exportToOGRPolygon(multiPolygon->polygon(i)));
  }

  return outMultiPolygon;
}

HCGeometry *GeometryFactory::importFromOGRGeometry(const OGRGeometry *geometry)
{
  //  registerGDAL();
  //  assert(geometry != nullptr);

  switch (geometry->getGeometryType())
  {
    case wkbPoint:
    case wkbPointM:
    case wkbPointZM:
    case wkbPoint25D:
      {
        const OGRPoint* inPoint = dynamic_cast<const OGRPoint*>(geometry);
        HCPoint* outPoint = importFromOGRPoint(inPoint);
        return outPoint;
      }
      break;
    case wkbLineString:
    case wkbLineStringM:
    case wkbLineStringZM:
    case wkbLineString25D:
      {
        const OGRLineString* inLineString = dynamic_cast<const OGRLineString*>(geometry);
        HCLineString* outLineString = importFromOGRLineString(inLineString);
        return outLineString;
      }
      break;
    case wkbPolygon:
    case wkbPolygonM:
    case wkbPolygonZM:
    case wkbPolygon25D:
      {
        const OGRPolygon* inPolygon = dynamic_cast<const OGRPolygon*>(geometry);
        HCPolygon* outPolygon = importFromOGRPolygon(inPolygon);
        return outPolygon;
      }
      break;
    case wkbMultiPoint:
    case wkbMultiPointM:
    case wkbMultiPointZM:
    case wkbMultiPoint25D:
      {
        const OGRMultiPoint* inMPoint = dynamic_cast<const OGRMultiPoint*>(geometry);
        HCMultiPoint* outMPoint = importFromOGRMultiPoint(inMPoint);
        return outMPoint;
      }
      break;

    case wkbMultiLineString:
    case wkbMultiLineStringM:
    case wkbMultiLineStringZM:
    case wkbMultiLineString25D:
      {
        const OGRMultiLineString* inMLineString = dynamic_cast<const OGRMultiLineString*>(geometry);
        HCMultiLineString* outMLineString = importFromOGRMultiLineString(inMLineString);
        return outMLineString;
      }
      break;
    case wkbMultiPolygon:
    case wkbMultiPolygonM:
    case wkbMultiPolygonZM:
    case wkbMultiPolygon25D:
      {
        const OGRMultiPolygon* inMPolygon = dynamic_cast<const OGRMultiPolygon*>(geometry);
        HCMultiPolygon* outMPolygon = importFromOGRMultiPolygon(inMPolygon);
        return outMPolygon;
      }
      break;
      //need to implement the rest
    default:
      break;
  }

  //throw i'm lazy exception
  return nullptr;
}

HCPoint *GeometryFactory::importFromOGRPoint(const OGRPoint *point)
{
  if(point->IsEmpty())
  {
    HCPoint* oPoint = new HCPoint(QUuid::createUuid().toString());
    SpatialReferenceSystem* srs = oPoint->spatialReferenceSystemInternal();

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
    HCPoint* oPoint = nullptr;

    if(point->getGeometryType() == wkbPoint)
    {
      oPoint = new HCPoint(point->getX() , point->getY());

      SpatialReferenceSystem* srs = oPoint->spatialReferenceSystemInternal();

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
    else if(point->getGeometryType() == wkbPoint25D)
    {
      oPoint = new HCPoint(point->getX() , point->getY() , point->getZ());

      SpatialReferenceSystem* srs = oPoint->spatialReferenceSystemInternal();

      if(srs && point->getSpatialReference())
      {
        char* wkt = nullptr;
        point->getSpatialReference()->exportToWkt(&wkt);
        srs->setSrText(QString(wkt));

        if(wkt)
          delete[] wkt;
      }
    }
    else if(point->getGeometryType() == wkbPointZM)
    {
      oPoint = new HCPoint(point->getX() , point->getY() , point->getZ() , point->getM());

      SpatialReferenceSystem* srs = oPoint->spatialReferenceSystemInternal();

      if(srs && point->getSpatialReference())
      {
        char* wkt = nullptr;
        point->getSpatialReference()->exportToWkt(&wkt);
        srs->setSrText(QString(wkt));

        if(wkt)
          delete[] wkt;
      }
    }

    return oPoint;
  }
}

HCLineString *GeometryFactory::importFromOGRLineString(const OGRLineString *lineString)
{
  HCLineString* outLineString = new HCLineString();

  SpatialReferenceSystem* srs = outLineString->spatialReferenceSystemInternal();

  if(srs && lineString->getSpatialReference())
  {
    char* wkt = nullptr;
    lineString->getSpatialReference()->exportToWkt(&wkt);
    srs->setSrText(QString(wkt));
    if(wkt)
      delete[] wkt;
  }

  if(lineString->getGeometryType() == wkbLineString)
  {
    outLineString->disable3D();
    outLineString->disableM();

    for(int i = 0 ; i < lineString->getNumPoints() ; i++)
    {
      double x = lineString->getX(i);
      double y = lineString->getY(i);
      HCPoint* point = new HCPoint(x,y, QUuid::createUuid().toString(), outLineString);
      outLineString->addPoint(point);
    }
  }
  else if(lineString->getGeometryType() == wkbLineString25D)
  {
    outLineString->enable3D();

    for(int i = 0 ; i < lineString->getNumPoints() ; i++)
    {
      double x = lineString->getX(i);
      double y = lineString->getY(i);
      double z = lineString->getZ(i);
      HCPoint* point = new HCPoint(x,y,z,QUuid::createUuid().toString(), outLineString);
      outLineString->addPoint(point);
      point->enable3D();
    }
  }
  else if(lineString->getGeometryType() == wkbLineStringM)
  {
    outLineString->enableM();

    for(int i = 0 ; i < lineString->getNumPoints() ; i++)
    {
      HCPoint* point = new HCPoint(QUuid::createUuid().toString(), outLineString);
      point->enableM();
      point->setX(lineString->getX(i));
      point->setY(lineString->getY(i));
      point->setM(lineString->getM(i));
      outLineString->addPoint(point);
      point->enableM();
    }
  }
  else if(lineString->getGeometryType() == wkbLineStringZM)
  {
    outLineString->enableM();
    outLineString->enable3D();

    for(int i = 0 ; i < lineString->getNumPoints() ; i++)
    {
      HCPoint* point = new HCPoint(QUuid::createUuid().toString(), outLineString);
      point->enableM();
      point->enable3D();

      point->setX(lineString->getX(i));
      point->setY(lineString->getY(i));
      point->setZ(lineString->getZ(i));
      point->setM(lineString->getM(i));

      outLineString->addPoint(point);
    }
  }

  return outLineString;
}

HCPolygon *GeometryFactory::importFromOGRPolygon(const OGRPolygon *polygon)
{
  HCPolygon* outPolygon = new HCPolygon();
  SpatialReferenceSystem* srs = outPolygon->spatialReferenceSystemInternal();

  if(polygon->getSpatialReference())
  {
    char* wkt = nullptr;
    polygon->getSpatialReference()->exportToWkt(&wkt);
    srs->setSrText(QString(wkt));
    if(wkt)
      delete[] wkt;
  }

  if(polygon->getGeometryType() == wkbPolygon)
  {
    for(int i = 0 ; i < polygon->getExteriorRing()->getNumPoints() ; i++)
    {
      double x = polygon->getExteriorRing()->getX(i);
      double y = polygon->getExteriorRing()->getY(i);
      outPolygon->exteriorRingInternal()->addPoint(new HCPoint(x,y,QUuid::createUuid().toString(),outPolygon));
    }

    for(int i = 0 ; i < polygon->getNumInteriorRings() ; i++)
    {
      const OGRLinearRing *inputIntLinearRing = polygon->getInteriorRing(i);
      HCLineString *outIntLinearRing = new HCLineString(QUuid::createUuid().toString(), outPolygon);

      for(int j = 0; j <inputIntLinearRing->getNumPoints() ; j++)
      {
        double x = inputIntLinearRing->getX(j);
        double y = inputIntLinearRing->getY(j);
        outIntLinearRing->addPoint(new HCPoint(x,y,QUuid::createUuid().toString(),outIntLinearRing));
      }

      outPolygon->addInteriorRing(outIntLinearRing);
    }
  }
  else if(polygon->getGeometryType() == wkbPolygon25D)
  {
    outPolygon->enable3D();

    for(int i = 0 ; i < polygon->getExteriorRing()->getNumPoints() ; i++)
    {
      double x = polygon->getExteriorRing()->getX(i);
      double y = polygon->getExteriorRing()->getY(i);
      double z = polygon->getExteriorRing()->getZ(i);

      outPolygon->exteriorRingInternal()->addPoint(new HCPoint(x,y,z,QUuid::createUuid().toString(), outPolygon));
    }

    for(int i = 0 ; i < polygon->getNumInteriorRings() ; i++)
    {
      const OGRLinearRing *inputIntLinearRing = polygon->getInteriorRing(i);
      HCLineString *outIntLinearRing = new HCLineString(QUuid::createUuid().toString(), outPolygon);
      outIntLinearRing->enable3D();

      for(int j = 0; j <inputIntLinearRing->getNumPoints() ; j++)
      {
        double x = inputIntLinearRing->getX(j);
        double y = inputIntLinearRing->getY(j);
        double z = inputIntLinearRing->getZ(j);
        outIntLinearRing->addPoint(new HCPoint(x,y,z, QUuid::createUuid().toString(), outIntLinearRing));
      }

      outPolygon->addInteriorRing(outIntLinearRing);
    }
  }
  else if(polygon->getGeometryType() == wkbLineStringM)
  {
    outPolygon->enableM();

    for(int i = 0 ; i < polygon->getExteriorRing()->getNumPoints() ; i++)
    {
      HCPoint *p = new HCPoint(QUuid::createUuid().toString(), outPolygon);
      p->enableM();
      p->setX(polygon->getExteriorRing()->getX(i));
      p->setY(polygon->getExteriorRing()->getY(i));
      p->setM(polygon->getExteriorRing()->getM(i));
      outPolygon->exteriorRingInternal()->addPoint(p);
    }

    for(int i = 0 ; i < polygon->getNumInteriorRings() ; i++)
    {
      const OGRLinearRing *inputIntLinearRing = polygon->getInteriorRing(i);
      HCLineString *outIntLinearRing = new HCLineString();
      outIntLinearRing->enableM();

      for(int j = 0; j <inputIntLinearRing->getNumPoints() ; j++)
      {
        HCPoint *p = new HCPoint(QUuid::createUuid().toString(), outIntLinearRing);
        p->enableM();
        p->setX(inputIntLinearRing->getX(j));
        p->setY(inputIntLinearRing->getY(j));
        p->setM(inputIntLinearRing->getM(j));

        outIntLinearRing->addPoint(p);
      }

      outPolygon->addInteriorRing(outIntLinearRing);
    }
  }
  else if(polygon->getGeometryType() == wkbLineStringZM)
  {
    outPolygon->enableM();
    outPolygon->enable3D();

    for(int i = 0 ; i < polygon->getExteriorRing()->getNumPoints() ; i++)
    {
      double x = polygon->getExteriorRing()->getX(i);
      double y = polygon->getExteriorRing()->getY(i);
      double z = polygon->getExteriorRing()->getZ(i);
      double m = polygon->getExteriorRing()->getM(i);

      outPolygon->exteriorRingInternal()->addPoint(new HCPoint(x,y,z,m, QUuid::createUuid().toString(), outPolygon));
    }

    for(int i = 0 ; i < polygon->getNumInteriorRings() ; i++)
    {
      const OGRLinearRing *inputIntLinearRing = polygon->getInteriorRing(i);
      HCLineString *outIntLinearRing = new HCLineString();
      outIntLinearRing->enable3D();
      outIntLinearRing->enableM();

      for(int j = 0; j <inputIntLinearRing->getNumPoints() ; j++)
      {
        double x = inputIntLinearRing->getX(j);
        double y = inputIntLinearRing->getY(j);
        double z = inputIntLinearRing->getZ(j);
        double m = inputIntLinearRing->getM(j);
        outIntLinearRing->addPoint(new HCPoint(x,y,z,m, QUuid::createUuid().toString(), outIntLinearRing));
      }

      outPolygon->addInteriorRing(outIntLinearRing);
    }
  }

  return outPolygon;

}

HCMultiPoint *GeometryFactory::importFromOGRMultiPoint(const OGRMultiPoint *multiPoint)
{
  HCMultiPoint *outMultiPoint = new HCMultiPoint();

  if(multiPoint->Is3D())
    outMultiPoint->enable3D();
  if(multiPoint->IsMeasured())
    outMultiPoint->enableM();

  SpatialReferenceSystem* srs = outMultiPoint->spatialReferenceSystemInternal();

  if(srs && multiPoint->getSpatialReference())
  {
    char* wkt = nullptr;
    multiPoint->getSpatialReference()->exportToWkt(&wkt);
    srs->setSrText(QString(wkt));
    if(wkt)
      delete[] wkt;
  }

  for(int i = 0 ; i < multiPoint->getNumGeometries() ; i++)
  {
    HCGeometry *geom = importFromOGRGeometry(multiPoint->getGeometryRef(i));
    outMultiPoint->addGeometry(geom);
  }

  return outMultiPoint;
}

HCMultiLineString *GeometryFactory::importFromOGRMultiLineString(const OGRMultiLineString *multiLineString)
{
  HCMultiLineString *outMultiLineString = new HCMultiLineString();

  if(multiLineString->Is3D())
    outMultiLineString->enable3D();

  if(multiLineString->IsMeasured())
    outMultiLineString->enableM();

  SpatialReferenceSystem* srs = outMultiLineString->spatialReferenceSystemInternal();

  if(srs && multiLineString->getSpatialReference())
  {
    char* wkt = nullptr;
    multiLineString->getSpatialReference()->exportToWkt(&wkt);
    srs->setSrText(QString(wkt));
    if(wkt)
      delete[] wkt;
  }

  for(int i = 0 ; i < multiLineString->getNumGeometries() ; i++)
  {
    HCGeometry *geom = importFromOGRGeometry(multiLineString->getGeometryRef(i));
    outMultiLineString->addGeometry(geom);
  }

  return outMultiLineString;
}

HCMultiPolygon *GeometryFactory::importFromOGRMultiPolygon(const OGRMultiPolygon *multiPolygon)
{
  HCMultiPolygon *outMultiPolygon = new HCMultiPolygon();

  if(multiPolygon->Is3D())
    outMultiPolygon->enable3D();
  if(multiPolygon->IsMeasured())
    outMultiPolygon->enableM();

  SpatialReferenceSystem* srs = outMultiPolygon->spatialReferenceSystemInternal();

  if(srs && multiPolygon->getSpatialReference())
  {
    char* wkt = nullptr;
    multiPolygon->getSpatialReference()->exportToWkt(&wkt);
    srs->setSrText(QString(wkt));
    if(wkt)
      delete[] wkt;
  }

  for(int i = 0 ; i < multiPolygon->getNumGeometries() ; i++)
  {
    HCGeometry *geom = importFromOGRGeometry(multiPolygon->getGeometryRef(i));
    outMultiPolygon->addGeometry(geom);
  }

  return outMultiPolygon;
}

HCGeometry *GeometryFactory::importFromWkt(const QString &wktData)
{
  OGRGeometry* geometry = nullptr;
  HCGeometry* oGeom = nullptr;

  char* wktDataCpy = new char[wktData.size() + 1];
  strcpy(wktDataCpy, wktData.toStdString().c_str());

  char *wktDataCpyCpy = wktDataCpy;

  OGRSpatialReference srs(nullptr);
  OGRErr error;

  if( (error = OGRGeometryFactory::createFromWkt(&wktDataCpyCpy,&srs,&geometry)) == OGRERR_NONE)
  {
    oGeom = importFromOGRGeometry(geometry);
    delete geometry;
  }

  delete[] wktDataCpy;

  return oGeom;
}

HCGeometry *GeometryFactory::importFromWkb(unsigned char *wkbData, int nBytes)
{
  OGRGeometry* geometry = nullptr;
  OGRSpatialReference* srs  = new OGRSpatialReference();

  if(OGRGeometryFactory::createFromWkb(wkbData,srs,&geometry,nBytes,wkbVariantIso) == OGRERR_NONE)
  {
    HCGeometry *oGeom = importFromOGRGeometry(geometry);
    delete geometry;
    return  oGeom;
  }
  else
  {
    delete srs;
    return nullptr;
  }
}

bool GeometryFactory::writeGeometryToFile(const GeometryComponentDataItem<double> *geometryComponentDataItem,
                                          const QString &dataFieldName, const QString &driverName,
                                          const QString &outputFile, QString &errorMessage)
{

  //need to make sure no overwrite.
  if(geometryComponentDataItem->getGeometryCount() /*&& !QFile::exists(outputFile)*/)
  {
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(driverName.toStdString().c_str());

    if(driver != NULL)
    {
      GDALDataset *data =driver->Create(outputFile.toStdString().c_str(),0,0,0,GDT_Unknown,NULL);

      if(data != NULL)
      {
        OGRSpatialReference srs(geometryComponentDataItem->getGeometry(0)->spatialReferenceSystem()->srText().toStdString().c_str());
        OGRLayer *layer = data->CreateLayer(geometryComponentDataItem->getId().toStdString().c_str(),&srs,HCGeometry::toOGRDataType(geometryComponentDataItem->getGeometryType()));

        OGRFieldDefn  idfield("Id",OFTString);
        layer->CreateField(&idfield);
        OGRFieldDefn  indexField("Index",OFTInteger64);
        layer->CreateField(&indexField);
        OGRFieldDefn  datafield(dataFieldName.toStdString().c_str() , OFTReal);
        layer->CreateField(&datafield);

        for(int i = 0 ; i < geometryComponentDataItem->length() ; i++)
        {
          HCGeometry *geometry = geometryComponentDataItem->getGeometry(i);
          double value = (*geometryComponentDataItem)[i];

          OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
          feature->SetField("Id", geometry->id().toStdString().c_str());
          feature->SetField("Index",(int)geometry->index());
          feature->SetField(dataFieldName.toStdString().c_str(), value);

          OGRGeometry *geom = exportToOGRGeometry(geometry);
          feature->SetGeometryDirectly(geom);

          if(layer->CreateFeature(feature) != OGRERR_NONE)
          {
            OGRFeature::DestroyFeature(feature);
            GDALClose(data);
            return false;
          }

          OGRFeature::DestroyFeature(feature);
        }

        GDALClose(data);
        return true;
      }
      else
      {
        errorMessage = "Driver specified was not able to create file :" + outputFile;
      }
    }
    else
    {
      errorMessage = "Driver specified was not found";
    }
  }
  else
  {
    errorMessage = "No geometries found";
  }

  return false;
}

bool GeometryFactory::writeGeometryToFile(const TimeGeometryComponentDataItem<double> *timeGeometryComponentDataItem,
                                          const QString &driverName, const QString &outputFile, QString &errorMessage)
{
  //need to make sure no overwrite.
  if(timeGeometryComponentDataItem->getGeometryCount() /*&& !QFile::exists(outputFile)*/)
  {
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(driverName.toStdString().c_str());

    if(driver != NULL)
    {
      GDALDataset *data =driver->Create(outputFile.toStdString().c_str(),0,0,0,GDT_Unknown,NULL);

      if(data != NULL)
      {
        OGRSpatialReference srs(timeGeometryComponentDataItem->getGeometry(0)->spatialReferenceSystem()->srText().toStdString().c_str());
        OGRLayer *layer = data->CreateLayer(timeGeometryComponentDataItem->getId().toStdString().c_str(),&srs,HCGeometry::toOGRDataType(timeGeometryComponentDataItem->getGeometryType()));

        OGRFieldDefn  idfield("Id",OFTString);
        layer->CreateField(&idfield);
        OGRFieldDefn  indexField("Index",OFTInteger64);
        layer->CreateField(&indexField);

        for(int i = 0 ; i < timeGeometryComponentDataItem->getGeometryCount() ; i++)
        {
          HCGeometry *geometry = timeGeometryComponentDataItem->getGeometry(i);

          OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
          feature->SetField("Id", geometry->id().toStdString().c_str());
          feature->SetField("Index",(int)geometry->index());

          OGRGeometry *geom = exportToOGRGeometry(geometry);
          feature->SetGeometryDirectly(geom);

          if(layer->CreateFeature(feature) != OGRERR_NONE)
          {
            OGRFeature::DestroyFeature(feature);
            GDALClose(data);
            return false;
          }

          OGRFeature::DestroyFeature(feature);
        }

        GDALClose(data);
        return true;
      }
      else
      {
        errorMessage = "Driver specified was not able to create file :" + outputFile;
      }
    }
    else
    {
      errorMessage = "Driver specified was not found";
    }
  }
  else
  {
    errorMessage = "No geometries found";
  }

  return false;
}


bool GeometryFactory::writeGeometryToFile(const QList<HCGeometry*> &geometries,
                                          const QString &name,
                                          IGeometry::GeometryType  type,
                                          const QString &gdalDriverName, const QString &outputFile, QString &errorMessage)
{
  //need to make sure no overwrite.
  if(geometries.size() /*&& !QFile::exists(outputFile)*/)
  {
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(gdalDriverName.toStdString().c_str());

    if(driver != NULL)
    {
      GDALDataset *data =driver->Create(outputFile.toStdString().c_str(),0,0,0,GDT_Unknown,NULL);

      if(data != NULL)
      {
        OGRSpatialReference srs(geometries.first()->spatialReferenceSystem()->srText().toStdString().c_str());
        OGRLayer *layer = data->CreateLayer(name.toStdString().c_str(),&srs,HCGeometry::toOGRDataType(type));

        OGRFieldDefn  idfield("Id",OFTString);
        layer->CreateField(&idfield);
        OGRFieldDefn  indexField("Index",OFTInteger64);
        layer->CreateField(&indexField);

        for(HCGeometry *geometry : geometries)
        {

          OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
          feature->SetField("Id", geometry->id().toStdString().c_str());
          feature->SetField("Index",(int)geometry->index());

          OGRGeometry *geom = exportToOGRGeometry(geometry);
          feature->SetGeometryDirectly(geom);

          if(layer->CreateFeature(feature) != OGRERR_NONE)
          {
            OGRFeature::DestroyFeature(feature);
            GDALClose(data);
            return false;
          }

          OGRFeature::DestroyFeature(feature);
        }

        GDALClose(data);
        return true;
      }
      else
      {
        errorMessage = "Driver specified was not able to create file :" + outputFile;
      }
    }
    else
    {
      errorMessage = "Driver specified was not found";
    }
  }
  else
  {
    errorMessage = "No geometries found";
  }

  return false;
}

bool GeometryFactory::readGeometryFromFile(const QString &filePath, QList<HCGeometry*> &geometries, Envelope &envelope, QString &errorMessage)
{

  GDALDataset *data = (GDALDataset*) GDALOpenEx(filePath.trimmed().toStdString().c_str(), GDAL_OF_VECTOR,NULL, NULL, NULL);
  envelope.resetExtentsToInfinity();

  if(data != NULL)
  {
    for(int i = 0 ; i < data->GetLayerCount() ; i++)
    {
      OGRLayer *dataLayer = data->GetLayer(i);
      dataLayer->ResetReading();

      OGRFeature *feature;

      while ((feature = dataLayer->GetNextFeature()) != NULL)
      {

        OGRGeometry *ogrGeom = feature->GetGeometryRef();

        HCGeometry* geom = importFromOGRGeometry(ogrGeom);

        if(geom)
        {
          geometries.append(geom);
          envelope.merge(geom->envelope());
        }

        OGRFeature::DestroyFeature(feature);
      }
    }

    GDALClose(data);

    return true;
  }
  else
  {
    errorMessage =  "Opening file failed File: " + filePath;
  }

  return false;
}

bool GeometryFactory::readGeometryFromFile(const QString &filePath, QString &dataFieldName,
                                           GeometryComponentDataItem<double> *geometryComponentDataItem,
                                           Envelope &envelope, QString &errorMessage)
{

  GDALDataset *data = (GDALDataset*) GDALOpenEx(filePath.trimmed().toStdString().c_str(), GDAL_OF_VECTOR,NULL, NULL, NULL);
  envelope.resetExtentsToInfinity();

  if(data != NULL)
  {
    std::vector<double> doubleData;
    QList<QSharedPointer<HCGeometry>> geometries;

    for(int i = 0 ; i < data->GetLayerCount() ; i++)
    {
      OGRLayer *dataLayer = data->GetLayer(i);
      dataLayer->ResetReading();

      if(HCGeometry::toOGRDataType(geometryComponentDataItem->getGeometryType()) == dataLayer->GetGeomType())
      {
        OGRFeatureDefn *dataFDef = dataLayer->GetLayerDefn();

        //find first double
        int fieldD = -1;

        for(int j = 0 ; j < dataFDef->GetFieldCount() ; j++)
        {
          OGRFieldDefn *fieldDef = dataFDef->GetFieldDefn(j);
          QString fieldName = QString(fieldDef->GetNameRef());;

          if(!fieldName.compare(dataFieldName))
          {
            fieldD = j;
            break;
          }
        }

        OGRFeature *feature ;

        if(dataLayer->GetGeomType() == wkbPoint ||
           dataLayer->GetGeomType() == wkbPointM ||
           dataLayer->GetGeomType() == wkbPoint25D ||
           dataLayer->GetGeomType() == wkbPointZM)
        {
          if(fieldD > -1)
          {
            while ((feature = dataLayer->GetNextFeature()) != NULL)
            {
              doubleData.push_back(feature->GetFieldAsDouble(fieldD));

              OGRGeometry *ogrGeom = feature->GetGeometryRef();
              HCPoint* geom = importFromOGRPoint(dynamic_cast<OGRPoint*>(ogrGeom));
              geometries.append(QSharedPointer<HCGeometry>(geom));
              envelope.merge(geom->envelope());
              OGRFeature::DestroyFeature(feature);
            }
          }
          else
          {
            while ((feature = dataLayer->GetNextFeature()) != NULL)
            {
              OGRGeometry *ogrGeom = feature->GetGeometryRef();
              HCPoint* geom = importFromOGRPoint(dynamic_cast<OGRPoint*>(ogrGeom));
              geometries.append(QSharedPointer<HCGeometry>(geom));
              envelope.merge(geom->envelope());
              OGRFeature::DestroyFeature(feature);
            }
          }
        }
        else
        {
          if(fieldD > -1)
          {
            while ((feature = dataLayer->GetNextFeature()) != NULL)
            {

              doubleData.push_back(feature->GetFieldAsDouble(fieldD));

              OGRGeometry *ogrGeom = feature->GetGeometryRef();

              HCGeometry* geom = importFromOGRGeometry(ogrGeom);
              envelope.merge(geom->envelope());

              geometries.append(QSharedPointer<HCGeometry>(geom));
              envelope.merge(geom->envelope());

              OGRFeature::DestroyFeature(feature);
            }
          }
          else
          {
            while ((feature = dataLayer->GetNextFeature()) != NULL)
            {
              OGRGeometry *ogrGeom = feature->GetGeometryRef();

              HCGeometry* geom = importFromOGRGeometry(ogrGeom);
              envelope.merge(geom->envelope());
              geometries.append(QSharedPointer<HCGeometry>(geom));
              envelope.merge(geom->envelope());

              OGRFeature::DestroyFeature(feature);
            }
          }
        }
      }
    }

    if(geometries.length())
    {
      geometryComponentDataItem->addGeometries(geometries);

      if(doubleData.size() && doubleData.size() == (size_t)geometries.length())
        geometryComponentDataItem->setValuesT(0,doubleData.size(),doubleData.data());
    }

    GDALClose(data);

    return true;
  }
  else
  {
    errorMessage =  "Opening file failed File: " + filePath;
  }

  return false;
}

bool GeometryFactory::readGeometryFromFile(const QString &filePath, TimeGeometryComponentDataItem<double> *timeGeometryComponentDataItem,
                                           Envelope &envelope, QString &errorMessage)
{

  GDALDataset *data = (GDALDataset*) GDALOpenEx(filePath.trimmed().toStdString().c_str(), GDAL_OF_VECTOR,NULL, NULL, NULL);
  envelope.resetExtentsToInfinity();

  if(data != NULL)
  {

    QList<QSharedPointer<HCGeometry>> geometries;

    for(int i = 0 ; i < data->GetLayerCount() ; i++)
    {
      OGRLayer *dataLayer = data->GetLayer(i);
      dataLayer->ResetReading();

      if(HCGeometry::toOGRDataType(timeGeometryComponentDataItem->getGeometryType()) == dataLayer->GetGeomType())
      {
        OGRFeature *feature ;

        if(dataLayer->GetGeomType() == wkbPoint ||
           dataLayer->GetGeomType() == wkbPointM ||
           dataLayer->GetGeomType() == wkbPoint25D ||
           dataLayer->GetGeomType() == wkbPointZM)
        {
          while ((feature = dataLayer->GetNextFeature()) != NULL)
          {
            OGRGeometry *ogrGeom = feature->GetGeometryRef();
            HCPoint* geom = importFromOGRPoint(dynamic_cast<OGRPoint*>(ogrGeom));
            geometries.append(QSharedPointer<HCGeometry>(geom));
            envelope.merge(geom->envelope());
            OGRFeature::DestroyFeature(feature);
          }
        }
        else
        {
          while ((feature = dataLayer->GetNextFeature()) != NULL)
          {
            OGRGeometry *ogrGeom = feature->GetGeometryRef();

            HCGeometry* geom = importFromOGRGeometry(ogrGeom);
            envelope.merge(geom->envelope());
            geometries.append(QSharedPointer<HCGeometry>(geom));

            OGRFeature::DestroyFeature(feature);
          }
        }
      }
    }

    if(geometries.length())
    {
      timeGeometryComponentDataItem->addGeometries(geometries);
    }

    GDALClose(data);

    return true;
  }
  else
  {
    errorMessage =  "Opening file failed File: " + filePath;
  }

  return false;
}

bool GeometryFactory::writeTINToNetCDF(HCTIN *tin, const QString &filePath, QString &errorMessage)
{
  try
  {

    NcFile dataFile(filePath.toStdString() , NcFile::replace);
    dataFile.putAtt("name",tin->id().toStdString());
    dataFile.putAtt("type","HCTIN");
    dataFile.putAtt("version","1.0.0");

    int vertCount = tin->vertexCount();
    int patchCount = tin->patchCount();

    NcDim verticesDim = dataFile.addDim("vertices",vertCount);
    NcDim trianglesDim = dataFile.addDim("triangles",patchCount);

    NcVar vertexX = dataFile.addVar("x",NcType::nc_DOUBLE, verticesDim);
    vertexX.putAtt("x:long_name","triangle_node_x_coordinate");
    vertexX.putAtt("x:units","");

    NcVar vertexY = dataFile.addVar("y",NcType::nc_DOUBLE, verticesDim);
    vertexY.putAtt("y:long_name","triangle_node_y_coordinate");
    vertexY.putAtt("y:units","");

    NcVar vertexZ = dataFile.addVar("z",NcType::nc_DOUBLE, verticesDim);
    vertexZ.putAtt("z:long_name","triangle_node_z_coordinate");
    vertexZ.putAtt("z:units","");

    double *vertx = new double[tin->vertexCount()];
    double *verty = new double[tin->vertexCount()];
    double *vertz = new double[tin->vertexCount()];


    for(int i = 0; i < vertCount ; i++)
    {
      HCVertex* vertex = tin->vertexInternal(i);
      vertex->setMarker(i);
      vertx[i] = vertex->x();
      verty[i] = vertex->y();
      vertz[i] = vertex->z();
    }

    //add patches later
    vertexX.putVar(vertx);
    vertexY.putVar(verty);
    vertexZ.putVar(vertz);

    delete[] vertx;
    delete[] verty;
    delete[] vertz;

    //add triangles

    NcVar triP1 = dataFile.addVar("trip1",NcType::nc_INT, trianglesDim);
    triP1.putAtt("trip1:long_name","triangle_node_p1_coordinate_index");
    triP1.putAtt("trip1:units","");

    NcVar triP2 = dataFile.addVar("trip2",NcType::nc_INT, trianglesDim);
    triP2.putAtt("trip2:long_name","triangle_node_p2_coordinate_index");
    triP2.putAtt("trip2:units","");

    NcVar triP3 = dataFile.addVar("trip3",NcType::nc_INT, trianglesDim);
    triP3.putAtt("trip3:long_name","triangle_node_p1_coordinate_index");
    triP3.putAtt("trip3:units","");

    NcVar triCx = dataFile.addVar("tri_cx", NcType::nc_DOUBLE, trianglesDim);
    triCx.putAtt("tri_cx:long_name","triangle_centroid_x_coordinate");
    triCx.putAtt("tri_cx:units","");

    NcVar triCy = dataFile.addVar("tri_cy", NcType::nc_DOUBLE, trianglesDim);
    triCy.putAtt("tri_cy:long_name","triangle_centroid_y_coordinate");
    triCy.putAtt("tri_cy:units","");

    NcVar triCz = dataFile.addVar("tri_cz", NcType::nc_DOUBLE, trianglesDim);
    triCz.putAtt("tri_cz:long_name","triangle_centroid_z_coordinate");
    triCz.putAtt("tri_cz:units","");

    int *p1 = new int[patchCount];
    int *p2 = new int[patchCount];
    int *p3 = new int[patchCount];

    double *cx = new double[patchCount];
    double *cy = new double[patchCount];
    double *cz = new double[patchCount];

    for(int i = 0; i < patchCount; i++)
    {
      HCTriangle *triangle = tin->triangleInternal(i);
      triangle->setMarker(i);

      HCPoint *pn1 = triangle->p1Internal();
      HCPoint *pn2 = triangle->p2Internal();
      HCPoint *pn3 = triangle->p3Internal();

      cx[i] = (pn1->x() + pn2->x() + pn3->x()) / 3.0;
      cy[i] = (pn1->y() + pn2->y() + pn3->y()) / 3.0;
      cz[i] = (pn1->z() + pn2->z() + pn3->z()) / 3.0;

      p1[i] = triangle->p1Internal()->marker();
      p2[i] = triangle->p2Internal()->marker();
      p3[i] = triangle->p3Internal()->marker();

    }

    triP1.putVar(p1);
    triP2.putVar(p2);
    triP3.putVar(p3);

    triCx.putVar(cx);
    triCy.putVar(cy);
    triCz.putVar(cz);

    delete[] p1;
    delete[] p2;
    delete[] p3;
    delete[] cx;
    delete[] cy;
    delete[] cz;

    /*NcDim edgesDim = */dataFile.addDim("edges",3);

    return true;
  }
  catch(NcException &e)
  {
    errorMessage = QString(e.what());
  }


  return false;
}

HCTIN *GeometryFactory::readTINFromNetCDF(const QString &filePath, QString &errorMessage)
{
  HCTIN *tin = nullptr;

  try
  {
    NcFile dataFile(filePath.toStdString() , NcFile::read);
    NcGroupAtt attribute = dataFile.getAtt("type");
    std::string fileType;
    attribute.getValues(fileType);

    std::cout << fileType << std::endl;

    if(!fileType.compare("HCTIN"))
    {
      NcDim verticesDim = dataFile.getDim("vertices");
      NcDim trianglesDim = dataFile.getDim("triangles");

      NcVar vertexX = dataFile.getVar("x");
      NcVar vertexY = dataFile.getVar("y");
      NcVar vertexZ = dataFile.getVar("z");

      NcVar triP1 = dataFile.getVar("trip1");
      NcVar triP2 = dataFile.getVar("trip2");
      NcVar triP3 = dataFile.getVar("trip3");

      if(!verticesDim.isNull() && !trianglesDim.isNull() && !vertexX.isNull() &&
         !vertexY.isNull() && !vertexZ.isNull() && !triP1.isNull()  &&
         !triP2.isNull() && !triP3.isNull())
      {
        tin = new HCTIN();
        tin->enable3D();

        int vertCount = verticesDim.getSize();
        int patchCount = trianglesDim.getSize();

        double *vertx = new double[vertCount];
        double *verty = new double[vertCount];
        double *vertz = new double[vertCount];

        vertexX.getVar(vertx);
        vertexY.getVar(verty);
        vertexZ.getVar(vertz);

        for(int i = 0; i < vertCount; i++)
        {
          new HCVertex(vertx[i],verty[i],vertz[i], QUuid::createUuid().toString(), tin);
        }

        delete[] vertx;
        delete[] verty;
        delete[] vertz;

        int *p1 = new int[patchCount];
        int *p2 = new int[patchCount];
        int *p3 = new int[patchCount];

        triP1.getVar(p1);
        triP2.getVar(p2);
        triP3.getVar(p3);

        for(int i = 0; i < patchCount; i++)
        {
          HCVertex *v1 = tin->vertexInternal(p1[i]);
          HCVertex *v2 = tin->vertexInternal(p2[i]);
          HCVertex *v3 = tin->vertexInternal(p3[i]);
          HCTriangle* tri = tin->createTriangle(v1,v2,v3);
          tri->reCreateOuterLineString();
        }

        delete[] p1;
        delete[] p2;
        delete[] p3;

      }
      else
      {
        errorMessage = "One of the required variables was not found";
      }
    }
    else
    {
      errorMessage = "Not a valid netCDF file that contains a HCTIN";
    }
  }
  catch(NcException& e)
  {
    errorMessage = QString(e.what());
  }

  return tin;
}

bool GeometryFactory::writeTINVertices(HCTIN *tin, const QString &filePath, const QString &gdalDriverName)
{

  //need to make sure no overwrite.
  if(tin->vertexCount())
  {
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(gdalDriverName.toStdString().c_str());

    if(driver != NULL)
    {
      GDALDataset *data =driver->Create(filePath.toStdString().c_str(),0,0,0,GDT_Unknown,NULL);

      if(data != NULL)
      {
        OGRSpatialReference srs(tin->spatialReferenceSystem()->srText().toStdString().c_str());
        OGRLayer *layer = data->CreateLayer(tin->id().toStdString().c_str(),&srs,HCGeometry::toOGRDataType(IGeometry::PointZ));

        OGRFieldDefn  idfield("Id",OFTString);
        layer->CreateField(&idfield);
        OGRFieldDefn  indexField("Index",OFTInteger64);
        layer->CreateField(&indexField);

        int vertCount = tin->vertexCount();

        for(int i = 0 ; i < vertCount ; i++)
        {
          HCGeometry *geometry = tin->vertexInternal(i);
          OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
          feature->SetField("Id", geometry->id().toStdString().c_str());
          feature->SetField("Index", (int)geometry->index());

          OGRGeometry *geom = exportToOGRGeometry(geometry);
          feature->SetGeometryDirectly(geom);

          if(layer->CreateFeature(feature) != OGRERR_NONE)
          {
            OGRFeature::DestroyFeature(feature);
            GDALClose(data);
            return false;
          }

          OGRFeature::DestroyFeature(feature);
        }

        GDALClose(data);
        return true;
      }
      else
      {
        //errorMessage = "Driver specified was not able to create file :" + outputFile;
      }
    }
    else
    {
      // errorMessage = "Driver specified was not found";
    }
  }
  else
  {
    //  errorMessage = "No geometries found";
  }

  return false;
}

bool GeometryFactory::writeTINPolygons( HCTIN *tin, const QString &filePath, const QString &gdalDriverName)
{

  //need to make sure no overwrite.
  if(tin->patchCount())
  {
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(gdalDriverName.toStdString().c_str());

    if(driver != NULL)
    {
      GDALDataset *data =driver->Create(filePath.toStdString().c_str(),0,0,0,GDT_Unknown,NULL);

      if(data != NULL)
      {
        OGRSpatialReference srs(tin->spatialReferenceSystem()->srText().toStdString().c_str());
        OGRLayer *layer = data->CreateLayer(tin->id().toStdString().c_str(),&srs,HCGeometry::toOGRDataType(IGeometry::PolygonZ));

        OGRFieldDefn  idfield("Id",OFTString);
        layer->CreateField(&idfield);
        OGRFieldDefn  indexField("Index",OFTInteger64);
        layer->CreateField(&indexField);

        OGRFieldDefn  p1("P1",OFTInteger64);
        layer->CreateField(&p1);

        OGRFieldDefn  p2("P2",OFTInteger64);
        layer->CreateField(&p2);


        OGRFieldDefn  p3("P3",OFTInteger64);
        layer->CreateField(&p3);

        OGRFieldDefn  p4("Area",OFTReal);
        layer->CreateField(&p4);

        for(int i = 0 ; i < tin->patchCount() ; i++)
        {
          HCTriangle *geometry = tin->triangleInternal(i);

          OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
          feature->SetField("Id", geometry->id().toStdString().c_str());
          feature->SetField("Index", (int)geometry->index());
          feature->SetField("P1",(int)geometry->p1()->index());
          feature->SetField("P2",(int)geometry->p2()->index());
          feature->SetField("P3",(int)geometry->p3()->index());
          feature->SetField("Area", geometry->area());

          OGRGeometry *geom = exportToOGRGeometry(geometry);
          feature->SetGeometryDirectly(geom);

          if(layer->CreateFeature(feature) != OGRERR_NONE)
          {
            OGRFeature::DestroyFeature(feature);
            GDALClose(data);
            return false;
          }

          OGRFeature::DestroyFeature(feature);
        }

        GDALClose(data);
        return true;
      }
      else
      {
        //errorMessage = "Driver specified was not able to create file :" + outputFile;
      }
    }
    else
    {
      //errorMessage = "Driver specified was not found";
    }
  }
  else
  {
    // errorMessage = "No geometries found";
  }

  return false;
}

void GeometryFactory::registerGDAL()
{
#ifdef USE_OPENMP
#pragma omp critical (GDALRegister)
#endif
  {
    if(!m_GDALRegistered)
    {
      OGRRegisterAll();
      GDALAllRegister();
      m_GDALRegistered = true;
    }
    else
    {
      std::cout << "gdal registered" <<std::endl;
    }
  }
}

bool GeometryFactory::contains(HCPolygon *polygon, HCPoint *point)
{
  if(polygon->envelopeInternal()->contains(point))
  {
    if(contains(polygon->exteriorRingInternal(),point))
    {
      for(int i = 0 ; i < polygon->interiorRingCount() ; i++)
      {
        HCLineString * interior = polygon->interiorRingInternal(i);

        if(interior->envelopeInternal()->contains(point))
        {
          if(contains(interior,point))
          {
            return false;
          }
        }
      }

      return true;
    }
  }

  return false;
}

bool GeometryFactory::contains(HCLineString *lnString, HCPoint *point)
{
  int nvert = lnString->pointCount();

  int i,j, c = 0;

  for(i = 0, j = nvert -1 ; i < nvert; j = i++ )
  {
    HCPoint *vi = lnString->pointInternal(i);
    HCPoint *vj = lnString->pointInternal(j);

    if ( ((vi->y() > point->y()) != (vj->y() > point->y())) &&
         (point->x() < (vj->x() - vi->x()) * (point->y() - vi->y()) / (vj->y() - vi->y()) + vi->x()) )
      c = !c;
  }

  return c;
}
