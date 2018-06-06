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
#include "core/abstractadaptedoutput.h"
#include "core/valuedefinition.h"
#include "core/idbasedargument.h"
#include "core/dimension.h"
#include "core/idbasedargument.h"
#include "core/unit.h"
#include "temporal/timedata.h"
#include "temporal/temporalinterpolationfactory.h"
#include "temporal/timeseriesinterpolationadaptedoutput.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Temporal;
using namespace std;

TimeSeriesInterpolationAdaptedOutput::TimeSeriesInterpolationAdaptedOutput(const QString &id,
                                                                           Quantity *valueDefinition,
                                                                           HydroCouple::Temporal::ITimeSeriesComponentDataItem *adaptee,
                                                                           AbstractAdaptedOutputFactory *temporalInterpolationFactory)
  : AbstractAdaptedOutput(id,QList<Dimension*>({Dimension::copy(adaptee->timeDimension(), nullptr)}), valueDefinition,
                          dynamic_cast<HydroCouple::IOutput*>(adaptee),temporalInterpolationFactory),
    TimeSeriesComponentDataItem<double>(id, std::list<SDKTemporal::DateTime*>({new SDKTemporal::DateTime(temporalInterpolationFactory)}), valueDefinition->defaultValue().toDouble()),
    m_interpolationOptionsArgument(nullptr),
    m_polynomialOrder(2),
    m_adaptee(adaptee),
    m_interpolationMode(SplineInterpolator::Linear)
{
  m_timeDimension = dimensionsInternal()[0];
  Dimension *identifierDimension = new Dimension("PolynomialOrderDimension","Dimension for the order of the polynomial",this);
  QStringList identifiers;
  identifiers.append("InterpolationMode");
  Quantity* quantity = Quantity::unitLessValues("IdentifiersUnit", QVariant::Int , this);

  m_interpolationOptionsArgument = new IdBasedArgumentInt("InterpolationOptions", identifiers, identifierDimension, quantity, nullptr);
  m_interpolationOptionsArgument->setCaption("Interpolation options");
  m_interpolationOptionsArgument->addComment("Interpolation Options 0 = Linear, 1 = Quadratic, 2 = Cubic, 3 = Lagrange");
  m_interpolationOptionsArgument->setValuesT(0,1,&m_polynomialOrder);
  addArgument(m_interpolationOptionsArgument);
}

TimeSeriesInterpolationAdaptedOutput::~TimeSeriesInterpolationAdaptedOutput()
{
  delete m_timeDimension;
  delete m_interpolationOptionsArgument;
}

QList<IDateTime*> TimeSeriesInterpolationAdaptedOutput::times() const
{
  QList<IDateTime*> otimes;
  std::vector<SDKTemporal::DateTime*> itimes = TimeSeriesComponentDataItem<double>::timesInternal();

  for(SDKTemporal::DateTime* time : itimes)
  {
    otimes.append(time);
  }

  return otimes;
}

IDateTime* TimeSeriesInterpolationAdaptedOutput::time(int timeIndex) const
{
  return timeInternal(timeIndex);
}

int TimeSeriesInterpolationAdaptedOutput::timeCount() const
{
  return timeCountInternal();
}

ITimeSpan* TimeSeriesInterpolationAdaptedOutput::timeSpan() const
{
  return TimeSeriesComponentDataItem<double>::timeSpanInternal();
}

IDimension* TimeSeriesInterpolationAdaptedOutput::timeDimension() const
{
  return m_timeDimension;
}

void TimeSeriesInterpolationAdaptedOutput::updateValues(IInput *querySpecifier)
{
  IOutput *adaptee = dynamic_cast<IOutput*>(m_adaptee);
  adaptee->updateValues(querySpecifier);

  ITimeComponentDataItem* timeExchangeItem = dynamic_cast<ITimeComponentDataItem*>(querySpecifier);
  double queryTime = timeExchangeItem->time(timeExchangeItem->timeCount() - 1)->julianDay();

  double currentTime = *m_timesBuffer.end();// [m_timeSeriesBuffer.keys().length() - 1];

  if(m_timesBuffer.size() > 1 && currentTime >= queryTime)
  {
    SplineInterpolator::InterpolationMethod method = getInterpolationMode(m_timesBuffer.size());
    double value = SplineInterpolator::interpolate(method, m_timesBuffer, m_valuesBuffer, queryTime);
    setValueT(timeCount() - 1, &value);

    refreshAdaptedOutputs();
  }

}

int TimeSeriesInterpolationAdaptedOutput::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  assert(dimensionIndexes.size() == 0);
  //assert(dimensionIndexes == nullptr);
  return length();
}

void TimeSeriesInterpolationAdaptedOutput::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(dimensionIndexes,data);
}

void TimeSeriesInterpolationAdaptedOutput::getValue(int timeIndex, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValueT(timeIndex,data);
}

void TimeSeriesInterpolationAdaptedOutput::getValues(int timeIndex, int stride, void *data) const
{
  TimeSeriesComponentDataItem<double>::getValuesT(timeIndex,stride,data);
}

void TimeSeriesInterpolationAdaptedOutput::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValueT(dimensionIndexes,data);
}

void TimeSeriesInterpolationAdaptedOutput::setValue(int timeIndex, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValueT(timeIndex,data);
}

void TimeSeriesInterpolationAdaptedOutput::setValues(int timeIndex, int stride, const void *data)
{
  TimeSeriesComponentDataItem<double>::setValuesT(timeIndex, stride , data);
}

void TimeSeriesInterpolationAdaptedOutput::initialize()
{
  m_interpolationOptionsArgument->getValues(0,1,&m_interpolationMode);
  setInterpolationMethod(m_interpolationMode);
  initializeAdaptedOutputs();
}

void TimeSeriesInterpolationAdaptedOutput::refresh()
{
  if(m_adaptee->timeCount())
  {
    //retrieve data into buffer and trim appropriately.
    double time = m_adaptee->time(m_adaptee->timeCount() - 1)->julianDay();
    double value = 0.0;
    m_adaptee->getValue(m_adaptee->timeCount() - 1,&value);

    if(m_timesBuffer.size() == 0)
    {
      m_timesBuffer.push_back(time);
      m_valuesBuffer.push_back(value);
    }
    else if(time == m_timesBuffer[m_timesBuffer.size() - 1])
    {
      m_valuesBuffer[m_valuesBuffer.size() - 1] = value;
    }
    else if(time > m_timesBuffer[m_timesBuffer.size() - 1])
    {
      m_timesBuffer.push_back(time);
      m_valuesBuffer.push_back(value);
    }

    while (m_timesBuffer.size() > (size_t)m_polynomialOrder)
    {
      m_timesBuffer.erase(m_timesBuffer.begin());
      m_valuesBuffer.erase(m_valuesBuffer.begin());
    }

    refreshAdaptedOutputs();
  }
}

SplineInterpolator::InterpolationMethod TimeSeriesInterpolationAdaptedOutput::interpolationMethod() const
{
  return m_interpolationMode;
}

void TimeSeriesInterpolationAdaptedOutput::setInterpolationMethod(SplineInterpolator::InterpolationMethod interpolationMethod)
{
  m_interpolationMode = interpolationMethod;

  switch (m_interpolationMode)
  {
    case SplineInterpolator::Linear:
      {
        m_polynomialOrder = 2;
      }
      break;
    case SplineInterpolator::Quadratic:
      {
        m_polynomialOrder = 3;
      }
      break;
    case SplineInterpolator::Cubic:
      {
        m_polynomialOrder = 4;
      }
      break;
    case SplineInterpolator::Lagrange:
      {
        m_polynomialOrder = 5;
      }
      break;
  }

}

SplineInterpolator::InterpolationMethod TimeSeriesInterpolationAdaptedOutput::getInterpolationMode(int count)
{
  switch (m_interpolationMode)
  {
    case SplineInterpolator::Linear:
      {
        return m_interpolationMode;
      }
      break;
    case SplineInterpolator::Quadratic:
      {
        switch (count)
        {
          case 2:
            {
              return SplineInterpolator::Linear;
            }
            break;
          default:
            return m_interpolationMode;
            break;
        }
      }
      break;
    case SplineInterpolator::Cubic:
      {
        switch (count)
        {
          case 2:
            {
              return SplineInterpolator::Linear;
            }
            break;
          case 3:
            {
              return SplineInterpolator::Quadratic;
            }
            break;
          default:
            return m_interpolationMode;
            break;
        }
      }
      break;
    case SplineInterpolator::Lagrange:
      {
        switch (count)
        {
          case 2:
            {
              return SplineInterpolator::Linear;
            }
            break;
          case 3:
            {
              return SplineInterpolator::Quadratic;
            }
            break;
          case 4:
            {
              return SplineInterpolator::Cubic;
            }
            break;
          default:
            return m_interpolationMode;
            break;
        }
      }
      break;
  }
}



