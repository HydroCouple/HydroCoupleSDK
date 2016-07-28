#include "stdafx.h"
#include "spatial/point.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCVertex::HCVertex(QObject* parent):
  HCPoint(parent),
  m_edge(nullptr),
  m_dataLength(0),
  m_data(nullptr)
{
}

HCVertex::HCVertex(double x, double y, QObject* parent):
  HCPoint(x,y,parent),
  m_edge(nullptr),
  m_dataLength(0),
  m_data(nullptr)
{
}

HCVertex::HCVertex(double x, double y, double z, QObject* parent):
  HCPoint(x,y,z,parent),
  m_edge(nullptr),
  m_dataLength(0),
  m_data(nullptr)
{
}

HCVertex::HCVertex(double x, double y, double z, double m, QObject* parent):
  HCPoint(x,y,z,m,parent),
  m_edge(nullptr),
  m_dataLength(0),
  m_data(nullptr)
{
}

HCVertex::~HCVertex()
{
}

IEdge* HCVertex::edge() const
{
  return m_edge;
}

void HCVertex::addEdge(IEdge *edge)
{
  if(edge && edge->orig() == this)
  {
    m_edge = edge;
  }
}

void HCVertex::removeEdge(IEdge *edge)
{
  IEdge *next = edge->origNext();
  m_edge = next!= edge ? next : nullptr;
}

void HCVertex::initializeData(int length)
{
  if(m_data)
  {
    delete[] m_data;
    m_data = nullptr;
  }

  if(length > 0)
  {
    m_data = new double[length];
  }
}

int HCVertex::dataLength() const
{
   return m_dataLength;
}

double* HCVertex::data() const
{
  return m_data;
}
