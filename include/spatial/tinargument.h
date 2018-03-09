/*!
 * \file tinargument.h
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


#ifndef TINARGUMENT_H
#define TINARGUMENT_H

#include "hydrocouplesdk.h"
#include "core/abstractargument.h"
#include "hydrocouplespatial.h"

#include <QFileInfo>
#include <vector>
#include <QSharedPointer>

class HYDROCOUPLESDK_EXPORT HCTIN;

/*!
 * \brief The TINArgumentDouble class
 * \todo Read associated data from NetCDF
 */
class HYDROCOUPLESDK_EXPORT TINArgumentDouble : public AbstractArgument,
    public virtual HydroCouple::Spatial::ITINComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IPolyhedralSurfaceComponentDataItem
                 HydroCouple::Spatial::ITINComponentDataItem)

  public:
    TINArgumentDouble(const QString &id,
                      HydroCouple::Spatial::MeshDataType dataType,
                      Dimension *cellDimension,
                      Dimension *edgeDimension,
                      Dimension *nodeDimension,
                      ValueDefinition *valueDefinition,
                      AbstractModelComponent *modelComponent);

    virtual ~TINArgumentDouble();

    HydroCouple::Spatial::ITIN *TIN() const override;

    HCTIN *TINInternal() const;

    HydroCouple::Spatial::MeshDataType meshDataType() const override;

    HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface() const override;

    HydroCouple::IDimension *cellDimension() const override;

    HydroCouple::IDimension *edgeDimension() const override;

    HydroCouple::IDimension *nodeDimension() const override;

    QSharedPointer<HCTIN> sharedTIN() const;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void getValue(int cellDimensionIndex, int faceDimensionIndex, int nodeDimensionIndex, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int cellDimensionIndex, int faceDimensionIndex, int nodeDimensionIndex, const void *data) override;

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) const override;

    void saveData() override;

    QString toString() const override;

    bool readValues(const QString &value, QString &message, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem, QString &message) override;

    virtual void resetDataArray();


  private:
    HydroCouple::Spatial::MeshDataType m_dataType;
    Dimension *m_cellDimension, *m_edgeDimension, *m_nodeDimension;
    std::vector<double> m_data;
    QSharedPointer<HCTIN> m_TINSurface;
    QFileInfo m_netCDFFile;
};

#endif // TINARGUMENT_H
