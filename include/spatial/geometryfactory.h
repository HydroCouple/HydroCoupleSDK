/*!
 * \file geometryfactory.h
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

#ifndef GEOMETRYFACTORY_H
#define GEOMETRYFACTORY_H

#include "hydrocouplesdk.h"
#include "hydrocouplespatial.h"

class OGRGeometry;
class OGRPoint;
class OGRLineString;
class OGRPolygon;
class OGRMultiPoint;
class OGRMultiLineString;
class OGRMultiPolygon;
class HCGeometry;
class HCPoint;
class HCLineString;
class HCPolygon;
class GeometryArgumentDouble;

template<class T>
class GeometryComponentDataItem;

template<class T>
class TimeGeometryComponentDataItem;

class GeometryCollection;
class QVariant;
class HCTIN;
class HCMultiPoint;
class HCMultiLineString;
class HCMultiPolygon;
class Envelope;

class HYDROCOUPLESDK_EXPORT GeometryFactory
{
  public:

    static OGRGeometry *exportToOGRGeometry(const HydroCouple::Spatial::IGeometry *geometry);

    static OGRPoint *exportToOGRPoint(const HydroCouple::Spatial::IPoint *point);

    static OGRLineString *exportToOGRLineString(const HydroCouple::Spatial::ILineString *lineString);

    static OGRPolygon *exportToOGRPolygon(const HydroCouple::Spatial::IPolygon *polygon);

    static OGRMultiPoint *exportToOGRMultiPoint(const HydroCouple::Spatial::IMultiPoint *multiPoint);

    static OGRMultiLineString *exportToOGRMultiLineString(const HydroCouple::Spatial::IMultiLineString *multiLineString);

    static OGRMultiPolygon *exportToOGRMultiPolygon(const HydroCouple::Spatial::IMultiPolygon *multiPolygon);


    static HCGeometry *importFromOGRGeometry(const OGRGeometry *geometry);

    static HCPoint *importFromOGRPoint(const OGRPoint *point);

    static HCLineString *importFromOGRLineString(const OGRLineString *point);

    static HCPolygon *importFromOGRPolygon(const OGRPolygon *polygon);

    static HCMultiPoint *importFromOGRMultiPoint(const OGRMultiPoint *multiPoint);

    static HCMultiLineString *importFromOGRMultiLineString(const OGRMultiLineString *multiLineString);

    static HCMultiPolygon *importFromOGRMultiPolygon(const OGRMultiPolygon *multiPolygon);

    static HCGeometry *importFromWkt(const QString &wktData);

    static HCGeometry *importFromWkb(unsigned char* wkbData , int nBytes = -1);

    static bool writeGeometryToFile(const GeometryComponentDataItem<double> *geometryComponentDataItem,
                                    const QString &dataFieldName, const QString& gdalDriverName,
                                    const QString& outputFile, QString &errorMessage);

    static bool writeGeometryToFile(const TimeGeometryComponentDataItem<double> *timeGeometryComponentDataItem,
                                    const QString &gdalDriverName, const QString &outputFile, QString &errorMessage);

    static bool writeGeometryToFile(const QList<HCGeometry*> &geometries,
                                    const QString &name,
                                    HydroCouple::Spatial::IGeometry::GeometryType  type,
                                    const QString &gdalDriverName, const QString &outputFile, QString &errorMessage);

    static bool readGeometryFromFile(const QString &filePath, QList<HCGeometry*> &geometries, Envelope &envelope, QString &errorMessage);

    static bool readGeometryFromFile(const QString &filePath, QString &dataFieldName,
                                     GeometryComponentDataItem<double> *geometryComponentDataItem,  Envelope &envelope, QString &errorMessage);

    static bool readGeometryFromFile(const QString& filePath, TimeGeometryComponentDataItem<double> *timeGeometryComponentDataItem,  Envelope &envelope, QString &errorMessage);

    static bool writeTINToNetCDF(HCTIN *tin, const QString &filePath, QString &errorMessage);

    static HCTIN *readTINFromNetCDF(const QString &filePath, QString &errorMessage);

    static bool writeTINVertices(HCTIN *tin, const QString &filePath, const QString &gdalDriverName);

    static bool writeTINPolygons(HCTIN *tin, const QString &filePath, const QString &gdalDriverName);

    static bool readTINFromFile(const QString &filePath, HCTIN *tin, QString &errorMessage);

    static void registerGDAL();

    static bool contains(HCPolygon *polygon, HCPoint *point);

    static bool contains(HCLineString *lineString, HCPoint *point);

  private:

    static bool m_GDALRegistered;
};

#endif // GEOMETRYFACTORY_H

