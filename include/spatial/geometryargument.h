#ifndef GEOMETRYARGUMENT_H
#define GEOMETRYARGUMENT_H

#include "core/abstractargument.h"
#include "core/exchangeitems1d.h"
#include "hydrocouplespatial.h"

#include <QFileInfo>

class HCGeometryCollection;
class QXmlStreamReader;
class HCGeometry;

class HYDROCOUPLESDK_EXPORT HCGeometryArgumentDouble : public AbstractArgument,
    public ComponentDataItem1D<double>,
    public virtual HydroCouple::Spatial::IGeometryArgument
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IGeometryComponentDataItem
                 HydroCouple::Spatial::IGeometryArgument)

  public:

    HCGeometryArgumentDouble(const QString& id,
                             HydroCouple::Spatial::GeometryType geometryType,
                             Dimension *geometryDimension,
                             ValueDefinition *valueDefinition,
                             AbstractModelComponent *modelComponent);

    virtual ~HCGeometryArgumentDouble();

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

    void readData(QXmlStreamReader &xmlReader) override;

    void writeData(QXmlStreamWriter &xmlWriter) override;

    bool writeToFile() const override;

    QString toString() const override;

    bool readValues(const QString &value, bool isFile = false) override;

    bool readValues(const HydroCouple::IComponentDataItem* componentDataItem) override;

    QList<HCGeometry*> geometries() const;

  private:
    HydroCouple::Spatial::GeometryType m_geometryType;
    QList<HCGeometry*> m_geometries;
    QString m_gdalDriverName;
    QFileInfo m_geometryFile;
    QString m_geometryFileDataField;
    bool m_writeToFile, m_overWriteFile;
    Dimension *m_geometryDimension;
};

Q_DECLARE_METATYPE(HCGeometryArgumentDouble*)

#endif // GEOMETRYARGUMENT_H
