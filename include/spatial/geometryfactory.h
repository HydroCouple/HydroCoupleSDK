#ifndef GEOMETRYFACTORY_H
#define GEOMETRYFACTORY_H

#include <ogrsf_frmts.h>
#include "hydrocouplespatial.h"
#include "hydrocouplesdk.h"

class HYDROCOUPLESDK_EXPORT GeometryFactory
{
  public:

    static OGRGeometry* exportToOGRGeometry(const HydroCouple::Spatial::IGeometry* geometry);

    static HydroCouple::Spatial::IGeometry* exportFromOGRGeometry(const OGRGeometry* geometry);

    static HydroCouple::Spatial::IGeometry* importFromWkt(const QString &wktData);

    static HydroCouple::Spatial::IGeometry* importFromWkb(unsigned char* wkbData , int nBytes = -1);

};

#endif // GEOMETRYFACTORY_H

