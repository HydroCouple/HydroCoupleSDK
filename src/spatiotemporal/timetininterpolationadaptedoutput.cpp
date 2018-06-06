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
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "temporal/timedata.h"
#include "spatiotemporal/timetininterpolationadaptedoutput.h"

using namespace HydroCouple;
using namespace HydroCouple::Temporal;
using namespace HydroCouple::Spatial;
using namespace SDKTemporal;

TimeTINInterpolationAdaptedOutput::TimeTINInterpolationAdaptedOutput(const QString &id,
                                                                     ValueDefinition *valueDefinition,
                                                                     HydroCouple::SpatioTemporal::ITimeTINComponentDataItem *adaptee,
                                                                     AbstractAdaptedOutputFactory *adaptedOutputFactory):
  AbstractAdaptedOutput(id, QList<Dimension*>({Dimension::copy(adaptee->timeDimension(), nullptr),
                                               Dimension::copy(adaptee->cellDimension(), nullptr),
                                               Dimension::copy(adaptee->edgeDimension(), nullptr),
                                               Dimension::copy(adaptee->nodeDimension(), nullptr)}),
                        valueDefinition, dynamic_cast<HydroCouple::IOutput*>(adaptee), adaptedOutputFactory),
  m_interpolationOptionsArgument(nullptr),
  m_adaptee(adaptee),
  m_polynomialOrder(2),
  m_timeCacheSize(2),
  m_interpolationMode(SplineInterpolator::Linear)
{
  QList<Dimension*> dimensions = dimensionsInternal();
  m_timeDimension = dimensions[0];
  m_cellDimension = dimensions[1];
  m_edgeDimension = dimensions[2];
  m_nodeDimension = dimensions[3];

  m_timeSpan = new SDKTemporal::TimeSpan(this);
  m_patchCount = adaptee->polyhedralSurface()->patchCount();
  m_dataType = adaptee->meshDataType();

  switch (m_dataType)
  {
    case HydroCouple::Spatial::Node:
      {
        m_dataLength = m_patchCount * 4;
      }
      break;
    case HydroCouple::Spatial::Face:
      {
        m_dataLength = m_patchCount * 3;
      }
      break;
    case HydroCouple::Spatial::Centroid:
      {
        m_dataLength = m_patchCount;
      }
      break;
  }

  Dimension *identifierDimension = new Dimension("PolynomialOrderDimension","Dimension for the order of the polynomial",this);
  QStringList identifiers;
  identifiers.append("InterpolationMode");
  identifiers.append("TimesCacheSize");
  Quantity* quantity = Quantity::unitLessValues("IdentifiersUnit", QVariant::Int , this);

  m_interpolationOptionsArgument = new IdBasedArgumentInt("InterpolationOptions", identifiers, identifierDimension, quantity, nullptr);
  m_interpolationOptionsArgument->setCaption("Interpolation options");
  m_interpolationOptionsArgument->addComment("Interpolation Options 0 = Linear, 1 = Quadratic, 2 = Cubic, 3 = Lagrange");
  m_interpolationOptionsArgument->setValuesT(0,1,&m_polynomialOrder);
  m_interpolationOptionsArgument->setValuesT(1,1,&m_timeCacheSize);
  addArgument(m_interpolationOptionsArgument);
}

TimeTINInterpolationAdaptedOutput::~TimeTINInterpolationAdaptedOutput()
{
  delete m_timeDimension;
  delete m_cellDimension;
  delete m_edgeDimension;
  delete m_nodeDimension;
}

HydroCouple::Spatial::MeshDataType TimeTINInterpolationAdaptedOutput::meshDataType() const
{
  return m_adaptee->meshDataType();
}

HydroCouple::Spatial::IPolyhedralSurface *TimeTINInterpolationAdaptedOutput::polyhedralSurface() const
{
  return m_adaptee->polyhedralSurface();
}

QList<IDateTime*> TimeTINInterpolationAdaptedOutput::times() const
{
  QList<IDateTime*> times;

  for(DateTime *dateTime : m_times)
  {
    times.append(dateTime);
  }

  return times;
}

IDateTime *TimeTINInterpolationAdaptedOutput::time(int timeIndex) const
{
  return m_times[timeIndex];
}

int TimeTINInterpolationAdaptedOutput::timeCount() const
{
  return m_times.size();
}

void TimeTINInterpolationAdaptedOutput::resetTimeSpan()
{
  if(m_times.size())
  {
    double duration = m_times[0]->julianDay() - m_times[m_times.size() -1]->julianDay();
    m_timeSpan->setJulianDay(m_times[0]->julianDay());
    m_timeSpan->setDuration(duration);
  }
}

