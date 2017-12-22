#include "stdafx.h"
#include "hydrocouplespatiotemporal.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"
#include "core/unit.h"
#include "temporal/temporalinterpolationfactory.h"
#include "temporal/timeseriesinterpolationadaptedoutput.h"
#include "spatiotemporal/timegeometryinterpolationadaptedoutput.h"
#include "spatiotemporal/timetininterpolationadaptedoutput.h"

using namespace HydroCouple;
using namespace HydroCouple::Temporal;
using namespace HydroCouple::SpatioTemporal;

TemporalInterpolationFactory::TemporalInterpolationFactory(const QString &id, QObject *parent)
  :AbstractAdaptedOutputFactory(id,parent)
{
  setCaption("Temporal Interpolation Factory");
  setDescription("Provides a series of interpolation routines for time varying datasets");

  Identity* tsInterpolatorId = new Identity("TimeSeriesInterpolator", this);
  tsInterpolatorId->setCaption("Time Series Interpolator");
  tsInterpolatorId->setDescription("Time Series Interpolator");
  m_availableAdapters[tsInterpolatorId->id()] = tsInterpolatorId;

  Identity* tsGeomInterpolatorId = new Identity("TimeGeometryInterpolator", this);
  tsGeomInterpolatorId->setCaption("Time Geometry Interpolator");
  tsGeomInterpolatorId->setDescription("Time Geometry Interpolator");
  m_availableAdapters[tsGeomInterpolatorId->id()] = tsGeomInterpolatorId;

  Identity* tsTINInterpolatorId = new Identity("TimeTINInterpolator", this);
  tsTINInterpolatorId->setCaption("Time TIN Interpolator");
  tsTINInterpolatorId->setDescription("Time TIN Interpolator");
  m_availableAdapters[tsTINInterpolatorId->id()] = tsTINInterpolatorId;
}

TemporalInterpolationFactory::~TemporalInterpolationFactory()
{

}

QList<IIdentity*> TemporalInterpolationFactory::getAvailableAdaptedOutputIds(const IOutput *provider, const IInput *consumer)
{
  QList<IIdentity*> available;

  if(dynamic_cast<const ITimeSeriesComponentDataItem*>(provider) &&
     (consumer == nullptr || dynamic_cast<const ITimeSeriesComponentDataItem*>(consumer)) &&
     dynamic_cast<IQuantity*>(provider->valueDefinition()) &&
     provider->valueDefinition()->type() == QVariant::Double)
  {
    available.append(m_availableAdapters["TimeSeriesInterpolator"]);
  }

  if(dynamic_cast<const ITimeGeometryComponentDataItem*>(provider) &&
     dynamic_cast<IQuantity*>(provider->valueDefinition()) &&
     provider->valueDefinition()->type() == QVariant::Double)
  {
    available.append(m_availableAdapters["TimeGeometryInterpolator"]);
  }

  if(dynamic_cast<const ITimeTINComponentDataItem*>(provider) &&
     dynamic_cast<IQuantity*>(provider->valueDefinition()) &&
     provider->valueDefinition()->type() == QVariant::Double)
  {
    available.append(m_availableAdapters["TimeTINInterpolator"]);
  }

  return available;
}

