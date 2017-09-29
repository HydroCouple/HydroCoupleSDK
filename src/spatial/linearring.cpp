#include "stdafx.h"
#include "spatial/linestring.h"

HCLinearRing::HCLinearRing(const QString &id, HCGeometry *parent)
  :HCLineString(id, parent)
{

}

HCLinearRing::~HCLinearRing()
{
}

bool HCLinearRing::isValid() const
{
  return isClosed() && isSimple();
}
