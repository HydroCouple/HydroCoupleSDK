#ifndef SPATIALREFERENCESYSTEM_H
#define SPATIALREFERENCESYSTEM_H

#include "core/description.h"
#include "hydrocouplespatial.h"

#include <map>
#include <string>

class OGRSpatialReference;

class SpatialReferenceSystem : public Description,
    public virtual HydroCouple::Spatial::ISpatialReferenceSystem
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::Spatial::ISpatialReferenceSystem)

    Q_PROPERTY(int AuthSRID READ authSRID NOTIFY propertyChanged)
    Q_PROPERTY(QString AuthName READ authName NOTIFY propertyChanged)
    Q_PROPERTY(QString WellKnownText READ srText WRITE setSrText NOTIFY propertyChanged)

  public:

    SpatialReferenceSystem(QObject* parent = nullptr);

    SpatialReferenceSystem(const QString& caption, const QString& srText , QObject* parent = nullptr);

    SpatialReferenceSystem(const QString& caption, const QString& authName, int authSRID , QObject* parent = nullptr);

    virtual ~SpatialReferenceSystem();

    int authSRID() const override;

    QString authName() const override;

    void setAuth(const QString& name , int authSRID);

    QString srText() const override;

    void setSrText(const QString& srtext);

    static std::string findEPSGWkt(int authSRID, bool& found);

  signals:

    void propertyChanged(const QString& propertyName) override;

  private:
    OGRSpatialReference* m_srs;
    static const std::map<int,std::string> m_EPGSAuthorityWkts;
};



#endif // SPATIALREFERENCESYSTEM_H

