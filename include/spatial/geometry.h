/*!
 * \file geometry.h
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

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "hydrocouplespatial.h"
#include "core/identity.h"

#include <ogrsf_frmts.h>
#include <set>

class SpatialReferenceSystem;
class Envelope;

/*!
 * \brief The HCGeometry class
 * \todo revisit inheritance from qobjects in costs are too high
 */
class HYDROCOUPLESDK_EXPORT HCGeometry:
    public virtual HydroCouple::Spatial::IGeometry
{
    Q_INTERFACES(HydroCouple::Spatial::IGeometry)


  private:
    Q_DISABLE_COPY(HCGeometry)

  public:

    enum GeometryFlag
    {
      None = 0x00,
      HasZ = 0x01,
      HasM = 0x02
    };

    //    Q_ENUM(GeometryFlag)
    Q_DECLARE_FLAGS(GeometryFlags, GeometryFlag)

    explicit HCGeometry(const QString &id = QUuid::createUuid().toString(), HCGeometry *parent = nullptr);

    virtual ~HCGeometry();

    QString id() const override;

    void setId(const QString& id);

    unsigned int index() const override;

    virtual void setIndex(unsigned int index);

    int coordinateDimension() const override;

    HydroCouple::Spatial::IGeometry::GeometryType geometryType() const override;

    HydroCouple::Spatial::ISpatialReferenceSystem* spatialReferenceSystem() const override;

    SpatialReferenceSystem *spatialReferenceSystemInternal() const;

    QString getWKT() const override;

    unsigned char* getWKB(int &size) const override;

    bool isEmpty() const override;

    bool isSimple() const override;

    bool is3D() const override;

    virtual void enable3D() = 0;

    virtual void disable3D() = 0;

    bool isMeasured() const override;

    virtual void enableM() = 0;

    virtual void disableM() = 0;

    HydroCouple::Spatial::IEnvelope* envelope() const override;

    HydroCouple::Spatial::IGeometry* boundary() const override;

    bool equals(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool disjoint(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool intersects(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool touches(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool crosses(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool within(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool contains(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool overlaps(const HydroCouple::Spatial::IGeometry *geom) const override;

    double distance(const HydroCouple::Spatial::IGeometry *geom) const override;

    bool relate(const HydroCouple::Spatial::IGeometry *geom) const override;

    HydroCouple::Spatial::IGeometry* locateAlong(double value) const override;

    HydroCouple::Spatial::IGeometry* locateBetween(double mStart, double mEnd) const override;

    HydroCouple::Spatial::IGeometry* buffer(double bufferDistance) const override;

    HydroCouple::Spatial::IGeometry* convexHull() const override;

    HydroCouple::Spatial::IGeometry* intersection(const HydroCouple::Spatial::IGeometry* geom) const override;

    HydroCouple::Spatial::IGeometry* unionG(const HydroCouple::Spatial::IGeometry* geom) const override;

    HydroCouple::Spatial::IGeometry* difference(const HydroCouple::Spatial::IGeometry* geom) const override;

    HydroCouple::Spatial::IGeometry* symmetricDifference(const HydroCouple::Spatial::IGeometry* geom) const override;

    GeometryFlags geometryFlags() const;

    virtual void setGeometryFlag(GeometryFlag flag, bool on = true);

    int marker() const;

    void setMarker(int index) ;

    std::set<HCGeometry*> children() const;

    void setParent(HCGeometry *parent);

    virtual Envelope *envelopeInternal() const = 0;

    static QString geometryTypeToString(HydroCouple::Spatial::IGeometry::GeometryType type);

    static OGRwkbGeometryType toOGRDataType(HydroCouple::Spatial::IGeometry::GeometryType type);

  protected:

    virtual void setIsEmpty(bool isEmpty);

  protected:

    Envelope *m_envelope;

  private:
    QString m_id, m_caption;
    int m_marker;
    unsigned int m_index;
    bool m_isEmpty;
    SpatialReferenceSystem *m_srs;
    HCGeometry::GeometryFlags m_geomFlags;
    std::set<HCGeometry*> m_children;
    HCGeometry *m_parent;
};

Q_DECLARE_METATYPE(HCGeometry*)
Q_DECLARE_OPERATORS_FOR_FLAGS(HCGeometry::GeometryFlags)

#endif // GEOMETRY_H

