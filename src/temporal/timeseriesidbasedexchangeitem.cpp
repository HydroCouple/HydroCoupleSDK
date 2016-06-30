#include "stdafx.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "temporal/timeseriesidbasedexchangeitem.h"
#include "temporal/timedata.h"

using namespace HydroCouple;
using namespace HydroCouple::Temporal;

TimeSeriesIdBasedOutputDouble::TimeSeriesIdBasedOutputDouble(const QString& id,
                                                             const QStringList& identifiers,
                                                             Dimension* identifierDimension,
                                                             const QList<SDKTemporal::Time*>& times,
                                                             Dimension* timeDimension,
                                                             ValueDefinition* valueDefinition,
                                                             AbstractModelComponent* modelComponent)
  : AbstractOutput(id,QList<Dimension*>({identifierDimension, timeDimension}),valueDefinition, modelComponent),
    TimeSeriesIdBasedComponentDataItem<double>(identifiers, identifierDimension, times, timeDimension,valueDefinition->defaultValue().toDouble())
{

}

TimeSeriesIdBasedOutputDouble::~TimeSeriesIdBasedOutputDouble()
{
}


QStringList TimeSeriesIdBasedOutputDouble::identifiers() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::identifiersInternal();
}

IDimension* TimeSeriesIdBasedOutputDouble::identifierDimension() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::identifierDimensionInternal();
}

QList<ITime*> TimeSeriesIdBasedOutputDouble::times() const
{
  QList<ITime*> otimes;
  QList<SDKTemporal::Time*> itimes = TimeSeriesIdBasedComponentDataItem<double>::timesInternal();

  for(SDKTemporal::Time* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

ITimeSpan* TimeSeriesIdBasedOutputDouble::timeSpan() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesIdBasedOutputDouble::timeDimension() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::timeDimensionInternal();
}

void TimeSeriesIdBasedOutputDouble::getValue(int dimensionIndexes[], QVariant &data) const
{
  ComponentDataItem2D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedOutputDouble::getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedOutputDouble::getValues(int dimensionIndexes[], int stride[],  void *data) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedOutputDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem2D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedOutputDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedOutputDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedOutputDouble::getValue(int timeIndex, int idIndex, QVariant &data) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedOutputDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride, QVariant data[]) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedOutputDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride, void *data) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedOutputDouble::setValue(int timeIndex, int idIndex, const QVariant &data)
{
  TimeSeriesIdBasedComponentDataItem<double>::setValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedOutputDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const QVariant data[])
{
  TimeSeriesIdBasedComponentDataItem<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedOutputDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data)
{
  TimeSeriesIdBasedComponentDataItem<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}


//==============================================================================================================================

TimeSeriesIdBasedInputDouble::TimeSeriesIdBasedInputDouble(const QString& id,
                                                           const QStringList& identifiers,
                                                           Dimension* identifierDimension,
                                                           const QList<SDKTemporal::Time*>& times,
                                                           Dimension* timeDimension,
                                                           ValueDefinition* valueDefinition,
                                                           AbstractModelComponent* modelComponent)
  : AbstractInput(id,QList<Dimension*>({identifierDimension, timeDimension}),valueDefinition, modelComponent),
    TimeSeriesIdBasedComponentDataItem<double>(identifiers, identifierDimension, times, timeDimension,valueDefinition->defaultValue().toDouble())
{

}

TimeSeriesIdBasedInputDouble::~TimeSeriesIdBasedInputDouble()
{
}


QStringList TimeSeriesIdBasedInputDouble::identifiers() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::identifiersInternal();
}

IDimension* TimeSeriesIdBasedInputDouble::identifierDimension() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::identifierDimensionInternal();
}

