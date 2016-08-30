#ifndef GEOMETRYFACTORY_H
#define GEOMETRYFACTORY_H

#include "hydrocouplesdk.h"
#include "hydrocouplespatial.h"

class OGRGeometry;
class OGRPoint;
class OGRLineString;
class OGRPolygon;

class HCGeometry;
class HCPoint;
class HCLineString;
class HCPolygon;
class HCGeometryArgumentDouble;
class HCGeometryCollection;
class QVariant;
class HCTIN;

class HYDROCOUPLESDK_EXPORT GeometryFactory
{
  public:

    static OGRGeometry *exportToOGRGeometry(const HydroCouple::Spatial::IGeometry *geometry);

    static OGRPoint *exportToOGRPoint(const HydroCouple::Spatial::IPoint *point);

    static OGRLineString *exportToOGRLineString(const HydroCouple::Spatial::ILineString *lineString);

    static OGRPolygon *exportToOGRPolygon(const HydroCouple::Spatial::IPolygon *polygon);


    static HCGeometry *importFromOGRGeometry(const OGRGeometry *geometry , QObject *parent = nullptr);

    static HCPoint *importFromOGRPoint(const OGRPoint *point, QObject *parent = nullptr);

    static HCLineString *importFromOGRLineString(const OGRLineString *point, QObject *parent = nullptr);

    static HCPolygon *importFromOGRPolygon(const OGRPolygon *polygon, QObject *parent = nullptr);


    static HCGeometry *importFromWkt(const QString &wktData);

    static HCGeometry *importFromWkb(unsigned char* wkbData , int nBytes = -1);


    static bool writeGeometryDataItemToFile(const HCGeometryArgumentDouble *geometryData, const QString &dataFieldName,
                                            const QString& gdalDriverName, const QString& outputFile, QString &errorMessage);

    static bool readGeometryDataItemFromFile(const QString& filePath, QString &dataFieldName, HCGeometryArgumentDouble* geometryArgument, QString &errorMessage);

    static bool writeTINToFile( HCTIN *tin, const QString &filePath, QString &errorMessage);

    static bool writeTINVertices( HCTIN *tin, const QString &filePath, const QString &gdalDriverName);

    static bool writeTINPolygons( HCTIN *tin, const QString &filePath, const QString &gdalDriverName);

//    static bool writeTINEdges( HCTIN *tin, const QString &filePath, const QString &gdalDriverName);

    static bool readTINFromFile(const QString &filePath, HCTIN *tin, QString &errorMessage);

    static void registerGDAL();



};

#endif // GEOMETRYFACTORY_H

