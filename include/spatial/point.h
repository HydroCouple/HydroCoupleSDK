/*!
 * \file  point.h
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

#ifndef POINT_H
#define POINT_H

#include "geometry.h"

class HCVertex;
class HCPolyhedralSurface;
class Edge;
class Network;

class HYDROCOUPLESDK_EXPORT HCPoint: public HCGeometry,
    public virtual HydroCouple::Spatial::IPoint
{
    Q_INTERFACES(HydroCouple::Spatial::IPoint)

    friend class HCLineString;
    friend class HCMultiPoint;
    friend class HCLine;

  public:

    explicit HCPoint(const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    explicit HCPoint(double x, double y, const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    explicit HCPoint(double x, double y, double z, const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    explicit HCPoint(double x, double y, double z, double m, const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

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

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    using HydroCouple::Spatial::IGeometry::equals;
    bool equals(const HydroCouple::Spatial::IPoint *point, double epsilon = std::numeric_limits<double>::epsilon(), bool threeDimensional = false) const;

    virtual HCPoint* clone() const;

    virtual HCVertex* cloneToVertex() const;

    void setGeometryFlag(HCGeometry::GeometryFlag flag, bool on) override;

    static double dist(const HCPoint *p1, const HCPoint* p2);

    static HCPoint* crossProduct(const HCPoint* p1, const HCPoint* p2);

    static double dotProduct(const HCPoint* p1,const HCPoint* p2);

    static unsigned int getNextId();

    static void resetId();

    Envelope *envelopeInternal() const override;

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
    friend class Network;

    Q_INTERFACES(HydroCouple::Spatial::IVertex)

  public:

    explicit HCVertex(const QString &id, HCGeometry *parent = nullptr);

    explicit HCVertex(bool hasZ = false,bool hasM = false, const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    explicit HCVertex(const QString &id, HCPolyhedralSurface* parent);

    explicit HCVertex(const QString &id, Network *parent);

    explicit HCVertex(double x, double y, const QString &id, HCGeometry *parent = nullptr);

    explicit HCVertex(double x, double y, const QString &id, HCPolyhedralSurface* parent);

    explicit HCVertex(double x, double y, const QString &id, Network *parent);

    explicit HCVertex(double x, double y, double z, const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    explicit HCVertex(double x, double y, double z,  const QString &id, HCPolyhedralSurface* parent);

    explicit HCVertex(double x, double y, double z,  const QString &id, Network *parent);

    explicit HCVertex(double x, double y, double z, double m, const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    explicit HCVertex(double x, double y, double z, double m, const QString &id, HCPolyhedralSurface* parent);

    explicit HCVertex(double x, double y, double z, double m, const QString &id, Network *parent);

    virtual ~HCVertex();

    HydroCouple::Spatial::IEdge* edge() const override;

    Edge *edgeInternal() const;

    void addEdge(Edge *edge);

    void removeEdge(Edge *edge);

    HCPolyhedralSurface *polyhydralSurfaceInternal() const;

  private:
    Edge *m_edge;
    HCPolyhedralSurface *m_polyhedralSurface;
    Network *m_network;
};


struct HYDROCOUPLESDK_EXPORT Vect
{

    Vect(const Vect &vect);

    Vect(const HydroCouple::Spatial::IPoint &point);

    Vect(const HydroCouple::Spatial::IVertex &point);

    Vect(double x = 0,  double y = 0,  double z = 0);

    Vect &operator =(const Vect &vect);

    virtual ~Vect();

    double x() const;

    double y() const;

    double z() const;

    double length() const;

    double lengthSquared() const;

    void normalize();

    Vect normalized() const;

    Vect normal2dToVector() const;

    Vect unitNormal2dToVector() const;

    Vect &operator*=(double factor);

    Vect &operator*=(const Vect &vector);

    Vect &operator+=(const Vect &vector) ;

    Vect &operator-=(const Vect &vector);

    Vect &operator/=(double divisor);

    Vect &operator/=(const Vect &vector);

    double &operator[](const int index);

    const double &operator[](const int index) const;

    friend Vect operator *(const Vect &v, double factor);

    friend Vect operator *(double factor, const Vect &v);

    friend Vect operator *(const Vect &v1 , const Vect &v2);

    friend Vect operator +(const Vect &v1 , const Vect &v2) ;

    friend Vect operator -(const Vect &v1 , const Vect &v2) ;

    friend Vect operator/(const Vect &v, double divisor);

    friend Vect operator/(const Vect &v1 , const Vect &v2);


    static double dotProduct(const Vect& u, const Vect &v);

    static double dotProduct(double x, double y, double z, const Vect &v);

    static double dotProduct(double x1, double y1, double z1, double x2, double y2, double z2);

    static Vect crossProduct(const Vect& u, const Vect &v);

    static bool linesIntersect2d(const Vect &l1p1, const Vect &l1p2,
                                 const Vect &l2p1, const Vect &l2p2, Vect &outVect);

    static Vect unitX();

    static Vect unitY();

    static Vect unitZ();

    void print();

    void zero();

    double *v = nullptr;

};

Q_DECLARE_METATYPE(HCPoint*)
Q_DECLARE_METATYPE(HCVertex*)


#endif // POINT_H

