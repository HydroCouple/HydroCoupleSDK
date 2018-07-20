/*!
 * \file  polyhedralsurface.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#ifndef POLYHEDRALSURFACE_H
#define POLYHEDRALSURFACE_H

#include "geometry.h"
#include "specialmap.h"

class HCPolygon;
class HCTriangle;
class HCVertex;
class Edge;
class ProgressChecker;
class HCLineString;
class HCPoint;
class Octree;

class HYDROCOUPLESDK_EXPORT HCPolyhedralSurface : public HCGeometry,
    public virtual HydroCouple::Spatial::IPolyhedralSurface
{
    friend class HCPolygon;
    friend class HCTriangle;
    friend class HCVertex;
    friend class HCTIN;

    Q_INTERFACES(HydroCouple::Spatial::IPolyhedralSurface)

  public:

    HCPolyhedralSurface(const QString &id = QUuid::createUuid().toString());

    virtual ~HCPolyhedralSurface();

    int dimension() const override;

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    Envelope *envelopeInternal() const override;

    double area() const override;

    HydroCouple::Spatial::IPoint *centroid() const override;

    HydroCouple::Spatial::IPoint *pointOnSurface() const override;

    HydroCouple::Spatial::IMultiCurve *boundaryMultiCurve() const override;

    int patchCount() const override;

    HydroCouple::Spatial::IPolygon *patch(int index) const override;

    HCPolygon *patchInternal(int index) const;

    const std::vector<HCPolygon*> &patches() const;

    int vertexCount() const override;

    HydroCouple::Spatial::IVertex *vertex(int index) const override;

    HCVertex* vertexInternal(int index) const;

    const std::vector<HCVertex*> &vertices() const;

    HydroCouple::Spatial::IMultiPolygon *boundingPolygons(const HydroCouple::Spatial::IPolygon *polygon) const override;

    bool isClosed() const override;

    void enable3D() override;

    void disable3D() override;

    void enableM() override;

    void disableM() override;

    Edge *createVertexEdge(HCVertex *vertex, HCVertex *destination, HCPolygon *left , HCPolygon *right);

    Edge *createVertexEdge(HCVertex *origin, HCVertex *destination, HCPolygon *face);

    void deleteVertexEdge(Edge *edge);

    virtual Edge *createFaceEdge(HCPolygon *patch, HCVertex *orig, HCVertex *dest);

    void deleteFaceEdge(Edge *edge);

    void deletePatch(HCPolygon *polygon);

    Edge *findDuplicateEdge(Edge *edge) const;

    Edge *findEdge(HCVertex *origin, HCVertex *destination) const;

    void reIndexVerticesAndPatches();

    static void printEdge(HydroCouple::Spatial::IEdge *edge);

    static void printAllOrigNext(HydroCouple::Spatial::IEdge *edge);

    static void printAllLeftNext(HydroCouple::Spatial::IEdge *edge);

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
     *       must be nonnull
     * org  -> the origin vertex to look for;
     *       must be nonnull
     * <- the edge in the same face orbit as _edge_ with origin vertex _org_;
     *  null if not found
     */
    static Edge *getOrbitOrg(Edge *edge, HCVertex *org) ;

    /*
     * Set the origin of the vertex orbit of a given edge to a given vertex.
     * edge -> an edge of the orbit to set the origin vertex of;
     *       must be nonnull
     * org  -> the new origin vertex;
     *       must be nonnull
     */
    static void setOrbitOrg(Edge *edge, HCVertex *org);

    /*
     * Return the edge with a given left face in the vertex orbit of a given
     * edge.
     * edge -> an edge of the orbit to look for the face in;
     *       must be nonnull
     * left -> the left face to look for;
     *       must be nonnull
     * <- the edge in the same vertex orbit as _edge_ with left face _left_;
     *  null if not found
     */
    static Edge *getOrbitLeft(Edge *edge, HCPolygon *left);

    /*!
     * \brief getClosestOrbitLeftNull
     * \param origin
     * \param destination
     * \return
     */
    static Edge *getClosestOrbitLeftNull(HCVertex* origin, HCVertex *destination);

    /*!
     * \brief getOrbitRight
     * \param edge
     * \param right
     * \return
     */
    static Edge *getOrbitRight(Edge *edge, HCPolygon *right);

    /*
     * Set the left face of the face orbit of a given edge to a given face.
     * edge -> an edge of the orbit to set the left face of;
     *       must be nonnull
     * left -> the new left face;
     *       must be nonnull
     */
    static void setOrbitLeft(Edge *edge, HCPolygon *left);

  protected:

    std::vector<HCPolygon*> m_patches;
    std::vector<HCVertex*>  m_vertices;
    int64_t m_patchCntr = 0,
    m_vertexCntr = 0;
};

class HYDROCOUPLESDK_EXPORT HCTIN : public HCPolyhedralSurface,
    public virtual HydroCouple::Spatial::ITIN
{
    Q_INTERFACES(HydroCouple::Spatial::ITIN)

  public:

    HCTIN(const QString &id = QUuid::createUuid().toString());

    virtual ~HCTIN();

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    HydroCouple::Spatial::ITriangle *triangle(int index) const override;

    HCTriangle *triangleInternal(int index) const;

    Edge *createFaceEdge(HCPolygon *patch, HCVertex *orig, HCVertex *dest) override;

    HCTriangle* createTriangle(HCVertex *v1, HCVertex *v2, HCVertex *v3);

    static void nullifyTriangleIOObjectPointers(struct triangulateio *triangleObject);

    static void deleteTriangleIOObject(struct triangulateio *triangleObject);

    static HCTIN *triangulateSFS(const QString &command,
                                 const std::vector<HCPoint*>& points,
                                 const std::vector<HCLineString*> &plsg,
                                 const std::vector<HCPoint*> &holes);

    static HCTIN *readTriangleIOObject(struct triangulateio *triangleObject);

  private:

    static HCPoint *findPointInOctree(HCPoint *point, Octree *octree, double tolerance = 0.000000001);

};

Q_DECLARE_METATYPE(HCPolyhedralSurface*)
Q_DECLARE_METATYPE(HCTIN*)

#endif // POLYHEDRALSURFACE_H
