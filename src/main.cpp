/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
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

#include "stdafx.h"
#include "spatial/point.h"
#include "spatial/linestring.h"
#include "spatial/octree.h"
#include "spatial/geometryfactory.h"
#include "spatial/envelope.h"
#include "spatial/geometrycollection.h"
#include "tests/geometrytest.h"
#include "tests/polyhedralsurfacetest.h"

#include <QtTest>
#include <QCoreApplication>
#include <QFileInfo>
#include <QTextStream>
#include <ogrsf_frmts.h>
#include <QDir>
#include <QDirIterator>
#include <QString>
#include <QSet>
#include <QVector>
#include <QtConcurrent>

using namespace HydroCouple::Spatial;

void createSWMMProjectFile(const QString &projectFile , const QString& nodeShapefile,  const QString& linkShapefile)
{

  GDALAllRegister();

  QFile outFile(projectFile);

  if(outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
  {
    QTextStream writer(&outFile);

    writer << "[TITLE]" << endl;
    writer << "Detailed SWMM Conveyance Model For Logan" << endl;

    writer << "[OPTIONS]" << endl;
    writer << "FLOW_UNITS           CMS" << endl;
    writer << "INFILTRATION         CURVE_NUMBER" << endl;
    writer << "FLOW_ROUTING         DYNWAVE" << endl;
    writer << "START_DATE           01/01/2002" << endl;
    writer << "START_TIME           00:00:00" << endl;
    writer << "REPORT_START_DATE    01/01/2002" << endl;
    writer << "REPORT_START_TIME    00:00:00" << endl;
    writer << "END_DATE             01/02/2002" << endl;
    writer << "END_TIME             23:00:00" << endl;
    writer << "SWEEP_START          01/01" << endl;
    writer << "SWEEP_END            12/31" << endl;
    writer << "DRY_DAYS             5.000000" << endl;
    writer << "REPORT_STEP          00:05:00" << endl;
    writer << "WET_STEP             00:10:00" << endl;
    writer << "DRY_STEP             23:00:00" << endl;
    writer << "ROUTING_STEP         0:00:05" << endl;
    writer << "ALLOW_PONDING        YES" << endl;
    writer << "INERTIAL_DAMPING     PARTIAL" << endl;
    writer << "VARIABLE_STEP        0.00" << endl;
    writer << "LENGTHENING_STEP     5" << endl;
    writer << "MIN_SURFAREA         0" << endl;
    writer << "NORMAL_FLOW_LIMITED  BOTH" << endl;
    writer << "SKIP_STEADY_STATE    NO" << endl;
    writer << "FORCE_MAIN_EQUATION  H-W" << endl;
    writer << "LINK_OFFSETS         DEPTH" << endl;
    writer << "MIN_SLOPE            0" << endl;
    writer << "" << endl;


    QList<OGRFeature*> nodeFeatures;
    QList<HCPoint*> nodes;

    QHash<int,QList<int>> nodeLinks;

    QList<OGRFeature*> linkFeatures;
    QList<HCLineString*> links;
    QHash<int,int> linkBegin;
    QHash<int,int> linkEnd;

    Envelope envelope;

    GDALDataset *nodesData = (GDALDataset*)GDALOpenEx(nodeShapefile.trimmed().toStdString().c_str(), GDAL_OF_VECTOR,NULL, NULL, NULL);

    if(nodesData)
    {
      int pointCount = 0;

      for(int i = 0; i < nodesData->GetLayerCount() ; i++)
      {
        OGRLayer *dataLayer = nodesData->GetLayer(i);

        OGRFeature *feature;

        while ((feature = dataLayer->GetNextFeature()) != NULL)
        {

          OGRGeometry *ogrGeom = feature->GetGeometryRef();
          HCGeometry* geom = GeometryFactory::importFromOGRGeometry(ogrGeom);

          int fid =  feature->GetFID();
          geom->setId("FID"  + QString::number(fid));

          envelope.merge(geom->envelopeInternal());

          geom->setMarker(pointCount);
          nodeFeatures.append(feature);
          nodes.append(dynamic_cast<HCPoint*>(geom));
          pointCount++;
        }
      }
    }

    GDALClose(nodesData);

    Octree *octree = new Octree( Octree::Octree2D, Octree::Middle , 8, 50);

    for(HCPoint* point : nodes)
    {
      octree->addGeometry(point);
    }

    GDALDataset *linksData = (GDALDataset*)GDALOpenEx(linkShapefile.trimmed().toStdString().c_str(), GDAL_OF_VECTOR,NULL, NULL, NULL);

    if(linksData)
    {
      int lineCount = 0;

      for(int i = 0; i < linksData->GetLayerCount() ; i++)
      {
        OGRLayer *dataLayer = linksData->GetLayer(i);

        OGRFeature *feature;

        while ((feature = dataLayer->GetNextFeature()) != NULL)
        {
          OGRGeometry *ogrGeom = feature->GetGeometryRef();
          HCGeometry* geom = GeometryFactory::importFromOGRGeometry(ogrGeom);
          int fid =  feature->GetFID();

          geom->setMarker(lineCount);
          geom->setId("FID"  + QString::number(fid));

          envelope.merge(geom->envelopeInternal());
          linkFeatures.append(feature);
          links.append(dynamic_cast<HCLineString*>(geom));
          lineCount++;
        }
      }
    }

    GDALClose(linksData);

    for(int i = 0; i < links.length() ; i++)
    {
      OGRFeature* feature = linkFeatures[i];

      QString linkType(feature->GetFieldAsString("Conveyance"));

      if(!linkType.compare("Pipe",Qt::CaseInsensitive))
      {

        HCLineString *lineString = links[i];
        HCPoint *begin = lineString->pointInternal(0);
        HCPoint *end = lineString->pointInternal(lineString->pointCount() - 1);

        std::vector<IGeometry*> geometries =  octree->findCollidingGeometries(begin);

        bool found = false;

        if(geometries.size())
        {

          for(IGeometry* geometry : geometries)
          {
            HCPoint* p1 = dynamic_cast<HCPoint*>(geometry);

            if(p1->equals(begin,0.1))
            {
              linkBegin[lineString->marker()] = p1->marker();
              nodeLinks[p1->marker()].append(lineString->marker());
              found = true;
              break;
            }
          }
        }

        if(!found)
        {
          nodes.append(begin);
          begin->setId("AFID" + QString::number(nodes.length()));
          begin->setMarker(nodes.length() - 1);
          linkBegin[lineString->marker()] = begin->marker();
          nodeLinks[begin->marker()].append(lineString->marker());
        }

        geometries =  octree->findCollidingGeometries(end);

        found = false;

        if(geometries.size())
        {
          for(IGeometry* geometry : geometries)
          {
            HCPoint* p1 = dynamic_cast<HCPoint*>(geometry);

            if(p1->equals(end,0.1))
            {
              linkEnd[lineString->marker()] = p1->marker();
              nodeLinks[p1->marker()].append(lineString->marker());
              found = true;
              break;
            }
          }
        }

        if(!found)
        {
          nodes.append(end);
          end->setId("AFID"+QString::number(nodes.length()));
          end->setMarker(nodes.length() - 1);
          linkEnd[lineString->marker()] = end->marker();
          nodeLinks[end->marker()].append(lineString->marker());
        }
      }
    }

    QList<HCPoint*> junctions;
    QList<HCPoint*> outfalls;

    for(HCPoint *p : nodes)
    {
      QList<int> links = nodeLinks[p->marker()];

      if(links.length())
      {
        if(p->marker() < nodeFeatures.length())
        {
          OGRFeature *feature = nodeFeatures[p->marker()];
          QString type(feature->GetFieldAsString("Structure"));

          if((!type.trimmed().compare("Outlet",Qt::CaseInsensitive) ||
              !type.trimmed().compare("Outfall",Qt::CaseInsensitive)) && nodeLinks[p->marker()].length() == 1 )
          {
            outfalls.append(p);
          }
          else
          {
            junctions.append(p);
          }
        }
        else
        {
          junctions.append(p);
        }
      }
    }

    writer << "[JUNCTIONS]" << endl;
    writer << ";;               Invert     Max.       Init.      Surcharge  Ponded" << endl;
    writer << ";;Name           Elev.      Depth      Depth      Depth      Area" << endl;
    writer << ";;-------------- ---------- ---------- ---------- ---------- ----------" << endl;

    for(HCPoint *p :  junctions)
    {

      //      QString invE = QString::number(p->z()) + "-D";
      QString invE = QString::number(p->z() - 0.9144);
      //QString maxD = QString("Dmax");
      QString maxD = QString("0.9144");
      QString initDepth = QString("0").leftJustified(11);

      if(p->marker() < nodeFeatures.length())
      {
        OGRFeature *feature = nodeFeatures[p->marker()];
        double depth = feature->GetFieldAsDouble("BoxDepth");

        if(depth > 0)
        {
          depth *= 0.3048;
          invE = QString::number((p->z() - depth)).leftJustified(12);
          maxD = QString::number(depth);
        }
      }

      writer << p->id().leftJustified(17) << invE.leftJustified(12) << maxD.leftJustified(11) << initDepth << initDepth << initDepth << endl;
    }



    writer << "" << endl;
    writer << "[OUTFALLS]" << endl;
    writer << ";;               Invert     Outfall    Stage/Table      Tide" << endl;
    writer << ";;Name           Elev.      Type       Time Series      Gate" << endl;
    writer << ";;-------------- ---------- ---------- ---------------- ----" << endl;

    for(HCPoint *p :  outfalls)
    {

      QString invE = QString::number(p->z());
      //      QString invE = QString::number(p->z()) + "-D";

      if(p->marker() < nodeFeatures.length())
      {
        OGRFeature *feature = nodeFeatures[p->marker()];
        double depth = feature->GetFieldAsDouble("BoxDepth");

        if(depth > 0)
        {
          depth = depth * 0.3048;
          invE = QString::number((p->z() - depth)).leftJustified(12);
        }
      }

      writer << p->id().leftJustified(17) << invE.leftJustified(12) << QString("FREE").leftJustified(11) << QString("").leftJustified(11) << QString("NO").leftJustified(11) << endl;
    }

    writer << "" << endl;
    writer << "[CONDUITS]" << endl;
    writer << ";;               Inlet            Outlet                      Manning    Inlet      Outlet     Init.      Max." << endl;
    writer << ";;Name           Node             Node             Length     N          Offset     Offset     Flow       Flow" << endl;
    writer << ";;-------------- ---------------- ---------------- ---------- ---------- ---------- ---------- ---------- ----------" << endl;

    QString mannings("0.03");
    QString flow("0.0");
    //    QString offset("UnknownOffset");
    QString offset("0.1524");

    for(HCLineString *link : links )
    {
      OGRFeature *feature = linkFeatures[link->marker()];
      QString linkType(feature->GetFieldAsString("Conveyance"));

      if(!linkType.compare("Pipe",Qt::CaseInsensitive))
      {

        int begin = linkBegin[link->marker()];
        int end = linkEnd[link->marker()];

        writer << link->id().leftJustified(17) << nodes[begin]->id().leftJustified(17) << nodes[end]->id().leftJustified(17) << QString::number(link->length()).leftJustified(11) << mannings.leftJustified(11)
               << offset.leftJustified(11) << offset.leftJustified(11) << flow.leftJustified(11) << flow.leftJustified(11) << endl;
      }
    }


    writer << "" << endl;
    writer << "[XSECTIONS]" << endl;
    writer << ";;Link           Shape        Geom1            Geom2      Geom3      Geom4      Barrels" << endl;
    writer << ";;-------------- ------------ ---------------- ---------- ---------- ---------- ----------" << endl;

    QString zero("0");

    for(HCLineString *link : links )
    {

      OGRFeature *feature = linkFeatures[link->marker()];
      QString linkType(feature->GetFieldAsString("Conveyance"));

      if(!linkType.compare("Pipe",Qt::CaseInsensitive))
      {


        QString shape("CIRCULAR");
        //      QString shape("UnknownShape");
        //      QString diameter("UnknownDiameter");
        QString diameter("0.3048");
        OGRFeature *feature = linkFeatures[link->marker()];
        QString geom2("0.3048");

        QString tempDiameter(feature->GetFieldAsString("Diameter"));
        tempDiameter = tempDiameter.replace("\"","");

        bool ok;

        double value = tempDiameter.toDouble(&ok) ;


        if(ok)
        {
          value *= 0.0254;
          tempDiameter = QString::number(value);
          diameter = tempDiameter ;
        }

        QString tempShape(feature->GetFieldAsString("PipeShape"));

        if(!tempShape.compare("circlar",Qt::CaseInsensitive)||
           !tempShape.compare("circular",Qt::CaseInsensitive) )
        {
          shape = "CIRCULAR";
        }
        else if (!tempShape.compare("Arch",Qt::CaseInsensitive))
        {
          shape = "ARCH";

          if(ok)
          {
            geom2 = tempDiameter;
          }

        }

        writer << link->id().leftJustified(17) << shape.leftJustified(13) << diameter.leftJustified(17) << geom2.leftJustified(11) << zero.leftJustified(11) << zero.leftJustified(11) << QString("1").leftJustified(11) << zero.leftJustified(11) << endl;
      }
    }

    writer << "" << endl;
    writer << "[MAP]" << endl;
    //write bounds;
    writer << "DIMENSIONS " << envelope.minX() << " " << envelope.minY() << " " << envelope.maxX() << " " << envelope.maxY() << endl;// 1526037.495 3764743.440 1570678.941 3816874.269" << endl;
    writer << "Units      None" << endl;

    writer << "" << endl;
    writer << "[COORDINATES]" << endl;
    writer << ";;Node           X-Coord            Y-Coord" << endl;
    writer << ";;-------------- ------------------ ------------------" << endl;
    //JCT-100          1537376.776        3786655.551
    for(HCPoint *p :  nodes)
    {
      QString xN = QString::number(p->x(),'g',16);
      QString yN = QString::number(p->y(),'g',16);

      xN = xN.leftJustified(18) + " ";
      yN = yN.leftJustified(19);

      writer << p->id().leftJustified(17) << xN << yN << endl;
    }

    for(HCPoint *p :  outfalls)
    {
      QString xN = QString::number(p->x(),'g',16);
      QString yN = QString::number(p->y(),'g',16);

      xN = xN.leftJustified(18) + " ";
      yN = yN.leftJustified(19);

      writer << p->id().leftJustified(17) << xN << yN << endl;
    }

    writer << "" << endl;
    writer << "[VERTICES]" << endl;
    writer << ";;Link           X-Coord            Y-Coord" << endl;
    writer << ";;-------------- ------------------ ------------------" << endl;
    //CDT-1035         1544371.734        3795339.567
    //CDT-1035         1543635.915        3795347.091
    //CDT-1085         1543980.226        3807270.175
    for(HCLineString *lineString : links)
    {
      OGRFeature *feature = linkFeatures[lineString->marker()];
      QString linkType(feature->GetFieldAsString("Conveyance"));

      if(!linkType.compare("Pipe",Qt::CaseInsensitive))
      {
        for(int i = 0 ; i < lineString->pointCount() ; i++)
        {
          HCPoint *p = lineString->pointInternal(i);

          QString xN = QString::number(p->x(),'g',16);
          QString yN = QString::number(p->y(),'g',16);

          xN = xN.leftJustified(18) + " ";
          yN = yN.leftJustified(19);

          writer << lineString->id().leftJustified(17) << xN << yN << endl;
        }
      }
    }

    for(OGRFeature *feature : nodeFeatures)
    {
      OGRFeature::DestroyFeature(feature);
    }

    nodeFeatures.clear();


    for(OGRFeature *feature : linkFeatures)
    {
      OGRFeature::DestroyFeature(feature);
    }

    linkFeatures.clear();

    for(HCLineString *line : links)
    {
      nodes.removeAll(line->pointInternal(0));
      nodes.removeAll(line->pointInternal(line->pointCount()-1));
    }


    qDeleteAll(links);
    qDeleteAll(nodes);

  }
}

void selectPointsFromPolygon()
{
  QString pointsFile = "/Users/calebbuahin/Documents/Projects/Models/1d-2d_coupling/GIS/Working_Elev/NED_Lid_Comb.shp";
  QString polygonFile = "/Users/calebbuahin/Documents/Projects/Models/1d-2d_coupling/GIS/Boundaries/Boundaries_Limited/limited_boundary_extended_clip.shp";
  QString outputFolder = "/Users/calebbuahin/Documents/Projects/Models/1d-2d_coupling/GIS/Working_Elev/Split/";
  QList<OGRPolygon*> polygons;

  GDALDataset *polygonDataSet = (GDALDataset*) GDALOpenEx(polygonFile.toStdString().c_str(), GDAL_OF_VECTOR,NULL, NULL, NULL);
  OGRLayer *polLayer = polygonDataSet->GetLayer(0); polLayer->ResetReading();

  OGRFeature *feature = NULL;

  while ((feature = polLayer->GetNextFeature()) != NULL)
  {
    polygons.append(dynamic_cast<OGRPolygon*>(feature->StealGeometry()));
    OGRFeature::DestroyFeature(feature);
  }

  GDALClose(polygonDataSet);
  QVector<QTextStream*> writers;
  QVector<QFile*> outputfiles;

  for(int i = 0 ; i < polygons.length() ; i++)
  {
    QFile *file = new QFile(outputFolder + "NED_Lid_Comb" + QString::number(i) + ".csv");
    file->open(QIODevice::WriteOnly | QIODevice::Truncate);
    QTextStream *textStreamWriter = new QTextStream(file);
    (*textStreamWriter) << "x,y,z" << endl;
    writers.append(textStreamWriter);
    outputfiles.append(file);
  }


  GDALDataset *pointsDataSet = (GDALDataset*) GDALOpenEx(pointsFile.trimmed().toStdString().c_str(), GDAL_OF_VECTOR,NULL, NULL, NULL);

  OGRLayer *pointsLayer = pointsDataSet->GetLayer(0);
  pointsLayer->ResetReading();

  int pointsnotfound = 0;

  while ((feature = pointsLayer->GetNextFeature()) != NULL)
  {
    OGRPoint *point = dynamic_cast<OGRPoint*>(feature->GetGeometryRef());
    bool found  = false;

    for(int i = 0; i < polygons.length() ; i++ )
    {
      if(polygons[i]->Contains(point))
      {
        (*writers[i]) << QString::number(point->getX(),'f',10) << "," << QString::number(point->getY(),'f',10) << "," << QString::number(point->getZ(),'f',10) << endl;
        found = true;
        break;
      }
    }

    if(!found)
    {
      pointsnotfound++;
    }

    OGRFeature::DestroyFeature(feature);
  }

  printf("Num points not found: %i \n", pointsnotfound );

  for(int i = 0; i < polygons.length() ; i++)
  {
    writers[0]->device()->close();
  }

  qDeleteAll(writers);
  writers.clear();

  GDALClose(pointsDataSet);
}

void selectPointsWithinPolygon(const QString &pointsFolder, const QString &polygonFile, const QString &outputFile)
{
  QList<HCGeometry*> polygons;
  Envelope polygonExtent;
  QString errorMessage;
  GeometryFactory::readGeometryFromFile(polygonFile, polygons, polygonExtent, errorMessage);

  QFile file(outputFile);
  file.open(QIODevice::WriteOnly | QIODevice::Truncate);
  QTextStream writer(&file);
  writer << "x,y,z" << endl;
  long pfound = 0;

  QDirIterator it(pointsFolder,QStringList() << "*.shp", QDir::Files , QDirIterator::NoIteratorFlags);

  while (it.hasNext())
  {
    QString pointsFile = it.next();

    QList<HCGeometry*> points;
    Envelope pointsExtent;
    GeometryFactory::readGeometryFromFile(pointsFile, points,pointsExtent,errorMessage);

    printf("Processing: %s....\n", qPrintable(pointsFile));

    int pointsFound = 0;
    int numPoints = points.size();

    for(HCGeometry *pGeom : polygons)
    {
      if(pGeom->envelopeInternal()->intersects(&pointsExtent))
      {
        QFutureSynchronizer<bool> synchronizer;
        HCPolygon *polygon = dynamic_cast<HCPolygon*>(pGeom);
        std::vector<HCPoint*> tempPoints(points.size());

        int pt = 0;

        for(HCGeometry *pntGeom : points)
        {
          HCPoint *p = dynamic_cast<HCPoint*>(pntGeom);
          QFuture<bool> future = QtConcurrent::run(polygon, &HCPolygon::contains, p);
          synchronizer.addFuture(future);
          tempPoints[pt] = p;
          pt++;
        }

        synchronizer.waitForFinished();

        QList<QFuture<bool>> futures = synchronizer.futures();

        for(int i = 0; i < futures.size() ; i++)
        {
          QFuture<bool> future = futures[i];

          if(future.result())
          {
            HCPoint *p = tempPoints[i];
            writer << QString::number(p->x(),'f',10) << "," << QString::number(p->y(),'f',10) << "," << QString::number(p->z(),'f',10) << endl;
            pointsFound++;
            pfound++;
          }
        }

        tempPoints.clear();
      }
    }

    qDeleteAll(points);
    points.clear();

    printf("File : %s, Points found: %i/%i/%i \n",qPrintable(pointsFile), pointsFound, numPoints, pfound);
  }

  writer.device()->close();
  qDeleteAll(polygons);

}

void seletPointsWithinPolygon(int argc, char* argv[])
{
  GDALAllRegister();

  QFileInfo pointsFolder(argv[1]);
  QFileInfo bufferFile(argv[2]);
  QFileInfo outputFile(argv[3]);

  QDir dir(QDir::currentPath());

  if(pointsFolder.isRelative())
    pointsFolder = QFileInfo(dir.absoluteFilePath(pointsFolder.filePath()));

  if(bufferFile.isRelative())
    bufferFile = QFileInfo(dir.absoluteFilePath(bufferFile.filePath()));

  if(outputFile.isRelative())
    outputFile = QFileInfo(dir.absoluteFilePath(outputFile.filePath()));

  printf("Current Dir: %s\n", qPrintable(QDir::currentPath()));

  printf("Running....\n");
  printf("Input points folder: %s\n" , qPrintable(pointsFolder.absoluteFilePath()));
  printf("Buffer Polygon: %s\n" , qPrintable(bufferFile.absoluteFilePath()));
  printf("Output File: %s\n" , qPrintable(outputFile.absoluteFilePath()));

  selectPointsWithinPolygon(pointsFolder.absoluteFilePath() , bufferFile.absoluteFilePath(), outputFile.absoluteFilePath());
}

void splitPointsByPolygon(int argc, char* argv[])
{
  GDALAllRegister();

  QFileInfo polygonFile(argv[1]);
  QFileInfo pointsFile(argv[2]);
  QFileInfo outputFile(argv[3]);

  QDir dir(QDir::currentPath());

  if(polygonFile.isRelative())
    polygonFile = QFileInfo(dir.absoluteFilePath(polygonFile.filePath()));

  if(pointsFile.isRelative())
    pointsFile = QFileInfo(dir.absoluteFilePath(pointsFile.filePath()));

  if(outputFile.isRelative())
    outputFile = QFileInfo(dir.absoluteFilePath(outputFile.filePath()));

  printf("Current Dir: %s\n", qPrintable(QDir::currentPath()));



  QVector<HCPolygon*> polygons;
  QVector<QTextStream*> writers;

  GDALDataset *polygonDataSet = (GDALDataset*) GDALOpenEx(polygonFile.absoluteFilePath().toStdString().c_str(), GDAL_OF_VECTOR,NULL, NULL, NULL);

  OGRLayer *polLayer = polygonDataSet->GetLayer(0); polLayer->ResetReading();

  OGRFeature *feature = NULL;

  while ((feature = polLayer->GetNextFeature()) != NULL)
  {

    QString file = outputFile.absoluteFilePath().replace("." + outputFile.suffix() , QString::number(polygons.size()) + "." + outputFile.suffix());
    QFile *inFile = new QFile(file);
    inFile->open(QIODevice::WriteOnly | QIODevice::Truncate);
    QTextStream *writer = new QTextStream(inFile);
    *writer << "x,y,z" << endl;
    writers.push_back(writer);

    HCGeometry *geom  = GeometryFactory::importFromOGRGeometry(feature->GetGeometryRef());
    polygons.append(dynamic_cast<HCPolygon*>(geom));
    OGRFeature::DestroyFeature(feature);
  }


  GDALDataset *pointsDataSet = (GDALDataset*) GDALOpenEx(pointsFile.absoluteFilePath().trimmed().toStdString().c_str(), GDAL_OF_VECTOR,NULL, NULL, NULL);

  OGRLayer *pointsLayer = pointsDataSet->GetLayer(0);
  pointsLayer->ResetReading();

  OGRFeature *tFeature = NULL;



  while ((tFeature = pointsLayer->GetNextFeature()) != NULL)
  {
    OGRPoint *point = dynamic_cast<OGRPoint*>(tFeature->GetGeometryRef());
    HCPoint *p = GeometryFactory::importFromOGRPoint(point);

    for(size_t i = 0; i < polygons.size(); i++)
    {
      if(polygons[i]->contains(p))
      {
        QTextStream &writer = *writers[i];
        writer << QString::number(p->x(),'f',10) << "," << QString::number(p->y(),'f',10) << "," << QString::number(p->z(),'f',10) << endl;
        break;
      }
    }

    delete p;
    OGRFeature::DestroyFeature(tFeature);
  }

  GDALClose(pointsDataSet);


  qDeleteAll(polygons);
  polygons.clear();


  for(QTextStream *writer : writers)
  {
    writer->flush();
    writer->device()->close();
  }

  qDeleteAll(writers);

}

void removeDuplicatePoints()
{
  QString outputDir = "/Users/calebbuahin/Documents/Projects/Models/1d-2d_coupling/GIS/Working_Elev/temp/";
  QString inputDir = "/Users/calebbuahin/Documents/Projects/Models/1d-2d_coupling/GIS/Working_Elev/";

  QDirIterator it(inputDir, QStringList() << "*.csv", QDir::Files, QDirIterator::NoIteratorFlags);

  while(it.hasNext())
  {
    QString infS = it.next();

    QFile infile(infS);
    QFileInfo infileInfo(infS);

    QFile outFile(outputDir + infileInfo.fileName());

    int duplicates = 0;

    if(infile.open(QIODevice::ReadOnly))
    {
      if(outFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
      {
        QHash<double,QSet<double>> data;
        QTextStream reader(&infile);
        QTextStream writer(&outFile);

        writer << "x,y,z" << endl;
        writer.setRealNumberNotation(QTextStream::FixedNotation);
        writer.setRealNumberPrecision(10);

        QString line = reader.readLine();

        while (!reader.atEnd())
        {
          line = reader.readLine();
          QStringList cols = line.split(",");

          double x = cols[0].toDouble();
          double y = cols[1].toDouble();
          double z = cols[2].toDouble();


          if(data.contains(x) && data[x].contains(y))
          {
            duplicates++;
          }
          else
          {
            writer << x << "," << y << "," << z << endl;
            data[x].insert(y);
          }
        }

        printf("duplicates: %i\n", duplicates);

        infile.close();
        outFile.close();
      }
    }
  }
}

int runTests(int argc, char* argv[])
{
  QCoreApplication a(argc, argv);

  int status = 0;

  {
    GeometryTest geomTest;
    status |= QTest::qExec(&geomTest, argc, argv);
  }

  {
    PolyhedralSurfaceTest polyhedralSurfaceTest;
    status |= QTest::qExec(&polyhedralSurfaceTest,argc,argv);
  }

  return a.exec();
}

int thinPoints(int argc, char* argv[])
{
  QFileInfo pointFile(argv[1]);
  QFileInfo outputPointsFile(argv[2]);

  QDir dir(QDir::currentPath());

  if(pointFile.isRelative())
    pointFile = QFileInfo(dir.absoluteFilePath(pointFile.filePath()));

  if(outputPointsFile.isRelative())
    outputPointsFile = QFileInfo(dir.absoluteFilePath(outputPointsFile.filePath()));

  if(pointFile.isFile() && pointFile.exists() &&
     outputPointsFile.dir().exists())
  {
    QList<HCGeometry*> points;
    QString error;
    Envelope envp;

    GeometryFactory::readGeometryFromFile(pointFile.absoluteFilePath(), points, envp, error);

    double change = 2.5;
    double xx = (envp.maxX() - envp.minX()) / change;
    double yy = (envp.maxY() - envp.minY()) / change;


    int dx = std::ceil(xx);
    int dy = std::ceil(yy);

    int maxN = dx * dy;

    HCPoint **geomPoints = new HCPoint*[maxN];
    double *distances = new double[maxN];

    for(int i = 0; i < maxN ; i++)
    {
      geomPoints[i] = nullptr;
      distances[i] = std::numeric_limits<double>::max();
    }

    for(int f = 0; f < points.length() ; f++)
    {
      HCPoint *currPoint = dynamic_cast<HCPoint*>(points[f]);

      double cxx = currPoint->x();
      double cyy = currPoint->y();

      int px = (cxx - envp.minX()) / change;
      int py = (cyy - envp.minY()) / change;

      int ci = 0;
      int cj = 0;
      double cl = std::numeric_limits<double>::max();
      bool found = false;

      for(int i = px - 1; i <= px + 1; i++)
      {
        for(int j = py - 1; j <= py + 1; j++)
        {
          if(i > -1 && i < dx &&  j > -1 && j < dy)
          {
            double x = envp.minX() + i * change;
            double y = envp.minY() + j * change;

            double dis = (Vect(x,y) - Vect(cxx,cyy)).length();

            if(dis < change && dis < distances[j + i*dy] && dis < cl)
            {
              ci = i;
              cj = j;
              cl = dis;
              found = true;
            }
          }
        }
      }

      if(found)
       geomPoints[cj+ci*dy] = currPoint;
    }


    QList<HCGeometry*> scannedPoints;
    for(int i = 0; i < dx*dy ; i++)
    {
      if(geomPoints[i] != nullptr)
      {
        scannedPoints.append(geomPoints[i]);
      }
    }

    GeometryFactory::writeGeometryToFile(scannedPoints, "scanned_points" , IGeometry::PointZ, "ESRI Shapefile", outputPointsFile.absoluteFilePath(), error);
    delete[] geomPoints;
    delete[] distances;
  }

  return 0;
}

QList<HCPoint*> perpendicularXSection(double xsLength, int numXSteps, double factor, const Vect &v1, const Vect &v2)
{
  QList<HCPoint*> xsPoints;

  Vect ev = v2 - v1;
  Vect unorm  = ev.unitNormal2dToVector();
  Vect centroid = v1 + (v2 - v1) * factor;
  Vect start = centroid + (unorm * xsLength / 2.0);
  double xStep = xsLength / (numXSteps * 1.0);

  unorm = unorm * -1.0;

  Vect p = start;

  for(int i = 0; i <= numXSteps; i++)
  {
    Vect nextP = p + unorm * (i * xStep);
    xsPoints.append(new HCPoint(nextP.v[0] , nextP.v[1], 0.0));
  }

  return xsPoints;
}

int createCrossSections(int argc, char* argv[])
{
  GDALAllRegister();

  double lengthStep = QString(argv[1]).toDouble();
  double xCrossLength  = QString(argv[2]).toDouble();
  int numXCrossSteps  = QString(argv[3]).toDouble();

  QFileInfo polyLineFile(argv[4]);
  QFileInfo outputPolyLine(argv[5]);

  QDir dir(QDir::currentPath());

  if(polyLineFile.isRelative())
    polyLineFile = QFileInfo(dir.absoluteFilePath(polyLineFile.filePath()));

  if(outputPolyLine.isRelative())
    outputPolyLine = QFileInfo(dir.absoluteFilePath(outputPolyLine.filePath()));

  if(polyLineFile.isFile() && polyLineFile.exists() &&
     outputPolyLine.dir().exists())
  {

    QList<HCGeometry*> polylines;
    QString error;
    Envelope envp;
    GeometryFactory::readGeometryFromFile(polyLineFile.absoluteFilePath(), polylines, envp, error);

    QList<HCGeometry*> outputPolyLines;

    int u = 0;

    for(HCGeometry *geometry : polylines)
    {
      HCMultiLineString *multiLineString = dynamic_cast<HCMultiLineString*>(geometry);
      HCLineString *polyline = dynamic_cast<HCLineString*>(geometry);


      if(multiLineString)
      {
        for(int f = 0; f < multiLineString->geometryCount() ; f++)
        {
          HCLineString *polyline = dynamic_cast<HCLineString*>(multiLineString->geometryInternal(f));

          HCLineString *centerline = new HCLineString();
          centerline->enable3D();

          HCLineString *leftPoints = new HCLineString();
          leftPoints->enable3D();

          HCLineString *rightPoints = new HCLineString();
          rightPoints->enable3D();

          double currLength = 0.0;
          Vect curPos(*polyline->point(0));
          curPos.v[2] = 0;

          for(int i = 0 ; i < polyline->pointCount(); i++)
          {
            Vect p1 (*polyline->point(i));
            p1.v[2] = 0;

            if(i == 0)
            {
              centerline->addPoint(polyline->pointInternal(i));

              Vect p2 (*polyline->point(i+1));
              p2.v[2] = 0;

              QList<HCPoint*> xsPoints = perpendicularXSection(xCrossLength, numXCrossSteps, 0.0, p1, p2);

              HCLineString *newLineString = new HCLineString();
              newLineString->enable3D();

              for(HCPoint* pp : xsPoints)
              {
                newLineString->addPoint(pp);
              }

              outputPolyLines.append(newLineString);

              leftPoints->addPoint(xsPoints.first());
              rightPoints->addPoint(xsPoints.last());
            }
            else
            {

              double tempLength = (p1 - curPos).length();

              while (currLength + tempLength >= lengthStep)
              {
                double factor = (lengthStep - currLength) / tempLength;

                QList<HCPoint*> xsPoints = perpendicularXSection(xCrossLength, numXCrossSteps, factor, curPos, p1);

                HCLineString *newLineString = new HCLineString();
                newLineString->enable3D();

                for(HCPoint* pp : xsPoints)
                {
                  newLineString->addPoint(pp);
                }

                leftPoints->addPoint(xsPoints.first());
                rightPoints->addPoint(xsPoints.last());

                outputPolyLines.append(newLineString);

                Vect v = curPos + factor * (p1 - curPos);
                curPos = v;
                centerline->addPoint(new HCPoint(v.v[0] , v.v[1], 0.0));
                currLength = 0;
                tempLength = (p1 - curPos).length();
              }

              if(currLength + tempLength < lengthStep)
              {
                currLength += tempLength;
                curPos  = p1;
              }
            }

            if(i == polyline->pointCount() - 1)
            {
              Vect p1 (*polyline->point(i-1));
              Vect p2 (*polyline->point(i));

              QList<HCPoint*> xsPoints = perpendicularXSection(xCrossLength, numXCrossSteps, 1.0, p1, p2);

              HCLineString *newLineString = new HCLineString();
              newLineString->enable3D();

              for(HCPoint* pp : xsPoints)
              {
                newLineString->addPoint(pp);
              }

              leftPoints->addPoint(xsPoints.first());
              rightPoints->addPoint(xsPoints.last());

              centerline->addPoint(new HCPoint(p2.v[0] , p2.v[1], 0.0));
              outputPolyLines.append(newLineString);
            }
          }

          outputPolyLines.append(centerline);
          outputPolyLines.append(rightPoints);
          outputPolyLines.append(leftPoints);
        }
      }
      else if(polyline)
      {
        HCLineString *centerline = new HCLineString();
        centerline->enable3D();

        HCLineString *leftPoints = new HCLineString();
        leftPoints->enable3D();

        HCLineString *rightPoints = new HCLineString();
        rightPoints->enable3D();

        double currLength = 0.0;
        Vect curPos(*polyline->point(0));
        curPos.v[2] = 0;

        for(int i = 0 ; i < polyline->pointCount(); i++)
        {
          Vect p1 (*polyline->point(i));
          p1.v[2] = 0;

          if(i == 0)
          {
            centerline->addPoint(polyline->pointInternal(i));

            Vect p2 (*polyline->point(i+1));
            p2.v[2] = 0;

            QList<HCPoint*> xsPoints = perpendicularXSection(xCrossLength, numXCrossSteps, 0.0, p1, p2);

            HCLineString *newLineString = new HCLineString();
            newLineString->enable3D();

            for(HCPoint* pp : xsPoints)
            {
              newLineString->addPoint(pp);
            }

            outputPolyLines.append(newLineString);

            leftPoints->addPoint(xsPoints.first());
            rightPoints->addPoint(xsPoints.last());
          }
          else
          {

            double tempLength = (p1 - curPos).length();

            while (currLength + tempLength >= lengthStep)
            {
              double factor = (lengthStep - currLength) / tempLength;

              QList<HCPoint*> xsPoints = perpendicularXSection(xCrossLength, numXCrossSteps, factor, curPos, p1);

              HCLineString *newLineString = new HCLineString();
              newLineString->enable3D();

              for(HCPoint* pp : xsPoints)
              {
                newLineString->addPoint(pp);
              }

              leftPoints->addPoint(xsPoints.first());
              rightPoints->addPoint(xsPoints.last());

              outputPolyLines.append(newLineString);

              Vect v = curPos + factor * (p1 - curPos);
              curPos = v;
              centerline->addPoint(new HCPoint(v.v[0] , v.v[1], 0.0));
              currLength = 0;
              tempLength = (p1 - curPos).length();
            }

            if(currLength + tempLength < lengthStep)
            {
              currLength += tempLength;
              curPos  = p1;
            }
          }

          if(i == polyline->pointCount() - 1)
          {
            Vect p1 (*polyline->point(i-1));
            Vect p2 (*polyline->point(i));

            QList<HCPoint*> xsPoints = perpendicularXSection(xCrossLength, numXCrossSteps, 1.0, p1, p2);

            HCLineString *newLineString = new HCLineString();
            newLineString->enable3D();

            for(HCPoint* pp : xsPoints)
            {
              newLineString->addPoint(pp);
            }

            leftPoints->addPoint(xsPoints.first());
            rightPoints->addPoint(xsPoints.last());

            centerline->addPoint(new HCPoint(p2.v[0] , p2.v[1], 0.0));
            outputPolyLines.append(newLineString);
          }
        }

        outputPolyLines.append(centerline);
        outputPolyLines.append(rightPoints);
        outputPolyLines.append(leftPoints);
      }

      u++;

      printf("Progress: %i/%i \n", u , polylines.size());
    }

    GeometryFactory::writeGeometryToFile(outputPolyLines, "xsections", IGeometry::LineStringZ, "ESRI Shapefile", outputPolyLine.absoluteFilePath(), error);

  }

  return 0;
}

int main(int argc, char* argv[])
{
  GeometryFactory::registerGDAL();

  thinPoints(argc, argv);
  //  createCrossSections(argc, argv);
  //  splitPointsByPolygon(argc,argv);
  //  seletPointsWithinPolygon(argc,argv);

  return 0;
}
