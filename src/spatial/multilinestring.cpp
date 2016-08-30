#include "stdafx.h"
#include "spatial/geometrycollection.h"
#include "spatial/linestring.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCMultiLineString::HCMultiLineString(QObject *parent)
  :HCGeometryCollection(parent)
{
}

HCMultiLineString::~HCMultiLineString()
{
}

HydroCouple::Spatial::GeometryType HCMultiLineString::geometryType() const
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

ILineString* HCMultiLineString::element(int index) const
{
  assert(index < m_lineStrings.length());
  return m_lineStrings[index];
}

QList<HCLineString*> HCMultiLineString::lineStrings() const
{
  return m_lineStrings;
}

void HCMultiLineString::addLineString(HCLineString *lineString)
{
  m_lineStrings.append(lineString);
  addGeometry(lineString);
}

bool HCMultiLineString::removeLineString(HCLineString *lineString)
{
  return m_lineStrings.removeOne(lineString) && removeGeometry(lineString);
}