ITimeSpan *TimeTINInterpolationAdaptedOutput::timeSpan() const
{
  return m_timeSpan;
}

IDimension *TimeTINInterpolationAdaptedOutput::timeDimension() const
{
  return m_timeDimension;
}

IDimension *TimeTINInterpolationAdaptedOutput::cellDimension() const
{
  return m_cellDimension;
}

IDimension *TimeTINInterpolationAdaptedOutput::edgeDimension() const
{
  return m_edgeDimension;
}

IDimension *TimeTINInterpolationAdaptedOutput::nodeDimension() const
{
  return m_nodeDimension;
}

ITIN *TimeTINInterpolationAdaptedOutput::TIN() const
{
  return m_adaptee->TIN();
}

void TimeTINInterpolationAdaptedOutput::updateValues(IInput *querySpecifier)
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
      std::vector<double> values(m_dataLength);

      for(int i = 0; i < m_dataLength ; i++)
      {
        for(size_t t = 0; t < m_bufferedDateTimes.size(); t++)
        {
          tvals[t] = m_bufferedValues[t][i];
        }

        SplineInterpolator::InterpolationMethod method = getInterpolationMode(m_bufferedDateTimes.size());
        double value = SplineInterpolator::interpolate(method, m_bufferedDateTimes, tvals, queryDateTime);
        values[i] = value;
      }

      if(m_times.size() < (size_t)m_timeCacheSize)
      {
        SDKTemporal::DateTime *dateTime = new SDKTemporal::DateTime(queryDateTime);
        m_times.push_back(dateTime);
        m_data.push_back(values);
      }
      else
      {
        moveDataToPrevTime();
        m_times[m_times.size() - 1]->setJulianDay(queryDateTime);
        m_data[m_times.size() - 1] = values;
      }

      resetTimeSpan();
      refreshAdaptedOutputs();

      return;
    }
    else if(m_bufferedDateTimes.size() && m_data.size() == 0)
    {
      SDKTemporal::DateTime *dateTime = new SDKTemporal::DateTime(m_bufferedDateTimes[0]);
      m_times.push_back(dateTime);
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

int TimeTINInterpolationAdaptedOutput::dimensionLength(const std::vector<int> &dimensionIndexes) const
{
  if(dimensionIndexes.size() == 0)
  {
    return m_times.size();
  }
  else
  {
    switch(m_dataType)
    {
      case HydroCouple::Spatial::Centroid:
        {
          switch (dimensionIndexes.size())
          {
            case 1:
              {
                return m_patchCount;
              }
              break;
            default:
              return 0;
              break;
          }
        }
        break;
      case HydroCouple::Spatial::Face:
        {
          switch (dimensionIndexes.size())
          {
            case 1:
              {
                return m_patchCount;
              }
              break;
            case 2:
              {
                return 3;
              }
              break;
            default:
              return 0;
              break;
          }
        }
        break;
      default:
        {
          switch (dimensionIndexes.size())
          {
            case 1:
              {
                return m_patchCount;
              }
              break;
            case 2:
              {
                return 3;
              }
              break;
            case 3:
              {
                return 2;
              }
            default:
              return 0;
              break;
          }
        }
        break;
    }
  }
}

void TimeTINInterpolationAdaptedOutput::getValue(const std::vector<int> &dimensionIndexes, void *data) const
{
  double* dataC = (double*) data;

  int timeIndex = dimensionIndexes[0];
  const std::vector<double> & tdata = m_data[timeIndex];

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Centroid:
      {
        int cellIndex = dimensionIndexes[1];
        *dataC = tdata[cellIndex];
      }
      break;
    case HydroCouple::Spatial::Face:
      {
        int cellIndex = dimensionIndexes[1];
        int faceIndex = dimensionIndexes[2];
        int index = cellIndex + m_patchCount * faceIndex;

        *dataC = tdata[index];
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int cellIndex = dimensionIndexes[1];
        int faceIndex = dimensionIndexes[2];
        int nodeIndex = dimensionIndexes[3];

        int index = cellIndex + m_patchCount * (faceIndex + nodeIndex);

        *dataC = tdata[index];
      }
      break;
  }
}

void TimeTINInterpolationAdaptedOutput::getValue(int timeIndex, int cellDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex, void *data) const
{
  double* dataC = (double*) data;

  const std::vector<double> & tdata = m_data[timeIndex];

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Centroid:
      {
        *dataC = tdata[cellDimensionIndex];
      }
      break;
    case HydroCouple::Spatial::Face:
      {
        int index = cellDimensionIndex + m_patchCount * edgeDimensionIndex;
        *dataC = tdata[index];
      }
      break;
    case HydroCouple::Spatial::Node:
      {

        int index = cellDimensionIndex + m_patchCount * (edgeDimensionIndex + nodeDimensionIndex);
        *dataC = tdata[index];
      }
      break;
  }
}