IAdaptedOutput* TemporalInterpolationFactory::createAdaptedOutput(IIdentity *adaptedProviderId, IOutput *provider, IInput *consumer)
{
  if(!adaptedProviderId->id().compare("TimeSeriesInterpolator" , Qt::CaseInsensitive))
  {
    ITimeSeriesComponentDataItem* timeSeries = dynamic_cast<ITimeSeriesComponentDataItem*>(provider);
    Quantity *quantity = Quantity::copy(dynamic_cast<HydroCouple::IQuantity*>(provider->valueDefinition()), this);
    TimeSeriesInterpolationAdaptedOutput* tsAdaptedOutput = new TimeSeriesInterpolationAdaptedOutput(adaptedProviderId->id() + "->" + provider->id(),
                                                                                                     quantity,
                                                                                                     timeSeries, this);
    tsAdaptedOutput->setCaption(adaptedProviderId->caption() + " " + provider->id());
    tsAdaptedOutput->setInput(consumer);
    provider->addAdaptedOutput(tsAdaptedOutput);

    return tsAdaptedOutput;
  }
  else if(!adaptedProviderId->id().compare("TimeGeometryInterpolator" , Qt::CaseInsensitive))
  {
    ITimeGeometryComponentDataItem* timeGeometryOutput = dynamic_cast<ITimeGeometryComponentDataItem*>(provider);
    Quantity *quantity = Quantity::copy(dynamic_cast<HydroCouple::IQuantity*>(provider->valueDefinition()), this);
    TimeGeometryInterpolationAdaptedOutput* timeGeometryAdaptedOutput = new TimeGeometryInterpolationAdaptedOutput(adaptedProviderId->id() + "->" +provider->id(),
                                                                                                                   quantity,
                                                                                                                   timeGeometryOutput, this);
    timeGeometryAdaptedOutput->setCaption(adaptedProviderId->caption() + " " + provider->id());
    timeGeometryAdaptedOutput->setInput(consumer);
    provider->addAdaptedOutput(timeGeometryAdaptedOutput);

    return timeGeometryAdaptedOutput;
  }
  else if(!adaptedProviderId->id().compare("TimeTINInterpolator" , Qt::CaseInsensitive))
  {
    ITimeTINComponentDataItem *timeTINOutput = dynamic_cast<ITimeTINComponentDataItem*>(provider);
    Quantity *quantity = Quantity::copy(dynamic_cast<HydroCouple::IQuantity*>(provider->valueDefinition()), this);
    TimeTINInterpolationAdaptedOutput *timeTINAdaptedOutput = new TimeTINInterpolationAdaptedOutput(adaptedProviderId->id() + "->" +provider->id(),
                                                                                                    quantity,
                                                                                                    timeTINOutput, this);
    timeTINAdaptedOutput->setCaption(adaptedProviderId->caption() + " " + provider->id());
    timeTINAdaptedOutput->setInput(consumer);
    provider->addAdaptedOutput(timeTINAdaptedOutput);

    return timeTINAdaptedOutput;
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

  Identity* tsGeomInterpolatorId = new Identity("TimeGeometryInterpolator", this);
  tsGeomInterpolatorId->setCaption("Time Geometry Interpolator");
  tsGeomInterpolatorId->setDescription("Time Geometry Interpolator");
  m_availableAdapters[tsGeomInterpolatorId->id()] = tsGeomInterpolatorId;

  Identity* tsTINInterpolatorId = new Identity("TimeTINInterpolator", this);
  tsTINInterpolatorId->setCaption("Time TIN Interpolator");
  tsTINInterpolatorId->setDescription("Time TIN Interpolator");
  m_availableAdapters[tsTINInterpolatorId->id()] = tsTINInterpolatorId;

}

TemporalInterpolationFactoryComponent::~TemporalInterpolationFactoryComponent()
{

}

QList<IIdentity*> TemporalInterpolationFactoryComponent::getAvailableAdaptedOutputIds(const IOutput *provider, const IInput *consumer)
{
  QList<IIdentity*> available;

  if(dynamic_cast<const ITimeSeriesComponentDataItem*>(provider) &&
     (consumer == nullptr || dynamic_cast<const ITimeSeriesComponentDataItem*>(consumer)))
  {
    available.append(m_availableAdapters["TimeSeriesInterpolator"]);
  }

  if(dynamic_cast<const ITimeGeometryComponentDataItem*>(provider))
  {
    available.append(m_availableAdapters["TimeGeometryInterpolator"]);
  }

  if(dynamic_cast<const ITimeTINComponentDataItem*>(provider))
  {
    available.append(m_availableAdapters["TimeTINInterpolator"]);
  }

  return available;
}

IAdaptedOutput* TemporalInterpolationFactoryComponent::createAdaptedOutput(IIdentity *adaptedProviderId, IOutput *provider, IInput *consumer)
{
  if(!adaptedProviderId->id().compare("TimeSeriesInterpolation" , Qt::CaseInsensitive))
  {
    ITimeSeriesComponentDataItem* timeSeries = dynamic_cast<ITimeSeriesComponentDataItem*>(provider);

    TimeSeriesInterpolationAdaptedOutput* tsAdaptedOutput = new TimeSeriesInterpolationAdaptedOutput(adaptedProviderId->id() + provider->id(),
                                                                                                     new Quantity(QVariant::Double , Unit::unitlessCoefficient(this), this),
                                                                                                     timeSeries, this);
    tsAdaptedOutput->setCaption(adaptedProviderId->caption() + " " + provider->id());
    tsAdaptedOutput->setInput(consumer);
    provider->addAdaptedOutput(tsAdaptedOutput);

    return tsAdaptedOutput;
  }
  else if(!adaptedProviderId->id().compare("TimeGeometryInterpolator" , Qt::CaseInsensitive))
  {
    ITimeGeometryComponentDataItem* timeGeometryOutput = dynamic_cast<ITimeGeometryComponentDataItem*>(provider);
    Quantity *quantity = Quantity::copy(dynamic_cast<HydroCouple::IQuantity*>(provider->valueDefinition()), this);
    TimeGeometryInterpolationAdaptedOutput* timeGeometryAdaptedOutput = new TimeGeometryInterpolationAdaptedOutput(adaptedProviderId->id() + "->" +provider->id(),
                                                                                                                   quantity,
                                                                                                                   timeGeometryOutput, this);
    timeGeometryAdaptedOutput->setCaption(adaptedProviderId->caption() + " " + provider->id());
    timeGeometryAdaptedOutput->setInput(consumer);
    provider->addAdaptedOutput(timeGeometryAdaptedOutput);

    return timeGeometryAdaptedOutput;
  }
  else if(!adaptedProviderId->id().compare("TimeTINInterpolator" , Qt::CaseInsensitive))
  {
    ITimeTINComponentDataItem *timeTINOutput = dynamic_cast<ITimeTINComponentDataItem*>(provider);
    Quantity *quantity = Quantity::copy(dynamic_cast<HydroCouple::IQuantity*>(provider->valueDefinition()), this);
    TimeTINInterpolationAdaptedOutput *timeTINAdaptedOutput = new TimeTINInterpolationAdaptedOutput(adaptedProviderId->id() + "->" +provider->id(),
                                                                                                    quantity,
                                                                                                    timeTINOutput, this);
    timeTINAdaptedOutput->setCaption(adaptedProviderId->caption() + " " + provider->id());
    timeTINAdaptedOutput->setInput(consumer);
    provider->addAdaptedOutput(timeTINAdaptedOutput);

    return timeTINAdaptedOutput;
  }

  return nullptr;

}

//******************************************************************************************************************************************************

TemporalInterpolationFactoryComponentInfo::TemporalInterpolationFactoryComponentInfo(QObject *parent)
  :AbstractAdaptedOutputFactoryComponentInfo(parent)
{
  setId("HydroCoupleSDK Temporal Interpolation AdaptedOutput Factory Component");
  setCaption("Temporal Interpolation Factory");
  setIconFilePath(":/HydroCoupleSDK/timeseriesinterpolator");
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
