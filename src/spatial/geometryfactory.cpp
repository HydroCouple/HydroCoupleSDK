#include "stdafx.h"
#include "spatial/geometryfactory.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

OGRGeometry* GeometryFactory::exportToOGRGeometry(const IGeometry *geometry)
{

  return nullptr;
}

IGeometry* GeometryFactory::exportFromOGRGeometry(const OGRGeometry *geometry)
{

  return nullptr;
}

IGeometry* GeometryFactory::importFromWkt(const QString &wktData)
{

  return nullptr;
}

IGeometry* GeometryFactory::importFromWkb(unsigned char *wkbData, int nBytes)
{

  return nullptr;
}
