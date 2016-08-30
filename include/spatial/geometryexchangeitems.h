#ifndef GEOMETRYEXCHANGEITEMS_H
#define GEOMETRYEXCHANGEITEMS_H

#include "core/exchangeitems1d.h"
#include "core/abstractoutput.h"
#include "hydrocouplespatial.h"

class HCGeometry;

class HYDROCOUPLESDK_EXPORT HCGeometryInputDouble  : public AbstractInput,
    public ComponentDataItem1D<double>,
    public virtual HydroCouple::Spatial::IGeometryExchangeItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IGeometryComponentDataItem
                 HydroCouple::Spatial::IGeometryExchangeItem)

  public:

    HCGeometryInputDouble(const QString& id,
                          HydroCouple::Spatial::GeometryType geomType,
                          Dimension *geometryDimesion,
                          ValueDefinition* valueDefinition,
                          AbstractModelComponent *modelComponent);

    virtual ~HCGeometryInputDouble();

    HydroCouple::Spatial::GeometryType geometryType() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry *geometry(int geometryIndex) const override;

    virtual HydroCouple::IDimension *geometryDimension() const override;

    void addGeometry(HCGeometry *geometry);

    void addGeometries(QList<HCGeometry*> geometries);

    bool removeGeometry(HCGeometry *geometry);

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int geometryDimensionIndex, QVariant &data) const override;

    void getValues(int geometryDimensionIndex, int stride,  QVariant data[]) const override;

    void getValues(int geometryDimensionIndex, int stride,  void *data) const override;

    void setValue(int geometryDimensionIndex, const QVariant &data) override;

    void setValues(int geometryDimensionIndex, int stride, const QVariant data[]) override;

    void setValues(int geometryDimensionIndex , int stride, const void* data) override;

    QList<HCGeometry*> geometries() const;

  private:
    HydroCouple::Spatial::GeometryType m_geometryType;
    QList<HCGeometry*> m_geometries;
};

//==============================================================================================================================

class HYDROCOUPLESDK_EXPORT HCGeometryOutputDouble  : public AbstractOutput,
    public ComponentDataItem1D<double>,
    public virtual HydroCouple::Spatial::IGeometryExchangeItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IGeometryComponentDataItem
                 HydroCouple::Spatial::IGeometryExchangeItem)

  public:

    HCGeometryOutputDouble(const QString& id,
                          HydroCouple::Spatial::GeometryType geomType,
                          Dimension *geometryDimesion,
                          ValueDefinition* valueDefinition,
                          AbstractModelComponent *modelComponent);

    virtual ~HCGeometryOutputDouble();

    HydroCouple::Spatial::GeometryType geometryType() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry *geometry(int geometryIndex) const override;

    virtual HydroCouple::IDimension *geometryDimension() const override;

    void addGeometry(HCGeometry *geometry);

    void addGeometries(QList<HCGeometry*> geometries);

    bool removeGeometry(HCGeometry *geometry);

    int dimensionLength(int dimensionIndexes[] , int dimensionIndexesLength) const override;

    void getValue(int dimensionIndexes[], QVariant &data) const override;

    void getValues(int dimensionIndexes[], int stride[],  QVariant data[]) const override;

    void getValues(int dimensionIndexes[], int stride[],  void *data) const override;

    void setValue(int dimensionIndexes[], const QVariant &data) override;

    void setValues(int dimensionIndexes[], int stride[], const QVariant data[]) override;

    void setValues(int dimensionIndexes[], int stride[], const void *data) override;

    void getValue(int geometryDimensionIndex, QVariant &data) const override;

    void getValues(int geometryDimensionIndex, int stride,  QVariant data[]) const override;

    void getValues(int geometryDimensionIndex, int stride,  void *data) const override;

    void setValue(int geometryDimensionIndex, const QVariant &data) override;

    void setValues(int geometryDimensionIndex, int stride, const QVariant data[]) override;

    void setValues(int geometryDimensionIndex , int stride, const void* data) override;

    QList<HCGeometry*> geometries() const;

  private:
    HydroCouple::Spatial::GeometryType m_geometryType;
    QList<HCGeometry*> m_geometries;
};

Q_DECLARE_METATYPE(HCGeometryInputDouble*)
Q_DECLARE_METATYPE(HCGeometryOutputDouble*)

#endif // GEOMETRYEXCHANGEITEMS_H
