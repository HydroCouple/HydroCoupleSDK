#include "stdafx.h"
#include "abstractmodelcomponent.h"
#include "modelcomponentinfo.h"
#include "componentstatuschangeeventargs.h"
#include "abstractargument.h"
#include "abstractinput.h"
#include "abstractoutput.h"

using namespace HydroCouple;


AbstractModelComponent::AbstractModelComponent(const QString &id, AbstractModelComponent *parent)
   : Identity(id, parent), m_status(HydroCouple::Created)
{
   m_parentModelComponent = parent;
   emit componentStatusChanged(std::shared_ptr<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(HydroCouple::Created , HydroCouple::Created , "Created SWMM Model" , 0 , this)));
}


AbstractModelComponent::AbstractModelComponent(const QString &id, const QString &caption, AbstractModelComponent *parent)
   : Identity(id,caption,parent), m_status(HydroCouple::Created)
{
   m_parentModelComponent = parent;
   emit componentStatusChanged(std::shared_ptr<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(HydroCouple::Created , HydroCouple::Created , "Created SWMM Model" , 0 , this)));
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

//   if(m_componentStatusChangedEventArg)
//   {
//      delete m_componentStatusChangedEventArg;
//      m_componentStatusChangedEventArg = nullptr;
//   }
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
   return m_adaptedOutputFactories.values();
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

QHash<QString,AbstractModelComponent*> AbstractModelComponent::clonesMap() const
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

QHash<QString,AbstractInput*> AbstractModelComponent::inputsMap() const
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

QHash<QString,AbstractOutput*> AbstractModelComponent::outputsMap() const
{
   return m_outputs;
}

void AbstractModelComponent::addAdaptedOutputFactory(IAdaptedOutputFactory *adaptedOutputFactory)
{
   if(!m_adaptedOutputFactories.contains(adaptedOutputFactory->id()))
   {
      m_adaptedOutputFactories[adaptedOutputFactory->id()] = adaptedOutputFactory;
      emit propertyChanged("AdaptedOutputFactories");
   }
}

bool AbstractModelComponent::removeAdaptedOutputFactory(IAdaptedOutputFactory *adaptedOutputFactory)
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

QHash<QString,AbstractArgument*> AbstractModelComponent::argumentsMap() const
{
   return m_arguments;
}

void AbstractModelComponent::setStatus(HydroCouple::ComponentStatus status)
{
   m_status = status;
}

void AbstractModelComponent::setStatus(ComponentStatus status, const QString &message)
{
   emit componentStatusChanged(std::shared_ptr<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(status , m_status ,message, this)));
   m_status = status;
}

void AbstractModelComponent::setStatus(ComponentStatus status, const QString &message, int progress)
{
   emit componentStatusChanged(std::shared_ptr<IComponentStatusChangeEventArgs>(new ComponentStatusChangeEventArgs(status , m_status ,message, progress, this)));
   m_status = status;
}
