#ifndef POLYHEDRALSURFACE_H
#define POLYHEDRALSURFACE_H

#include "geometry.h"

class HCPolygon;
class HCVertex;

class HCPolyhedralSurface : public HCGeometry,
    public virtual HydroCouple::Spatial::IPolyhedralSurface
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IPolyhedralSurface)

  public:

    HCPolyhedralSurface(QObject* parent);

    virtual ~HCPolyhedralSurface();

    int patchCount() const override;

    HydroCouple::Spatial::IPolygon* patch(int index) const override;

    bool addPatch(HCPolygon* patch);

    bool deletePatch(HCPolygon* patch);

    QList<HCVertex*> vertices() const;

    bool addVertex(HCVertex* vertex);

    bool deleteVertex(HCVertex* vertex);

    HydroCouple::Spatial::IMultiPolygon* boundingPolygons(const HydroCouple::Spatial::IPolygon *polygon) const override;

    bool isClosed() const override;

  private:
    QList<HCPolygon*> m_patches;
    QList<HCVertex*> m_vertices;

};

#endif // POLYHEDRALSURFACE_H
