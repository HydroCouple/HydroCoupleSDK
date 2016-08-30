#ifndef POINT_H
#define POINT_H

#include "geometry.h"

class HCVertex;
class HCPolyhedralSurface;
class HCEdge;

class HYDROCOUPLESDK_EXPORT HCPoint: public HCGeometry,
    public virtual HydroCouple::Spatial::IPoint
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IPoint)

    friend class HCLineString;
    friend class HCMultiPoint;
    friend class HCLine;

  public:

    HCPoint(QObject* parent = nullptr);

    HCPoint(double x, double y, QObject* parent = nullptr);

    HCPoint(double x, double y, double z, QObject* parent = nullptr);

    HCPoint(double x, double y, double z, double m, QObject* parent = nullptr);

    virtual ~HCPoint();

    double x() const override;

    double y() const override;

    double z() const override;

    double m() const override;

    void setX(double x);

    void setY(double y);

    void setZ(double z);

    void setM(double m);

    void enable3D() override;

    void disable3D() override;

    void enableM() override;

    void disableM() override;

    int dimension() const override;

    HydroCouple::Spatial::GeometryType geometryType() const override;

    HydroCouple::Spatial::IGeometry* envelope() const override;

    bool compare(const HydroCouple::Spatial::IPoint *point) const;

    virtual HCPoint* clone(QObject* parent = nullptr) const;

    virtual HCVertex* cloneToVertex(QObject* parent = nullptr) const;

    void setGeometryFlag(HCGeometry::GeometryFlag flag, bool on) override;

    static double dist(const HCPoint *p1, const HCPoint* p2);

    static HCPoint* crossProduct(const HCPoint* p1, const HCPoint* p2);

    static double dotProduct(const HCPoint* p1,const HCPoint* p2);

    static unsigned int getNextId();

    static void resetId();

  private:
    double m_x , m_y, m_z, m_m;
    bool m_isEmpty;
    static unsigned int m_nextId;
};

class HYDROCOUPLESDK_EXPORT HCVertex : public HCPoint,
    public virtual HydroCouple::Spatial::IVertex
{
    friend class HCPolyhedralSurface;
    friend class HCTIN;

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IVertex)

  public:

    HCVertex(QObject* parent = nullptr);

    HCVertex(HCPolyhedralSurface* parentPolyhSurface);

    HCVertex(double x, double y, QObject* parent = nullptr);

    HCVertex(double x, double y, HCPolyhedralSurface* parentPolyhSurface);

    HCVertex(double x, double y, double z, QObject* parent = nullptr);

    HCVertex(double x, double y, double z, HCPolyhedralSurface* parentPolyhSurface);

    HCVertex(double x, double y, double z, double m, QObject* parent = nullptr);

    HCVertex(double x, double y, double z, double m, HCPolyhedralSurface* parentPolyhSurface);

    virtual ~HCVertex();

    HydroCouple::Spatial::IEdge* edge() const override;

    HCEdge* edgeInternal() const ;

    void addEdge(HCEdge *edge) ;

    void removeEdge(HCEdge *edge) ;

    int getMarker() const;

    void setMarker(int marker);

  private:
    HCEdge* m_edge;
    HCPolyhedralSurface* m_polyhedralSurface;
    int m_marker;
};

Q_DECLARE_METATYPE(HCPoint*)
Q_DECLARE_METATYPE(HCVertex*)

#endif // POINT_H

