#include "stdafx.h"
#include "spatial/geometrycollection.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCGeometryCollection::HCGeometryCollection(QObject *parent)
  :HCGeometry(parent)
{
}

HCGeometryCollection::~HCGeometryCollection()
{
}

HydroCouple::Spatial::GeometryType HCGeometryCollection::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::GeometryCollectionZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return GeometryType::GeometryCollectionZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::GeometryCollectionM;
  }
  else
  {
    return GeometryType::GeometryCollection;
  }
}

int HCGeometryCollection::geometryCount() const
{
  return m_geometries.length();
}

IGeometry* HCGeometryCollection::geometry(int index) const
{
  assert(index < m_geometries.length());
  return m_geometries[index];
}

void HCGeometryCollection::initializeData(int length)
{
  for(HCGeometry *geometry : m_geometries)
  {
    geometry->initializeData(length);
  }
  initializeData(length);
}

QList<HCGeometry*> HCGeometryCollection::geometries() const
{
  return m_geometries;
}

void HCGeometryCollection::addGeometry(HCGeometry *geometry)
{
  assert(geometry->spatialReferenceSystem()->authSRID() == spatialReferenceSystem()->authSRID());
  assert(!geometry->spatialReferenceSystem()->authName().compare(spatialReferenceSystem()->authName()));

  if(!m_geometries.contains(geometry))
  {
    geometry->setIndex(m_geometries.length());
    m_geometries.append(geometry);
  }
}

bool HCGeometryCollection::removeGeometry(HCGeometry *geometry)
{
  return m_geometries.removeAll(geometry);
}
