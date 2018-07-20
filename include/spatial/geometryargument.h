/*!
 * \file geometryargument.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
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

#ifndef GEOMETRYARGUMENT_H
#define GEOMETRYARGUMENT_H

#include "core/abstractargument.h"
#include "spatial/geometrycomponentdataitem.h"
#include "hydrocouplespatial.h"

#include <QFileInfo>

class HCGeometry;
class Envelope;

class HYDROCOUPLESDK_EXPORT GeometryArgumentDouble : public AbstractArgument,
    public GeometryComponentDataItem<double>,
    public virtual HydroCouple::Spatial::IGeometryComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IGeometryComponentDataItem)


  public:

    GeometryArgumentDouble(const QString& id,
                           HydroCouple::Spatial::IGeometry::GeometryType geometryType,
                           Dimension *geometryDimension,
                           ValueDefinition *valueDefinition,
                           AbstractModelComponent *modelComponent);

    virtual ~GeometryArgumentDouble();

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry *geometry(int geometryIndex) const override;

    HydroCouple::IDimension *geometryDimension() const override;

    HydroCouple::Spatial::IEnvelope *envelope() const override;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void getValue(int geometryDimensionIndex, void *data) const override;

    void getValues(int geometryDimensionIndex, int stride,  void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int geometryDimensionIndex, const void *data) override;

    void setValues(int geometryDimensionIndex , int stride, const void* data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;

    void saveData() override;

    QString toString() const override;

    bool readValues(const QString &value, QString &message, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem, QString &message) override;

    QList<QSharedPointer<HCGeometry>> geometries() const;

  signals:

    void propertyChanged(const QString &propertyName) override;

  private:
    QString m_gdalDriverName;
    QFileInfo m_geometryFile;
    QString m_geometryFileDataField;
    bool m_readFromFile, m_saveFile;
    Dimension *m_geometryDimension;
};

Q_DECLARE_METATYPE(GeometryArgumentDouble*)

#endif // GEOMETRYARGUMENT_H
