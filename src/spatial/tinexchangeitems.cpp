#include "stdafx.h"
#include "spatial/tinexchangeitem.h"

#include "spatial/polyhedralsurface.h"
#include "spatial/polygon.h"
#include "core/dimension.h"
#include "spatial/edge.h"
#include "spatial/point.h"
#include "core/valuedefinition.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

TINOutputDouble::TINOutputDouble(const QString &id,
                                 MeshDataType dataType,
                                 const QSharedPointer<HCTIN> & TINSurface,
                                 Dimension *cellDimension,
                                 Dimension *edgeDimension,
                                 Dimension *nodeDimension,
                                 ValueDefinition *valueDefinition,
                                 AbstractModelComponent *modelComponent):
  AbstractOutput(id,QList<Dimension*>({cellDimension,edgeDimension,nodeDimension}),
                 valueDefinition,modelComponent),
  m_dataType(dataType),
  m_cellDimension(cellDimension),
  m_edgeDimension(edgeDimension),
  m_nodeDimension(nodeDimension),
  m_TINSurface(TINSurface)
{
  resetDataArray();
}

TINOutputDouble::~TINOutputDouble()
{
}

MeshDataType TINOutputDouble::meshDataType() const
{
  return m_dataType;
}

IPolyhedralSurface *TINOutputDouble::polyhedralSurface() const
{
  return m_TINSurface.data();
}

IDimension *TINOutputDouble::cellDimension() const
{
  return m_cellDimension;
}

IDimension *TINOutputDouble::edgeDimension() const
{
  return m_edgeDimension;
}

IDimension *TINOutputDouble::nodeDimension() const
{
  return m_nodeDimension;
}

ITIN *TINOutputDouble::TIN() const
{
  return m_TINSurface.data();
}

QSharedPointer<HCTIN> TINOutputDouble::sharedTIN() const
{
  return m_TINSurface;
}

int TINOutputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Centroid:
      {
        assert((int)dimensionIndexes.size() == 0);
        return m_TINSurface->patchCount();
      }
      break;
    case HydroCouple::Spatial::Face:
      {
        assert((int)dimensionIndexes.size() == 1);
        return 3;
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        assert((int)dimensionIndexes.size() == 2);
        return 2;
      }
      break;
  }
}

void TINOutputDouble::getValue(const std::vector<int> &dimensionIndexes,  void *data) const
{
  double* dataC = (double*) data;

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Centroid:
      {
        *dataC = m_data[dimensionIndexes[0]];
      }
      break;
    case HydroCouple::Spatial::Face:
      {
        int index = dimensionIndexes[0] + dimensionIndexes[1] * m_TINSurface->patchCount();
        *dataC = m_data[index];
      }
      break;
    case HydroCouple::Spatial::Node:
      {

        int pindex = dimensionIndexes[1] + dimensionIndexes[2];
        pindex = pindex > 2 ? 0 : pindex;

        int index = dimensionIndexes[0] + pindex * m_TINSurface->patchCount();
        *dataC = m_data[index];
      }
      break;
  }
}

void TINOutputDouble::getValue(int cellDimensionIndex, int faceDimensionIndex, int nodeDimensionIndex, void *data) const
{
  double* dataC = (double*) data;

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Centroid:
      {
        *dataC = m_data[cellDimensionIndex];
      }
      break;
    case HydroCouple::Spatial::Face:
      {
        int index = cellDimensionIndex + faceDimensionIndex * m_TINSurface->patchCount();
        *dataC = m_data[index];
      }
      break;
    case HydroCouple::Spatial::Node:
      {

        int pindex = faceDimensionIndex + nodeDimensionIndex;
        pindex = pindex > 2 ? 0 : pindex;

        int index = cellDimensionIndex + pindex * m_TINSurface->patchCount();
        *dataC = m_data[index];
      }
      break;
  }
}

void TINOutputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  double *dataC = (double*) data;

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Centroid:
      {
        m_data[dimensionIndexes[0]] = *dataC;
      }
      break;
    case HydroCouple::Spatial::Face:
      {
        int i = dimensionIndexes[0];
        int j = dimensionIndexes[1];

        m_data[i + j * m_TINSurface->patchCount()] = *dataC;
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int i = dimensionIndexes[0];
        int j = dimensionIndexes[1] + dimensionIndexes[2];

        j = j > 2 ? 0 : j;
        m_data[i + j * m_TINSurface->patchCount() ] = *dataC;
      }
      break;
  }
}

void TINOutputDouble::setValue(int cellDimensionIndex, int faceDimensionIndex, int nodeDimensionIndex,  const void *data)
{
  double *dataC = (double*) data;

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Centroid:
      {
        m_data[cellDimensionIndex] = *dataC;
      }
      break;
    case HydroCouple::Spatial::Face:
      {
        int i = cellDimensionIndex;
        int j = faceDimensionIndex;

        m_data[i + j * m_TINSurface->patchCount()] = *dataC;
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int i = cellDimensionIndex;
        int j = faceDimensionIndex + nodeDimensionIndex;

        j = j > 2 ? 0 : j;
        m_data[i + j * m_TINSurface->patchCount() ] = *dataC;
      }
      break;
  }
}

void TINOutputDouble::setTIN(const QSharedPointer<HCTIN> &TINSurface)
{
  m_TINSurface = TINSurface;
  resetDataArray();
}

void TINOutputDouble::resetDataArray()
{
  if(!m_TINSurface.isNull())
  {
    switch(m_dataType)
    {
      case HydroCouple::Spatial::Centroid:
        {
          m_data.resize(m_TINSurface->patchCount());
        }
        break;
      case HydroCouple::Spatial::Face:
      case HydroCouple::Spatial::Node:
        {
          m_data.resize(m_TINSurface->patchCount() * 3);
        }
        break;
    }

    double defaultValue = valueDefinitionInternal()->defaultValue().toDouble();
    for(size_t i = 0 ; i < m_data.size() ; i++)
    {
      m_data[i] = defaultValue;
    }
  }
}

void TINOutputDouble::updateValues()
{

}

