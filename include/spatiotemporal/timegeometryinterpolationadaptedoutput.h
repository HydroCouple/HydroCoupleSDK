/*!
 * \file timegeometryinterpolationadaptedoutput.h
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

#ifndef TIMEGEOMETRYADAPTEDOUTPUT_H
#define TIMEGEOMETRYADAPTEDOUTPUT_H

#include "splineinterpolator.h"
#include "hydrocouplespatiotemporal.h"
#include "core/abstractadaptedoutput.h"
#include "core/idbasedargument.h"

#include <vector>

namespace SDKTemporal
{
  class DateTime;
  class TimeSpan;
}

class IdBasedArgumentInt;

class HYDROCOUPLESDK_EXPORT TimeGeometryInterpolationAdaptedOutput:
    public AbstractAdaptedOutput,
    public virtual HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem)

  public:

    TimeGeometryInterpolationAdaptedOutput(const QString &id,
                                           ValueDefinition *valueDefinition,
                                           HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem *adaptee,
                                           AbstractAdaptedOutputFactory *adaptedOutputFactory);

    virtual ~TimeGeometryInterpolationAdaptedOutput() override;

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    void resetTimeSpan();

    HydroCouple::IDimension* timeDimension() const override;

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry *geometry(int geometryIndex) const override;

    HydroCouple::IDimension *geometryDimension() const override;

    HydroCouple::Spatial::IEnvelope *envelope() const override;

    void updateValues(HydroCouple::IInput* querySpecifier) override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, int geometryIndex, void *data) const override;

    void getValues(int timeIndex, int geometryIndex, int timeStride, int geomStride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, int geometryIndex, const void *data) override;

    void setValues(int timeIndex, int geometryIndex, int timeStride, int geomStride, const void *data) override;

    void initialize() override;

    void refresh() override;

    void moveDataToPrevTime();

    SplineInterpolator::InterpolationMethod interpolationMethod() const;

    void setInterpolationMethod(SplineInterpolator::InterpolationMethod interpolationMethod);

  private:

    SplineInterpolator::InterpolationMethod getInterpolationMode(int count);

  private:

    HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem *m_adaptee;
    IdBasedArgumentInt* m_interpolationOptionsArgument;
    Dimension *m_timeDimension, *m_geometryDimension;
    SplineInterpolator::InterpolationMethod m_interpolationMode;
    int m_polynomialOrder, m_timeCacheSize;
    std::vector<SDKTemporal::DateTime*> m_dateTimes;
    std::vector<std::vector<double>> m_data;
    SDKTemporal::TimeSpan *m_timeSpan;
    std::vector<std::vector<double>> m_bufferedValues;
    std::vector<double> m_bufferedDateTimes;

};


#endif // TIMEGEOMETRYADAPTEDOUTPUT_H
