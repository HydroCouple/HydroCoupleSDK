#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "hydrocouplespatial.h"
#include "core/identity.h"
#include "ogrsf_frmts.h"

class SpatialReferenceSystem;

class HYDROCOUPLESDK_EXPORT HCGeometry : public Identity,
    public virtual HydroCouple::Spatial::IGeometry
{

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IGeometry)

  public:

    enum GeometryFlag
    {
      Node = 0x00,
      IsEmpty = 0x01,
      HasZ = 0x02,
      HasM = 0x04
    };

    Q_ENUM(GeometryFlag)
    Q_DECLARE_FLAGS(GeometryFlags, GeometryFlag)

    HCGeometry(QObject* parent =  nullptr);

    HCGeometry(const QString& id = QUuid::createUuid().toString(), QObject* parent =  nullptr);

    virtual ~HCGeometry();

    unsigned int index() const override;

    virtual void setIndex(unsigned int index);

    int coordinateDimension() const override;

    HydroCouple::Spatial::GeometryType geometryType() const override;

    HydroCouple::Spatial::ISpatialReferenceSystem* spatialReferenceSystem() const override;

    QString wkt() const override;

    bool isEmpty() const override;

    bool isSimple() const override;

    bool is3D() const override;

    bool isMeasured() const override;

    HydroCouple::Spatial::IGeometry* boundary() const override;

    bool equals(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool disjoint(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool intersects(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool touches(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool crosses(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool within(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool contains(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool overlaps(const HydroCouple::Spatial::IGeometry *geom) const override;

    double distance(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool relate(const HydroCouple::Spatial::IGeometry *geom) const override;

    HydroCouple::Spatial::IGeometry* locateAlong(double value) const override;

    HydroCouple::Spatial::IGeometry* locateBetween(double mStart, double mEnd) const override;

    HydroCouple::Spatial::IGeometry* buffer(double bufferDistance) const override;

    HydroCouple::Spatial::IGeometry* convexHull() const override;

    HydroCouple::Spatial::IGeometry* intersection(const HydroCouple::Spatial::IGeometry* geom) const override;

    HydroCouple::Spatial::IGeometry* unionG(const HydroCouple::Spatial::IGeometry* geom) const override;

    HydroCouple::Spatial::IGeometry* difference(const HydroCouple::Spatial::IGeometry* geom) const override;

    HydroCouple::Spatial::IGeometry* symmetricDifference(const HydroCouple::Spatial::IGeometry* geom) const override;

    GeometryFlags geometryFlags() const;

  protected:

    virtual void setGeometryFlag(GeometryFlag flag, bool on = true);

    virtual void setCoordinateDimension(int dimension);

  private:
    unsigned int m_index;
    int m_coordinateDimension;
    HydroCouple::Spatial::GeometryType m_geometryType;
    SpatialReferenceSystem *m_srs;
    GeometryFlags m_geomFlags;
};

Q_DECLARE_METATYPE(HCGeometry*)
Q_DECLARE_OPERATORS_FOR_FLAGS(HCGeometry::GeometryFlags)

#endif // GEOMETRY_H

