#ifndef LINESTRING_H
#define LINESTRING_H

#include "geometry.h"

class HCPoint;

class HYDROCOUPLESDK_EXPORT HCLineString: public HCGeometry,
    public virtual HydroCouple::Spatial::ICurve,
    public virtual HydroCouple::Spatial::ILineString
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::ICurve
                 HydroCouple::Spatial::ILineString)

  public:

    HCLineString(QObject* parent = nullptr);

    virtual ~HCLineString();

    int dimension() const override;

    HydroCouple::Spatial::GeometryType geometryType() const override;

    HydroCouple::Spatial::IGeometry* envelope() const override;

    unsigned char* wkb(int &size) const override;

    double length() const override;

    HydroCouple::Spatial::IPoint* startPoint() const override;

    HydroCouple::Spatial::IPoint* endPoint() const override;

    bool isClosed() const override;

    bool isRing() const override;

    int pointCount() const override;

    HydroCouple::Spatial::IPoint* point(int index) const override;

    virtual void enable3D();

    virtual void disable3D();

    virtual void enableM();

    virtual void disableM();

    QList<HCPoint*> points() const;

    virtual void addPoint(HCPoint *point);

    virtual bool removePoint(HCPoint *point);

  protected:

    QList<HCPoint*> m_points;

};

class HYDROCOUPLESDK_EXPORT HCLine : public HCLineString,
    public virtual HydroCouple::Spatial::ILine
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::ILine)

  public:

    HCLine(QObject *parent = nullptr);

    virtual ~HCLine();

    bool isValid() const;

    void addPoint(HCPoint *point) override;

    bool removePoint(HCPoint *point) override;
    
    void setP1(HCPoint *point);
    
    void setP2(HCPoint *point);

};

class HYDROCOUPLESDK_EXPORT HCLinearRing : public HCLineString,
    public virtual HydroCouple::Spatial::ILinearRing
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::ILinearRing)

  public:

    HCLinearRing(QObject *parent = nullptr);

    virtual ~HCLinearRing();

    bool isValid() const;

};

Q_DECLARE_METATYPE(HCLineString*)
Q_DECLARE_METATYPE(HCLine*)
Q_DECLARE_METATYPE(HCLinearRing*)

#endif // LINESTRING_H

