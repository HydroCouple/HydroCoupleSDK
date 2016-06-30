#include "stdafx.h"
#include "core/abstractmodelcomponent.h"
#include "core/modelcomponentinfo.h"
#include "core/componentstatuschangeeventargs.h"
#include "core/abstractargument.h"
#include "core/abstractinput.h"
#include "core/abstractoutput.h"
#include "core/abstractadaptedoutputfactory.h"
#include <QSharedPointer>

using namespace HydroCouple;


AbstractModelComponent::AbstractModelComponent(const QString &id, AbstractModelComponent *parent)
   : Identity(id, parent), m_status(HydroCouple::Created)
{
   m_parentModelComponent = parent;
   emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(HydroCouple::Created , HydroCouple::Created , "Created SWMM Model" , 0 , this)));
}


AbstractModelComponent::AbstractModelComponent(const QString &id, const QString &caption, AbstractModelComponent *parent)
   : Identity(id,caption,parent), m_status(HydroCouple::Created)
{
   m_parentModelComponent = parent;
   emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(HydroCouple::Created , HydroCouple::Created , "Created SWMM Model" , 0 , this)));
}

AbstractModelComponent::~AbstractModelComponent()
{
   qDeleteAll(m_children.values());
   m_children.clear();

   qDeleteAll(m_inputs.values());
   m_inputs.clear();

   qDeleteAll(m_outputs.values());
   m_outputs.clear();

   qDeleteAll(m_adaptedOutputFactories.values());
   m_adaptedOutputFactories.clear();

   qDeleteAll(m_arguments.values());
   m_arguments.clear();
}

IModelComponentInfo* AbstractModelComponent::componentInfo() const
{
   return m_modelComponentInfo;
}

IModelComponent* AbstractModelComponent::parent() const
{
   return m_parentModelComponent;
}

QList<IModelComponent*> AbstractModelComponent::clones() const
{
   QList<IModelComponent*> clones ;

   for(AbstractModelComponent *modelComponent : m_children.values())
   {
      clones.append(modelComponent);
   }

   return clones;
}

QList<IArgument*>  AbstractModelComponent::arguments() const
{
   QList<IArgument*> arguments;

   for(AbstractArgument *argument : m_arguments.values())
   {
      arguments.append(argument);
   }

   return arguments;
}

ComponentStatus AbstractModelComponent::status() const
{
   return m_status;
}

QList<IInput*> AbstractModelComponent::inputs() const
{
   QList<IInput*> inputs;

   for(AbstractInput *input : m_inputs)
   {
      inputs.append(input);
   }

   return inputs;
}

QList<IOutput*> AbstractModelComponent::outputs() const
{
   QList<IOutput*> outputs;

   for(AbstractOutput *output : m_outputs.values())
   {
      outputs.append(output);
   }

   return outputs;
}

QList<IAdaptedOutputFactory*> AbstractModelComponent::adaptedOutputFactories() const
{

  QList<IAdaptedOutputFactory*> factories;

  for(IAdaptedOutputFactory* factory : m_adaptedOutputFactories.values())
  {
     factories.append(factory);
  }

  return factories;
}

void AbstractModelComponent::setComponentInfo(ModelComponentInfo* modelComponentInfo)
{
   m_modelComponentInfo = modelComponentInfo;
   emit propertyChanged("ComponentInfo");
}

void AbstractModelComponent::addChildComponent(AbstractModelComponent *modelComponent)
{
   if(!m_children.contains(modelComponent->id()))
   {
      m_children[modelComponent->id()] = modelComponent;
      emit propertyChanged("Clones");
   }
}

bool AbstractModelComponent::removeChildComponent(AbstractModelComponent *modelComponent)
{
   if(m_children.contains(modelComponent->id()))
   {
      m_children.remove(modelComponent->id());
      emit propertyChanged("Clones");
      return true;
   }

   return false;
}

void AbstractModelComponent::clearChildComponents()
{
   qDeleteAll(m_children.values());
   m_children.clear();
   emit propertyChanged("Clones");
}

QHash<QString,AbstractModelComponent*> AbstractModelComponent::clonesInternal() const
{
   return m_children;
}

