#ifndef POLYHEDRALSURFACE_H
#define POLYHEDRALSURFACE_H

#include "geometry.h"

class HCPolygon;
class HCVertex;
class HCEdge;

class HCPolyhedralSurface : public HCGeometry,
    public virtual HydroCouple::Spatial::IPolyhedralSurface
{
    friend class HCPolygon;
    friend class HCVertex;
    friend class HCTIN;

    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IPolyhedralSurface)

  public:

    HCPolyhedralSurface(QObject *parent);

    virtual ~HCPolyhedralSurface();

    int dimension() const override;

    HydroCouple::Spatial::IGeometry* envelope() const override;

    double area() const override;

    HydroCouple::Spatial::IPoint *centroid() const override;

    HydroCouple::Spatial::IPoint *pointOnSurface() const override;

    HydroCouple::Spatial::IMultiCurve *boundaryMultiCurve() const override;

    int patchCount() const override;

    HydroCouple::Spatial::IPolygon *patch(int index) const override;

    QList<HCPolygon*> patches() const;

    int vertexCount() const override;

    HydroCouple::Spatial::IVertex *vertex(int index) override;

    QList<HCVertex*> vertices() const;

    HydroCouple::Spatial::IMultiPolygon *boundingPolygons(const HydroCouple::Spatial::IPolygon *polygon) const override;

    bool isClosed() const override;

    void enable3D() override;

    void disable3D() override;

    void enableM() override;

    void disableM() override;

    virtual HCEdge *createVertexEdge(HCVertex *vertex, HCVertex *destination, HCPolygon *left , HCPolygon *right);

    virtual HCEdge *createVertexEdge(HCVertex *origin, HCVertex *destination, HCPolygon *face);

    void deleteVertexEdge(HCEdge *edge);

    virtual HCEdge *createFaceEdge(HCPolygon *patch, HCVertex *orig, HCVertex *dest);

    void deleteFaceEdge(HCEdge *edge);

    HCEdge *findDuplicateEdge(HCEdge *edge);

    HCEdge *findEdge(HCVertex *origin, HCVertex *destination);

    static void printEdge(HydroCouple::Spatial::IEdge *edge);

    static void printAllOrigNext(HydroCouple::Spatial::IEdge *edge);

    static void printAllLeftNext(HydroCouple::Spatial::IEdge *edge);

    void initializePatchData(int length);

    void initializeEdgeData(int length);

    void initializeNodeData(int length);

  private:

    void addPatch(HCPolygon *patch);

    bool removePatch(HCPolygon *patch);

    void addVertex(HCVertex *vertex);

    bool removeVertex(HCVertex *vertex);

    void deleteFaceEdges(HCPolygon* face);

    /*
     * Return the edge with a given origin vertex in the face orbit of a given
     * edge.
     * edge -> an edge of the orbit to look for the vertex in;
     *         must be nonnull
     * org  -> the origin vertex to look for;
     *         must be nonnull
     * <- the edge in the same face orbit as _edge_ with origin vertex _org_;
     *    null if not found
     */
    static HCEdge *getOrbitOrg(HCEdge *edge, HCVertex *org);

    /*
     * Set the origin of the vertex orbit of a given edge to a given vertex.
     * edge -> an edge of the orbit to set the origin vertex of;
     *         must be nonnull
     * org  -> the new origin vertex;
     *         must be nonnull
     */
    static void setOrbitOrg(HCEdge *edge, HCVertex *org);

    /*
     * Return the edge with a given left face in the vertex orbit of a given
     * edge.
     * edge -> an edge of the orbit to look for the face in;
     *         must be nonnull
     * left -> the left face to look for;
     *         must be nonnull
     * <- the edge in the same vertex orbit as _edge_ with left face _left_;
     *    null if not found
     */
    static HCEdge *getOrbitLeft(HCEdge *edge, HCPolygon *left);

    /*!
     * \brief getOrbitRight
     * \param edge
     * \param right
     * \return
     */
    static HCEdge *getOrbitRight(HCEdge *edge, HCPolygon *right);

    /*
     * Set the left face of the face orbit of a given edge to a given face.
     * edge -> an edge of the orbit to set the left face of;
     *         must be nonnull
     * left -> the new left face;
     *         must be nonnull
     */
    static void setOrbitLeft(HCEdge *edge, HCPolygon *left);

  protected:
    QList<HCPolygon*> m_patches;
    QList<HCVertex*> m_vertices;

};

class HCTIN : public HCPolyhedralSurface,
    public virtual HydroCouple::Spatial::ITIN
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::ITIN)

  public:

    HCTIN(QObject *parent);

    virtual ~HCTIN();

    HydroCouple::Spatial::ITriangle *patch(int index) const override;

};

Q_DECLARE_METATYPE(HCPolyhedralSurface*)
Q_DECLARE_METATYPE(HCTIN*)

#endif // POLYHEDRALSURFACE_H
