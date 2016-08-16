#ifndef POLYHEDRALSURFACETEST_H
#define POLYHEDRALSURFACETEST_H

#include "stdafx.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/edge.h"
#include "spatial/polygon.h"
#include "spatial/point.h"

#include <QtTest/QtTest>

class PolyhedralSurfaceTest : public QObject
{
    Q_OBJECT

  private slots:

    void init()
    {
    }

    void testCreatePolyhedralSurface()
    {
      HCPolyhedralSurface* surface = new HCPolyhedralSurface(this);

      HCPoint::resetId();
      HCPoint::getNextId();

      HCVertex *v0 = new HCVertex(-114.5,39.5,surface);
      HCVertex *v1 = new HCVertex(-114.0,39.5,surface);

      HCVertex *v2 = new HCVertex(-113.5,39.5,surface);
      HCVertex *v3 = new HCVertex(-113.0,39.5,surface);

      HCVertex *v4 = new HCVertex(-114.5,40.0,surface);
      HCVertex *v5 = new HCVertex(-114.0,40.0,surface);
      HCVertex *v6 = new HCVertex(-113.5,40.0,surface);
      HCVertex *v7 = new HCVertex(-113.0,40.0,surface);

      HCVertex *v8  = new HCVertex(-114.5,40.5,surface);
      HCVertex *v9 = new HCVertex(-114.0,40.5,surface);
      HCVertex *v10 = new HCVertex(-113.5,40.5,surface);
      HCVertex *v11 = new HCVertex(-113.0,40.5,surface);

      HCVertex *v12 = new HCVertex(-114.5,41.0,surface);
      HCVertex *v13 = new HCVertex(-114.0,41.0,surface);
      HCVertex *v14 = new HCVertex(-113.5,41.0,surface);
      HCVertex *v15 = new HCVertex(-113.0,41.0,surface);

      //1
      HCPolygon *poly = new HCPolygon (surface);
      HCEdge *tedge = surface->createVertexEdge(v0,v1,poly);
      tedge = surface->createVertexEdge(v1,v5,poly);
      tedge = surface->createVertexEdge(v5,v4,poly);
      tedge = surface->createVertexEdge(v4,v0,poly);

      //2
      poly  = new HCPolygon (surface);
      tedge = surface->createVertexEdge(v1,v2,poly);
      tedge = surface->createVertexEdge(v2,v6,poly);
      tedge = surface->createVertexEdge(v6,v5,poly);
      tedge = surface->createVertexEdge(v5,v1,poly);

      //3
      poly = new HCPolygon (surface);
      tedge = surface->createVertexEdge(v2,v3,poly);
      tedge = surface->createVertexEdge(v3,v7,poly);
      tedge =  surface->createVertexEdge(v7,v6,poly);
      tedge =  surface->createVertexEdge(v6,v2,poly);

      //4
      poly = new HCPolygon (surface);
      tedge =  surface->createVertexEdge(v4,v5,poly);
      tedge =  surface->createVertexEdge(v5,v9,poly);
      tedge =  surface->createVertexEdge(v9,v8,poly);
      tedge =  surface->createVertexEdge(v8,v4,poly);

      //5
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v5,v6,poly);
      tedge =   surface->createVertexEdge(v6,v10,poly);
      tedge =   surface->createVertexEdge(v10,v9,poly);
      tedge =   surface->createVertexEdge(v9,v5,poly);

