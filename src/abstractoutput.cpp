#include "stdafx.h"
#include "abstractoutput.h"


using namespace HydroCouple;

AbstractOutput::AbstractOutput(const QString &id, AbstractModelComponent *modelComponent)
   : AbstractExchangeItem(id, modelComponent)
{

}

AbstractOutput::AbstractOutput(const QString &id, const QString &caption, AbstractModelComponent *modelComponent)
   : AbstractExchangeItem(id, caption , modelComponent)
{

}

AbstractOutput::~AbstractOutput()
{

}


QList<HydroCouple::IInput*> AbstractOutput::consumers() const
{
   return m_consumers;
}


void AbstractOutput::addConsumer(HydroCouple::IInput* consumer)
{
   if(!m_consumers.contains(consumer))
      m_consumers.append(consumer);

   emit propertyChanged("Consumers");
}


bool AbstractOutput::removeConsumer(HydroCouple::IInput *consumer)
{
   bool happened = m_consumers.removeAll(consumer);
   emit propertyChanged("Consumers");
   return happened;
}


QList<HydroCouple::IAdaptedOutput*> AbstractOutput::adaptedOutputs()  const
{
   return m_adaptedOutputs;
}


void AbstractOutput::addAdaptedOutput(HydroCouple::IAdaptedOutput* adaptedOutput)
{
   if(!m_adaptedOutputs.contains(adaptedOutput))
      m_adaptedOutputs.append(adaptedOutput);

   emit propertyChanged("Consumers");
}


bool AbstractOutput::removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput)
{
   bool happened = m_adaptedOutputs.removeAll(adaptedOutput);
   emit propertyChanged("Consumers");
   return happened;
}

