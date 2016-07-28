#ifndef POLYGON_H
#define POLYGON_H

#include "geometry.h"

class HCLineString;
class HCVertex;
class HCEdge;
class HCPolyhedralSurface;

class HCPolygon : public HCGeometry,
    public virtual HydroCouple::Spatial::ISurface,
    public virtual HydroCouple::Spatial::IPolygon
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::ISurface
                 HydroCouple::Spatial::IPolygon)

  public:

    HCPolygon(QObject* parent);

    HCPolygon(HCPolyhedralSurface* parent);

    virtual ~HCPolygon();

//    double area() const override;

//    HydroCouple::Spatial::IPoint* centroid() const override;

//    HydroCouple::Spatial::IMultiCurve* boundaryMultiCurve() const override;

//    HydroCouple::Spatial::ILineString* exteriorRing() const override;

//    void addExteriorRingPoint(HCVertex* vertex);

    void addEdge(HCEdge* edge);

//    void removeExteriorRingPoint(HCVertex* vertex);

//    void removeEdge(HCEdge* edge);

//    int interiorRingCount() const override;

//    HydroCouple::Spatial::ILineString* interiorRing(int index) const override;

//    QList<HCLineString*> interiorRings() const;

//    bool addInteriorRing(HCLineString* interiorRing);

//    bool removeInteriorRing(HCLineString* interiorRing);

//    HydroCouple::Spatial::IEdge* edge() const override;

//    HydroCouple::Spatial::IPolyhedralSurface* polyhydralSurface() const override;

  private:
    HCLineString* m_exteriorRing;
    QList<HCLineString*> m_interiorRings;
    HCEdge* m_edge;
    HCPolyhedralSurface* m_polyhedralSurface;
};

#endif // POLYGON_H
