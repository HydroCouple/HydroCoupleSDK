/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */


#include "stdafx.h"
#include "spatial/geometryexchangeitems.h"
#include "spatial/geometrycollection.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "core/abstractmodelcomponent.h"
#include "spatial/envelope.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;

GeometryInputDouble::GeometryInputDouble(const QString &id,
                                         IGeometry::GeometryType geometryType,
                                         Dimension *geometryDimension,
                                         ValueDefinition *valueDefinition,
                                         AbstractModelComponent *modelComponent)
  : AbstractInput(id, QList<Dimension*>({geometryDimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<double>(id, 0, valueDefinition->defaultValue().toDouble()),
    m_geometryType(geometryType),
    m_envelope(nullptr)
{
  m_envelope = new Envelope();
}

GeometryInputDouble::~GeometryInputDouble()
{
  delete m_envelope;
  m_geometries.clear();
}

IGeometry::GeometryType GeometryInputDouble::geometryType() const
{
  return m_geometryType;
}

int GeometryInputDouble::geometryCount() const
{
  return m_geometries.length();
}

IGeometry *GeometryInputDouble::geometry(int geometryIndex) const
{
  return m_geometries[geometryIndex].data();
}

IDimension *GeometryInputDouble::geometryDimension() const
{
  return dimensionsInternal()[0];
}

IEnvelope *GeometryInputDouble::envelope() const
{
  return m_envelope;
}

void GeometryInputDouble::addGeometry(const QSharedPointer<HCGeometry> &geometry)
{
  m_geometries.append(geometry);
  resizeDataArray(m_geometries.length());
  emit propertyChanged("Geometries");
}

void GeometryInputDouble::addGeometries(const QList<QSharedPointer<HCGeometry>> & geometries)
{
  for(QSharedPointer<HCGeometry> geometry :geometries)
  {
    assert(geometry->geometryType() == geometryType());
    m_geometries.append(geometry);
  }

  resizeDataArray(m_geometries.length());
  emit propertyChanged("Geometries");
}


bool GeometryInputDouble::removeGeometry(const QSharedPointer<HCGeometry> &geometry)
{
  int index = m_geometries.indexOf(geometry);

  if(index >= 0)
  {
    m_geometries.removeAt(index);
    removeItemAt(index);
    emit propertyChanged("Geometries");

    return true;

  }

  return false;
}

int GeometryInputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert((int)dimensionIndexes.size() == 0);

  return length();
}

void GeometryInputDouble::getValue(const std::vector<int> &dimensionIndexes,  void *data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void GeometryInputDouble::getValue(int geometryDimensionIndex, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(geometryDimensionIndex,1,data);
}

void GeometryInputDouble::getValues(int geometryDimensionIndex, int stride,  void *data) const
{
  ComponentDataItem1D<double>::getValuesT(geometryDimensionIndex,stride,data);
}

void GeometryInputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void GeometryInputDouble::setValue(int geometryDimensionIndex, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(geometryDimensionIndex,1,data);
}

void GeometryInputDouble::setValues(int geometryDimensionIndex , int stride, const void* data)
{
  ComponentDataItem1D<double>::setValuesT(geometryDimensionIndex,stride,data);
}

QList<QSharedPointer<HCGeometry>> GeometryInputDouble::geometries() const
{
  return m_geometries;
}

//==============================================================================================================================

GeometryOutputDouble::GeometryOutputDouble(const QString &id,
                                           IGeometry::GeometryType geometryType,
                                           Dimension *geometryDimension,
                                           ValueDefinition *valueDefinition,
                                           AbstractModelComponent *modelComponent)
  : AbstractOutput(id, QList<Dimension*>({geometryDimension}),valueDefinition,modelComponent),
    ComponentDataItem1D<double>(id, 0, valueDefinition->defaultValue().toDouble()),
    m_geometryType(geometryType),
    m_envelope(nullptr)
{
  m_envelope = new Envelope();
}

GeometryOutputDouble::~GeometryOutputDouble()
{
  delete m_envelope;
  m_geometries.clear();
}

IGeometry::GeometryType GeometryOutputDouble::geometryType() const
{
  return m_geometryType;
}

int GeometryOutputDouble::geometryCount() const
{
  return m_geometries.length();
}

IGeometry *GeometryOutputDouble::geometry(int geometryIndex) const
{
  return m_geometries[geometryIndex].data();
}

IDimension *GeometryOutputDouble::geometryDimension() const
{
  return dimensionsInternal()[0];
}

IEnvelope *GeometryOutputDouble::envelope() const
{
  return m_envelope;
}

void GeometryOutputDouble::addGeometry(const QSharedPointer<HCGeometry> &geometry)
{
  assert(geometry->geometryType() == geometryType());
  m_geometries.append(geometry);
  resizeDataArray(m_geometries.length());
  emit propertyChanged("Geometries");
}

void GeometryOutputDouble::addGeometries(const QList<QSharedPointer<HCGeometry>> &geometries)
{
  for(QSharedPointer<HCGeometry> geometry :geometries)
  {
    assert(geometry->geometryType() == geometryType());
    m_geometries.append(geometry);
  }

  resizeDataArray(m_geometries.length());
  emit propertyChanged("Geometries");
}

bool GeometryOutputDouble::removeGeometry(const QSharedPointer<HCGeometry> &geometry)
{
  int index = m_geometries.indexOf(geometry);

  if(index >= 0)
  {
    m_geometries.removeAt(index);
    removeItemAt(index);
    emit propertyChanged("Geometries");
    return true;
  }

  return false;
}

int GeometryOutputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert((int)dimensionIndexes.size() == 0);

  return length();
}

void GeometryOutputDouble::getValue(const std::vector<int> &dimensionIndexes,  void *data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void GeometryOutputDouble::getValue(int geometryDimensionIndex, void *data) const
{
  ComponentDataItem1D<double>::getValuesT(geometryDimensionIndex,1,data);
}

void GeometryOutputDouble::getValues(int geometryDimensionIndex, int stride,  void *data) const
{
  ComponentDataItem1D<double>::getValuesT(geometryDimensionIndex,stride,data);
}

void GeometryOutputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void GeometryOutputDouble::setValue(int geometryDimensionIndex, const void *data)
{
  ComponentDataItem1D<double>::setValuesT(geometryDimensionIndex,1,data);
}

void GeometryOutputDouble::setValues(int geometryDimensionIndex , int stride, const void* data)
{
  ComponentDataItem1D<double>::setValuesT(geometryDimensionIndex,stride,data);
}

QList<QSharedPointer<HCGeometry>> GeometryOutputDouble::geometries() const
{
  return m_geometries;
}

void GeometryOutputDouble::updateValues()
{

}
