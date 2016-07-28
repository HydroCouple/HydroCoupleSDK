#ifndef EDGE
#define EDGE

#include "linestring.h"

class HCEdge;
class HCVertex;
class HCPolygon;

class QuadEdge
{
  public:

    QuadEdge(QObject* parent);

    HCEdge* m_edges;
};


class HYDROCOUPLESDK_EXPORT HCEdge : public HCLine,
    public virtual HydroCouple::Spatial::IEdge
{

    friend class QuadEdge;

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IEdge)

  protected:

    HCEdge(QObject* parent = nullptr);

    virtual ~HCEdge();

  public:

    static HCEdge* createEdge(QObject* parent);

    static void deleteEdge(HCEdge* edge);

    static void splice(HCEdge* a, HCEdge* b);

    static unsigned int getNextId();

    void setIndex(unsigned int index) override;

    HydroCouple::Spatial::IVertex* orig() const override;

    void setOrig(HCVertex* origin);

    HydroCouple::Spatial::IVertex* dest() const override;

    void setDest(HCVertex* destination);

    virtual HydroCouple::Spatial::IPolygon* left() const override;

    void setLeft(HCPolygon* left);

    virtual HydroCouple::Spatial::IPolygon* right() const override;

    void setRight(HCPolygon* right);

    HydroCouple::Spatial::IEdge* rot()  override;

    HydroCouple::Spatial::IEdge* invRot() override;

    HydroCouple::Spatial::IEdge* sym() override;

    HydroCouple::Spatial::IEdge* origNext() const override;

    HydroCouple::Spatial::IEdge* origPrev() const override;

    HydroCouple::Spatial::IEdge* destNext() const override;

    HydroCouple::Spatial::IEdge* destPrev() const override;

    HydroCouple::Spatial::IEdge* leftNext() const override;

    HydroCouple::Spatial::IEdge* leftPrev() const override;

    HydroCouple::Spatial::IEdge* rightNext() const override;

    HydroCouple::Spatial::IEdge* rightPrev() const override;

  private:

    unsigned int m_index, m_id;
    static unsigned int m_nextId;

    /*
     * The next ccw edge around (from) the origin of this edge.
     * Nonnull.
     */
    HCEdge* m_next;

    /*
     * The origin vertex of this edge, if prime.
     * Null if not prime.
     */
    HCVertex *m_vertex;

    /*
     * The target face of this edge, if dual.
     * Null if not dual.
     */
    HCPolygon *m_face;
};


//Q_DECLARE_METATYPE(HCEdge*)

#endif // EDGE