      //6
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v6,v7,poly);
      tedge =   surface->createVertexEdge(v7,v11,poly);
      tedge =   surface->createVertexEdge(v11,v10,poly);
      tedge =   surface->createVertexEdge(v10,v6,poly);

      //7
      poly = new HCPolygon (surface);
      tedge =  surface->createVertexEdge(v8,v9,poly);
      tedge =   surface->createVertexEdge(v9,v13,poly);
      tedge =    surface->createVertexEdge(v13,v12,poly);
      tedge =   surface->createVertexEdge(v12,v8,poly);

      //8
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v9,v10,poly);
      tedge =   surface->createVertexEdge(v10,v14,poly);
      tedge =   surface->createVertexEdge(v14,v13,poly);
      tedge =   surface->createVertexEdge(v13,v9,poly);

      //9
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v10,v11,poly);
      tedge =   surface->createVertexEdge(v11,v15,poly);
      tedge =   surface->createVertexEdge(v15,v14,poly);
      tedge =   surface->createVertexEdge(v14,v10,poly);


      qDebug() << "\n\n";

      QFile file("polyhedralsurface_create.csv");

      if(file.open(QIODevice::WriteOnly))
      {
        QTextStream stream(&file);

        stream << "HGID,ShapeWKT" << endl;

        for(HCPolygon *polygon : surface->patches())
        {
          QString wkt = polygon->wkt();
          stream << "\"" << QString::number(polygon->index()) << "\" , \"" << wkt  << "\"" << endl;
        }

        file.close();
      }

      QFile fileVRT("polyhedralsurface_create.vrt");
      if(fileVRT.open(QIODevice::WriteOnly))
      {
        QXmlStreamWriter writer(&fileVRT);
        writer.setAutoFormatting(true);

        writer.writeStartDocument();
        {
          writer.writeStartElement("OGRVRTDataSource");
          {
            writer.writeStartElement("OGRVRTLayer");
            {
              writer.writeAttribute("name","polyhedralsurface_create");
              writer.writeTextElement("SrcDataSource","polyhedralsurface_create.csv");
              writer.writeTextElement("GeometryType","wkbPolygon");

              writer.writeStartElement("GeometryField");
              {
                writer.writeAttribute("encoding","WKT");
                writer.writeAttribute("field","ShapeWKT");
              }
              writer.writeEndElement();
            }
            writer.writeEndElement();
          }
          writer.writeEndElement();
        }
        writer.writeEndDocument();

        fileVRT.close();
      }

      system("./ogr2ogr -f KML polyhedralsurface_create.kml polyhedralsurface_create.vrt -dsco NameField=HGID");

      delete surface;
    }

    void testDeletePatchFromPolyhedralSurface()
    {
      HCPolyhedralSurface* surface = new HCPolyhedralSurface(this);

      HCPoint::resetId();
      HCPoint::getNextId();

      HCVertex *v0 = new HCVertex(-114.5,39.5,surface);
      HCVertex *v1 = new HCVertex(-114.0,39.5,surface);

      HCVertex *v2 = new HCVertex(-113.5,39.5,surface);
      HCVertex *v3 = new HCVertex(-113.0,39.5,surface);

      HCVertex *v4 = new HCVertex(-114.5,40.0,surface);
      HCVertex *v5 = new HCVertex(-114.0,40.0,surface);
      HCVertex *v6 = new HCVertex(-113.5,40.0,surface);
      HCVertex *v7 = new HCVertex(-113.0,40.0,surface);

      HCVertex *v8  = new HCVertex(-114.5,40.5,surface);
      HCVertex *v9 = new HCVertex(-114.0,40.5,surface);
      HCVertex *v10 = new HCVertex(-113.5,40.5,surface);
      HCVertex *v11 = new HCVertex(-113.0,40.5,surface);

      HCVertex *v12 = new HCVertex(-114.5,41.0,surface);
      HCVertex *v13 = new HCVertex(-114.0,41.0,surface);
      HCVertex *v14 = new HCVertex(-113.5,41.0,surface);
      HCVertex *v15 = new HCVertex(-113.0,41.0,surface);

      //1
      HCPolygon *poly = new HCPolygon (surface);
      HCEdge *tedge = surface->createVertexEdge(v0,v1,poly);
      tedge = surface->createVertexEdge(v1,v5,poly);
      tedge = surface->createVertexEdge(v5,v4,poly);
      tedge = surface->createVertexEdge(v4,v0,poly);

      //2
      poly  = new HCPolygon (surface);
      tedge = surface->createVertexEdge(v1,v2,poly);
      tedge = surface->createVertexEdge(v2,v6,poly);
      tedge = surface->createVertexEdge(v6,v5,poly);
      tedge = surface->createVertexEdge(v5,v1,poly);

      //3
      poly = new HCPolygon (surface);
      HCPolygon *poly3 = poly;
      tedge = surface->createVertexEdge(v2,v3,poly);
      tedge = surface->createVertexEdge(v3,v7,poly);
      tedge =  surface->createVertexEdge(v7,v6,poly);
      tedge =  surface->createVertexEdge(v6,v2,poly);

      //4
      poly = new HCPolygon (surface);
      tedge =  surface->createVertexEdge(v4,v5,poly);
      tedge =  surface->createVertexEdge(v5,v9,poly);
      tedge =  surface->createVertexEdge(v9,v8,poly);
      tedge =  surface->createVertexEdge(v8,v4,poly);

      //5
      poly = new HCPolygon (surface);
      HCPolygon *poly5 = poly;
      tedge =   surface->createVertexEdge(v5,v6,poly);
      tedge =   surface->createVertexEdge(v6,v10,poly);
      tedge =   surface->createVertexEdge(v10,v9,poly);
      tedge =   surface->createVertexEdge(v9,v5,poly);

      //6
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v6,v7,poly);
      tedge =   surface->createVertexEdge(v7,v11,poly);
      tedge =   surface->createVertexEdge(v11,v10,poly);
      tedge =   surface->createVertexEdge(v10,v6,poly);

      //7
      poly = new HCPolygon (surface);
      HCPolygon *poly7 = poly;
      tedge =  surface->createVertexEdge(v8,v9,poly);
      tedge =   surface->createVertexEdge(v9,v13,poly);
      tedge =    surface->createVertexEdge(v13,v12,poly);
      tedge =   surface->createVertexEdge(v12,v8,poly);

      //8
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v9,v10,poly);
      tedge =   surface->createVertexEdge(v10,v14,poly);
      tedge =   surface->createVertexEdge(v14,v13,poly);
      tedge =   surface->createVertexEdge(v13,v9,poly);

      //9
      poly = new HCPolygon (surface);
      HCPolygon *poly8 = poly;
      tedge =   surface->createVertexEdge(v10,v11,poly);
      tedge =   surface->createVertexEdge(v11,v15,poly);
      tedge =   surface->createVertexEdge(v15,v14,poly);
      tedge =   surface->createVertexEdge(v14,v10,poly);


      delete poly3;
      delete poly5;
      delete poly7;
      delete poly8;

      qDebug() << "\n\n";

      QFile file("polyhedralsurface_delete_patch.csv");

      if(file.open(QIODevice::WriteOnly))
      {
        QTextStream stream(&file);

        stream << "HGID,ShapeWKT" << endl;

        for(HCPolygon *polygon : surface->patches())
        {
          HCPolyhedralSurface::printAllLeftNext(polygon->edge());
          QString wkt = polygon->wkt();
          stream << "\"" << QString::number(polygon->index()) << "\" , \"" << wkt  << "\"" << endl;
        }

        file.close();
      }

      QFile fileVRT("polyhedralsurface_delete_patch.vrt");
      if(fileVRT.open(QIODevice::WriteOnly))
      {
        QXmlStreamWriter writer(&fileVRT);
        writer.setAutoFormatting(true);

        writer.writeStartDocument();
        {
          writer.writeStartElement("OGRVRTDataSource");
          {
            writer.writeStartElement("OGRVRTLayer");
            {
              writer.writeAttribute("name","polyhedralsurface_delete_patch");
              writer.writeTextElement("SrcDataSource","polyhedralsurface_delete_patch.csv");
              writer.writeTextElement("GeometryType","wkbPolygon");

              writer.writeStartElement("GeometryField");
              {
                writer.writeAttribute("encoding","WKT");
                writer.writeAttribute("field","ShapeWKT");
              }
              writer.writeEndElement();
            }
            writer.writeEndElement();
          }
          writer.writeEndElement();
        }
        writer.writeEndDocument();

        fileVRT.close();
      }

      system("ogr2ogr -f KML polyhedralsurface_delete_patch.kml polyhedralsurface_delete_patch.vrt -dsco NameField=HGID");

      delete surface;
    }

    void testMakeAndDeleteVertexEdgeFromPolyhedralSurface()
    {
      HCPolyhedralSurface* surface = new HCPolyhedralSurface(this);

      HCPoint::resetId();
      HCPoint::getNextId();

      HCVertex *v0 = new HCVertex(-114.5,39.5,surface);
      HCVertex *v1 = new HCVertex(-114.0,39.5,surface);

      HCVertex *v2 = new HCVertex(-113.5,39.5,surface);
      HCVertex *v3 = new HCVertex(-113.0,39.5,surface);

      HCVertex *v4 = new HCVertex(-114.5,40.0,surface);
      HCVertex *v5 = new HCVertex(-114.0,40.0,surface);
      HCVertex *v6 = new HCVertex(-113.5,40.0,surface);
      HCVertex *v7 = new HCVertex(-113.0,40.0,surface);

      HCVertex *v8  = new HCVertex(-114.5,40.5,surface);
      HCVertex *v9 = new HCVertex(-114.0,40.5,surface);
      HCVertex *v10 = new HCVertex(-113.5,40.5,surface);
      HCVertex *v11 = new HCVertex(-113.0,40.5,surface);

      HCVertex *v12 = new HCVertex(-114.5,41.0,surface);
      HCVertex *v13 = new HCVertex(-114.0,41.0,surface);
      HCVertex *v14 = new HCVertex(-113.5,41.0,surface);
      HCVertex *v15 = new HCVertex(-113.0,41.0,surface);

      //1
      HCPolygon *poly = new HCPolygon (surface);
      HCEdge *tedge = surface->createVertexEdge(v0,v1,poly);
      tedge = surface->createVertexEdge(v1,v5,poly);
      tedge = surface->createVertexEdge(v5,v4,poly);
      tedge = surface->createVertexEdge(v4,v0,poly);

      //2
      poly  = new HCPolygon (surface);
      tedge = surface->createVertexEdge(v1,v2,poly);
      tedge = surface->createVertexEdge(v2,v6,poly);
      tedge = surface->createVertexEdge(v6,v5,poly);
      tedge = surface->createVertexEdge(v5,v1,poly);

      //3
      poly = new HCPolygon (surface);
      tedge = surface->createVertexEdge(v2,v3,poly);
      tedge = surface->createVertexEdge(v3,v7,poly);
      tedge =  surface->createVertexEdge(v7,v6,poly);
      tedge =  surface->createVertexEdge(v6,v2,poly);

      //4
      poly = new HCPolygon (surface);
      HCPolygon *poly4 = poly;
      tedge =  surface->createVertexEdge(v4,v5,poly);
      tedge =  surface->createVertexEdge(v5,v9,poly);
      tedge =  surface->createVertexEdge(v9,v8,poly);
      tedge =  surface->createVertexEdge(v8,v4,poly);

      //5
      poly = new HCPolygon (surface);
      HCPolygon *poly5 = poly;
      tedge =   surface->createVertexEdge(v5,v6,poly);
      tedge =   surface->createVertexEdge(v6,v10,poly);
      tedge =   surface->createVertexEdge(v10,v9,poly);
      tedge =   surface->createVertexEdge(v9,v5,poly);

      //6
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v6,v7,poly);
      tedge =   surface->createVertexEdge(v7,v11,poly);
      tedge =   surface->createVertexEdge(v11,v10,poly);
      tedge =   surface->createVertexEdge(v10,v6,poly);

      //7
      poly = new HCPolygon (surface);
      tedge =  surface->createVertexEdge(v8,v9,poly);
      tedge =   surface->createVertexEdge(v9,v13,poly);
      tedge =    surface->createVertexEdge(v13,v12,poly);
      tedge =   surface->createVertexEdge(v12,v8,poly);

      //8
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v9,v10,poly);
      tedge =   surface->createVertexEdge(v10,v14,poly);
      tedge =   surface->createVertexEdge(v14,v13,poly);
      tedge =   surface->createVertexEdge(v13,v9,poly);

      //9
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v10,v11,poly);
      tedge =   surface->createVertexEdge(v11,v15,poly);
      tedge =   surface->createVertexEdge(v15,v14,poly);
      tedge =   surface->createVertexEdge(v14,v10,poly);

      HCVertex *v16 = new HCVertex(-114.0,40.75,surface);
      surface->createVertexEdge(v9,v16,poly4,poly5);

      HCEdge *e_6_10 = surface->findEdge(v6,v10);
      HCPolyhedralSurface::printEdge(e_6_10);
      surface->deleteVertexEdge(e_6_10);

      qDebug() << "\n\n";

      QFile file("polyhedralsurface_vertex_edge.csv");

      if(file.open(QIODevice::WriteOnly))
      {
        QTextStream stream(&file);

        stream << "HGID,ShapeWKT" << endl;

        for(HCPolygon *polygon : surface->patches())
        {
          HCPolyhedralSurface::printAllLeftNext(polygon->edge());
          QString wkt = polygon->wkt();
          stream << "\"" << QString::number(polygon->index()) << "\" , \"" << wkt  << "\"" << endl;
        }

        file.close();
      }

      QFile fileVRT("polyhedralsurface_vertex_edge.vrt");
      if(fileVRT.open(QIODevice::WriteOnly))
      {
        QXmlStreamWriter writer(&fileVRT);
        writer.setAutoFormatting(true);

        writer.writeStartDocument();
        {
          writer.writeStartElement("OGRVRTDataSource");
          {
            writer.writeStartElement("OGRVRTLayer");
            {
              writer.writeAttribute("name","polyhedralsurface_vertex_edge");
              writer.writeTextElement("SrcDataSource","polyhedralsurface_vertex_edge.csv");
              writer.writeTextElement("GeometryType","wkbPolygon");

              writer.writeStartElement("GeometryField");
              {
                writer.writeAttribute("encoding","WKT");
                writer.writeAttribute("field","ShapeWKT");
              }
              writer.writeEndElement();
            }
            writer.writeEndElement();
          }
          writer.writeEndElement();
        }
        writer.writeEndDocument();

        fileVRT.close();
      }

      system("ogr2ogr -f KML polyhedralsurface_vertex_edge.kml polyhedralsurface_vertex_edge.vrt -dsco NameField=HGID");

      delete surface;
    }

    void testMakeAndDeleteFaceEdgeFromPolyhedralSurface()
    {
      HCPolyhedralSurface* surface = new HCPolyhedralSurface(this);

      HCPoint::resetId();
      HCPoint::getNextId();

      HCVertex *v0 = new HCVertex(-114.5,39.5,surface);
      HCVertex *v1 = new HCVertex(-114.0,39.5,surface);

      HCVertex *v2 = new HCVertex(-113.5,39.5,surface);
      HCVertex *v3 = new HCVertex(-113.0,39.5,surface);

      HCVertex *v4 = new HCVertex(-114.5,40.0,surface);
      HCVertex *v5 = new HCVertex(-114.0,40.0,surface);
      HCVertex *v6 = new HCVertex(-113.5,40.0,surface);
      HCVertex *v7 = new HCVertex(-113.0,40.0,surface);

      HCVertex *v8  = new HCVertex(-114.5,40.5,surface);
      HCVertex *v9 = new HCVertex(-114.0,40.5,surface);
      HCVertex *v10 = new HCVertex(-113.5,40.5,surface);
      HCVertex *v11 = new HCVertex(-113.0,40.5,surface);

      HCVertex *v12 = new HCVertex(-114.5,41.0,surface);
      HCVertex *v13 = new HCVertex(-114.0,41.0,surface);
      HCVertex *v14 = new HCVertex(-113.5,41.0,surface);
      HCVertex *v15 = new HCVertex(-113.0,41.0,surface);

      //1
      HCPolygon *poly = new HCPolygon (surface);
      HCEdge *tedge = surface->createVertexEdge(v0,v1,poly);
      tedge = surface->createVertexEdge(v1,v5,poly);
      tedge = surface->createVertexEdge(v5,v4,poly);
      tedge = surface->createVertexEdge(v4,v0,poly);

      //2
      poly  = new HCPolygon (surface);
      tedge = surface->createVertexEdge(v1,v2,poly);
      tedge = surface->createVertexEdge(v2,v6,poly);
      tedge = surface->createVertexEdge(v6,v5,poly);
      tedge = surface->createVertexEdge(v5,v1,poly);

      //3
      poly = new HCPolygon (surface);
      tedge = surface->createVertexEdge(v2,v3,poly);
      tedge = surface->createVertexEdge(v3,v7,poly);
      tedge =  surface->createVertexEdge(v7,v6,poly);
      tedge =  surface->createVertexEdge(v6,v2,poly);

      //4
      poly = new HCPolygon (surface);
      HCPolygon *poly4 = poly;
      tedge =  surface->createVertexEdge(v4,v5,poly);
      tedge =  surface->createVertexEdge(v5,v9,poly);
      tedge =  surface->createVertexEdge(v9,v8,poly);
      tedge =  surface->createVertexEdge(v8,v4,poly);

      //5
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v5,v6,poly);
      tedge =   surface->createVertexEdge(v6,v10,poly);
      tedge =   surface->createVertexEdge(v10,v9,poly);
      tedge =   surface->createVertexEdge(v9,v5,poly);

      //6
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v6,v7,poly);
      tedge =   surface->createVertexEdge(v7,v11,poly);
      tedge =   surface->createVertexEdge(v11,v10,poly);
      tedge =   surface->createVertexEdge(v10,v6,poly);

      //7
      poly = new HCPolygon (surface);
      tedge =  surface->createVertexEdge(v8,v9,poly);
      tedge =   surface->createVertexEdge(v9,v13,poly);
      tedge =    surface->createVertexEdge(v13,v12,poly);
      tedge =   surface->createVertexEdge(v12,v8,poly);

      //8
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v9,v10,poly);
      tedge =   surface->createVertexEdge(v10,v14,poly);
      tedge =   surface->createVertexEdge(v14,v13,poly);
      tedge =   surface->createVertexEdge(v13,v9,poly);

      //9
      poly = new HCPolygon (surface);
      tedge =   surface->createVertexEdge(v10,v11,poly);
      tedge =   surface->createVertexEdge(v11,v15,poly);
      tedge =   surface->createVertexEdge(v15,v14,poly);
      tedge =   surface->createVertexEdge(v14,v10,poly);

      surface->createFaceEdge(poly4,v4,v9);
      surface->deleteFaceEdge(surface->findEdge(v6,v10));

      qDebug() << "\n\n";

      QFile file("polyhedralsurface_face_edge.csv");

      if(file.open(QIODevice::WriteOnly))
      {
        QTextStream stream(&file);

        stream << "HGID,ShapeWKT" << endl;

        for(HCPolygon *polygon : surface->patches())
        {
          HCPolyhedralSurface::printAllLeftNext(polygon->edge());
          QString wkt = polygon->wkt();
          stream << "\"" << QString::number(polygon->index()) << "\" , \"" << wkt  << "\"" << endl;
        }

        file.close();
      }

      QFile fileVRT("polyhedralsurface_face_edge.vrt");
      if(fileVRT.open(QIODevice::WriteOnly))
      {
        QXmlStreamWriter writer(&fileVRT);
        writer.setAutoFormatting(true);

        writer.writeStartDocument();
        {
          writer.writeStartElement("OGRVRTDataSource");
          {
            writer.writeStartElement("OGRVRTLayer");
            {
              writer.writeAttribute("name","polyhedralsurface_face_edge");
              writer.writeTextElement("SrcDataSource","polyhedralsurface_face_edge.csv");
              writer.writeTextElement("GeometryType","wkbPolygon");

              writer.writeStartElement("GeometryField");
              {
                writer.writeAttribute("encoding","WKT");
                writer.writeAttribute("field","ShapeWKT");
              }
              writer.writeEndElement();
            }
            writer.writeEndElement();
          }
          writer.writeEndElement();
        }
        writer.writeEndDocument();

        fileVRT.close();
      }

      system("ogr2ogr -f KML polyhedralsurface_face_edge.kml polyhedralsurface_face_edge.vrt -dsco NameField=HGID");

      delete surface;
    }

    void cleanup()
    {

    }
};

#endif // POLYHEDRALSURFACETEST_H