QList<ITime*> TimeSeriesIdBasedInputDouble::times() const
{
  QList<ITime*> otimes;
  QList<SDKTemporal::Time*> itimes = TimeSeriesIdBasedComponentDataItem<double>::timesInternal();

  for(SDKTemporal::Time* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

ITimeSpan* TimeSeriesIdBasedInputDouble::timeSpan() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesIdBasedInputDouble::timeDimension() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::timeDimensionInternal();
}

void TimeSeriesIdBasedInputDouble::getValue(int dimensionIndexes[], QVariant &data) const
{
  ComponentDataItem2D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedInputDouble::getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedInputDouble::getValues(int dimensionIndexes[], int stride[],  void *data) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedInputDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem2D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedInputDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedInputDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedInputDouble::getValue(int timeIndex, int idIndex, QVariant &data) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedInputDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride, QVariant data[]) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedInputDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride, void *data) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedInputDouble::setValue(int timeIndex, int idIndex, const QVariant &data)
{
  TimeSeriesIdBasedComponentDataItem<double>::setValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedInputDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const QVariant data[])
{
  TimeSeriesIdBasedComponentDataItem<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedInputDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data)
{
  TimeSeriesIdBasedComponentDataItem<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

//==============================================================================================================================

TimeSeriesIdBasedMultiInputDouble::TimeSeriesIdBasedMultiInputDouble(const QString& id,
                                                                     const QStringList& identifiers,
                                                                     Dimension* identifierDimension,
                                                                     const QList<SDKTemporal::Time*>& times,
                                                                     Dimension* timeDimension,
                                                                     ValueDefinition* valueDefinition,
                                                                     AbstractModelComponent* modelComponent)
  : AbstractMultiInput(id,QList<Dimension*>({identifierDimension, timeDimension}),valueDefinition, modelComponent),
    TimeSeriesIdBasedComponentDataItem<double>(identifiers, identifierDimension, times, timeDimension,valueDefinition->defaultValue().toDouble())
{

}

TimeSeriesIdBasedMultiInputDouble::~TimeSeriesIdBasedMultiInputDouble()
{
}


QStringList TimeSeriesIdBasedMultiInputDouble::identifiers() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::identifiersInternal();
}

IDimension* TimeSeriesIdBasedMultiInputDouble::identifierDimension() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::identifierDimensionInternal();
}

QList<ITime*> TimeSeriesIdBasedMultiInputDouble::times() const
{
  QList<ITime*> otimes;
  QList<SDKTemporal::Time*> itimes = TimeSeriesIdBasedComponentDataItem<double>::timesInternal();

  for(SDKTemporal::Time* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

ITimeSpan* TimeSeriesIdBasedMultiInputDouble::timeSpan() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesIdBasedMultiInputDouble::timeDimension() const
{
  return TimeSeriesIdBasedComponentDataItem<double>::timeDimensionInternal();
}

void TimeSeriesIdBasedMultiInputDouble::getValue(int dimensionIndexes[], QVariant &data) const
{
  ComponentDataItem2D<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedMultiInputDouble::getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedMultiInputDouble::getValues(int dimensionIndexes[], int stride[],  void *data) const
{
  ComponentDataItem2D<double>::getValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedMultiInputDouble::setValue(int dimensionIndexes[], const QVariant &data)
{
  ComponentDataItem2D<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesIdBasedMultiInputDouble::setValues(int dimensionIndexes[], int stride[], const QVariant data[])
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedMultiInputDouble::setValues(int dimensionIndexes[], int stride[], const void *data)
{
  ComponentDataItem2D<double>::setValuesT(dimensionIndexes,stride,data);
}

void TimeSeriesIdBasedMultiInputDouble::getValue(int timeIndex, int idIndex, QVariant &data) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedMultiInputDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride, QVariant data[]) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedMultiInputDouble::getValues(int timeIndex, int idIndex, int timeStride, int idStride, void *data) const
{
  TimeSeriesIdBasedComponentDataItem<double>::getValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedMultiInputDouble::setValue(int timeIndex, int idIndex, const QVariant &data)
{
  TimeSeriesIdBasedComponentDataItem<double>::setValueT(timeIndex,idIndex,data);
}

void TimeSeriesIdBasedMultiInputDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const QVariant data[])
{
  TimeSeriesIdBasedComponentDataItem<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}

void TimeSeriesIdBasedMultiInputDouble::setValues(int timeIndex, int idIndex, int timeStride, int idStride, const void *data)
{
  TimeSeriesIdBasedComponentDataItem<double>::setValuesT(timeIndex,idIndex,timeStride,idStride,data);
}


template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesIdBasedComponentDataItem<QString>;
