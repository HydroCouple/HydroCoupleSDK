#include "stdafx.h"
#include "spatial/polyhedralsurfaceexchangeitem.h"
#include "spatial/polyhedralsurface.h"
#include "spatial/polygon.h"
#include "core/dimension.h"
#include "spatial/edge.h"
#include "spatial/point.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCPolyhedralSurfaceOutputDouble::HCPolyhedralSurfaceOutputDouble(const QString &id,
                                                                 HydroCouple::Spatial::PolyhedralSurfaceDataType dataType,
                                                                 HCPolyhedralSurface *polyhedralSurface,
                                                                 Dimension *patchDimension,
                                                                 Dimension *patchEdgeDimension,
                                                                 Dimension *edgeNodeDimension,
                                                                 ValueDefinition *valueDefinition,
                                                                 AbstractModelComponent *modelComponent):
  AbstractOutput(id,QList<Dimension*>({patchDimension,patchEdgeDimension,edgeNodeDimension}),
                 valueDefinition,modelComponent),
  m_dataType(dataType),
  m_patchDimension(patchDimension),
  m_patchEdgeDimension(patchEdgeDimension),
  m_edgeNodeDimension(edgeNodeDimension),
  m_polyhedralSurface(polyhedralSurface)
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        m_polyhedralSurface->initializePatchData(1);
        m_polyhedralSurface->initializeEdgeData(0);
        m_polyhedralSurface->initializeNodeData(0);
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        m_polyhedralSurface->initializePatchData(0);
        m_polyhedralSurface->initializeEdgeData(1);
        m_polyhedralSurface->initializeNodeData(0);
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        m_polyhedralSurface->initializePatchData(0);
        m_polyhedralSurface->initializeEdgeData(0);
        m_polyhedralSurface->initializeNodeData(1);
      }
      break;
  }
}

HCPolyhedralSurfaceOutputDouble::~HCPolyhedralSurfaceOutputDouble()
{
}

PolyhedralSurfaceDataType HCPolyhedralSurfaceOutputDouble::polyhedralSurfaceDataType() const
{
  return m_dataType;
}

IPolyhedralSurface *HCPolyhedralSurfaceOutputDouble::polyhedralSurface() const
{
  return m_polyhedralSurface;
}

void HCPolyhedralSurfaceOutputDouble::setPolyhedralSurface(HCPolyhedralSurface *polyhedralSurface)
{
  m_polyhedralSurface = polyhedralSurface;
}

IDimension *HCPolyhedralSurfaceOutputDouble::patchDimension() const
{
  return m_patchDimension;
}

IDimension *HCPolyhedralSurfaceOutputDouble::patchEdgeDimension() const
{
  return m_patchEdgeDimension;
}

IDimension *HCPolyhedralSurfaceOutputDouble::edgeNodeDimension() const
{
  return m_edgeNodeDimension;
}

int HCPolyhedralSurfaceOutputDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        assert(dimensionIndexesLength == 0);
        return m_polyhedralSurface->patchCount();
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        assert(dimensionIndexesLength == 1);

        IPolygon *patch = m_polyhedralSurface->patch(dimensionIndexes[0]);

        assert(dimensionIndexes[0] <= patch->exteriorRing()->pointCount() - 1);

        return patch->exteriorRing()->pointCount() - 1;
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        assert(dimensionIndexesLength == 2);

        IPolygon *patch = m_polyhedralSurface->patch(dimensionIndexes[0]);

        assert(dimensionIndexes[0] <= patch->exteriorRing()->pointCount() - 1);
        assert(dimensionIndexes[1] <= patch->exteriorRing()->pointCount());

        return patch->exteriorRing()->pointCount();
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::getValue(int dimensionIndexes[], QVariant &data) const
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[dimensionIndexes[0]];
        data = QVariant::fromValue(patch->data[0]);
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[dimensionIndexes[0]];
        HCEdge *edge = patch->getEdge(dimensionIndexes[1]);
        data = QVariant::fromValue(edge->data[0]);
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[dimensionIndexes[0]];
        HCEdge *edge = patch->getEdge(dimensionIndexes[1]);
        data = QVariant::fromValue(edge->point(dimensionIndexes[2])->data[0]);
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::getValues(int dimensionIndexes[], int stride[], QVariant data[]) const
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[i+istart];
          data[i-istart] = QVariant::fromValue(patch->data[0]);
        }
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];
        int jstart = dimensionIndexes[1];
        int jstride = stride[1];

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            data[i + j*istride] = QVariant::fromValue(edge->data[0]);
          }
        }
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];
        int jstart = dimensionIndexes[1];
        int jstride = stride[1];
        int kstart = dimensionIndexes[2];
        int kstride = stride[2];


        for(int i = 0 ; i <  istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            for(int k = 0 ; k < kstride ; k++)
            {
              HCPoint* point = edge->point(kstart + k);
              data[i + j*istride + k*istride*jstride] = QVariant::fromValue(point->data[0]);
            }
          }
        }
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  double* dataC = (double*) data;

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[i+istart];
          dataC[i] = patch->data[0];
        }
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];
        int jstart = dimensionIndexes[1];
        int jstride = stride[1];

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            dataC[i + j*istride] = edge->data[0];
          }
        }
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];
        int jstart = dimensionIndexes[1];
        int jstride = stride[1];
        int kstart = dimensionIndexes[2];
        int kstride = stride[2];


        for(int i = 0 ; i <  istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            for(int k = 0 ; k < kstride ; k++)
            {
              HCPoint* point = edge->point(kstart + k);
              dataC[i + j*istride + k*istride*jstride] = point->data[0];
            }
          }
        }
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[dimensionIndexes[0]];
        patch->data[0] = data.toDouble();
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[dimensionIndexes[0]];
        HCEdge *edge = patch->getEdge(dimensionIndexes[1]);
        edge->data[0] = data.toDouble();
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[dimensionIndexes[0]];
        HCEdge *edge = patch->getEdge(dimensionIndexes[1]);
        edge->point(dimensionIndexes[2])->data[0] = data.toDouble();
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[i+istart];
          patch->data[0] = data[i].toDouble();
        }
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];
        int jstart = dimensionIndexes[1];
        int jstride = stride[1];

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            edge->data[0] = data[i + j*istride].toDouble();
          }
        }
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];
        int jstart = dimensionIndexes[1];
        int jstride = stride[1];
        int kstart = dimensionIndexes[2];
        int kstride = stride[2];


        for(int i = 0 ; i <  istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            for(int k = 0 ; k < kstride ; k++)
            {
              HCPoint* point = edge->point(kstart + k);
              point->data[0] = data[i + j*istride + k*istride*jstride].toDouble() ;
            }
          }
        }
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  double * dataC = (double*) data;

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[i+istart];
          patch->data[0] = dataC[i];
        }
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];
        int jstart = dimensionIndexes[1];
        int jstride = stride[1];

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            edge->data[0] = dataC[i + j*istride];
          }
        }
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int istart = dimensionIndexes[0];
        int istride = stride[0];
        int jstart = dimensionIndexes[1];
        int jstride = stride[1];
        int kstart = dimensionIndexes[2];
        int kstride = stride[2];


        for(int i = 0 ; i <  istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            for(int k = 0 ; k < kstride ; k++)
            {
              HCPoint* point = edge->point(kstart + k);
              point->data[0] = dataC[i + j*istride + k*istride*jstride];
            }
          }
        }
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::getValue(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex, QVariant &data) const
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[patchDimensionIndex];
        data = QVariant::fromValue(patch->data[0]);
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[patchDimensionIndex];
        HCEdge *edge = patch->getEdge(edgeDimensionIndex);
        data = QVariant::fromValue(edge->data[0]);
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[patchDimensionIndex];
        HCEdge *edge = patch->getEdge(edgeDimensionIndex);
        data = QVariant::fromValue(edge->point(nodeDimensionIndex)->data[0]);
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::getValues(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                                                int patchStride, int edgeStride, int nodeStride, QVariant data[]) const
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[i+istart];
          data[i-istart] = QVariant::fromValue(patch->data[0]);
        }
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;
        int jstart = edgeDimensionIndex;
        int jstride = edgeStride;

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            data[i + j*istride] = QVariant::fromValue(edge->data[0]);
          }
        }
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;
        int jstart = edgeDimensionIndex;
        int jstride = edgeStride;
        int kstart = nodeDimensionIndex;
        int kstride = nodeStride;


        for(int i = 0 ; i <  istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            for(int k = 0 ; k < kstride ; k++)
            {
              HCPoint* point = edge->point(kstart + k);
              data[i + j*istride + k*istride*jstride] = QVariant::fromValue(point->data[0]);
            }
          }
        }
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::getValues(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                                                int patchStride, int edgeStride, int nodeStride, void *data) const
{
  double* dataC = (double*) data;

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[i+istart];
          dataC[i] = patch->data[0];
        }
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;
        int jstart = edgeDimensionIndex;
        int jstride = edgeStride;

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            dataC[i + j*istride] = edge->data[0];
          }
        }
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;
        int jstart = edgeDimensionIndex;
        int jstride = edgeStride;
        int kstart = nodeDimensionIndex;
        int kstride = nodeStride;


        for(int i = 0 ; i <  istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            for(int k = 0 ; k < kstride ; k++)
            {
              HCPoint* point = edge->point(kstart + k);
              dataC[i + j*istride + k*istride*jstride] = point->data[0];
            }
          }
        }
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::setValue(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                                               const QVariant &data)
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[patchDimensionIndex];
        patch->data[0] = data.toDouble();
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[patchDimensionIndex];
        HCEdge *edge = patch->getEdge(edgeDimensionIndex);
        edge->data[0] = data.toDouble();
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        HCPolygon *patch = m_polyhedralSurface->patches()[patchDimensionIndex];
        HCEdge *edge = patch->getEdge(edgeDimensionIndex);
        edge->point(nodeDimensionIndex)->data[0] = data.toDouble();
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::setValues(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                                                int patchStride, int edgeStride, int nodeStride, const QVariant data[])
{
  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[i+istart];
          patch->data[0] = data[i].toDouble();
        }
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;
        int jstart = edgeDimensionIndex;
        int jstride = edgeStride;

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            edge->data[0] = data[i + j*istride].toDouble();
          }
        }
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;
        int jstart = edgeDimensionIndex;
        int jstride = edgeStride;
        int kstart = nodeDimensionIndex ;
        int kstride = nodeStride;


        for(int i = 0 ; i <  istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            for(int k = 0 ; k < kstride ; k++)
            {
              HCPoint* point = edge->point(kstart + k);
              point->data[0] = data[i + j*istride + k*istride*jstride].toDouble() ;
            }
          }
        }
      }
      break;
  }
}

void HCPolyhedralSurfaceOutputDouble::setValues(int patchDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex,
                                                int patchStride, int edgeStride, int nodeStride, const void *data)
{
  double * dataC = (double*) data;

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Patch:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[i+istart];
          patch->data[0] = dataC[i];
        }
      }
      break;
    case HydroCouple::Spatial::Edge:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;
        int jstart = edgeDimensionIndex;
        int jstride = edgeStride;

        for(int i = 0 ; i < istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            edge->data[0] = dataC[i + j*istride];
          }
        }
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int istart = patchDimensionIndex;
        int istride = patchStride;
        int jstart = edgeDimensionIndex;
        int jstride = edgeStride;
        int kstart = nodeDimensionIndex;
        int kstride = nodeStride;


        for(int i = 0 ; i <  istride ; i++)
        {
          HCPolygon *patch = m_polyhedralSurface->patches()[istart + i];

          for(int j = 0 ; j < jstride ; j++)
          {
            HCEdge *edge = patch->getEdge(jstart + j);

            for(int k = 0 ; k < kstride ; k++)
            {
              HCPoint* point = edge->point(kstart + k);
              point->data[0] = dataC[i + j*istride + k*istride*jstride];
            }
          }
        }
      }
      break;
  }
}
