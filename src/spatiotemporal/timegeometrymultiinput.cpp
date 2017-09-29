#include "stdafx.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "temporal/timedata.h"
#include "spatial/geometry.h"
#include "spatial/envelope.h"
#include "spatiotemporal/timegeometrymultiinput.h"

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::Temporal;

TimeGeometryMultiInputDouble::TimeGeometryMultiInputDouble(const QString &id,
                                                           IGeometry::GeometryType geometryType,
                                                           Dimension *timeDimension,
                                                           Dimension *geometryDimension,
                                                           ValueDefinition *valueDefinition,
                                                           AbstractModelComponent *modelComponent)
  :AbstractMultiInput(id , QList<Dimension*>({timeDimension,geometryDimension}), valueDefinition, modelComponent),
    TimeGeometryComponentDataItem(id, geometryType, valueDefinition->defaultValue().toDouble()),
    m_timeDimension(timeDimension),
    m_geometryDimension(geometryDimension)
{

}

TimeGeometryMultiInputDouble::~TimeGeometryMultiInputDouble()
{

}

QList<IDateTime*> TimeGeometryMultiInputDouble::times() const
{
  QList<IDateTime*> otimes;
  std::vector<SDKTemporal::DateTime*> itimes = timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeGeometryMultiInputDouble::time(int timeIndex) const
{
  return m_times[timeIndex];
}

int TimeGeometryMultiInputDouble::timeCount() const
{
  return m_times.size();
}

ITimeSpan *TimeGeometryMultiInputDouble::timeSpan() const
{
  return timeSpanInternal();
}

IDimension *TimeGeometryMultiInputDouble::timeDimension() const
{
  return m_timeDimension;
}

IGeometry::GeometryType TimeGeometryMultiInputDouble::geometryType() const
{
  return getGeometryType();
}

int TimeGeometryMultiInputDouble::geometryCount() const
{
  return m_geometries.size();
}

HydroCouple::Spatial::IGeometry *TimeGeometryMultiInputDouble::geometry(int geometryIndex) const
{
  return m_geometries[geometryIndex].data();
}

IDimension *TimeGeometryMultiInputDouble::geometryDimension() const
{
  return m_geometryDimension;
}

IEnvelope *TimeGeometryMultiInputDouble::envelope() const
{
  return envelopeInternal();
}

int TimeGeometryMultiInputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  if(dimensionIndexes.size() == 0)
    return m_times.size();

  return m_geometries.size();
}

void TimeGeometryMultiInputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  TimeGeometryComponentDataItem<double>::getValuesT(dimensionIndexes[0],dimensionIndexes[1],1,1,data);
}

void TimeGeometryMultiInputDouble::getValue(int timeIndex, int geometryIndex, void *data) const
{
  TimeGeometryComponentDataItem<double>::getValuesT(timeIndex,geometryIndex,1,1,data);
}

void TimeGeometryMultiInputDouble::getValues(int timeIndex, int geometryIndex,
                                             int timeStride, int geomStride, void *data) const
{
  TimeGeometryComponentDataItem<double>::getValuesT(timeIndex, geometryIndex,timeStride, geomStride,data);
}

void TimeGeometryMultiInputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  TimeGeometryComponentDataItem<double>::setValuesT(dimensionIndexes[0],dimensionIndexes[1],1,1,data);
}

void TimeGeometryMultiInputDouble::setValue(int timeIndex, int geometryIndex, const void *data)
{
  TimeGeometryComponentDataItem<double>::setValuesT(timeIndex,geometryIndex,1,1,data);
}

void TimeGeometryMultiInputDouble::setValues(int timeIndex, int geometryIndex,
                                             int timeStride, int geomStride, const void *data)
{
  TimeGeometryComponentDataItem<double>::setValuesT(timeIndex,geometryIndex,timeStride,geomStride,data);
}
