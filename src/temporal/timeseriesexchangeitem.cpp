#include "stdafx.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "temporal/timedata.h"
#include "temporal/timeseriesexchangeitem.h"
#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Temporal;

TimeSeriesOutputDouble::TimeSeriesOutputDouble(const QString& id,
                                               const QList<SDKTemporal::Time*>& times,
                                               Dimension* timeDimension,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent* modelComponent)
  : AbstractOutput(id, QList<Dimension*>({timeDimension}), valueDefinition, modelComponent),
    TimeSeriesComponentDataItem<double>(times, valueDefinition->defaultValue().toDouble()),
    m_timeDimension(timeDimension)
{
}

TimeSeriesOutputDouble::~TimeSeriesOutputDouble()
{
}


QList<ITime*> TimeSeriesOutputDouble::times() const
{
  QList<ITime*> otimes;
  QList<SDKTemporal::Time*> itimes = TimeSeriesComponentDataItem<double>::timesInternal();

  for(SDKTemporal::Time* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

ITimeSpan* TimeSeriesOutputDouble::timeSpan() const
{
  return TimeSeriesComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesOutputDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesOutputDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void TimeSeriesOutputDouble::getValue(int dimensionIndexes[], QVariant &data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesOutputDouble::getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesOutputDouble::getValues(int dimensionIndexes[], int stride[],  void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesOutputDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesOutputDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesOutputDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesOutputDouble::getValue(int timeIndex, QVariant &data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(timeIndex,data);
}

void TimeSeriesOutputDouble::getValues(int timeIndex, int stride, QVariant data[]) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesOutputDouble::getValues(int timeIndex, int stride, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesOutputDouble::setValue(int timeIndex, const QVariant &data)
{
  TimeSeriesComponentDataItem<double>::setValueT(timeIndex,data);
}

void TimeSeriesOutputDouble::setValues(int timeIndex, int stride, const QVariant data[])
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex,stride,data);
}

void TimeSeriesOutputDouble::setValues(int timeIndex, int stride, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex,stride,data);
}

//==============================================================================================================================

TimeSeriesInputDouble::TimeSeriesInputDouble(const QString& id,
                                               const QList<SDKTemporal::Time*>& times,
                                               Dimension* timeDimension,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent* modelComponent)
  : AbstractInput(id, QList<Dimension*>({timeDimension}), valueDefinition, modelComponent),
    TimeSeriesComponentDataItem<double>(times, valueDefinition->defaultValue().toDouble()),
    m_timeDimension(timeDimension)
{
}

TimeSeriesInputDouble::~TimeSeriesInputDouble()
{
}


QList<ITime*> TimeSeriesInputDouble::times() const
{
  QList<ITime*> otimes;
  QList<SDKTemporal::Time*> itimes = TimeSeriesComponentDataItem<double>::timesInternal();

  for(SDKTemporal::Time* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

ITimeSpan* TimeSeriesInputDouble::timeSpan() const
{
  return TimeSeriesComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesInputDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesInputDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void TimeSeriesInputDouble::getValue(int dimensionIndexes[], QVariant &data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesInputDouble::getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesInputDouble::getValues(int dimensionIndexes[], int stride[],  void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesInputDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesInputDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesInputDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesInputDouble::getValue(int timeIndex, QVariant &data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(timeIndex,data);
}

void TimeSeriesInputDouble::getValues(int timeIndex, int stride, QVariant data[]) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesInputDouble::getValues(int timeIndex, int stride, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesInputDouble::setValue(int timeIndex, const QVariant &data)
{
  TimeSeriesComponentDataItem<double>::setValueT(timeIndex,data);
}

void TimeSeriesInputDouble::setValues(int timeIndex, int stride, const QVariant data[])
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex,stride,data);
}

void TimeSeriesInputDouble::setValues(int timeIndex, int stride, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex,stride,data);
}

//==============================================================================================================================

TimeSeriesMultiInputDouble::TimeSeriesMultiInputDouble(const QString& id,
                                               const QList<SDKTemporal::Time*>& times,
                                               Dimension* timeDimension,
                                               ValueDefinition* valueDefinition,
                                               AbstractModelComponent* modelComponent)
  : AbstractMultiInput(id, QList<Dimension*>({timeDimension}), valueDefinition, modelComponent),
    TimeSeriesComponentDataItem<double>(times, valueDefinition->defaultValue().toDouble())
{
}

TimeSeriesMultiInputDouble::~TimeSeriesMultiInputDouble()
{
}


QList<ITime*> TimeSeriesMultiInputDouble::times() const
{
  QList<ITime*> otimes;
  QList<SDKTemporal::Time*> itimes = TimeSeriesComponentDataItem<double>::timesInternal();

  for(SDKTemporal::Time* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

ITimeSpan* TimeSeriesMultiInputDouble::timeSpan() const
{
  return TimeSeriesComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesMultiInputDouble::timeDimension() const
{
  return m_timeDimension;
}

int TimeSeriesMultiInputDouble::dimensionLength(int dimensionIndexes[], int dimensionIndexesLength) const
{
  assert(dimensionIndexesLength == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void TimeSeriesMultiInputDouble::getValue(int dimensionIndexes[], QVariant &data) const
{
  ComponentDataItem1D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesMultiInputDouble::getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesMultiInputDouble::getValues(int dimensionIndexes[], int stride[],  void *data) const
{
  ComponentDataItem1D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesMultiInputDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem1D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesMultiInputDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesMultiInputDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem1D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesMultiInputDouble::getValue(int timeIndex, QVariant &data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(timeIndex,data);
}

void TimeSeriesMultiInputDouble::getValues(int timeIndex, int stride, QVariant data[]) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesMultiInputDouble::getValues(int timeIndex, int stride, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesMultiInputDouble::setValue(int timeIndex, const QVariant &data)
{
  TimeSeriesComponentDataItem<double>::setValueT(timeIndex,data);
}

void TimeSeriesMultiInputDouble::setValues(int timeIndex, int stride, const QVariant data[])
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex,stride,data);
}

void TimeSeriesMultiInputDouble::setValues(int timeIndex, int stride, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex,stride,data);
}

template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<QString>;
