#include "stdafx.h"
#include "abstractmodelcomponent.h"
#include <QUuid>

using namespace HydroCouple;

AbstractModelComponent::AbstractModelComponent(AbstractModelComponent* parent)
   : Identity(parent), m_status(HydroCouple::Created)
{
   setId("1.0.00000.11" + QUuid::createUuid().toString());
   m_caption = "Test Model";
   m_description = "Test Model Description";
  // m_status = ComponentStatus::Created;
   m_parentModelComponent = parent;
}

AbstractModelComponent::AbstractModelComponent(const QString& id, const QString& caption, const QString& description, AbstractModelComponent* parent)
   : Identity(id,caption,description, parent), m_status(HydroCouple::Created)
{
   m_parentModelComponent = parent;
}

AbstractModelComponent::~AbstractModelComponent()
{
   qDeleteAll(m_children);
   m_children.clear();

   qDeleteAll(m_inputs);
   m_inputs.clear();

   qDeleteAll(m_outputs);
   m_outputs.clear();

   qDeleteAll(m_adaptedOutputFactories);
   m_adaptedOutputFactories.clear();

   qDeleteAll(m_arguments);
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

IModelComponent* AbstractModelComponent::clone() const
{

   AbstractModelComponent* parent = const_cast<AbstractModelComponent*>(this);
   AbstractModelComponent* component = new AbstractModelComponent(parent);
   component->setComponentInfo(m_modelComponentInfo);
   return component;
}

QList<IModelComponent*> AbstractModelComponent::children() const
{
   return m_children;
}

QList<IArgument*>  AbstractModelComponent::arguments() const
{
   return m_arguments;
}

ComponentStatus AbstractModelComponent::status() const
{
   return m_status;
}

QList<IInput*> AbstractModelComponent::inputs() const
{
   return m_inputs;
}

QList<IOutput*> AbstractModelComponent::outputs() const
{
   return m_outputs;
}

QList<IAdaptedOutputFactory*> AbstractModelComponent::adaptedOutputFactories() const
{
   return m_adaptedOutputFactories;
}

void AbstractModelComponent::initialize() 
{

}

QList<QString> AbstractModelComponent::validate() 
{
   return QList<QString>();
}

void AbstractModelComponent::prepare()
{

}

void AbstractModelComponent::update(const QList<IOutput*> & requiredOutputs)
{

}

void AbstractModelComponent::finish()
{

}

void AbstractModelComponent::setComponentInfo(IModelComponentInfo* const modelComponentInfo)
{
   m_modelComponentInfo = modelComponentInfo;
}
