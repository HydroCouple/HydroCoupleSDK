/*!
 *  \file    geometryexchangeitems.h
 *  \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version 1.0.0.0
 *  \section Description
 *  \section License
 *  geometryexchangeitems.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  abstractadaptedoutput.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2017
 *  \pre
 *  \bug
 *  \todo
 *  \warning
 *  \copyright Copyright 2017 Caleb Buahin
 */

#ifndef GEOMETRYEXCHANGEITEMS_H
#define GEOMETRYEXCHANGEITEMS_H

#include "core/exchangeitems1d.h"
#include "core/abstractoutput.h"
#include "hydrocouplespatial.h"

class HCGeometry;
class Envelope;


//typedef void (*NotifyGeometryListChanged) (const GeometryInputDouble*);

class HYDROCOUPLESDK_EXPORT GeometryInputDouble  : public AbstractInput,
    public ComponentDataItem1D<double>,
    public virtual HydroCouple::Spatial::IGeometryComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IGeometryComponentDataItem)

  public:

    GeometryInputDouble(const QString& id,
                        HydroCouple::Spatial::IGeometry::GeometryType geomType,
                        Dimension *geometryDimesion,
                        ValueDefinition* valueDefinition,
                        AbstractModelComponent *modelComponent);

    virtual ~GeometryInputDouble();

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry *geometry(int geometryIndex) const override;

    HydroCouple::IDimension *geometryDimension() const override;

    HydroCouple::Spatial::IEnvelope *envelope() const override;

    virtual void addGeometry(const QSharedPointer<HCGeometry> &geometry);

    virtual void addGeometries(const QList<QSharedPointer<HCGeometry>> &geometries);

    virtual bool removeGeometry(const QSharedPointer<HCGeometry> &geometry);

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void getValue(int geometryDimensionIndex, void *data) const override;

    void getValues(int geometryDimensionIndex, int stride,  void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int geometryDimensionIndex, const void *data) override;

    void setValues(int geometryDimensionIndex , int stride, const void* data) override;

    QList<QSharedPointer<HCGeometry>> geometries() const;

  private:
    HydroCouple::Spatial::IGeometry::GeometryType m_geometryType;
    QList<QSharedPointer<HCGeometry>> m_geometries;
    Envelope *m_envelope;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT GeometryOutputDouble  : public AbstractOutput,
    public ComponentDataItem1D<double>,
    public virtual HydroCouple::Spatial::IGeometryComponentDataItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IGeometryComponentDataItem)

  public:

    GeometryOutputDouble(const QString& id,
                         HydroCouple::Spatial::IGeometry::GeometryType geomType,
                         Dimension *geometryDimesion,
                         ValueDefinition* valueDefinition,
                         AbstractModelComponent *modelComponent);

    virtual ~GeometryOutputDouble();

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry *geometry(int geometryIndex) const override;

    HydroCouple::IDimension *geometryDimension() const override;

    HydroCouple::Spatial::IEnvelope *envelope() const override;

    void addGeometry(const QSharedPointer<HCGeometry> &geometry);

    void addGeometries(const QList<QSharedPointer<HCGeometry>> &geometries);

    bool removeGeometry(const QSharedPointer<HCGeometry> &geometry);

    int dimensionLength(const std::vector<int> &dimensionIndexes) const override;

    void getValue(const std::vector<int> &dimensionIndexes,  void *data) const override;

    void getValue(int geometryDimensionIndex, void *data) const override;

    void getValues(int geometryDimensionIndex, int stride,  void *data) const override;

    void setValue(const std::vector<int> &dimensionIndexes, const void *data) override;

    void setValue(int geometryDimensionIndex, const void *data) override;

    void setValues(int geometryDimensionIndex , int stride, const void* data) override;

    QList<QSharedPointer<HCGeometry>> geometries() const;

    virtual void updateValues() override;

  private:
    HydroCouple::Spatial::IGeometry::GeometryType m_geometryType;
    QList<QSharedPointer<HCGeometry>> m_geometries;
    Envelope *m_envelope;
};

Q_DECLARE_METATYPE(GeometryInputDouble*)
Q_DECLARE_METATYPE(GeometryOutputDouble*)

#endif // GEOMETRYEXCHANGEITEMS_H
