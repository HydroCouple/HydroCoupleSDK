#include "stdafx.h"
#include "spatial/geometryfactory.h"
#include "spatial/spatialreferencesystem.h"
#include "spatial/geometry.h"
#include "spatial/point.h"
#include "spatial/linestring.h"
#include "spatial/polygon.h"
#include "spatial/geometry.h"
#include  "spatial/geometryargument.h"
#include "spatial/polyhedralsurface.h"


#include <gdal/ogrsf_frmts.h>
#include <cstring>
#include <QDebug>
#include <assert.h>
#include <QVariant>
#include <iostream>
#include <QFile>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

OGRGeometry *GeometryFactory::exportToOGRGeometry(const IGeometry *geometry)
{
  registerGDAL();
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
  registerGDAL();

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
  registerGDAL();
  assert(lineString != nullptr);

  OGRLineString* outLineString = new OGRLineString();
  outLineString->getSpatialReference()->SetWellKnownGeogCS(lineString->spatialReferenceSystem()->srText().toStdString().c_str());

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
  registerGDAL();

  OGRPolygon* outPolygon = new OGRPolygon();
  outPolygon->getSpatialReference()->SetWellKnownGeogCS(polygon->spatialReferenceSystem()->srText().toStdString().c_str());

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

HCGeometry *GeometryFactory::importFromOGRGeometry(const OGRGeometry *geometry, QObject *parent)
{
  registerGDAL();
  assert(geometry != nullptr);

  switch (geometry->getGeometryType())
  {
    case wkbPoint:
    case wkbPointM:
    case wkbPointZM:
    case wkbPoint25D:
      {
        const OGRPoint* inPoint = static_cast<const OGRPoint*>(geometry);
        HCPoint* outPoint = importFromOGRPoint(inPoint,parent);
        return outPoint;
      }
      break;
    case wkbLineString:
    case wkbLineStringM:
    case wkbLineStringZM:
    case wkbLineString25D:
      {
        const OGRLineString* inLineString = static_cast<const OGRLineString*>(geometry);
        HCLineString* outLineString = importFromOGRLineString(inLineString,parent);
        return outLineString;
      }
      break;
    case wkbPolygon:
    case wkbPolygonM:
    case wkbPolygonZM:
    case wkbPolygon25D:
      {
        const OGRPolygon* inPolygon = static_cast<const OGRPolygon*>(geometry);
        HCPolygon* outPolygon = importFromOGRPolygon(inPolygon, parent);
        return outPolygon;
      }
      break;
    default:
      break;
  }

  return nullptr;
}

HCPoint *GeometryFactory::importFromOGRPoint(const OGRPoint *point, QObject *parent)
{

  if(point->IsEmpty())
  {
    HCPoint* oPoint = new HCPoint(parent);
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
    HCPoint* oPoint = new HCPoint(point->getX() , point->getY(),parent);

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

HCLineString *GeometryFactory::importFromOGRLineString(const OGRLineString *lineString, QObject *parent)
{
  HCLineString* outLineString = new HCLineString(parent);

  SpatialReferenceSystem* srs = dynamic_cast<SpatialReferenceSystem*>(outLineString->spatialReferenceSystem());

  if(srs && lineString->getSpatialReference())
  {
    char* wkt = nullptr;
    lineString->getSpatialReference()->exportToWkt(&wkt);
    srs->setSrText(QString(wkt));
    if(wkt)
      delete[] wkt;
  }


  for(int i = 0 ; i < lineString->getNumPoints() ; i++)
  {
    double x = lineString->getX(i);
    double y = lineString->getY(i);

    HCPoint* point = new HCPoint(x,y, outLineString);
    outLineString->addPoint(point);
  }

  if(lineString->getGeometryType() == wkbLineString25D)
  {
    outLineString->enable3D();

    for(int i = 0 ; i < lineString->getNumPoints() ; i++)
    {
      outLineString->points()[i]->setZ(lineString->getZ(i));
    }
  }
  else if(lineString->getGeometryType() == wkbLineStringM)
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

HCPolygon *GeometryFactory::importFromOGRPolygon(const OGRPolygon *polygon, QObject *parent)
{
  HCPolygon* outPolygon = new HCPolygon(parent);
  SpatialReferenceSystem* srs = dynamic_cast<SpatialReferenceSystem*>(outPolygon->spatialReferenceSystem());

  if(srs && polygon->getSpatialReference())
  {
    char* wkt = nullptr;
    polygon->getSpatialReference()->exportToWkt(&wkt);
    srs->setSrText(QString(wkt));
    if(wkt)
      delete[] wkt;
  }


  //core
  {
    for(int i = 0 ; i < polygon->getExteriorRing()->getNumPoints() ; i++)
    {
      double x = polygon->getExteriorRing()->getX(i);
      double y = polygon->getExteriorRing()->getY(i);
      HCPoint* point = new HCPoint(x,y, outPolygon->exteriorRingInternal());
      outPolygon->exteriorRingInternal()->addPoint(point);
    }

    for(int i = 0 ; i < polygon->getNumInteriorRings() ; i++)
    {
      const OGRLinearRing *inputIntLinearRing = polygon->getInteriorRing(i);
      HCLineString *outIntLinearRing = new HCLineString(outPolygon);

      for(int j = 0; j <inputIntLinearRing->getNumPoints() ; j++)
      {
        double x = inputIntLinearRing->getX(j);
        double y = inputIntLinearRing->getY(j);
        outIntLinearRing->addPoint(new HCPoint(x,y,outIntLinearRing));
      }

      outPolygon->addInteriorRing(outIntLinearRing);

    }
  }

  if(polygon->getGeometryType() == wkbPolygon25D)
  {
    outPolygon->enable3D();

    for(int i = 0 ; i < polygon->getExteriorRing()->getNumPoints() ; i++)
    {
      double z = polygon->getExteriorRing()->getZ(i);
      outPolygon->exteriorRingInternal()->points()[i]->setZ(z);
    }

    for(int i = 0 ; i < polygon->getNumInteriorRings() ; i++)
    {
      const OGRLinearRing *inputIntLinearRing = polygon->getInteriorRing(i);
      HCLineString *outIntLinearRing = new HCLineString(outPolygon);

      for(int j = 0; j <inputIntLinearRing->getNumPoints() ; j++)
      {
        double z = inputIntLinearRing->getZ(j);
        outIntLinearRing->points()[j]->setZ(z);
      }

      outPolygon->addInteriorRing(outIntLinearRing);

    }
  }
  else if(polygon->getGeometryType() == wkbLineStringM)
  {
    outPolygon->enableM();

    for(int i = 0 ; i < polygon->getExteriorRing()->getNumPoints() ; i++)
    {
      double m = polygon->getExteriorRing()->getM(i);
      outPolygon->exteriorRingInternal()->points()[i]->setM(m);
    }

    for(int i = 0 ; i < polygon->getNumInteriorRings() ; i++)
    {
      const OGRLinearRing *inputIntLinearRing = polygon->getInteriorRing(i);
      HCLineString *outIntLinearRing = new HCLineString(outPolygon);

      for(int j = 0; j <inputIntLinearRing->getNumPoints() ; j++)
      {
        double m = inputIntLinearRing->getM(j);
        outIntLinearRing->points()[j]->setM(m);
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
      double m = polygon->getExteriorRing()->getM(i);
      double z = polygon->getExteriorRing()->getZ(i);

      outPolygon->exteriorRingInternal()->points()[i]->setM(m);
      outPolygon->exteriorRingInternal()->points()[i]->setZ(z);
    }

    for(int i = 0 ; i < polygon->getNumInteriorRings() ; i++)
    {
      const OGRLinearRing *inputIntLinearRing = polygon->getInteriorRing(i);
      HCLineString *outIntLinearRing = new HCLineString(outPolygon);

      for(int j = 0; j <inputIntLinearRing->getNumPoints() ; j++)
      {
        double m = inputIntLinearRing->getZ(j);
        double z = inputIntLinearRing->getZ(j);

        outIntLinearRing->points()[j]->setM(m);
        outIntLinearRing->points()[j]->setZ(z);
      }

      outPolygon->addInteriorRing(outIntLinearRing);

    }
  }

  return outPolygon;
}

HCGeometry *GeometryFactory::importFromWkt(const QString &wktData)
{
  registerGDAL();
  OGRGeometry* geometry = nullptr;
  char* wktDataCpy =  new char[wktData.length() + 1 ];
  strcpy(wktDataCpy , wktData.toStdString().c_str());
  char* wktDataCpyCpy = wktDataCpy;

  OGRSpatialReference* srs  = new OGRSpatialReference();

  OGRErr error;

  if( (error = OGRGeometryFactory::createFromWkt(&wktDataCpy,srs,&geometry)) == OGRERR_NONE)
  {
    HCGeometry* oGeom = importFromOGRGeometry(geometry);

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

HCGeometry *GeometryFactory::importFromWkb(unsigned char *wkbData, int nBytes)
{
  registerGDAL();
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

bool GeometryFactory::writeGeometryDataItemToFile(const HCGeometryArgumentDouble *geometryData, const QString &dataFieldName,
                                                  const QString &driverName, const QString &outputFile, QString &errorMessage)
{
  registerGDAL();

  //need to make sure no overwrite.
  if(geometryData->geometryCount() && !QFile::exists(outputFile))
  {
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(driverName.toStdString().c_str());

    if(driver != NULL)
    {
      GDALDataset *data =driver->Create(outputFile.toStdString().c_str(),0,0,0,GDT_Unknown,NULL);

      if(data != NULL)
      {
        OGRSpatialReference srs(geometryData->geometry(0)->spatialReferenceSystem()->srText().toStdString().c_str());
        OGRLayer *layer = data->CreateLayer(geometryData->id().toStdString().c_str(),&srs,HCGeometry::toOGRDataType(geometryData->geometryType()));

        OGRFieldDefn  idfield("Id",OFTString);
        layer->CreateField(&idfield);
        OGRFieldDefn  indexField("Index",OFTInteger64);
        layer->CreateField(&indexField);
        OGRFieldDefn  datafield(dataFieldName.toStdString().c_str() , OFTReal);
        layer->CreateField(&datafield);

        for(int i = 0 ; i < geometryData->geometryCount() ; i++)
        {
          HCGeometry *geometry = geometryData->geometries()[i];
          double value ;
          geometryData->getValues(i,1,&value);

          OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
          feature->SetField("Id", geometry->id().toStdString().c_str());
          feature->SetField("Index",(int)geometry->index());
          feature->SetField(dataFieldName.toStdString().c_str(), value);

          OGRGeometry *geom = exportToOGRGeometry(geometry);
          feature->SetGeometry(geom);

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

bool GeometryFactory::readGeometryDataItemFromFile(const QString &filePath, QString &dataFieldName,
                                                   HCGeometryArgumentDouble *geometryArgument, QString &errorMessage)
{
  registerGDAL();

  GDALDataset *data = (GDALDataset*) GDALOpenEx(filePath.toStdString().c_str(), GDAL_OF_VECTOR,NULL, NULL, NULL);

  if(data != NULL)
  {
    for(int i = 0 ; i < data->GetLayerCount() ; i++)
    {
      OGRLayer *dataLayer = data->GetLayer(i);


      if(HCGeometry::toOGRDataType(geometryArgument->geometryType()) == dataLayer->GetGeomType())
      {
        dataLayer->ResetReading();

        OGRFeature *feature = dataLayer->GetNextFeature();

        if(feature != NULL)
        {
          QVector<double> doubleData;
          QList<HCGeometry*> geometries;

          OGRFeatureDefn *dataFDef = dataLayer->GetLayerDefn();

          //find first double
          int fieldD = -1;
          int fieldI = -1;

          for(int j = 0 ; j < dataFDef->GetFieldCount() ; j++)
          {
            OGRFieldDefn *fieldDef = dataFDef->GetFieldDefn(i);

            std::cout <<  fieldDef->GetNameRef() << std::endl;
            std::cout <<  OGRFieldDefn::GetFieldTypeName(fieldDef->GetType()) << std::endl;

            if(fieldDef->GetType() == OFTReal)
            {
              fieldD = j;
              dataFieldName = QString(fieldDef->GetNameRef());
              break;
            }
            else if(fieldDef->GetType() == OFTInteger ||
                    fieldDef->GetType() == OFTInteger64 )
            {
              fieldI = j;
            }
          }

          if(fieldD < 0 && fieldI >= 0)
          {
            dataFieldName = QString(dataFDef->GetFieldDefn(fieldI)->GetNameRef());
            fieldD = fieldI;
          }

          do
          {
            if(fieldD > -1)
            {
              doubleData.append(feature->GetFieldAsDouble(fieldD));
            }

            OGRGeometry *ogrGeom = feature->GetGeometryRef();

            HCGeometry* geom = importFromOGRGeometry(ogrGeom,geometryArgument);

            geometries.append(geom);

            OGRFeature::DestroyFeature(feature);

          }while (((feature = dataLayer->GetNextFeature()) != NULL));

          if(geometries.length())
          {
            geometryArgument->addGeometries(geometries);

            if(doubleData.length() && doubleData.length() == geometries.length())
              geometryArgument->setValues(0,doubleData.length(),doubleData.data());
          }
        }

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

bool GeometryFactory::writeTINToFile( HCTIN *tin, const QString &filePath, QString &errorMessage)
{

  return false;
}

bool GeometryFactory::writeTINVertices( HCTIN *tin, const QString &filePath, const QString &gdalDriverName)
{
  registerGDAL();

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
        OGRLayer *layer = data->CreateLayer(tin->id().toStdString().c_str(),&srs,HCGeometry::toOGRDataType(tin->vertex(0)->geometryType()));

        OGRFieldDefn  idfield("Id",OFTString);
        layer->CreateField(&idfield);
        OGRFieldDefn  indexField("Index",OFTInteger64);
        layer->CreateField(&indexField);

        for(int i = 0 ; i < tin->vertexCount() ; i++)
        {
          HCGeometry *geometry = tin->vertices()[i];
          OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
          feature->SetField("Id", geometry->id().toStdString().c_str());
          feature->SetField("Index", (int)geometry->index());

          OGRGeometry *geom = exportToOGRGeometry(geometry);
          feature->SetGeometry(geom);

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
  registerGDAL();

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
        OGRLayer *layer = data->CreateLayer(tin->id().toStdString().c_str(),&srs,HCGeometry::toOGRDataType(tin->patch(0)->geometryType()));

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


        for(int i = 0 ; i < tin->patchCount() ; i++)
        {
          HCTriangle *geometry = dynamic_cast<HCTriangle*>(tin->patches()[i]);

          OGRFeature *feature = OGRFeature::CreateFeature(layer->GetLayerDefn());
          feature->SetField("Id", geometry->id().toStdString().c_str());
          feature->SetField("Index", (int)geometry->index());
          feature->SetField("P1",(int)geometry->p1()->index());
          feature->SetField("P2",(int)geometry->p2()->index());
          feature->SetField("P3",(int)geometry->p3()->index());

          OGRGeometry *geom = exportToOGRGeometry(geometry);
          feature->SetGeometry(geom);

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


bool GeometryFactory::readTINFromFile(const QString &filePath, HCTIN *tin, QString &errorMessage)
{
  return false;
}

void GeometryFactory::registerGDAL()
{
  static bool registered(false) ;

  if(!registered)
  {
    GDALAllRegister();
  }
  else
  {
    std::cout << "gdal registered" <<std::endl;
  }
}
