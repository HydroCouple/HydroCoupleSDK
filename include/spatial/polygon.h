/*!
 * \file  polygon.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
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

#ifndef POLYGON_H
#define POLYGON_H

#include "geometry.h"
#include "point.h"

class HCLineString;
class Edge;
class HCPolyhedralSurface;
class HCTIN;
class HCTriangle;

class HYDROCOUPLESDK_EXPORT HCPolygon : public HCGeometry,
    public virtual HydroCouple::Spatial::ISurface,
    public virtual HydroCouple::Spatial::IPolygon
{
    friend class HCPolyhedralSurface;
    friend class HCTIN;
    friend class Edge;
    friend class HCTriangle;

    Q_INTERFACES(HydroCouple::Spatial::ISurface
                 HydroCouple::Spatial::IPolygon)

    using HydroCouple::Spatial::IGeometry::contains;

  public:
    explicit HCPolygon(const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    explicit HCPolygon(const QString &id, HCPolyhedralSurface *parent);

    virtual ~HCPolygon();

    int dimension() const override;

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    double area() const override;

    virtual double area3D() const;

    HydroCouple::Spatial::IPoint *centroid() const override;

    HydroCouple::Spatial::IPoint *pointOnSurface() const override;

    HydroCouple::Spatial::IMultiCurve* boundaryMultiCurve() const override;

    HydroCouple::Spatial::ILineString* exteriorRing() const override;

    HCLineString* exteriorRingInternal() const;

    int interiorRingCount() const override;

    HydroCouple::Spatial::ILineString* interiorRing(int index) const override;

    HCLineString *interiorRingInternal(int index) const;

    QList<HCLineString*> interiorRingsInternal() const;

    virtual bool addInteriorRing(HCLineString* interiorRing);

    virtual bool removeInteriorRing(HCLineString* interiorRing);

    HydroCouple::Spatial::IEdge* edge() const override;

    Edge* edgeInternal() const;

    Edge *getEdge(int index) const;

    void moveCurrentEdgeToNext();

    HydroCouple::Spatial::IPolyhedralSurface* polyhydralSurface() const override;

    HCPolyhedralSurface* polyhydralSurfaceInternal() const;

    bool contains(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool isConvex() const;

    void enable3D() override;

    void disable3D() override;

    void enableM() override;

    void disableM() override;

    void setGeometryFlag(GeometryFlag flag, bool on = true) override;

    Envelope *envelopeInternal() const override;

    static unsigned int getNextId();

    virtual void reCreateOuterLineString() const;

  private:

    void addEdge(Edge *edge);

    void removeEdge(Edge *edge);

  protected:

    HCLineString* m_exteriorRing;
    HCPolyhedralSurface* m_polyhedralSurface;

  private:
    QList<HCLineString*> m_interiorRings;
    Edge* m_edge;
    static unsigned int m_nextId;
};

class HYDROCOUPLESDK_EXPORT HCTriangle : public HCPolygon,
    public virtual HydroCouple::Spatial::ITriangle
{

    friend class HCTIN;

    Q_INTERFACES(HydroCouple::Spatial::ITriangle)

  public:

    explicit HCTriangle(const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    explicit HCTriangle(const QString &id, HCTIN *tin);

    virtual ~HCTriangle();

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    bool isValid() const;

    HydroCouple::Spatial::IVertex *p1() const override;

    HCVertex *p1Internal() const;

    HydroCouple::Spatial::IVertex *p2() const override;

    HCVertex *p2Internal() const;

    HydroCouple::Spatial::IVertex *p3() const override;

    HCVertex *p3Internal() const;

    double area() const override;

    double area3D() const override;

    Vect normal() const;

    double interpolateZ(double x, double y);

    bool contains(const HydroCouple::Spatial::IGeometry *geom) const override;

    static double sign(const HydroCouple::Spatial::IPoint *p1, const HydroCouple::Spatial::IPoint *p2, const HydroCouple::Spatial::IPoint *p3);

    static double sign(const Vect &p1, const Vect &p2, const Vect &p3);

    static double triArea3D(const HydroCouple::Spatial::IPoint *p1, const HydroCouple::Spatial::IPoint *p2, const HydroCouple::Spatial::IPoint *p3);

    static double triArea3D(const Vect &p1, const Vect &p2, const Vect &p3);

    static double signedVolume(const HydroCouple::Spatial::IPoint *p1, const HydroCouple::Spatial::IPoint *p2, const HydroCouple::Spatial::IPoint *p3);

    static double signedVolume(const Vect &p1, const Vect &p2, const Vect &p3);

  private:

    HCTIN *m_tin;
};

Q_DECLARE_METATYPE(HCPolygon*)
Q_DECLARE_METATYPE(HCTriangle*)

#endif // POLYGON_H
