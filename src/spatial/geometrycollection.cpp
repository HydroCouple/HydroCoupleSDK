#include "stdafx.h"
#include "spatial/geometrycollection.h"
#include "spatial/envelope.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCGeometryCollection::HCGeometryCollection(const QString &id, HCGeometry *parent)
  :HCGeometry(id, parent)
{
}

HCGeometryCollection::~HCGeometryCollection()
{
  qDeleteAll(m_geometries);
  m_geometries.clear();
}

IGeometry::GeometryType HCGeometryCollection::geometryType() const
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
   int dimension = 0;

   for(HCGeometry *geometry : m_geometries)
   {
     dimension = std::max(dimension, geometry->dimension());
   }

   return dimension;
}

Envelope *HCGeometryCollection::envelopeInternal() const
{
  m_envelope->resetExtentsToInfinity();

  for(HCGeometry *geometry : m_geometries)
    m_envelope->merge(geometry->envelopeInternal());

  return m_envelope;
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

HCGeometry* HCGeometryCollection::geometryInternal(int index) const
{
  assert(index < m_geometries.length());
  return m_geometries[index];
}

//void HCGeometryCollection::initializeData(int length, double defaultValue)
//{
//  for(HCGeometry *geometry : m_geometries)
//  {
//    geometry->initializeData(length,defaultValue);
//  }

//  initializeData(length);
//}

QList<HCGeometry*> HCGeometryCollection::geometries() const
{
  return m_geometries;
}

bool HCGeometryCollection::addGeometry(HCGeometry *geometry)
{
  if(geometry->geometryFlags().testFlag(GeometryFlag::HasZ) == this->is3D() &&
     geometry->geometryFlags().testFlag(GeometryFlag::HasM) == this->isMeasured() &&
     !m_geometries.contains(geometry))
  {
    geometry->setIndex(m_geometries.length());
    m_geometries.append(geometry);

    return true;
  }
  else
  {
    return false;
  }

}

bool HCGeometryCollection::removeGeometry(HCGeometry *geometry)
{
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
