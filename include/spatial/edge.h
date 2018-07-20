/*!
 * \file edge.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
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

#ifndef EDGE
#define EDGE

#include "geometry.h"

#include <QMutex>

//class Edge;
class HCVertex;
class HCPolygon;
class QuadEdge;
class HCPolyhedralSurface;
class HCTIN;

class HYDROCOUPLESDK_EXPORT Edge final:
    public virtual HydroCouple::Spatial::IEdge
{

    friend class QuadEdge;
    friend class HCPolyhedralSurface;
    friend class HCTIN;
    friend class HCPolygon;
    friend class HCVertex;

//    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IEdge)

    //  public:
  private:

    Edge();

    virtual ~Edge();

  public:

    static Edge *createEdge();

    static void deleteEdge(Edge *edge);

    static void splice(Edge *a, Edge *b);

    unsigned int index() const override;

    void setIndex(unsigned int index);

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

    Edge *rotInternal() ;

    HydroCouple::Spatial::IEdge *invRot()  override;

    Edge *invRotInternal() ;

    HydroCouple::Spatial::IEdge *sym()  override;

    Edge *symInternal() ;

    HydroCouple::Spatial::IEdge *origNext()  override;

    Edge *origNextInternal()  ;

    HydroCouple::Spatial::IEdge *origPrev()  override;

    Edge *origPrevInternal()  ;

    HydroCouple::Spatial::IEdge *destNext()  override;

    Edge *destNextInternal()  ;

    HydroCouple::Spatial::IEdge *destPrev()  override;

    Edge *destPrevInternal()  ;

    HydroCouple::Spatial::IEdge *leftNext()  override;

    Edge *leftNextInternal()  ;

    HydroCouple::Spatial::IEdge *leftPrev()  override;

    Edge *leftPrevInternal()  ;

    HydroCouple::Spatial::IEdge *rightNext()  override;

    Edge *rightNextInternal()  ;

    HydroCouple::Spatial::IEdge *rightPrev()  override;

    Edge *rightPrevInternal()  ;

    double length();

    int marker() const;

    void setMarker(int index) ;

    static unsigned int getNextId();

    void initializeData(int dataLength);

    int dataLength() const;

    double &data(int index);

    const double &data(int index) const;

  private:


    int m_marker;

    unsigned int m_index = -1,
    m_id = -1;

    static unsigned int m_nextId;

    int m_dataLength;

    double *m_data;

    /*
     *The next ccw edge around (from) the origin of this edge.
     *Nonnull.
     */
    Edge *m_next;

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

    /*!
     * \brief m_quadeEdge
     */
    QuadEdge *m_quadeEdge;

};


Q_DECLARE_METATYPE(Edge*)

#endif // EDGE

