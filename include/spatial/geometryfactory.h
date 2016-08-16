#ifndef GEOMETRYFACTORY_H
#define GEOMETRYFACTORY_H

#include "hydrocouplesdk.h"
#include "spatial/geometry.h"
#include "spatial/point.h"
#include "spatial/linestring.h"
#include "spatial/polygon.h"

#include <gdal/ogrsf_frmts.h>

class HYDROCOUPLESDK_EXPORT GeometryFactory
{
  public:

    static OGRGeometry *exportToOGRGeometry(const HydroCouple::Spatial::IGeometry *geometry);

    static OGRPoint *exportToOGRPoint(const HydroCouple::Spatial::IPoint *point);

    static OGRLineString *exportToOGRLineString(const HydroCouple::Spatial::ILineString *lineString);

    static OGRPolygon *exportToOGRPolygon(const HydroCouple::Spatial::IPolygon *polygon);


    static HCGeometry *importFromOGRGeometry(const OGRGeometry *geometry);

    static HCPoint *importFromOGRPoint(const OGRPoint *point);

    static HCLineString *importFromOGRLineString(const OGRLineString *point);

    static HCPolygon *importFromOGRPolygon(const OGRPolygon *polygon);


    static HydroCouple::Spatial::IGeometry *importFromWkt(const QString &wktData);

    static HydroCouple::Spatial::IGeometry *importFromWkb(unsigned char* wkbData , int nBytes = -1);

};

#endif // GEOMETRYFACTORY_H

