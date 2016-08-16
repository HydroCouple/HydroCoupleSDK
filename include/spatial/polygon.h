#ifndef POLYGON_H
#define POLYGON_H

#include "geometry.h"

class HCLineString;
class HCVertex;
class HCEdge;
class HCPolyhedralSurface;
class HCTIN;
class HCTriangle;

class HCPolygon : public HCGeometry,
    public virtual HydroCouple::Spatial::ISurface,
    public virtual HydroCouple::Spatial::IPolygon
{
    friend class HCPolyhedralSurface;
    friend class HCEdge;
    friend class HCTriangle;

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::ISurface
                 HydroCouple::Spatial::IPolygon)

  public:

    HCPolygon(HCPolyhedralSurface *parent);

    HCPolygon(QObject* parent);

    virtual ~HCPolygon();

    int dimension() const override;

    HydroCouple::Spatial::GeometryType geometryType() const override;

    HydroCouple::Spatial::IGeometry* envelope() const override;

    double area() const override;

    HydroCouple::Spatial::IPoint *centroid() const override;

    HydroCouple::Spatial::IPoint *pointOnSurface() const override;

    HydroCouple::Spatial::IMultiCurve* boundaryMultiCurve() const override;

    HydroCouple::Spatial::ILineString* exteriorRing() const override;

    HCLineString* exteriorRingInternal() const;

    int interiorRingCount() const override;

    HydroCouple::Spatial::ILineString* interiorRing(int index) const override;

    QList<HCLineString*> interiorRings() const;

    virtual bool addInteriorRing(HCLineString* interiorRing);

    virtual bool removeInteriorRing(HCLineString* interiorRing);

    HydroCouple::Spatial::IEdge* edge() const override;

    HydroCouple::Spatial::IPolyhedralSurface* polyhydralSurface() const override;

    HCPolyhedralSurface* polyhydralSurfaceInternal() const;

    bool isConvex() const;

    void enable3D() override;

    void disable3D() override;

    void enableM() override;

    void disableM() override;

    void setGeometryFlag(GeometryFlag flag, bool on = true) override;

    static unsigned int getNextId();

  private:

    void addEdge(HCEdge *edge);

    void removeEdge(HCEdge *edge);

    void reCreateOuterLineString();

  protected:

    HCLineString* m_exteriorRing;

  private:
    QList<HCLineString*> m_interiorRings;
    HCEdge* m_edge;
    HCPolyhedralSurface* m_polyhedralSurface;
    static unsigned int m_nextId;
};

class HCTriangle : public HCPolygon,
    public virtual HydroCouple::Spatial::ITriangle
{

    friend class HCTIN;

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::ITriangle)

  private:

    HCTriangle(HCTIN *tin);

  public:

    HCTriangle(QObject *parent);

    virtual ~HCTriangle();

    bool isValid() const;

    HydroCouple::Spatial::IVertex *p1() const override;

    HydroCouple::Spatial::IVertex *p2() const override;

    HydroCouple::Spatial::IVertex *p3() const override;

  private:

    HCTIN *m_tin;
};

Q_DECLARE_METATYPE(HCPolygon*)
Q_DECLARE_METATYPE(HCTriangle*)

#endif // POLYGON_H
