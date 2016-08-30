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

int HCGeometryCollection::dimension() const
{

}

IGeometry *HCGeometryCollection::envelope() const
{
  return nullptr;
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

void HCGeometryCollection::initializeData(int length, double defaultValue)
{
  for(HCGeometry *geometry : m_geometries)
  {
    geometry->initializeData(length,defaultValue);
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
    emit propertyChanged("Geometries");
  }
}

bool HCGeometryCollection::removeGeometry(HCGeometry *geometry)
{
  emit propertyChanged("Geometries");
  return m_geometries.removeOne(geometry);
}


void HCGeometryCollection::enable3D()
{
  for(HCGeometry* geom : geometries())
  {
    geom->setGeometryFlag(GeometryFlag::HasZ , true);
  }
  setGeometryFlag(GeometryFlag::HasZ , true);
}

void HCGeometryCollection::disable3D()
{
  for(HCGeometry* geom : geometries())
  {
    geom->setGeometryFlag(GeometryFlag::HasZ , false);
  }
  setGeometryFlag(GeometryFlag::HasZ , false);
}

void HCGeometryCollection::enableM()
{
  for(HCGeometry* geom : geometries())
  {
    geom->setGeometryFlag(GeometryFlag::HasM , true);
  }
  setGeometryFlag(GeometryFlag::HasM , true);
}

void HCGeometryCollection::disableM()
{
  for(HCGeometry* geom : geometries())
  {
    geom->setGeometryFlag(GeometryFlag::HasM , false);
  }
  setGeometryFlag(GeometryFlag::HasM , false);
}
