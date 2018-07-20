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
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "temporal/timedata.h"
#include "spatiotemporal/timegeometryinterpolationadaptedoutput.h"

using namespace HydroCouple;
using namespace HydroCouple::Temporal;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::SpatioTemporal;
using namespace SDKTemporal;

TimeGeometryInterpolationAdaptedOutput::TimeGeometryInterpolationAdaptedOutput(const QString &id,
                                                                               ValueDefinition *valueDefinition,
                                                                               HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem *adaptee,
                                                                               AbstractAdaptedOutputFactory *adaptedOutputFactory):
  AbstractAdaptedOutput(id, QList<Dimension*>({Dimension::copy(adaptee->timeDimension(), nullptr),
                                               Dimension::copy(adaptee->geometryDimension(), nullptr)}),
                        valueDefinition, dynamic_cast<HydroCouple::IOutput*>(adaptee), adaptedOutputFactory),
  m_adaptee(adaptee),
  m_interpolationOptionsArgument(nullptr),
  m_interpolationMode(SplineInterpolator::Linear),
  m_polynomialOrder(2),
  m_timeCacheSize(2)
{
  m_timeDimension = dimensionsInternal()[0];
  m_geometryDimension = dimensionsInternal()[1];

  m_timeSpan = new SDKTemporal::TimeSpan(this);

  Dimension *identifierDimension = new Dimension("PolynomialOrderDimension","Dimension for the order of the polynomial",this);
  QStringList identifiers;
  identifiers.append("InterpolationMode");
  identifiers.append("TimesCacheSize");
  Quantity* quantity = Quantity::unitLessValues("IdentifiersUnit", QVariant::Int , this);

  m_interpolationOptionsArgument = new IdBasedArgumentInt("InterpolationOptions", identifiers, identifierDimension, quantity, nullptr);
  m_interpolationOptionsArgument->setCaption("Interpolation options");
  m_interpolationOptionsArgument->addComment("Interpolation Options 0 = Linear, 1 = Quadratic, 2 = Cubic, 3 = Lagrange");
  m_interpolationOptionsArgument->setValuesT(0,1,&m_interpolationMode);
  m_interpolationOptionsArgument->setValuesT(1,1,&m_timeCacheSize);
  addArgument(m_interpolationOptionsArgument);
}

TimeGeometryInterpolationAdaptedOutput::~TimeGeometryInterpolationAdaptedOutput()
{
  delete m_timeDimension;
  delete m_geometryDimension;

  for(SDKTemporal::DateTime *dateTime : m_dateTimes)
  {
    delete dateTime;
  }

  m_dateTimes.clear();
}

QList<IDateTime*> TimeGeometryInterpolationAdaptedOutput::times() const
{
  QList<IDateTime*> times;

  for(SDKTemporal::DateTime *time : m_dateTimes)
    times.append(time);

  return times;
}

IDateTime *TimeGeometryInterpolationAdaptedOutput::time(int timeIndex) const
{
  return m_dateTimes[timeIndex];
}

int TimeGeometryInterpolationAdaptedOutput::timeCount() const
{
  return m_dateTimes.size();
}

ITimeSpan *TimeGeometryInterpolationAdaptedOutput::timeSpan() const
{
  return m_timeSpan;
}

void TimeGeometryInterpolationAdaptedOutput::resetTimeSpan()
{
  if(m_dateTimes.size())
  {
    double duration = m_dateTimes[0]->julianDay() - m_dateTimes[m_dateTimes.size() -1]->julianDay();
    m_timeSpan->setJulianDay(m_dateTimes[0]->julianDay());
    m_timeSpan->setDuration(duration);
  }
}

IDimension *TimeGeometryInterpolationAdaptedOutput::timeDimension() const
{
  return m_timeDimension;
}

IGeometry::GeometryType TimeGeometryInterpolationAdaptedOutput::geometryType() const
{
  return m_adaptee->geometryType();
}

int TimeGeometryInterpolationAdaptedOutput::geometryCount() const
{
  return m_adaptee->geometryCount();
}

HydroCouple::Spatial::IGeometry *TimeGeometryInterpolationAdaptedOutput::geometry(int geometryIndex) const
{
  return m_adaptee->geometry(geometryIndex);
}

IDimension *TimeGeometryInterpolationAdaptedOutput::geometryDimension() const
{
  return m_geometryDimension;
}

IEnvelope *TimeGeometryInterpolationAdaptedOutput::envelope() const
{
  return m_adaptee->envelope();
}

