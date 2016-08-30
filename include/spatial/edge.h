#ifndef EDGE
#define EDGE

#include "geometry.h"

#include <QMutex>

//class HCEdge;
class HCVertex;
class HCPolygon;
class QuadEdge;
class HCPolyhedralSurface;
class HCTIN;

class HYDROCOUPLESDK_EXPORT HCEdge final : public Identity,
    public virtual HydroCouple::Spatial::IEdge
{

    friend class QuadEdge;
    friend class HCPolyhedralSurface;
    friend class HCTIN;
    friend class HCPolygon;

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IEdge)

    //  public:
  private:

    HCEdge();

    virtual ~HCEdge();

    static HCEdge *createEdge(HCPolyhedralSurface *parent);

    static void deleteEdge(HCEdge *edge);

  public:

    static void splice(HCEdge *a, HCEdge *b);

    unsigned int index() const override;

    void setIndex(unsigned int index);

    HCGeometry::GeometryFlags geometryFlags() const;

    bool is3D() const;

    bool isMeasured() const;

    HCVertex *point(int index);

    HydroCouple::Spatial::IVertex *orig() override;

    HCVertex *origInternal() ;

    void setOrig(HCVertex *origin);

    HydroCouple::Spatial::IVertex *dest()  override;

    HCVertex *destInternal() ;

    void setDest(HCVertex *destination);

    virtual HydroCouple::Spatial::IPolygon *left() override;

    HCPolygon *leftInternal() ;

    void setLeft(HCPolygon *left);

    virtual HydroCouple::Spatial::IPolygon *right()  override;

    HCPolygon *rightInternal() ;

    void setRight(HCPolygon *right);

    HydroCouple::Spatial::IPolygon *face()  override;

    HCPolygon *faceInternal()  ;

    HydroCouple::Spatial::IEdge *rot()   override;

    HCEdge *rotInternal() ;

    HydroCouple::Spatial::IEdge *invRot()  override;

    HCEdge *invRotInternal() ;

    HydroCouple::Spatial::IEdge *sym()  override;

    HCEdge *symInternal() ;

    HydroCouple::Spatial::IEdge *origNext()  override;

    HCEdge *origNextInternal()  ;

    HydroCouple::Spatial::IEdge *origPrev()  override;

    HCEdge *origPrevInternal()  ;

    HydroCouple::Spatial::IEdge *destNext()  override;

    HCEdge *destNextInternal()  ;

    HydroCouple::Spatial::IEdge *destPrev()  override;

    HCEdge *destPrevInternal()  ;

    HydroCouple::Spatial::IEdge *leftNext()  override;

    HCEdge *leftNextInternal()  ;

    HydroCouple::Spatial::IEdge *leftPrev()  override;

    HCEdge *leftPrevInternal()  ;

    HydroCouple::Spatial::IEdge *rightNext()  override;

    HCEdge *rightNextInternal()  ;

    HydroCouple::Spatial::IEdge *rightPrev()  override;

    HCEdge *rightPrevInternal()  ;

    void initializeData(int length, double defaultValue = std::numeric_limits<double>::quiet_NaN());

    static unsigned int getNextId();

  private:
    void setGeometryFlag(HCGeometry::GeometryFlag flag, bool on = true);

  public:
    double* data;
    int dataLength;


  private:

    unsigned int m_index, m_id;
    static unsigned int m_nextId;

    /*
      *The next ccw edge around (from) the origin of this edge.
      *Nonnull.
     */
    HCEdge *m_next;

    /*
      *The origin vertex of this edge, if prime.
      *Null if not prime.
     */
    HCVertex *m_vertex;

    /*
      *The target face of this edge, if dual.
      *Null if not dual.
     */
    HCPolygon *m_face;

    QuadEdge *m_quadeEdge;
    HCGeometry::GeometryFlags m_geomFlags;

};

class QuadEdge
{
  public:

    QuadEdge(HCPolyhedralSurface *parent);

    virtual ~QuadEdge(){}

    HCEdge m_edges[4];
};

Q_DECLARE_METATYPE(HCEdge*)

#endif // EDGE

