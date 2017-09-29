#include "stdafx.h"
#include "spatial/geometrycollection.h"
#include "spatial/linestring.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCMultiLineString::HCMultiLineString(const QString &id, HCGeometry *parent)
  :HCGeometryCollection(id, parent)
{
}

HCMultiLineString::~HCMultiLineString()
{
  m_lineStrings.clear();
}

IGeometry::GeometryType HCMultiLineString::geometryType() const
{
  if(geometryFlags().testFlag(GeometryFlag::HasZ) &&
     geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::MultiLineStringZM;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasZ))
  {
    return GeometryType::MultiLineStringZ;
  }
  else if(geometryFlags().testFlag(GeometryFlag::HasM))
  {
    return GeometryType::MultiLineStringM;
  }
  else
  {
    return GeometryType::MultiLineString;
  }
}

bool HCMultiLineString::isClosed() const
{
  for(HCLineString* lineString : m_lineStrings)
  {
    if(!lineString->isClosed())
    {
      return false;
    }
  }

  return true;
}

double HCMultiLineString::length() const
{
  double outLength = 0;

  for(HCLineString* lineString : m_lineStrings)
  {
    outLength += lineString->length();
  }

  return outLength;
}

ILineString* HCMultiLineString::lineString(int index) const
{
  assert(index < m_lineStrings.length());
  return m_lineStrings[index];
}



HCLineString* HCMultiLineString::lineStringInternal(int index) const
{
  assert(index < m_lineStrings.length());
  return m_lineStrings[index];
}


QList<HCLineString*> HCMultiLineString::lineStrings() const
{
  return m_lineStrings;
}

bool HCMultiLineString::addGeometry(HCGeometry *geometry)
{
  HCLineString *lineString = dynamic_cast<HCLineString*>(geometry);

  return lineString != nullptr && addLineString(lineString);
}

bool HCMultiLineString::addLineString(HCLineString *lineString)
{
  if(HCGeometryCollection::addGeometry(lineString))
  {
    m_lineStrings.append(lineString);
    return true;
  }

  return false;
}

bool HCMultiLineString::removeGeometry(HCGeometry *geometry)
{
  HCLineString *lineString = dynamic_cast<HCLineString*>(geometry);
  return   lineString != nullptr && removeLineString(lineString);
}

bool HCMultiLineString::removeLineString(HCLineString *lineString)
{
 return HCGeometryCollection::removeGeometry(lineString) && m_lineStrings.removeOne(lineString);
}

