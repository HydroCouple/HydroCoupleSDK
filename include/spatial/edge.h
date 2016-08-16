#ifndef EDGE
#define EDGE

#include "linestring.h"

//class HCEdge;
class HCVertex;
class HCPolygon;

class HYDROCOUPLESDK_EXPORT HCEdge : public HCLine,
    public virtual HydroCouple::Spatial::IEdge
{

    friend class QuadEdge;
    friend class HCPolyhedralSurface;
    friend class HCTIN;

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IEdge)

  public:
//  private:

    HCEdge();

    virtual ~HCEdge();

    static HCEdge *createEdge(QObject *parent);

    static void deleteEdge(HCEdge *edge);

  public:

    static void splice(HCEdge *a, HCEdge *b);

    void setIndex(unsigned int index) override;

    HydroCouple::Spatial::IVertex *orig() override;

    void setOrig(HCVertex *origin);

    HydroCouple::Spatial::IVertex *dest() override;

    void setDest(HCVertex *destination);

    virtual HydroCouple::Spatial::IPolygon *left() override;

    void setLeft(HCPolygon *left);

    virtual HydroCouple::Spatial::IPolygon *right()  override;

    void setRight(HCPolygon *right);

    HydroCouple::Spatial::IPolygon *face() override;

    HydroCouple::Spatial::IEdge *rot()  override;

    HydroCouple::Spatial::IEdge *invRot() override;

    HydroCouple::Spatial::IEdge *sym() override;

    HydroCouple::Spatial::IEdge *origNext() override;

    HydroCouple::Spatial::IEdge *origPrev() override;

    HydroCouple::Spatial::IEdge *destNext() override;

    HydroCouple::Spatial::IEdge *destPrev() override;

    HydroCouple::Spatial::IEdge *leftNext() override;

    HydroCouple::Spatial::IEdge *leftPrev() override;

    HydroCouple::Spatial::IEdge *rightNext() override;

    HydroCouple::Spatial::IEdge *rightPrev() override;

    static unsigned int getNextId();

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
};

class QuadEdge
{
  public:

    QuadEdge(QObject *parent);

    HCEdge m_edges[4];
};

Q_DECLARE_METATYPE(HCEdge*)

#endif // EDGE

