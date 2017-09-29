/*!
 *  \file    linestring.h
 *  \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version 1.0.0.0
 *  \section Description
 *  \section License
 *  linestring.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  abstractadaptedoutput.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2017
 *  \pre
 *  \bug
 *  \todo
 *  \warning
 *  \copyright Copyright 2017 Caleb Buahin
 */

#ifndef LINESTRING_H
#define LINESTRING_H

#include "geometry.h"

class HCPoint;
class HCLine;
class HCLinearRing;
class Edge;

class HYDROCOUPLESDK_EXPORT HCLineString: public HCGeometry,
    public virtual HydroCouple::Spatial::ICurve,
    public virtual HydroCouple::Spatial::ILineString
{
    friend class HCPolygon;
    friend class HCTriangle;
    friend class HCPolyhedralSurface;

    Q_INTERFACES(HydroCouple::Spatial::ICurve
                 HydroCouple::Spatial::ILineString)

  public:

    HCLineString(const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    virtual ~HCLineString();

    int dimension() const override;

    bool isEmpty() const override;

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    double length() const override;

    HydroCouple::Spatial::IPoint* startPoint() const override;

    HCPoint *startPointInternal() const;

    HydroCouple::Spatial::IPoint* endPoint() const override;

    HCPoint *endPointInternal() const;

    bool isClosed() const override;

    bool isRing() const override;

    int pointCount() const override;

    HydroCouple::Spatial::IPoint* point(int index) const override;

    HCPoint *pointInternal(int index) const;

    void enable3D() override;

    virtual void disable3D() override;

    virtual void enableM() override;

    virtual void disableM() override;

    QList<HCPoint*> points() const;

    virtual bool addPoint(HCPoint *point);

    virtual bool removePoint(HCPoint *point);

    void setGeometryFlag(GeometryFlag flag, bool on = true) override;

    HCLine* toLine() const;

    HCLinearRing* toLinearRing() const;

    double area() const;

    bool isClockWise() const;

    void flip();

    Envelope *envelopeInternal() const override;

    bool contains(const IGeometry *geom) const override;

  protected:

    QList<HCPoint*> m_points;

};

class HYDROCOUPLESDK_EXPORT HCLine : public HCLineString,
    public virtual HydroCouple::Spatial::ILine
{

    Q_INTERFACES(HydroCouple::Spatial::ILine)

  public:

    HCLine(const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    virtual ~HCLine();

    bool isValid() const;

    bool addPoint(HCPoint *point) override;

    bool removePoint(HCPoint *point) override;
    
    void setP1(HCPoint *point);
    
    void setP2(HCPoint *point);

};

class HYDROCOUPLESDK_EXPORT HCLinearRing : public HCLineString,
    public virtual HydroCouple::Spatial::ILinearRing
{

    Q_INTERFACES(HydroCouple::Spatial::ILinearRing)

  public:

    HCLinearRing(const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    virtual ~HCLinearRing();

    bool isValid() const;

};

Q_DECLARE_METATYPE(HCLineString*)
Q_DECLARE_METATYPE(HCLine*)
Q_DECLARE_METATYPE(HCLinearRing*)

#endif // LINESTRING_H

