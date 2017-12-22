#ifndef ABSTRACTTIMEMODELCOMPONENT_H
#define ABSTRACTTIMEMODELCOMPONENT_H

#include "hydrocoupletemporal.h"
#include "core/abstractmodelcomponent.h"

namespace SDKTemporal
{
  class DateTime;
  class TimeSpan;
}

class HYDROCOUPLESDK_EXPORT AbstractTimeModelComponent : public AbstractModelComponent,
    public virtual HydroCouple::Temporal::ITimeModelComponent
{
    Q_INTERFACES(HydroCouple::Temporal::ITimeModelComponent)
    Q_PROPERTY(SDKTemporal::DateTime* CurrentDateTime READ currentDateTime NOTIFY propertyChanged)
    Q_PROPERTY(SDKTemporal::TimeSpan* TimeHorizon READ timeHorizon NOTIFY propertyChanged)

  public:

    AbstractTimeModelComponent(const QString &id, AbstractModelComponentInfo *modelComponentInfo = nullptr);

    AbstractTimeModelComponent(const QString &id, const QString &caption, AbstractModelComponentInfo *modelComponentInfo = nullptr);

    virtual ~AbstractTimeModelComponent();

    HydroCouple::Temporal::IDateTime *currentDateTime() const override;

    HydroCouple::Temporal::ITimeSpan *timeHorizon() const override;

  protected:

    SDKTemporal::DateTime *currentDateTimeInternal() const;

    SDKTemporal::TimeSpan *timeHorizonInternal() const;

  private:

    SDKTemporal::DateTime *m_dateTime;
    SDKTemporal::TimeSpan *m_timeHorizon;
};

Q_DECLARE_METATYPE(AbstractTimeModelComponent*)

#endif // ABSTRACTTIMEMODELCOMPONENT_H
