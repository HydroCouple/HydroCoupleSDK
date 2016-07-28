#include "stdafx.h"
#include "spatial/linestring.h"

HCLinearRing::HCLinearRing(QObject *parent)
  :HCLineString(parent)
{

}

HCLinearRing::~HCLinearRing()
{
}

bool HCLinearRing::isValid() const
{
  return isClosed() && isSimple();
}