void TimeTINInterpolationAdaptedOutput::setValue(const std::vector<int> &dimensionIndexes, const void *data)
{
  double* dataC = (double*) data;

  int timeIndex = dimensionIndexes[0];
  std::vector<double> & tdata = m_data[timeIndex];

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Centroid:
      {
        int cellIndex = dimensionIndexes[1];
        tdata[cellIndex] = *dataC;
      }
      break;
    case HydroCouple::Spatial::Face:
      {
        int cellIndex = dimensionIndexes[1];
        int faceIndex = dimensionIndexes[2];
        int index = cellIndex + m_patchCount * faceIndex;

        tdata[index] = *dataC;
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int cellIndex = dimensionIndexes[1];
        int faceIndex = dimensionIndexes[2];
        int nodeIndex = dimensionIndexes[3];

        int index = cellIndex + m_patchCount * (faceIndex + nodeIndex);

        tdata[index] = *dataC;
      }
      break;
  }
}

void TimeTINInterpolationAdaptedOutput::setValue(int timeIndex, int cellDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex, const void *data)
{
  double* dataC = (double*) data;

  std::vector<double> & tdata = m_data[timeIndex];

  switch(m_dataType)
  {
    case HydroCouple::Spatial::Centroid:
      {
        tdata[cellDimensionIndex] = *dataC;
      }
      break;
    case HydroCouple::Spatial::Face:
      {
        int index = cellDimensionIndex + m_patchCount * edgeDimensionIndex;
        tdata[index] = *dataC;
      }
      break;
    case HydroCouple::Spatial::Node:
      {
        int index = cellDimensionIndex + m_patchCount * (edgeDimensionIndex + nodeDimensionIndex);
        tdata[index] = *dataC;
      }
      break;
  }
}

void TimeTINInterpolationAdaptedOutput::initialize()
{
  m_interpolationOptionsArgument->getValues(0,1,&m_interpolationMode);
  m_interpolationOptionsArgument->getValues(1,1,&m_timeCacheSize);

  if(m_timeCacheSize < 1)
    m_timeCacheSize = 1;

  setInterpolationMethod(m_interpolationMode);
  initializeAdaptedOutputs();

}

void TimeTINInterpolationAdaptedOutput::refresh()
{
  if(m_adaptee->timeCount())
  {
    double adapteeTime = m_adaptee->time(m_adaptee->timeCount() - 1)->julianDay();
    std::vector<double> values(m_dataLength);
    double value;

    switch (m_dataType)
    {
      case HydroCouple::Spatial::Centroid:
        {
          for(int c = 0; c < m_patchCount; c++)
          {
            m_adaptee->getValue(m_adaptee->timeCount() - 1, c, 0, 0, &value);
            values[c] = value;
          }
        }
        break;
      case HydroCouple::Spatial::Face:
        {
          for(int c = 0; c < m_patchCount; c++)
          {
            for(int f = 0; f < 3; f++)
            {
              m_adaptee->getValue(m_adaptee->timeCount() - 1, c, f, 0, &value);
              values[c + f * m_patchCount] = value;
            }
          }
        }
        break;
      default:
        {
          for(int c = 0; c < m_patchCount; c++)
          {
            for(int f = 0; f < 3; f++)
            {
              for(int n = 0; n < 2; n++)
              {
                m_adaptee->getValue(m_adaptee->timeCount() - 1, c, f, n, &value);
                values[c + f * m_patchCount + n * m_patchCount * 3] = value;
              }
            }
          }
        }
        break;
    }


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

void TimeTINInterpolationAdaptedOutput::moveDataToPrevTime()
{
  if(m_times.size() > 1)
  {
    for(size_t i = 0; i < m_times.size() - 1; i++)
    {
      m_times[i]->setJulianDay(m_times[i+1]->julianDay());
      m_data[i] = m_data[i+1];
    }
  }
}

SplineInterpolator::InterpolationMethod TimeTINInterpolationAdaptedOutput::interpolationMethod() const
{
  return m_interpolationMode;
}

void TimeTINInterpolationAdaptedOutput::setInterpolationMethod(SplineInterpolator::InterpolationMethod interpolationMethod)
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

SplineInterpolator::InterpolationMethod TimeTINInterpolationAdaptedOutput::getInterpolationMode(int count)
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
