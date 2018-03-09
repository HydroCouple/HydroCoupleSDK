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
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "temporal/timedata.h"
#include "spatial/geometry.h"
#include "spatial/envelope.h"
#include "spatiotemporal/timegeometryoutput.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::Temporal;

TimeGeometryOutputDouble::TimeGeometryOutputDouble(const QString &id,
                                                 IGeometry::GeometryType geometryType,
                                                 Dimension *timeDimension,
                                                 Dimension *geometryDimension,
                                                 ValueDefinition *valueDefinition,
                                                 AbstractModelComponent *modelComponent)
  :AbstractOutput(id , QList<Dimension*>({timeDimension,geometryDimension}), valueDefinition, modelComponent),
   TimeGeometryComponentDataItem(id, geometryType, valueDefinition->defaultValue().toDouble()),
   m_timeDimension(timeDimension),
   m_geometryDimension(geometryDimension)
{

}

TimeGeometryOutputDouble::~TimeGeometryOutputDouble()
{

}

QList<IDateTime*> TimeGeometryOutputDouble::times() const
{
  QList<IDateTime*> otimes;
  std::vector<SDKTemporal::DateTime*> itimes = timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeGeometryOutputDouble::time(int timeIndex) const
{
  return m_times[timeIndex];
}

int TimeGeometryOutputDouble::timeCount() const
{
  return m_times.size();
}

ITimeSpan *TimeGeometryOutputDouble::timeSpan() const
{
  return timeSpanInternal();
}

IDimension *TimeGeometryOutputDouble::timeDimension() const
{
  return m_timeDimension;
}

IGeometry::GeometryType TimeGeometryOutputDouble::geometryType() const
{
  return getGeometryType();
}

int TimeGeometryOutputDouble::geometryCount() const
{
  return m_geometries.size();
}

HydroCouple::Spatial::IGeometry *TimeGeometryOutputDouble::geometry(int geometryIndex) const
{
  return m_geometries[geometryIndex].data();
}

IDimension *TimeGeometryOutputDouble::geometryDimension() const
{
  return m_geometryDimension;
}

IEnvelope *TimeGeometryOutputDouble::envelope() const
{
  return envelopeInternal();
}

int TimeGeometryOutputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  if(dimensionIndexes.size() == 0)
    return m_times.size();

  return m_geometries.size();
}

void TimeGeometryOutputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  TimeGeometryComponentDataItem<double>::getValuesT(dimensionIndexes[0],dimensionIndexes[1],1,1,data);
}

void TimeGeometryOutputDouble::getValue(int timeIndex, int geometryIndex, void *data) const
{
  TimeGeometryComponentDataItem<double>::getValuesT(timeIndex,geometryIndex,1,1,data);
}

void TimeGeometryOutputDouble::getValues(int timeIndex, int geometryIndex,
                                           int timeStride, int geomStride, void *data) const
{
  TimeGeometryComponentDataItem<double>::getValuesT(timeIndex, geometryIndex,timeStride, geomStride,data);
}

void TimeGeometryOutputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  TimeGeometryComponentDataItem<double>::setValuesT(dimensionIndexes[0],dimensionIndexes[1],1,1,data);
}

void TimeGeometryOutputDouble::setValue(int timeIndex, int geometryIndex, const void *data)
{
  TimeGeometryComponentDataItem<double>::setValuesT(timeIndex,geometryIndex,1,1,data);
}

void TimeGeometryOutputDouble::setValues(int timeIndex, int geometryIndex,
                                           int timeStride, int geomStride, const void *data)
{
  TimeGeometryComponentDataItem<double>::setValuesT(timeIndex,geometryIndex,timeStride,geomStride,data);
}

