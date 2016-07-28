#ifndef POINT_H
#define POINT_H

#include "geometry.h"

class HYDROCOUPLESDK_EXPORT HCPoint: public HCGeometry,
    public virtual HydroCouple::Spatial::IPoint
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IPoint)

    friend class HCLineString;
    friend class HCMultiPoint;
    friend class HCLine;

  public:

    HCPoint(QObject* parent = nullptr);

    HCPoint(double x, double y, QObject* parent = nullptr);

    HCPoint(double x, double y, double z, QObject* parent = nullptr);

    HCPoint(double x, double y, double z, double m, QObject* parent = nullptr);

    virtual ~HCPoint();

    double x() const override;

    double y() const override;

    double z() const override;

    double m() const override;

    void setX(double x);

    void setY(double y);

    void setZ(double z);

    void setM(double m);

    int dimension() const override;

    HydroCouple::Spatial::GeometryType geometryType() const override;

    HydroCouple::Spatial::IGeometry* envelope() const override;

    unsigned char* wkb(int &size) const override;

    bool compare(const HydroCouple::Spatial::IPoint *point) const;

    double dist(const HydroCouple::Spatial::IPoint *point) const;

  protected:

    void setGeometryFlag(HCGeometry::GeometryFlag flag, bool on) override;

  private:
    double m_x , m_y, m_z, m_m;
    bool m_isEmpty;
};

class HYDROCOUPLESDK_EXPORT HCVertex : public HCPoint,
    public virtual HydroCouple::Spatial::IVertex
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::Spatial::IVertex)

  public:

    HCVertex(QObject* parent = nullptr);

    HCVertex(double x, double y, QObject* parent = nullptr);

    HCVertex(double x, double y, double z, QObject* parent = nullptr);

    HCVertex(double x, double y, double z, double m, QObject* parent = nullptr);

    virtual ~HCVertex();

    HydroCouple::Spatial::IEdge* edge() const override;

    void addEdge(HydroCouple::Spatial::IEdge *edge) ;

    void removeEdge(HydroCouple::Spatial::IEdge *edge) ;

    void initializeData(int length);

    int dataLength() const override;

    double* data() const override;

  private:
    HydroCouple::Spatial::IEdge* m_edge;
    int m_dataLength;
    double* m_data;
};

Q_DECLARE_METATYPE(HCPoint*)
Q_DECLARE_METATYPE(HCVertex*)

#endif // POINT_H

