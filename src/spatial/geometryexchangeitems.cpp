#include "stdafx.h"
#include "spatial/geometryexchangeitems.h"
#include "spatial/geometrycollection.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/abstractmodelcomponent.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

HCGeometryInputDouble::HCGeometryInputDouble(const QString &id,
                                             GeometryType geometryType,
                                             Dimension *geometryDimension,
                                             ValueDefinition *valueDefinition,
                                             AbstractModelComponent *modelComponent)
  : AbstractInput(id, QList<Dimension*>({geometryDimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<double>(0, valueDefinition->defaultValue().toDouble()),
    m_geometryType(geometryType)
{
}

HCGeometryInputDouble::~HCGeometryInputDouble()
{
}

GeometryType HCGeometryInputDouble::geometryType() const
{
  return m_geometryType;
}

int HCGeometryInputDouble::geometryCount() const
{
  return m_geometries.length();
}

IGeometry *HCGeometryInputDouble::geometry(int geometryIndex) const
{
  return m_geometries[geometryIndex];
}

IDimension *HCGeometryInputDouble::geometryDimension() const
{
  return dimensionsInternal()[0];
}

void HCGeometryInputDouble::addGeometry(HCGeometry *geometry)
{
  m_geometries.append(geometry);
  resetDataArray(m_geometries.length());
}

void HCGeometryInputDouble::addGeometries(QList<HCGeometry*> geometries)
{
  for(HCGeometry *geometry :geometries)
  {
    assert(geometry->geometryType() == geometryType());
    m_geometries.append(geometry);
  }

  resetDataArray(m_geometries.length());
}


bool HCGeometryInputDouble::removeGeometry(HCGeometry *geometry)
{
  bool removed = m_geometries.removeOne(geometry);

  if(removed)
  {
    resetDataArray(m_geometries.length());
  }
}

int HCGeometryInputDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  return length();
}

void HCGeometryInputDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void HCGeometryInputDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void HCGeometryInputDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void HCGeometryInputDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void HCGeometryInputDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void HCGeometryInputDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void HCGeometryInputDouble::getValue(int geometryDimensionIndex, QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(&geometryDimensionIndex,data);
}

void HCGeometryInputDouble::getValues(int geometryDimensionIndex, int stride, QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(&geometryDimensionIndex,&stride,data);
}

void HCGeometryInputDouble::getValues(int geometryDimensionIndex, int stride, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(&geometryDimensionIndex,&stride,data);
}

void HCGeometryInputDouble::setValue(int geometryDimensionIndex, const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(&geometryDimensionIndex,data);
}

void HCGeometryInputDouble::setValues(int geometryDimensionIndex, int stride, const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(&geometryDimensionIndex,&stride,data);
}

void HCGeometryInputDouble::setValues(int geometryDimensionIndex, int stride, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(&geometryDimensionIndex,&stride,data);
}

QList<HCGeometry*> HCGeometryInputDouble::geometries() const
{
  return m_geometries;
}

//==============================================================================================================================

HCGeometryOutputDouble::HCGeometryOutputDouble(const QString &id,
                                               GeometryType geometryType,
                                               Dimension *geometryDimension,
                                               ValueDefinition *valueDefinition,
                                               AbstractModelComponent *modelComponent)
  : AbstractOutput(id, QList<Dimension*>({geometryDimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<double>(0, valueDefinition->defaultValue().toDouble()),
    m_geometryType(geometryType)
{
}

HCGeometryOutputDouble::~HCGeometryOutputDouble()
{
}

GeometryType HCGeometryOutputDouble::geometryType() const
{
  return m_geometryType;
}

int HCGeometryOutputDouble::geometryCount() const
{
  return m_geometries.length();
}

IGeometry *HCGeometryOutputDouble::geometry(int geometryIndex) const
{
  return m_geometries[geometryIndex];
}

IDimension *HCGeometryOutputDouble::geometryDimension() const
{
  return dimensionsInternal()[0];
}

void HCGeometryOutputDouble::addGeometry(HCGeometry *geometry)
{
  assert(geometry->geometryType() == geometryType());
  m_geometries.append(geometry);
  resetDataArray(m_geometries.length());
}

void HCGeometryOutputDouble::addGeometries(QList<HCGeometry*> geometries)
{
  for(HCGeometry *geometry :geometries)
  {
    assert(geometry->geometryType() == geometryType());
    m_geometries.append(geometry);
  }

  resetDataArray(m_geometries.length());
}

bool HCGeometryOutputDouble::removeGeometry(HCGeometry *geometry)
{
  bool removed = m_geometries.removeOne(geometry);

  if(removed)
  {
    resetDataArray(m_geometries.length());
  }
}

int HCGeometryOutputDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  return length();
}

void HCGeometryOutputDouble::getValue(int dimensionIndexes[], QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void HCGeometryOutputDouble::getValues(int dimensionIndexes[], int stride[], QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void HCGeometryOutputDouble::getValues(int dimensionIndexes[], int stride[], void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void HCGeometryOutputDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void HCGeometryOutputDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void HCGeometryOutputDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void HCGeometryOutputDouble::getValue(int geometryDimensionIndex, QVariant & data) const
{
  ComponentDataItem1D<double>::getValueT(&geometryDimensionIndex,data);
}

void HCGeometryOutputDouble::getValues(int geometryDimensionIndex, int stride, QVariant* data) const
{
  ComponentDataItem1D<double>::getValuesT(&geometryDimensionIndex,&stride,data);
}

void HCGeometryOutputDouble::getValues(int geometryDimensionIndex, int stride, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(&geometryDimensionIndex,&stride,data);
}

void HCGeometryOutputDouble::setValue(int geometryDimensionIndex, const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(&geometryDimensionIndex,data);
}

void HCGeometryOutputDouble::setValues(int geometryDimensionIndex, int stride, const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(&geometryDimensionIndex,&stride,data);
}

void HCGeometryOutputDouble::setValues(int geometryDimensionIndex, int stride, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(&geometryDimensionIndex,&stride,data);
}

QList<HCGeometry*> HCGeometryOutputDouble::geometries() const
{
  return m_geometries;
}
