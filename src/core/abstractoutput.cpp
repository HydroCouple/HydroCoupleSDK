#include "stdafx.h"
#include "core/abstractoutput.h"


using namespace HydroCouple;

AbstractOutput::AbstractOutput(const QString& id,
                               const QList<Dimension*>& dimensions,
                               ValueDefinition* valueDefinition,
                               AbstractModelComponent *modelComponent)
   : AbstractExchangeItem(id, dimensions,valueDefinition, modelComponent)
{

}

AbstractOutput::AbstractOutput(const QString& id,
                               const QString& caption,
                               const QList<Dimension*>& dimensions,
                               ValueDefinition* valueDefinition,
                               AbstractModelComponent *modelComponent)
   : AbstractExchangeItem(id, caption , dimensions, valueDefinition, modelComponent)
{

}

AbstractOutput::~AbstractOutput()
{

}

QList<HydroCouple::IInput*> AbstractOutput::consumers() const
{
   return m_consumers.values();
}

void AbstractOutput::addConsumer(HydroCouple::IInput* consumer)
{
   if(!m_consumers.contains(consumer->id()))
      m_consumers[consumer->id()] = consumer;

   emit propertyChanged("Consumers");
}

bool AbstractOutput::removeConsumer(HydroCouple::IInput *consumer)
{
   bool happened = m_consumers.remove(consumer->id());

   emit propertyChanged("Consumers");
   return happened;
}

QList<HydroCouple::IAdaptedOutput*> AbstractOutput::adaptedOutputs()  const
{
   return m_adaptedOutputs.values();
}

void AbstractOutput::addAdaptedOutput(HydroCouple::IAdaptedOutput* adaptedOutput)
{
   if(!m_adaptedOutputs.contains(adaptedOutput->id()))
      m_adaptedOutputs[adaptedOutput->id()] = adaptedOutput;

   emit propertyChanged("Consumers");
}

bool AbstractOutput::removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput)
{
   bool happened = m_adaptedOutputs.remove(adaptedOutput->id());
   emit propertyChanged("Consumers");
   return happened;
}

void AbstractOutput::update(IInput *querySpecifier)
{
  while(modelComponent()->status() != HydroCouple::Done &&
        modelComponent()->status() != HydroCouple::Failed &&
        modelComponent()->status() != HydroCouple::Finished)
  {
    modelComponent()->update();
  }
}
