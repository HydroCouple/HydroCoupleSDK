/*!
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
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
#include "spatial/tinargument.h"
#include "spatial/polyhedralsurface.h"
#include "core/dimension.h"
#include "hydrocouple.h"
#include "core/valuedefinition.h"
#include "core/abstractmodelcomponent.h"
#include "spatial/geometryfactory.h"
#include "temporal/timedata.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;


TINArgumentDouble::TINArgumentDouble(const QString &id,
                                     MeshDataType dataType,
                                     Dimension *patchDimension,
                                     Dimension *patchEdgeDimension,
                                     Dimension *edgeNodeDimension,
                                     ValueDefinition *valueDefinition,
                                     AbstractModelComponent *modelComponent):
  AbstractArgument(id,QList<Dimension*>({patchDimension,patchEdgeDimension,edgeNodeDimension}),
                   valueDefinition,modelComponent),
  m_dataType(dataType),
  m_cellDimension(patchDimension),
  m_edgeDimension(patchEdgeDimension),
  m_nodeDimension(edgeNodeDimension),
  m_TINSurface(nullptr)
{
  setCanReadFromFile(true);
  setCanReadFromString(false);
  setArgumentIOType(IArgument::ArgumentIOType::File);
  setIsReadOnly(true);
  addFileFilter("NetCDF (*.nc *.netcdf)");
}

TINArgumentDouble::~TINArgumentDouble()
{
}

ITIN *TINArgumentDouble::TIN() const
{
  return m_TINSurface.data();
}

HCTIN *TINArgumentDouble::TINInternal() const
{
  return m_TINSurface.data();
}

MeshDataType TINArgumentDouble::meshDataType() const
{
  return m_dataType;
}

IPolyhedralSurface *TINArgumentDouble::polyhedralSurface() const
{
  return m_TINSurface.data();
}

IDimension *TINArgumentDouble::cellDimension() const
{
  return m_cellDimension;
}

IDimension *TINArgumentDouble::edgeDimension() const
{
  return m_edgeDimension;
}

IDimension *TINArgumentDouble::nodeDimension() const
{
  return m_nodeDimension;
}

QSharedPointer<HCTIN> TINArgumentDouble::sharedTIN() const
{
  return m_TINSurface;
}

int TINArgumentDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
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
    default:
      {
        assert((int)dimensionIndexes.size() == 2);
        return 2;
      }
      break;
  }
}

void TINArgumentDouble::getValue(const std::vector<int> &dimensionIndexes,  void *data) const
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
        int index = dimensionIndexes[0] + pindex * m_TINSurface->patchCount();
        *dataC = m_data[index];
      }
      break;
  }
}

void TINArgumentDouble::getValue(int cellDimensionIndex, int faceDimensionIndex, int nodeDimensionIndex, void *data) const
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
        int index = cellDimensionIndex + pindex * m_TINSurface->patchCount();
        *dataC = m_data[index];
      }
      break;
  }
}

void TINArgumentDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
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
        m_data[i + j * m_TINSurface->patchCount()] = *dataC;
      }
      break;
  }
}

void TINArgumentDouble::setValue(int cellDimensionIndex, int faceDimensionIndex, int nodeDimensionIndex,  const void *data)
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

void TINArgumentDouble::readData(QXmlStreamReader &xmlReader)
{

}

void TINArgumentDouble::writeData(QXmlStreamWriter &xmlWriter) const
{

}

void TINArgumentDouble::saveData()
{

}

QString TINArgumentDouble::toString() const
{
  if(m_netCDFFile.exists() && m_netCDFFile.isFile())
  {
    return modelComponentInternal()->getRelativeFilePath(m_netCDFFile.absoluteFilePath()).filePath();
  }

  return "";
}

bool TINArgumentDouble::readValues(const QString &value, QString &message, bool isFile)
{
  if(isFile && !value.isEmpty() && !value.isNull())
  {
    QFileInfo tempF = modelComponentInternal()->getAbsoluteFilePath(value);

    if(tempF.exists() && tempF.isFile())
    {
      HCTIN* tin = GeometryFactory::readTINFromNetCDF(tempF.absoluteFilePath() , message);

      if(tin != nullptr)
      {
        m_TINSurface = QSharedPointer<HCTIN>(tin);
        m_netCDFFile = tempF;
        resetDataArray();
        return true;
      }
    }
  }

  m_TINSurface.reset();

  return false;
}

bool TINArgumentDouble::readValues(const IComponentDataItem *componentDataItem, QString &message)
{
  message = "";

  return false;
}

void TINArgumentDouble::resetDataArray()
{
  if(m_TINSurface)
  {
    switch(m_dataType)
    {
      case HydroCouple::Spatial::Centroid:
        {
          m_data.resize(m_TINSurface->patchCount());
        }
        break;
      case HydroCouple::Spatial::Face:
        {
          m_data.resize(m_TINSurface->patchCount() * 3);
        }
      case HydroCouple::Spatial::Node:
        {
          m_data.resize(m_TINSurface->patchCount() * 4);
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
