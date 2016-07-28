#include "stdafx.h"
#include "spatial/polygon.h"
#include "spatial/polyhedralsurface.h"

HCPolygon::HCPolygon(QObject *parent)
  : HCGeometry(parent),
    m_polyhedralSurface(nullptr),
    m_edge(nullptr)
{
}

HCPolygon::HCPolygon(HCPolyhedralSurface *parent)
  : HCGeometry(parent),
    m_polyhedralSurface(parent),
    m_edge(nullptr)
{
}

HCPolygon::~HCPolygon()
{
}


void HCPolygon::addEdge(HCEdge *edge)
{

}
