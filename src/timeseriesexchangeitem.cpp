#include "stdafx.h"
#include "timeseriesexchangeitem.h"

TimeSeriesOutputDouble::TimeSeriesOutputDouble(const QString &id, const QList<HTime *> &times,
                                   Dimension *timeDimension, ValueDefinition *valueDefinition,
                                   AbstractModelComponent *parentModelComponent)
   : AbstractOutput(id,parentModelComponent),
     TimeSeriesComponentDataItem<double>(times, timeDimension,valueDefinition)
{

}

TimeSeriesOutputDouble::~TimeSeriesOutputDouble()
{

}


TimeSeriesInputDouble::TimeSeriesInputDouble(const QString &id, const QList<HTime *> &times,
                                   Dimension *timeDimension, ValueDefinition *valueDefinition,
                                   AbstractModelComponent *parentModelComponent)
   : AbstractInput(id,parentModelComponent) ,
     TimeSeriesComponentDataItem<double>(times, timeDimension,valueDefinition)
{

}

TimeSeriesInputDouble::~TimeSeriesInputDouble()
{

}

TimeSeriesMultiInputDouble::TimeSeriesMultiInputDouble(const QString &id, const QList<HTime *> &times,
                                   Dimension *timeDimension, ValueDefinition *valueDefinition,
                                   AbstractModelComponent *parentModelComponent)
   : AbstractMultiInput(id,parentModelComponent) ,
     TimeSeriesComponentDataItem<double>(times, timeDimension,valueDefinition)

{

}

TimeSeriesMultiInputDouble::~TimeSeriesMultiInputDouble()
{

}

template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<int>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<double>;
template class HYDROCOUPLESDK_EXPORT TimeSeriesComponentDataItem<QString>;
