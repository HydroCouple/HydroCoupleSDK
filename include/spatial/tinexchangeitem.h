/*!
 * \file tinexchangeitem.h
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

#ifndef TINEXCHANGEITEM_H
#define TINEXCHANGEITEM_H

#include "core/abstractoutput.h"
#include "hydrocouplespatial.h"

class HCPolyhedralSurface;
class HCTIN;

class HYDROCOUPLESDK_EXPORT TINOutputDouble : public AbstractOutput,
        public virtual HydroCouple::Spatial::ITINComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::ITINComponentDataItem)

public:

    TINOutputDouble(const QString &id,
                    HydroCouple::Spatial::MeshDataType dataType,
                    const QSharedPointer<HCTIN> &TINSurface,
                    Dimension *cellDimension,
                    Dimension *edgeDimension,
                    Dimension *nodeDimension,
                    ValueDefinition *valueDefinition,
                    AbstractModelComponent *modelComponent);

    virtual ~TINOutputDouble();

    HydroCouple::Spatial::MeshDataType meshDataType() const override;

    HydroCouple::Spatial::IPolyhedralSurface *polyhedralSurface() const override;

    HydroCouple::IDimension *cellDimension() const override;

    HydroCouple::IDimension *edgeDimension() const override;

    HydroCouple::IDimension *nodeDimension() const override;

    HydroCouple::Spatial::ITIN *TIN() const override;

    QSharedPointer<HCTIN> sharedTIN() const;

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void getValue(int cellDimensionIndex, int faceDimensionIndex, int nodeDimensionIndex, void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int cellDimensionIndex, int faceDimensionIndex, int nodeDimensionIndex, const void *data) override;

    virtual void setTIN(const QSharedPointer<HCTIN> &TINSurface);

    virtual void resetDataArray();

    void updateValues() override;

private:
    HydroCouple::Spatial::MeshDataType m_dataType;
    Dimension *m_cellDimension, *m_edgeDimension, *m_nodeDimension;
    std::vector<double> m_data;
    QSharedPointer<HCTIN> m_TINSurface;
};

Q_DECLARE_METATYPE(TINOutputDouble*)

#endif // TINEXCHANGEITEM_H
