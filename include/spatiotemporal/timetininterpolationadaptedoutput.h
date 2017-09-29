/*!
 *  \file    timetininterpolationadaptedoutput.h
 *  \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version 1.0.0.0
 *  \section Description
 *  \section License
 *  timetininterpolationadaptedoutput.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  AbstractAdaptedOutputFactoryComponentInfo.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2016
 *  \pre
 *  \bug
 *  \todo
 *  \warning
 */

#ifndef TIMETININTERPOLATIONADAPTEDOUTPUT_H
#define TIMETININTERPOLATIONADAPTEDOUTPUT_H

#include "splineinterpolator.h"
#include "hydrocouplespatiotemporal.h"
#include "core/idbasedargument.h"
#include "core/abstractadaptedoutput.h"

namespace SDKTemporal
{
  class DateTime;
  class TimeSpan;
}

class HYDROCOUPLESDK_EXPORT TimeTINInterpolationAdaptedOutput:
    public AbstractAdaptedOutput,
    public virtual HydroCouple::SpatioTemporal::ITimeTINComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::SpatioTemporal::ITimeTINComponentDataItem)

  public:

    TimeTINInterpolationAdaptedOutput(const QString &id,
                                      ValueDefinition *valueDefinition,
                                      HydroCouple::SpatioTemporal::ITimeTINComponentDataItem *adaptee,
                                      AbstractAdaptedOutputFactory *adaptedOutputFactory);

    virtual ~TimeTINInterpolationAdaptedOutput();

    HydroCouple::Spatial::MeshDataType meshDataType() const override;

    HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface() const override;

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    void resetTimeSpan();

    HydroCouple::Temporal::ITimeSpan *timeSpan() const override;

    HydroCouple::IDimension *timeDimension() const override;

    HydroCouple::IDimension *cellDimension() const override;

    HydroCouple::IDimension *edgeDimension() const override;

    HydroCouple::IDimension *nodeDimension() const override;

    HydroCouple::Spatial::ITIN *TIN() const override;

    void updateValues(HydroCouple::IInput* querySpecifier) override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, int cellDimensionIndex,
                  int edgeDimensionIndex, int nodeDimensionIndex, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, int cellDimensionIndex,
                  int edgeDimensionIndex, int nodeDimensionIndex,  const void *data) override;

    void initialize() override;

    void refresh() override;

    void moveDataToPrevTime();

    SplineInterpolator::InterpolationMethod interpolationMethod() const;

    void setInterpolationMethod(SplineInterpolator::InterpolationMethod interpolationMethod);

  private:

    SplineInterpolator::InterpolationMethod getInterpolationMode(int count);

  private:
    IdBasedArgumentInt* m_interpolationOptionsArgument;
    HydroCouple::SpatioTemporal::ITimeTINComponentDataItem *m_adaptee;
    HydroCouple::Spatial::MeshDataType m_dataType;
    int m_polynomialOrder, m_timeCacheSize;
    Dimension *m_timeDimension,
    *m_cellDimension,
    *m_edgeDimension,
    *m_nodeDimension;
    std::vector<SDKTemporal::DateTime*> m_times;
    SDKTemporal::TimeSpan *m_timeSpan;
    std::vector<std::vector<double>> m_bufferedValues;
    std::vector<double> m_bufferedDateTimes;
    std::vector<std::vector<double>> m_data;
    int m_dataLength, m_patchCount;
    SplineInterpolator::InterpolationMethod m_interpolationMode;

};

#endif // TIMETININTERPOLATIONADAPTEDOUTPUT_H
