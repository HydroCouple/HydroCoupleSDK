/*!
 *  \file    geometrycollection.h
 *  \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version 1.0.0.0
 *  \section Description
 *  \section License
 *  geometrycollection.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  abstractadaptedoutput.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2016
 *  \pre
 *  \bug
 *  \todo
 *  \warning
 */

#ifndef GEOMETRYCOLLECTION_H
#define GEOMETRYCOLLECTION_H

#include "geometry.h"

class HCLineString;
class HCPoint;
class HCPolygon;

class HYDROCOUPLESDK_EXPORT HCGeometryCollection : public HCGeometry,
    public virtual HydroCouple::Spatial::IGeometryCollection
{
    Q_INTERFACES(HydroCouple::Spatial::IGeometryCollection)

  public:

    HCGeometryCollection(const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    virtual ~HCGeometryCollection();

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    int dimension() const override;

    int geometryCount() const override;

    HydroCouple::Spatial::IGeometry* geometry(int index) const override;

    HCGeometry *geometryInternal(int index) const;

    virtual bool addGeometry(HCGeometry *geometry);

    virtual bool removeGeometry(HCGeometry *geometry);

    void enable3D() override;

    void disable3D() override;

    void enableM() override;

    void disableM() override;

    Envelope *envelopeInternal() const override;

  protected:

    QList<HCGeometry*> geometries() const;

  private:

    QList<HCGeometry*> m_geometries;
};


class HYDROCOUPLESDK_EXPORT HCMultiPoint : public HCGeometryCollection,
    public virtual HydroCouple::Spatial::IMultiPoint
{
    Q_INTERFACES(HydroCouple::Spatial::IMultiPoint)

  public:

    HCMultiPoint(const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr );

    virtual ~HCMultiPoint();

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    HydroCouple::Spatial::IPoint* point(int index) const override;

    HCPoint *pointInternal(int index) const;

    bool addGeometry(HCGeometry *geometry) override;

    bool addPoint(HCPoint *point);

    bool removeGeometry(HCGeometry *geometry) override;

    bool removePoint(HCPoint *point);

  private:
    QList<HCPoint*> m_points;
};


class HYDROCOUPLESDK_EXPORT HCMultiLineString : public HCGeometryCollection,
    public virtual HydroCouple::Spatial::IMultiCurve,
    public virtual HydroCouple::Spatial::IMultiLineString
{
    Q_INTERFACES(HydroCouple::Spatial::IMultiCurve
                 HydroCouple::Spatial::IMultiLineString)

  public:

    HCMultiLineString(const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    virtual ~HCMultiLineString();

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    bool isClosed() const override;

    double length() const override;

    HydroCouple::Spatial::ILineString* lineString(int index) const override;

    HCLineString *lineStringInternal(int index) const;

    QList<HCLineString*> lineStrings() const;

    bool addGeometry(HCGeometry *geometry) override;

    bool addLineString(HCLineString* lineString);

    bool removeGeometry(HCGeometry *geometry) override;

    bool removeLineString(HCLineString *lineString);

  private:

    QList<HCLineString*> m_lineStrings;

};

class HYDROCOUPLESDK_EXPORT HCMultiPolygon : public HCGeometryCollection,
    public virtual HydroCouple::Spatial::IMultiSurface,
    public virtual HydroCouple::Spatial::IMultiPolygon
{
    Q_INTERFACES(HydroCouple::Spatial::IMultiSurface
                 HydroCouple::Spatial::IMultiPolygon)

  public:
    HCMultiPolygon(const QString &id = QUuid::createUuid().toString() , HCGeometry *parent = nullptr);

    virtual ~HCMultiPolygon();

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    double area() const override;

    HydroCouple::Spatial::IPoint* centroid() const override;

    HydroCouple::Spatial::IPoint* pointOnSurface() const override;

    HydroCouple::Spatial::IPolygon *polygon(int index) const override;

    HCPolygon *polygonInternal(int index) const;

    bool addGeometry(HCGeometry *geometry) override;

    bool addPolygon(HCPolygon *polygon);

    bool removeGeometry(HCGeometry *geometry) override;

    bool removePolygon(HCPolygon *polygon);

   private:

    QList<HCPolygon*> m_polygons;
};

Q_DECLARE_METATYPE(HCGeometryCollection*)
Q_DECLARE_METATYPE(HCMultiPoint*)
Q_DECLARE_METATYPE(HCMultiLineString*)

#endif // GEOMETRYCOLLECTION_H

