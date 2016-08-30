#ifndef GEOMETRYCOLLECTION_H
#define GEOMETRYCOLLECTION_H

#include "geometry.h"

class HCLineString;
class HCPoint;
class HCPolygon;

class HYDROCOUPLESDK_EXPORT HCGeometryCollection : public HCGeometry,
    public virtual HydroCouple::Spatial::IGeometryCollection
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IGeometryCollection)

  public:

    HCGeometryCollection(QObject* parent = nullptr);

    virtual ~HCGeometryCollection();

    HydroCouple::Spatial::GeometryType geometryType() const override;

    int dimension() const override;

    HydroCouple::Spatial::IGeometry *envelope() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry* geometry(int index) const override;

    void initializeData(int length, double defaultValue = std::numeric_limits<double>::quiet_NaN()) override;

    void addGeometry(HCGeometry *geometry);

    bool removeGeometry(HCGeometry *geometry);

    void enable3D() override;

    void disable3D() override;

    void enableM() override;

    void disableM() override;

  signals:
    void propertyChanged(const QString& propertyName) override;

  protected:
    QList<HCGeometry*> geometries() const;

  private:
    QList<HCGeometry*> m_geometries;
};


class HYDROCOUPLESDK_EXPORT HCMultiPoint : public HCGeometryCollection,
    public virtual HydroCouple::Spatial::IMultiPoint
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IMultiPoint)

  public:

    HCMultiPoint(QObject *parent);

    virtual ~HCMultiPoint();

    HydroCouple::Spatial::GeometryType geometryType() const override;

    HydroCouple::Spatial::IPoint* point(int index) const override;

    void addPoint(HCPoint *point);

    bool removePoint(HCPoint *point);
};


class HYDROCOUPLESDK_EXPORT HCMultiLineString : public HCGeometryCollection,
    public virtual HydroCouple::Spatial::IMultiCurve,
    public virtual HydroCouple::Spatial::IMultiLineString
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IMultiCurve
                 HydroCouple::Spatial::IMultiLineString)

  public:

    HCMultiLineString(QObject* parent = nullptr);

    virtual ~HCMultiLineString();

    HydroCouple::Spatial::GeometryType geometryType() const override;

    bool isClosed() const override;

    double length() const override;

    HydroCouple::Spatial::ILineString* element(int index) const override;

    QList<HCLineString*> lineStrings() const;

    void addLineString(HCLineString* lineString);

    bool removeLineString(HCLineString *lineString);

  private:

    QList<HCLineString*> m_lineStrings;

};

class HCMultiPolygon : public HCGeometryCollection,
    public virtual HydroCouple::Spatial::IMultiSurface,
    public virtual HydroCouple::Spatial::IMultiPolygon
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IMultiSurface
                 HydroCouple::Spatial::IMultiPolygon)

  public:
    HCMultiPolygon(QObject *parent);

    virtual ~HCMultiPolygon();

    HydroCouple::Spatial::GeometryType geometryType() const override;

    double area() const override;

    HydroCouple::Spatial::IPoint* centroid() const override;

    HydroCouple::Spatial::IPoint* pointOnSurface() const override;

    HydroCouple::Spatial::IPolygon *element(int index) const override;

    void addPolygon(HCPolygon *polygon);

    bool removePolygon(HCPolygon *polygon);

   private:

    QList<HCPolygon*> m_polygons;
};

Q_DECLARE_METATYPE(HCGeometryCollection*)
Q_DECLARE_METATYPE(HCMultiPoint*)
Q_DECLARE_METATYPE(HCMultiLineString*)

#endif // GEOMETRYCOLLECTION_H

