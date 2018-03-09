/*!
 * \file geometrytest.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
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

#ifndef GEOMETRYTEST_H
#define GEOMETRYTEST_H

#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/geometryfactory.h"
#include "spatial/spatialreferencesystem.h"
#include "spatial/linestring.h"

#include <ogrsf_frmts.h>
#include <QtTest/QtTest>
#include <random>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

class GeometryTest : public QObject
{
    Q_OBJECT

  private slots:
    void init()
    {

    }

    void testSpatialReferenceSystem()
    {
      QBENCHMARK
      {
        SpatialReferenceSystem srs;

        QVERIFY(!srs.authName().compare("EPSG"));
        QVERIFY(srs.authSRID() == 4326);

        OGRSpatialReference ogrSrs;
        QString combAuth = srs.authName() + ":" + QString::number(srs.authSRID());
        ogrSrs.SetWellKnownGeogCS(combAuth.toStdString().c_str());
        QVERIFY(ogrSrs.IsGeographic() == true);

        SpatialReferenceSystem srs1("EPSG",26912);
        QString wkt = srs1.srText();
        OGRSpatialReference ogrSrs1(wkt.toStdString().c_str());
        QVERIFY(ogrSrs1.IsProjected() == true);

      }
    }

    void testPoint()
    {
      QBENCHMARK
      {
        HCPoint* point = new HCPoint(5.068,896.566);
        QCOMPARE(point->geometryType() , IGeometry::Point);

        point->setZ(50.2);
        QCOMPARE(point->geometryType() , IGeometry::PointZ);

        point->setM(0.2);
        QCOMPARE(point->geometryType() , IGeometry::PointZM);

        delete point;
      }
    }

    void testExportPointToWkt()
    {
      QBENCHMARK
      {
        HCPoint* point = new HCPoint(5.068,896.566);

        QString wktData =  point->getWKT();
        HydroCouple::Spatial::IGeometry* outGeom1 = GeometryFactory::importFromWkt(wktData);
        QVERIFY(outGeom1->geometryType() == IGeometry::Point);
        QCOMPARE(dynamic_cast<IPoint*>(outGeom1)->x() , point->x());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom1)->y() , point->y());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom1)->z() , point->z());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom1)->m() , point->m());


        point->setM(2.55);
        wktData =  point->getWKT();
        HydroCouple::Spatial::IGeometry* outGeom2 = GeometryFactory::importFromWkt(wktData);
        QVERIFY(outGeom2->geometryType() == IGeometry::PointM);
        QCOMPARE(dynamic_cast<IPoint*>(outGeom2)->x() , point->x());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom2)->y() , point->y());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom2)->z() , point->z());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom2)->m() , point->m());
        point->setGeometryFlag(HCGeometry::HasM,false);

        point->setZ(50.25);
        wktData =  point->getWKT();
        HydroCouple::Spatial::IGeometry* outGeom3 = GeometryFactory::importFromWkt(wktData);
        QVERIFY(outGeom3->geometryType() == IGeometry::PointZ);
        QCOMPARE(dynamic_cast<IPoint*>(outGeom3)->x() , point->x());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom3)->y() , point->y());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom3)->z() , point->z());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom3)->m() , point->m());

        point->setM(2.55);
        point->setZ(50.25);
        wktData =  point->getWKT();
        HydroCouple::Spatial::IGeometry* outGeom4 = GeometryFactory::importFromWkt(wktData);
        QVERIFY(outGeom4->geometryType() == IGeometry::PointZM);
        QCOMPARE(dynamic_cast<IPoint*>(outGeom4)->x() , point->x());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom4)->y() , point->y());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom4)->z() , point->z());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom4)->m() , point->m());

        delete point;
        delete outGeom1;
        delete outGeom2;
        delete outGeom3;
        delete outGeom4;
      }
    }

    void testExportPointToWkb()
    {
      QBENCHMARK
      {
        HCPoint* point = new HCPoint(5.068,896.566);

        int size = 0;
        unsigned char* wktData =  point->getWKB(size);
        HydroCouple::Spatial::IGeometry* outGeom1 = GeometryFactory::importFromWkb(wktData,size);
        QVERIFY(outGeom1->geometryType() == IGeometry::Point);
        delete[] wktData;
        QCOMPARE(dynamic_cast<IPoint*>(outGeom1)->x() , point->x());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom1)->y() , point->y());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom1)->z() , point->z());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom1)->m() , point->m());


        point->setM(2.55);
        wktData =  point->getWKB(size);
        HydroCouple::Spatial::IGeometry* outGeom2 = GeometryFactory::importFromWkb(wktData,size);
        QVERIFY(outGeom2->geometryType() == IGeometry::PointM);
        delete[] wktData;
        QCOMPARE(dynamic_cast<IPoint*>(outGeom2)->x() , point->x());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom2)->y() , point->y());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom2)->z() , point->z());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom2)->m() , point->m());
        point->setGeometryFlag(HCGeometry::HasM,false);

        point->setZ(50.25);
        wktData =  point->getWKB(size);
        HydroCouple::Spatial::IGeometry* outGeom3 = GeometryFactory::importFromWkb(wktData,size);
        QVERIFY(outGeom3->geometryType() == IGeometry::PointZ);
        delete[] wktData;
        QCOMPARE(dynamic_cast<IPoint*>(outGeom3)->x() , point->x());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom3)->y() , point->y());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom3)->z() , point->z());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom3)->m() , point->m());

        point->setM(2.55);
        point->setZ(50.25);
        wktData =  point->getWKB(size);
        HydroCouple::Spatial::IGeometry* outGeom4 = GeometryFactory::importFromWkb(wktData,size);
        QVERIFY(outGeom4->geometryType() == IGeometry::PointZM);
        QCOMPARE(dynamic_cast<IPoint*>(outGeom4)->x() , point->x());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom4)->y() , point->y());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom4)->z() , point->z());
        QCOMPARE(dynamic_cast<IPoint*>(outGeom4)->m() , point->m());

        delete[] wktData;
        delete point;
        delete outGeom1;
        delete outGeom2;
        delete outGeom3;
        delete outGeom4;
      }
    }

    void testLineString()
    {
      QBENCHMARK
      {
        static std::default_random_engine gen;
        std::uniform_real_distribution<float> uniRandNum(0,10000000.00);

        HCLineString* lineString = new HCLineString();
        lineString->enable3D();

        for(int i = 0 ; i < 1000 ; i++)
        {
          lineString->addPoint(new HCPoint(uniRandNum(gen) , uniRandNum(gen) , uniRandNum(gen),"", lineString));
        }

        QCOMPARE(lineString->geometryType() , IGeometry::LineString);

        lineString->enable3D();
        QCOMPARE(lineString->geometryType() , IGeometry::LineStringZ);

        lineString->disable3D();
        lineString->enableM();
        QCOMPARE(lineString->geometryType() , IGeometry::LineStringM);

        lineString->enable3D();
        QCOMPARE(lineString->geometryType() , IGeometry::LineStringZM);

        delete lineString;
      }
    }

    void testExportLineStringToWkt()
    {
      QBENCHMARK
      {
        static std::default_random_engine gen;
        std::uniform_real_distribution<float> uniRandNum(0,10000000.00);

        HCLineString* lineString = new HCLineString();

        for(int i = 0 ; i < 1000 ; i++)
        {
          lineString->addPoint(new HCPoint(uniRandNum(gen), uniRandNum(gen), "", lineString));
        }

        QString wktData =  lineString->getWKT();
        HydroCouple::Spatial::IGeometry* outGeom1 = GeometryFactory::importFromWkt(wktData);
        QVERIFY(outGeom1->geometryType() == IGeometry::LineString);
        ILineString* outLineString1 = dynamic_cast<ILineString*>(outGeom1);

        QCOMPARE(lineString->pointCount() , outLineString1->pointCount());

        for(int i = 0 ; i < lineString->pointCount() ; i++)
        {
          QCOMPARE(lineString->point(i)->x() ,outLineString1->point(i)->x());
          QCOMPARE(lineString->point(i)->y() ,outLineString1->point(i)->y());
        }


        lineString->enable3D();
        lineString->enableM();
        QVERIFY(lineString->geometryType() == IGeometry::LineStringZM);

        for(int i = 0 ; i < lineString->pointCount() ; i++)
        {
          lineString->points()[i]->setZ(uniRandNum(gen));
          lineString->points()[i]->setZ(uniRandNum(gen));
        }

        wktData =  lineString->getWKT();
        HydroCouple::Spatial::IGeometry* outGeom2 = GeometryFactory::importFromWkt(wktData);
        ILineString* outLineString2 = dynamic_cast<ILineString*>(outGeom2);

        for(int i = 0 ; i < lineString->pointCount() ; i++)
        {
          QCOMPARE(lineString->point(i)->x() ,outLineString2->point(i)->x());
          QCOMPARE(lineString->point(i)->y() ,outLineString2->point(i)->y());
          QCOMPARE(lineString->point(i)->z() ,outLineString2->point(i)->z());
          QCOMPARE(lineString->point(i)->m() ,outLineString2->point(i)->m());
        }

        delete lineString;
        delete outGeom1;
        delete outGeom2;
      }

    }

    void testExportLineStringToWkb()
    {
      QBENCHMARK
      {
        static std::default_random_engine gen;
        std::uniform_real_distribution<float> uniRandNum(0,10000000.00);

        HCLineString* lineString = new HCLineString();

        for(int i = 0 ; i < 1000 ; i++)
        {
          lineString->addPoint(new HCPoint(uniRandNum(gen), uniRandNum(gen), "", lineString));
        }

        int size = 0;
        unsigned char* wktData = lineString->getWKB(size);
        HydroCouple::Spatial::IGeometry* outGeom1 = GeometryFactory::importFromWkb(wktData,size);
        QVERIFY(outGeom1->geometryType() == IGeometry::LineString);
        ILineString* outLineString1 = dynamic_cast<ILineString*>(outGeom1);
        delete[]wktData;

        QCOMPARE(lineString->pointCount() , outLineString1->pointCount());

        for(int i = 0 ; i < lineString->pointCount() ; i++)
        {
          QCOMPARE(lineString->point(i)->x() ,outLineString1->point(i)->x());
          QCOMPARE(lineString->point(i)->y() ,outLineString1->point(i)->y());
        }


        lineString->enable3D();
        lineString->enableM();
        QVERIFY(lineString->geometryType() == IGeometry::LineStringZM);

        for(int i = 0 ; i < lineString->pointCount() ; i++)
        {
          lineString->points()[i]->setZ(uniRandNum(gen));
          lineString->points()[i]->setZ(uniRandNum(gen));
        }

        wktData =  lineString->getWKB(size);
        HydroCouple::Spatial::IGeometry* outGeom2 = GeometryFactory::importFromWkb(wktData,size);
        ILineString* outLineString2 = dynamic_cast<ILineString*>(outGeom2);
        delete[] wktData;

        for(int i = 0 ; i < lineString->pointCount() ; i++)
        {
          QCOMPARE(lineString->point(i)->x() ,outLineString2->point(i)->x());
          QCOMPARE(lineString->point(i)->y() ,outLineString2->point(i)->y());
          QCOMPARE(lineString->point(i)->z() ,outLineString2->point(i)->z());
          QCOMPARE(lineString->point(i)->m() ,outLineString2->point(i)->m());
        }

        delete lineString;
        delete outGeom1;
        delete outGeom2;
      }
    }


    void cleanup()
    {
    }
};

#endif // GEOMETRYTEST_H
