/*!
 * \file timegeometryargument.h
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

#ifndef TIMEGEOMETRYARGUMENT_H
#define TIMEGEOMETRYARGUMENT_H

#include "core/abstractargument.h"
#include "timegeometrycomponentdataitem.h"
#include "hydrocouplespatiotemporal.h"

#include <QFileInfo>

class HYDROCOUPLESDK_EXPORT TimeGeometryArgumentDouble: public AbstractArgument,
    public TimeGeometryComponentDataItem<double>,
    public virtual HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Temporal::ITimeComponentDataItem
                 HydroCouple::SpatioTemporal::ITimeGeometryComponentDataItem)

  public:

    TimeGeometryArgumentDouble(const QString &id,
                               HydroCouple::Spatial::IGeometry::GeometryType geometryType,
                               Dimension *timeDimension,
                               Dimension *geometryDimension,
                               ValueDefinition *valueDefinition,
                               AbstractModelComponent *modelComponent);

    virtual ~TimeGeometryArgumentDouble();

    QList<HydroCouple::Temporal::IDateTime*> times() const override;

    HydroCouple::Temporal::IDateTime *time(int timeIndex) const override;

    int timeCount() const override;

    HydroCouple::Temporal::ITimeSpan* timeSpan() const override;

    HydroCouple::IDimension* timeDimension() const override;

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry *geometry(int geometryIndex) const override;

    HydroCouple::IDimension *geometryDimension() const override;

    HydroCouple::Spatial::IEnvelope *envelope() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes, void *data) const override;

    void getValue(int timeIndex, int geometryIndex, void *data) const override;

    void getValues(int timeIndex, int geometryIndex, int timeStride, int geomStride, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int timeIndex, int geometryIndex, const void *data) override;

    void setValues(int timeIndex, int geometryIndex, int timeStride, int geomStride, const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;

    void saveData() override;

    QString toString() const override;

    bool readValues(const QString &value, QString &message, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem, QString &message) override;


  private:

    QString m_gdalDriverName;
    QFileInfo m_geometryFile, m_timeSeriesFile;
    Dimension *m_timeDimension, *m_geometryDimension;
    bool m_readGeometryFromFile,m_readTimeSeriesFromFile,
    m_overwriteGeometryFile, m_overwriteTimeSeriesFile;
};

Q_DECLARE_METATYPE(TimeGeometryArgumentDouble*)

#endif // TIMEGEOMETRYARGUMENT_H
