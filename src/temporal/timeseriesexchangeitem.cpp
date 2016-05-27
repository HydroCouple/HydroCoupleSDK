#include "stdafx.h"
#include "temporal/timeseriesexchangeitem.h"

using namespace Temporal;


TimeSeriesOutputDouble::TimeSeriesOutputDouble(const QString &id, const QList<Time *> &times,
                                   Dimension *timeDimension, ValueDefinition *valueDefinition,
                                   AbstractModelComponent *parentModelComponent)
   : AbstractOutput(id,parentModelComponent),
     TimeSeriesComponentDataItem<double>(times, timeDimension,valueDefinition)
{

}

TimeSeriesOutputDouble::~TimeSeriesOutputDouble()
{

}


TimeSeriesInputDouble::TimeSeriesInputDouble(const QString &id, const QList<Time *> &times,
                                   Dimension *timeDimension, ValueDefinition *valueDefinition,
                                   AbstractModelComponent *parentModelComponent)
   : AbstractInput(id,parentModelComponent) ,
     TimeSeriesComponentDataItem<double>(times, timeDimension,valueDefinition)
{

}

TimeSeriesInputDouble::~TimeSeriesInputDouble()
{

}

TimeSeriesMultiInputDouble::TimeSeriesMultiInputDouble(const QString &id, const QList<Time *> &times,
                                   Dimension *timeDimension, ValueDefinition *valueDefinition,
                                   AbstractModelComponent *parentModelComponent)
   : AbstractMultiInput(id,parentModelComponent) ,
     TimeSeriesComponentDataItem<double>(times, timeDimension,valueDefinition)

{

}

TimeSeriesMultiInputDouble::~TimeSeriesMultiInputDouble()
{

}

template class HYDROCOUPLESDK_EXPORT Temporal::TimeSeriesComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT Temporal::TimeSeriesComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT Temporal::TimeSeriesComponentDataItem<QString>;