void TimeGeometryInterpolationAdaptedOutput::updateValues(IInput *querySpecifier)
{
  ITimeComponentDataItem* timeExchangeItem = dynamic_cast<ITimeComponentDataItem*>(querySpecifier);

  if(timeExchangeItem)
  {
    IOutput *adaptee = dynamic_cast<IOutput*>(m_adaptee);
    adaptee->updateValues(querySpecifier);

    double queryDateTime =  timeExchangeItem->time(timeExchangeItem->timeCount() - 1)->julianDay();

    if(m_bufferedDateTimes.size() > 1)
    {
      std::vector<double> tvals(m_bufferedDateTimes.size());
      std::vector<double> newGeomValues(m_adaptee->geometryCount());

      for(int i = 0; i < m_adaptee->geometryCount() ; i++)
      {
        for(size_t t = 0; t < m_bufferedDateTimes.size(); t++)
        {
          tvals[t] = m_bufferedValues[t][i];
        }

        SplineInterpolator::InterpolationMethod method = getInterpolationMode(m_bufferedDateTimes.size());
        double value = SplineInterpolator::interpolate(method, m_bufferedDateTimes, tvals, queryDateTime);
        newGeomValues[i] = value;
      }

      if(m_dateTimes.size() < (size_t)m_timeCacheSize)
      {
        SDKTemporal::DateTime *dateTime = new SDKTemporal::DateTime(queryDateTime);
        m_dateTimes.push_back(dateTime);
        m_data.push_back(newGeomValues);
      }
      else
      {
        moveDataToPrevTime();
        m_dateTimes[m_dateTimes.size() - 1]->setJulianDay(queryDateTime);
        m_data[m_data.size() - 1] = newGeomValues;
      }

      resetTimeSpan();
      refreshAdaptedOutputs();

      return;
    }
    else if(m_bufferedDateTimes.size() && m_data.size() == 0)
    {
      SDKTemporal::DateTime *dateTime = new SDKTemporal::DateTime(m_bufferedDateTimes[0]);
      m_dateTimes.push_back(dateTime);
      m_data.push_back(m_bufferedValues[0]);

      resetTimeSpan();
      refreshAdaptedOutputs();

      return;
    }
  }

  if(modelComponent()->status() == IModelComponent::Updated)
  {
    modelComponent()->update(QList<IOutput*>({this}));
  }

  resetTimeSpan();
  refreshAdaptedOutputs();
}

int TimeGeometryInterpolationAdaptedOutput::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  if(dimensionIndexes.size() == 0)
    return m_dateTimes.size();

  return m_adaptee->geometryCount();
}

void TimeGeometryInterpolationAdaptedOutput::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  double *dataC = (double*) data;
  dataC[0] = m_data[dimensionIndexes[0]][dimensionIndexes[1]];
}

void TimeGeometryInterpolationAdaptedOutput::getValue(int timeIndex, int geometryIndex, void *data) const
{
  double *dataC = (double*) data;
  dataC[0] = m_data[timeIndex][geometryIndex];
}

void TimeGeometryInterpolationAdaptedOutput::getValues(int timeIndex, int geometryIndex, int timeStride, int geomStride, void *data) const
{
  double *dataC = (double*) data;

  for(int i = 0; i < timeStride; i++)
  {
    for(int j = 0 ; j <  geomStride ; j++)
    {
      dataC[i + j*timeStride] = m_data[timeIndex+i][geometryIndex + j];
    }
  }
}

void TimeGeometryInterpolationAdaptedOutput::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  double *dataC = (double*) data;
  m_data[dimensionIndexes[0]][dimensionIndexes[1]] = dataC[0] ;
}

void TimeGeometryInterpolationAdaptedOutput::setValue(int timeIndex, int geometryIndex, const void *data)
{
  double *dataC = (double*) data;
  m_data[timeIndex][geometryIndex] = dataC[0];
}

void TimeGeometryInterpolationAdaptedOutput::setValues(int timeIndex, int geometryIndex, int timeStride, int geomStride, const void *data)
{
  double *dataC = (double*) data;

  for(int i = 0; i < timeStride; i++)
  {
    for(int j = 0 ; j <  geomStride ; j++)
    {
      m_data[timeIndex+i][geometryIndex + j] = dataC[i + j*timeStride] ;
    }
  }
}

void TimeGeometryInterpolationAdaptedOutput::initialize()
{
  m_interpolationOptionsArgument->getValues(0,1,&m_interpolationMode);
  m_interpolationOptionsArgument->getValues(1,1,&m_timeCacheSize);

  if(m_timeCacheSize < 1)
    m_timeCacheSize = 1;

  setInterpolationMethod(m_interpolationMode);
  initializeAdaptedOutputs();
}

void TimeGeometryInterpolationAdaptedOutput::refresh()
{
  if(m_adaptee->timeCount())
  {
    double adapteeTime = m_adaptee->time(m_adaptee->timeCount() - 1)->julianDay();
    std::vector<double> values(m_adaptee->geometryCount());
    m_adaptee->getValues(m_adaptee->timeCount() - 1, 0, 1, m_adaptee->geometryCount(), values.data());

    if(m_bufferedDateTimes.size() == 0)
    {
      m_bufferedDateTimes.push_back(adapteeTime);
      m_bufferedValues.push_back(values);
    }
    //for iteration
    else if(adapteeTime == m_bufferedDateTimes[m_bufferedDateTimes.size() - 1])
    {
      m_bufferedValues[m_bufferedValues.size() - 1] = values;
    }
    else if(adapteeTime > m_bufferedDateTimes[m_bufferedDateTimes.size() - 1])
    {
      m_bufferedDateTimes.push_back(adapteeTime);
      m_bufferedValues.push_back(values);
    }

    while (m_bufferedDateTimes.size() > (size_t)m_polynomialOrder)
    {
      m_bufferedDateTimes.erase(m_bufferedDateTimes.begin());
      m_bufferedValues.erase(m_bufferedValues.begin());
    }

    refreshAdaptedOutputs();
  }
}

void TimeGeometryInterpolationAdaptedOutput::moveDataToPrevTime()
{
  if(m_data.size() > 1)
  {
    for(size_t i = 0; i < m_dateTimes.size() -1; i++)
    {
      m_dateTimes[i]->setJulianDay(m_dateTimes[i+1]->julianDay());
      m_data[i] = m_data[i+1];
    }
  }
}

SplineInterpolator::InterpolationMethod TimeGeometryInterpolationAdaptedOutput::interpolationMethod() const
{
  return m_interpolationMode;
}

void TimeGeometryInterpolationAdaptedOutput::setInterpolationMethod(SplineInterpolator::InterpolationMethod interpolationMethod)
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

SplineInterpolator::InterpolationMethod TimeGeometryInterpolationAdaptedOutput::getInterpolationMode(int count)
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
    default:
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

