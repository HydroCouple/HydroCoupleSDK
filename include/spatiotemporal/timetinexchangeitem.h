/*!
 * \file timetinexchangeitem.h
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

#ifndef TIMETINEXCHANGEITEM_H
#define TIMETINEXCHANGEITEM_H

#include "core/abstractoutput.h"
#include "core/abstractinput.h"
#include "hydrocouplespatiotemporal.h"


class HCTIN;

namespace SDKTemporal
{
  class DateTime;
  class TimeSpan;
}

class HYDROCOUPLESDK_EXPORT TimeTINOutputDouble : public AbstractOutput,
    public virtual HydroCouple::SpatioTemporal::ITimeTINComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::SpatioTemporal::ITimePolyhedralSurfaceComponentDataItem
                 HydroCouple::SpatioTemporal::ITimeTINComponentDataItem)

    using HydroCouple::IComponentDataItem::getValue;
    using HydroCouple::IComponentDataItem::setValue;

  public:

    TimeTINOutputDouble(const QString &id,
                        const QSharedPointer<HCTIN> &tinSurface,
                        HydroCouple::Spatial::MeshDataType dataType,
                        Dimension *timeDimension,
                        Dimension *cellDimension,
                        Dimension *edgeDimension,
                        Dimension *nodeDimension,
                        ValueDefinition *valueDefinition,
                        AbstractModelComponent *modelComponent);

    virtual ~TimeTINOutputDouble();

    HydroCouple::Spatial::MeshDataType meshDataType() const override;

    HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface() const override;

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    bool addTime(SDKTemporal::DateTime* time);

    bool removeTime(SDKTemporal::DateTime *time);

    void resetTimeSpan();

    HydroCouple::Temporal::ITimeSpan *timeSpan() const override;

    HydroCouple::IDimension *timeDimension() const override;

    HydroCouple::IDimension *cellDimension() const override;

    HydroCouple::IDimension *edgeDimension() const override;

    HydroCouple::IDimension *nodeDimension() const override;

    HydroCouple::Spatial::ITIN *TIN() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, int cellDimensionIndex,
                  int edgeDimensionIndex, int nodeDimensionIndex, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, int cellDimensionIndex,
                  int edgeDimensionIndex, int nodeDimensionIndex,  const void *data) override;

    bool moveData(int fromTimeIndex , int toTimeIndex);

    void moveDataToPrevTime();

  protected:

    SDKTemporal::DateTime* timeInternal(int timeIndex) const;

  protected:

    std::vector<SDKTemporal::DateTime*> m_times;

  private:
    HydroCouple::Spatial::MeshDataType m_dataType;
    Dimension *m_timeDimension,
    *m_cellDimension,
    *m_edgeDimension,
    *m_nodeDimension;
    QSharedPointer<HCTIN> m_TINSurface;
    SDKTemporal::TimeSpan *m_timeSpan;
    std::vector<std::vector<double>> m_data;
    int m_dataLength, m_patchCount;
};


class HYDROCOUPLESDK_EXPORT TimeTINInputDouble : public AbstractInput,
    public virtual HydroCouple::SpatioTemporal::ITimeTINComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::SpatioTemporal::ITimePolyhedralSurfaceComponentDataItem
                 HydroCouple::SpatioTemporal::ITimeTINComponentDataItem)

    using HydroCouple::IComponentDataItem::getValue;
    using HydroCouple::IComponentDataItem::setValue;

  public:

    TimeTINInputDouble(const QString &id,
                       const QSharedPointer<HCTIN> &tinSurface,
                       HydroCouple::Spatial::MeshDataType dataType,
                       Dimension *timeDimension,
                       Dimension *cellDimension,
                       Dimension *edgeDimension,
                       Dimension *nodeDimension,
                       ValueDefinition *valueDefinition,
                       AbstractModelComponent *modelComponent);

    virtual ~TimeTINInputDouble();

    HydroCouple::Spatial::MeshDataType meshDataType() const override;

    HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface() const override;

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    bool addTime(SDKTemporal::DateTime* time);

    bool removeTime(SDKTemporal::DateTime *time);

    void resetTimeSpan();

    HydroCouple::Temporal::ITimeSpan *timeSpan() const override;

    HydroCouple::IDimension *timeDimension() const override;

    HydroCouple::IDimension *cellDimension() const override;

    HydroCouple::IDimension *edgeDimension() const override;

    HydroCouple::IDimension *nodeDimension() const override;

    HydroCouple::Spatial::ITIN *TIN() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, int cellDimensionIndex,
                  int edgeDimensionIndex, int nodeDimensionIndex, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, int cellDimensionIndex,
                  int edgeDimensionIndex, int nodeDimensionIndex,  const void *data) override;

    bool moveData(int fromTimeIndex , int toTimeIndex);

    void moveDataToPrevTime();

  protected:

    SDKTemporal::DateTime* timeInternal(int timeIndex) const;

  protected:

    std::vector<SDKTemporal::DateTime*> m_times;

  private:
    HydroCouple::Spatial::MeshDataType m_dataType;
    Dimension *m_timeDimension,
    *m_cellDimension,
    *m_edgeDimension,
    *m_nodeDimension;
    QSharedPointer<HCTIN> m_TINSurface;
    SDKTemporal::TimeSpan *m_timeSpan;
    std::vector<std::vector<double>> m_data;
    int m_dataLength, m_patchCount;
};


class HYDROCOUPLESDK_EXPORT TimeTINMultiInputDouble : public AbstractMultiInput,
    public virtual HydroCouple::SpatioTemporal::ITimeTINComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::SpatioTemporal::ITimePolyhedralSurfaceComponentDataItem
                 HydroCouple::SpatioTemporal::ITimeTINComponentDataItem)

    using HydroCouple::IComponentDataItem::getValue;
    using HydroCouple::IComponentDataItem::setValue;

  public:

    TimeTINMultiInputDouble(const QString &id,
                            const QSharedPointer<HCTIN> &tinSurface,
                            HydroCouple::Spatial::MeshDataType dataType,
                            Dimension *timeDimension,
                            Dimension *cellDimension,
                            Dimension *edgeDimension,
                            Dimension *nodeDimension,
                            ValueDefinition *valueDefinition,
                            AbstractModelComponent *modelComponent);

    virtual ~TimeTINMultiInputDouble();

    HydroCouple::Spatial::MeshDataType meshDataType() const override;

    HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface() const override;

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    bool addTime(SDKTemporal::DateTime* time);

    bool removeTime(SDKTemporal::DateTime *time);

    void resetTimeSpan();

    HydroCouple::Temporal::ITimeSpan *timeSpan() const override;

    HydroCouple::IDimension *timeDimension() const override;

    HydroCouple::IDimension *cellDimension() const override;

    HydroCouple::IDimension *edgeDimension() const override;

    HydroCouple::IDimension *nodeDimension() const override;

    HydroCouple::Spatial::ITIN *TIN() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, int cellDimensionIndex,
                  int edgeDimensionIndex, int nodeDimensionIndex, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, int cellDimensionIndex,
                  int edgeDimensionIndex, int nodeDimensionIndex,  const void *data) override;

    bool moveData(int fromTimeIndex , int toTimeIndex);

    void moveDataToPrevTime();

  protected:

    SDKTemporal::DateTime* timeInternal(int timeIndex) const;

  protected:

    std::vector<SDKTemporal::DateTime*> m_times;

  private:
    HydroCouple::Spatial::MeshDataType m_dataType;
    Dimension *m_timeDimension,
    *m_cellDimension,
    *m_edgeDimension,
    *m_nodeDimension;
    QSharedPointer<HCTIN> m_TINSurface;
    SDKTemporal::TimeSpan *m_timeSpan;
    std::vector<std::vector<double>> m_data;
    int m_dataLength, m_patchCount;
};
#endif // TIMETINEXCHANGEITEM_H
