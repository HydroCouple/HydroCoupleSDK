#ifndef TIMESERIESINTERPOLATIONFACTORY_H
#define TIMESERIESINTERPOLATIONFACTORY_H

#include "core/abstractadaptedoutputfactory.h"
#include "core/componentinfo.h"



class TemporalInterpolationFactory : public AbstractAdaptedOutputFactory
{
    Q_OBJECT

  public:

    TemporalInterpolationFactory(const QString& id, QObject* parent);

    virtual ~TemporalInterpolationFactory();

    QList<IIdentity*> getAvailableAdaptedOutputIds(const HydroCouple::IOutput *provider, const HydroCouple::IInput *consumer) override;

    HydroCouple::IAdaptedOutput* createAdaptedOutput(HydroCouple::IIdentity *adaptedProviderId, HydroCouple::IOutput *provider, HydroCouple::IInput *consumer) override;

  private:
    QHash<QString,IIdentity*> m_availableAdapters;
};

//******************************************************************************************************************************************************

class TemporalInterpolationFactoryComponent : public AbstractAdaptedOutputFactoryComponent,
    public virtual HydroCouple::IAdaptedOutputFactoryComponent
{

    Q_OBJECT
    Q_INTERFACES(HydroCouple::IAdaptedOutputFactoryComponent)

  public:
    TemporalInterpolationFactoryComponent(const QString& id, QObject* parent);

    virtual ~TemporalInterpolationFactoryComponent();

    QList<IIdentity*> getAvailableAdaptedOutputIds(const HydroCouple::IOutput *provider, const HydroCouple::IInput *consumer) override;

    HydroCouple::IAdaptedOutput* createAdaptedOutput(HydroCouple::IIdentity *adaptedProviderId, HydroCouple::IOutput *provider, HydroCouple::IInput *consumer) override;

  private:
    QHash<QString,IIdentity*> m_availableAdapters;
};

//******************************************************************************************************************************************************

class TemporalInterpolationFactoryComponentInfo : public AbstractAdaptedOutputFactoryComponentInfo
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "TemporalInterpolationComponentInfo")

  public:
    TemporalInterpolationFactoryComponentInfo(QObject* parent = nullptr);

    virtual ~TemporalInterpolationFactoryComponentInfo();

    HydroCouple::IAdaptedOutputFactoryComponent* createComponentInstance() override;

};

Q_DECLARE_METATYPE(TemporalInterpolationFactory*)

#endif // TIMESERIESINTERPOLATIONFACTORY_H

