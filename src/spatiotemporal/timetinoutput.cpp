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
#include "core/valuedefinition.h"
#include "core/dimension.h"
#include "temporal/timedata.h"
#include "spatial/polyhedralsurface.h"
#include "spatiotemporal/timetinexchangeitem.h"

#include <assert.h>

using namespace HydroCouple;
using namespace HydroCouple::Spatial;
using namespace HydroCouple::Temporal;

TimeTINOutputDouble::TimeTINOutputDouble(const QString &id,
                                         const QSharedPointer<HCTIN> &tinSurface,
                                         HydroCouple::Spatial::MeshDataType dataType,
                                         Dimension *timeDimension,
                                         Dimension *cellDimension,
                                         Dimension *edgeDimension,
                                         Dimension *nodeDimension,
                                         ValueDefinition *valueDefinition,
                                         AbstractModelComponent *modelComponent)
  :AbstractOutput(id, QList<Dimension*>({timeDimension, cellDimension, edgeDimension, nodeDimension}),
                  valueDefinition,modelComponent),
    m_dataType(dataType),
    m_timeDimension(timeDimension),
    m_cellDimension(cellDimension),
    m_edgeDimension(edgeDimension),
    m_nodeDimension(nodeDimension),
    m_TINSurface(tinSurface)
{
  m_timeSpan = new SDKTemporal::TimeSpan(this);
  m_patchCount = tinSurface->patchCount();

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
}

TimeTINOutputDouble::~TimeTINOutputDouble()
{
  for(SDKTemporal::DateTime *dt : m_times)
    delete dt;

  m_times.clear();
  m_data.clear();
}

HydroCouple::Spatial::MeshDataType TimeTINOutputDouble::meshDataType() const
{
  return m_dataType;
}

HydroCouple::Spatial::IPolyhedralSurface *TimeTINOutputDouble::polyhedralSurface() const
{
  return m_TINSurface.data();
}

QList<IDateTime*> TimeTINOutputDouble::times() const
{
  QList<IDateTime*> dateTimes;

  for(SDKTemporal::DateTime* dateTime : m_times)
  {
    dateTimes.append(dateTime);
  }

  return dateTimes;
}

IDateTime *TimeTINOutputDouble::time(int timeIndex) const
{
  return m_times[timeIndex];
}

int TimeTINOutputDouble::timeCount() const
{
  return m_times.size();
}

bool TimeTINOutputDouble::addTime(SDKTemporal::DateTime *time)
{
  if(m_times.size() && m_times[m_times.size() - 1]->dateTime() >= time->dateTime())
  {
    return false;
  }

  m_times.push_back(time);
  m_data.push_back(std::vector<double>(m_dataLength , valueDefinitionInternal()->defaultValue().toDouble()));
  resetTimeSpan();
  return true;
}

bool TimeTINOutputDouble::removeTime(SDKTemporal::DateTime *time)
{
   std::vector<SDKTemporal::DateTime*>::iterator index = std::find(m_times.begin(), m_times.end(), time);

  if(index != m_times.end())
  {
    int fi = index - m_times.begin();
    m_times.erase(index);
    m_data.erase(m_data.begin() + fi);
    resetTimeSpan();
    return true;
  }
  else
  {
    return false;
  }
}

void TimeTINOutputDouble::resetTimeSpan()
{
  if(m_times.size())
  {
    double duration = m_times[0]->julianDay() - m_times[m_times.size() -1]->julianDay();
    m_timeSpan->setDateTime(m_times[0]->dateTime());
    m_timeSpan->setDuration(duration);
  }
}

ITimeSpan *TimeTINOutputDouble::timeSpan() const
{
  return m_timeSpan;
}

HydroCouple::IDimension *TimeTINOutputDouble::timeDimension() const
{
  return m_timeDimension;
}

HydroCouple::IDimension *TimeTINOutputDouble::cellDimension() const
{
  return m_cellDimension;
}

HydroCouple::IDimension *TimeTINOutputDouble::edgeDimension() const
{
  return m_edgeDimension;
}

HydroCouple::IDimension *TimeTINOutputDouble::nodeDimension() const
{
  return m_nodeDimension;
}

HydroCouple::Spatial::ITIN *TimeTINOutputDouble::TIN() const
{
  return m_TINSurface.data();
}

int TimeTINOutputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
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

void TimeTINOutputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
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

void TimeTINOutputDouble::getValue(int timeIndex, int cellDimensionIndex,
                                   int edgeDimensionIndex, int nodeDimensionIndex, void *data) const
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

void TimeTINOutputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
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

void TimeTINOutputDouble::setValue(int timeIndex, int cellDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex, const void *data)
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

bool TimeTINOutputDouble::moveData(int fromTimeIndex, int toTimeIndex)
{
  if(fromTimeIndex < (int)m_data.size() && toTimeIndex < (int)m_data.size())
  {
    m_data[toTimeIndex] = m_data[fromTimeIndex];
    return true;
  }

  return false;
}

void TimeTINOutputDouble::moveDataToPrevTime()
{
  if(m_times.size() > 1)
  {
    for(size_t i = 1; i < m_times.size(); i++)
    {
      m_times[i-1]->setJulianDay(m_times[i]->julianDay());
      m_data[i-1] = m_data[i];
    }
  }
}