void AbstractModelComponent::addInputExchangeItem(AbstractInput *input)
{
   if(!m_inputs.contains(input->id()))
   {
      m_inputs[input->id()] = input;
      emit propertyChanged("Inputs");
   }
}

bool AbstractModelComponent::removeInputExchangeItem(AbstractInput *input)
{
   if(m_inputs.contains(input->id()))
   {
      m_inputs.remove(input->id());
      emit propertyChanged("Inputs");
      return true;
   }

   return false;
}

void AbstractModelComponent::clearInputExchangeItems()
{
   qDeleteAll(m_inputs.values());
   m_inputs.clear();
   emit propertyChanged("Inputs");
}

QHash<QString,AbstractInput*> AbstractModelComponent::inputsInternal() const
{
   return m_inputs;
}

void AbstractModelComponent::addOutputExchangeItem(AbstractOutput *output)
{
   if(!m_outputs.contains(output->id()))
   {
      m_outputs[output->id()] = output;
      emit propertyChanged("Outputs");
   }
}

bool AbstractModelComponent::removeOutputExchangeItem(AbstractOutput *output)
{
   if(m_outputs.contains(output->id()))
   {
      m_outputs.remove(output->id());
      emit propertyChanged("Outputs");
      return true;
   }

   return false;
}

void AbstractModelComponent::clearOutputExchangeItems()
{
   qDeleteAll(m_outputs.values());
   m_outputs.clear();

   emit propertyChanged("Outputs");
}

QHash<QString,AbstractAdaptedOutputFactory*> AbstractModelComponent::adaptedOutputFactoriesInternal() const
{
   return m_adaptedOutputFactories;
}

void AbstractModelComponent::addAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory)
{
   if(!m_adaptedOutputFactories.contains(adaptedOutputFactory->id()))
   {
      m_adaptedOutputFactories[adaptedOutputFactory->id()] = adaptedOutputFactory;
      emit propertyChanged("AdaptedOutputFactories");
   }
}

bool AbstractModelComponent::removeAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory)
{
   if(m_adaptedOutputFactories.contains(adaptedOutputFactory->id()))
   {
      m_adaptedOutputFactories.remove(adaptedOutputFactory->id());
      emit propertyChanged("AdaptedOutputFactories");
      return true;
   }

   return false;
}

void AbstractModelComponent::clearAdaptedOutputFactory()
{
   qDeleteAll(m_adaptedOutputFactories.values());
   m_adaptedOutputFactories.clear();

   emit propertyChanged("AdaptedOutputFactories");
}

QHash<QString,AbstractOutput*> AbstractModelComponent::outputsInternal() const
{
   return m_outputs;
}

void AbstractModelComponent::addArgument(AbstractArgument *argument)
{
   if(!m_arguments.contains(argument->id()))
   {
      m_arguments[argument->id()] = argument;
      emit propertyChanged("Arguments");
   }
}

bool AbstractModelComponent::removeArgument(AbstractArgument *argument)
{
   if(m_arguments.contains(argument->id()))
   {
      m_arguments.remove(argument->id());
      emit propertyChanged("Arguments");
      return true;
   }

   return false;
}

void AbstractModelComponent::clearArguments()
{
   qDeleteAll(m_arguments.values());
   m_arguments.clear();

   emit propertyChanged("Arguments");
}

QHash<QString,AbstractArgument*> AbstractModelComponent::argumentsInternal() const
{
   return m_arguments;
}

void AbstractModelComponent::setStatus(HydroCouple::ComponentStatus status)
{
   m_status = status;
}

void AbstractModelComponent::setStatus(ComponentStatus status, const QString &message)
{
   HydroCouple::ComponentStatus oldStatus = m_status;
   m_status = status;
   emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(m_status , oldStatus ,message, this)));
}

void AbstractModelComponent::setStatus(ComponentStatus status, const QString &message, int progress)
{
   HydroCouple::ComponentStatus oldStatus = m_status;
   m_status = status;
   emit componentStatusChanged(QSharedPointer<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(m_status , oldStatus ,message, progress, this)));
}
