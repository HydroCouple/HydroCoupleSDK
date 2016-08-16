#include "stdafx.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/polygon.h"
#include "spatial/edge.h"
#include "spatial/point.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCTIN::HCTIN(QObject *parent):
  HCPolyhedralSurface(parent)
{
}

HCTIN::~HCTIN()
{
}

ITriangle *HCTIN::patch(int index) const
{
  assert(index < m_patches.length());

  return dynamic_cast<ITriangle*>(m_patches[index]);
}
