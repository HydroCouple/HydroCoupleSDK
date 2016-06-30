#ifndef ABSTRACTADAPTEDOUTPUTFACTORY_H
#define ABSTRACTADAPTEDOUTPUTFACTORY_H

#include "identity.h"
#include "abstractadaptedoutputfactorycomponentinfo.h"

class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactory : public Identity,
    public virtual HydroCouple::IAdaptedOutputFactory
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IAdaptedOutputFactory)

  public:

    AbstractAdaptedOutputFactory(const QString& id, QObject* parent);

    virtual ~AbstractAdaptedOutputFactory();
};

class HYDROCOUPLESDK_EXPORT AbstractAdaptedOutputFactoryComponent : public AbstractAdaptedOutputFactory,
    public virtual HydroCouple::IAdaptedOutputFactoryComponent
{

    friend class AbstractAdaptedOutputFactoryComponentInfo;
    friend class TemporalInterpolationFactoryComponentInfo;

    Q_OBJECT
    Q_INTERFACES(HydroCouple::IAdaptedOutputFactoryComponent)
    Q_PROPERTY(HydroCouple::IAdaptedOutputFactoryComponentInfo* ComponentInfo READ componentInfo NOTIFY propertyChanged)

  public:

    AbstractAdaptedOutputFactoryComponent(const QString& id, QObject* parent);

    virtual ~AbstractAdaptedOutputFactoryComponent();

    HydroCouple::IAdaptedOutputFactoryComponentInfo* componentInfo() const override;

  signals:

    void propertyChanged(const QString &propertyName) override;

  protected:
    void setComponentInfo(AbstractAdaptedOutputFactoryComponentInfo* componentInfo);

  private:
    AbstractAdaptedOutputFactoryComponentInfo* m_componentInfo;

};


Q_DECLARE_METATYPE(AbstractAdaptedOutputFactory*)
Q_DECLARE_METATYPE(AbstractAdaptedOutputFactoryComponent*)

#endif // ABSTRACTADAPTEDOUTPUTFACTORY_H

