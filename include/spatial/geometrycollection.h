#ifndef GEOMETRYCOLLECTION_H
#define GEOMETRYCOLLECTION_H

#include "geometry.h"

class HCLineString;
class HCPoint;

class HYDROCOUPLESDK_EXPORT HCGeometryCollection : public HCGeometry,
    public virtual HydroCouple::Spatial::IGeometryCollection
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IGeometryCollection)

  public:

    HCGeometryCollection(QObject* parent = nullptr);

    virtual ~HCGeometryCollection();

    HydroCouple::Spatial::GeometryType geometryType() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry* geometry(int index) const override;

  protected:

    QList<HCGeometry*> geometries() const;

    void addGeometry(HCGeometry* geometry);

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

    void addPoint(HCPoint* point);

    virtual void enable3D();

    virtual void disable3D();

    virtual void enableM();

    virtual void disableM();
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

    virtual void enable3D();

    virtual void disable3D();

    virtual void enableM();

    virtual void disableM();

  private:

    QList<HCLineString*> m_lineStrings;

};

Q_DECLARE_METATYPE(HCGeometryCollection*)
Q_DECLARE_METATYPE(HCMultiPoint*)
Q_DECLARE_METATYPE(HCMultiLineString*)

#endif // GEOMETRYCOLLECTION_H

