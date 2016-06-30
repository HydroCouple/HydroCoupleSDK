#ifndef ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H
#define ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H

#include "componentinfo.h"

class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactoryComponentInfo : public ComponentInfo,
    public virtual HydroCouple::IAdaptedOutputFactoryComponentInfo
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IAdaptedOutputFactoryComponentInfo)

  public:
    AbstractAdaptedOutputFactoryComponentInfo(QObject* parent);

    virtual ~AbstractAdaptedOutputFactoryComponentInfo();

    bool validateLicense(QString &validationMessage) const override;

    bool hasValidLicense() const override;


};

Q_DECLARE_METATYPE(AbstractAdaptedOutputFactoryComponentInfo*)

#endif // ABSTRACTADAPTEDOUTPUTFACTORYCOMPONENTINFO_H

