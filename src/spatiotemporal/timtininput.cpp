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

TimeTINInputDouble::TimeTINInputDouble(const QString &id,
                                         const QSharedPointer<HCTIN> &tinSurface,
                                         HydroCouple::Spatial::MeshDataType dataType,
                                         Dimension *timeDimension,
                                         Dimension *cellDimension,
                                         Dimension *edgeDimension,
                                         Dimension *nodeDimension,
                                         ValueDefinition *valueDefinition,
                                         AbstractModelComponent *modelComponent)
  :AbstractInput(id, QList<Dimension*>({timeDimension, cellDimension, edgeDimension, nodeDimension}),
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

TimeTINInputDouble::~TimeTINInputDouble()
{

}

HydroCouple::Spatial::MeshDataType TimeTINInputDouble::meshDataType() const
{
  return m_dataType;
}

HydroCouple::Spatial::IPolyhedralSurface *TimeTINInputDouble::polyhedralSurface() const
{
  return m_TINSurface.data();
}

QList<IDateTime*> TimeTINInputDouble::times() const
{
  QList<IDateTime*> dateTimes;

  for(SDKTemporal::DateTime* dateTime : m_times)
  {
    dateTimes.append(dateTime);
  }

  return dateTimes;
}

IDateTime *TimeTINInputDouble::time(int timeIndex) const
{
  return m_times[timeIndex];
}

int TimeTINInputDouble::timeCount() const
{
  return m_times.size();
}

bool TimeTINInputDouble::addTime(SDKTemporal::DateTime *time)
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

bool TimeTINInputDouble::removeTime(SDKTemporal::DateTime *time)
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

void TimeTINInputDouble::resetTimeSpan()
{
  if(m_times.size())
  {
    double duration = m_times[0]->modifiedJulianDay() - m_times[m_times.size() -1]->modifiedJulianDay();
    m_timeSpan->setDateTime(m_times[0]->dateTime());
    m_timeSpan->setDuration(duration);
  }
}

ITimeSpan *TimeTINInputDouble::timeSpan() const
{
  return m_timeSpan;
}

HydroCouple::IDimension *TimeTINInputDouble::timeDimension() const
{
  return m_timeDimension;
}

HydroCouple::IDimension *TimeTINInputDouble::cellDimension() const
{
  return m_cellDimension;
}

HydroCouple::IDimension *TimeTINInputDouble::edgeDimension() const
{
  return m_edgeDimension;
}

HydroCouple::IDimension *TimeTINInputDouble::nodeDimension() const
{
  return m_nodeDimension;
}

HydroCouple::Spatial::ITIN *TimeTINInputDouble::TIN() const
{
  return m_TINSurface.data();
}

int TimeTINInputDouble::dimensionLength(const std::vector<int> &dimensionIndexes) const
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

void TimeTINInputDouble::getValue(const std::vector<int> &dimensionIndexes, void *data) const
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

void TimeTINInputDouble::getValue(int timeIndex, int cellDimensionIndex,
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

void TimeTINInputDouble::setValue(const std::vector<int> &dimensionIndexes, const void *data)
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

void TimeTINInputDouble::setValue(int timeIndex, int cellDimensionIndex, int edgeDimensionIndex, int nodeDimensionIndex, const void *data)
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

bool TimeTINInputDouble::moveData(int fromTimeIndex, int toTimeIndex)
{
  if(fromTimeIndex < (int) m_data.size() && toTimeIndex < (int)m_data.size())
  {
    m_data[toTimeIndex] = m_data[fromTimeIndex];
    return true;
  }

  return false;
}

void TimeTINInputDouble::moveDataToPrevTime()
{
  if(m_times.size() > 1)
  {
    for(size_t i = 1; i < m_times.size(); i++)
    {
      m_times[i-1]->setModifiedJulianDay(m_times[i]->modifiedJulianDay());
      m_data[i-1] = m_data[i];
    }
  }
}
