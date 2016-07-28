#include "stdafx.h"
#include "temporal/temporalinterpolationfactory.h"
#include "temporal/timeseriesinterpolationadaptedoutput.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/unit.h"

using namespace HydroCouple;
using namespace HydroCouple::Temporal;

TemporalInterpolationFactory::TemporalInterpolationFactory(const QString &id, QObject *parent)
  :AbstractAdaptedOutputFactory(id,parent)
{
  setCaption("Temporal Interpolation Factory");
  setDescription("Provides a series of interpolation routines for time varying datasets");

  Identity* tsInterpolatorId = new Identity("TimeSeriesInterpolator", this);
  tsInterpolatorId->setCaption("Time Series Interpolator");
  tsInterpolatorId->setDescription("Time Series Interpolator");
  m_availableAdapters[tsInterpolatorId->id()] = tsInterpolatorId;
}

TemporalInterpolationFactory::~TemporalInterpolationFactory()
{

}

QList<IIdentity*> TemporalInterpolationFactory::getAvailableAdaptedOutputIds(const IOutput *provider, const IInput *consumer)
{
  QList<IIdentity*> available;

  if(dynamic_cast<const ITimeExchangeItem*>(provider) &&
     (consumer == nullptr || dynamic_cast<const ITimeExchangeItem*>(consumer)))
  {
    available.append(m_availableAdapters["TimeSeriesInterpolator"]);
  }

  return available;
}

IAdaptedOutput* TemporalInterpolationFactory::createAdaptedOutput(IIdentity *adaptedProviderId, IOutput *provider, IInput *consumer)
{
  if(adaptedProviderId->id().compare("TimeSeriesInterpolation" , Qt::CaseInsensitive))
  {

    ITimeExchangeItem* timeSeries = dynamic_cast<ITimeExchangeItem*>(provider);

    TimeSeriesInterpolationAdaptedOutput* tsAdaptedOutput = new TimeSeriesInterpolationAdaptedOutput(adaptedProviderId->id() + provider->id(),
                                                                                                     new Quantity(QVariant::Double , Unit::unitlessCoefficient(this), this),
                                                                                                     timeSeries, this);
    tsAdaptedOutput->setCaption(adaptedProviderId->caption() + " " + provider->id());
    tsAdaptedOutput->setInput(consumer);
    provider->addAdaptedOutput(tsAdaptedOutput);

    return tsAdaptedOutput;
  }

  return nullptr;

}

//******************************************************************************************************************************************************

TemporalInterpolationFactoryComponent::TemporalInterpolationFactoryComponent(const QString &id, QObject *parent)
 : AbstractAdaptedOutputFactoryComponent(id, parent)
{

  setCaption("Temporal Interpolation Factory");
  setDescription("Provides a series of interpolation routines for time varying datasets");

  Identity* tsInterpolatorId = new Identity("TimeSeriesInterpolator", this);
  tsInterpolatorId->setCaption("Time Series Interpolator");
  tsInterpolatorId->setDescription("Time Series Interpolator");
  m_availableAdapters[tsInterpolatorId->id()] = tsInterpolatorId;

}

TemporalInterpolationFactoryComponent::~TemporalInterpolationFactoryComponent()
{

}

QList<IIdentity*> TemporalInterpolationFactoryComponent::getAvailableAdaptedOutputIds(const IOutput *provider, const IInput *consumer)
{
  QList<IIdentity*> available;

  if(dynamic_cast<const ITimeExchangeItem*>(provider) &&
     (consumer == nullptr || dynamic_cast<const ITimeExchangeItem*>(consumer)))
  {
    available.append(m_availableAdapters["TimeSeriesInterpolator"]);
  }

  return available;
}

IAdaptedOutput* TemporalInterpolationFactoryComponent::createAdaptedOutput(IIdentity *adaptedProviderId, IOutput *provider, IInput *consumer)
{
  if(adaptedProviderId->id().compare("TimeSeriesInterpolation" , Qt::CaseInsensitive))
  {

    ITimeExchangeItem* timeSeries = dynamic_cast<ITimeExchangeItem*>(provider);

    TimeSeriesInterpolationAdaptedOutput* tsAdaptedOutput = new TimeSeriesInterpolationAdaptedOutput(adaptedProviderId->id() + provider->id(),
                                                                                                     new Quantity(QVariant::Double , Unit::unitlessCoefficient(this), this),
                                                                                                     timeSeries, this);
    tsAdaptedOutput->setCaption(adaptedProviderId->caption() + " " + provider->id());
    tsAdaptedOutput->setInput(consumer);
    provider->addAdaptedOutput(tsAdaptedOutput);

    return tsAdaptedOutput;
  }

  return nullptr;

}

//******************************************************************************************************************************************************

TemporalInterpolationFactoryComponentInfo::TemporalInterpolationFactoryComponentInfo(QObject *parent)
  :AbstractAdaptedOutputFactoryComponentInfo(parent)
{
  setCaption("Temporal Interpolation");
  setIconFilePath(":/HydroCoupleComposer/adaptercomponent");
}

TemporalInterpolationFactoryComponentInfo::~TemporalInterpolationFactoryComponentInfo()
{

}

IAdaptedOutputFactoryComponent* TemporalInterpolationFactoryComponentInfo::createComponentInstance()
{
   AbstractAdaptedOutputFactoryComponent* factoryComponent = new TemporalInterpolationFactoryComponent("Temporal Interpolation Factory" , this);
   factoryComponent->setComponentInfo(this);
   return factoryComponent;
}
