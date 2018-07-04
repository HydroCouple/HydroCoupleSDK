#include "stdafx.h"
#include "composition/composition.h"
#include "composition/modelcomponent.h"

#include <QVector>

using namespace Composition;
using namespace HydroCouple;

ModelComposition::ModelComposition(bool disposeComponentsOnDelete, const QList<IModelComponent*> &searchComponents, QObject *parent)
  : QObject(parent),
    m_disposeComponentOnDelete(disposeComponentsOnDelete),
    m_workflowComponent(nullptr)
{
  findComponentsInComposition(searchComponents);
}

ModelComposition::~ModelComposition()
{
  while(m_modelComponents.size())
  {
    ModelComponent *modelComponent = m_modelComponents.first();
    delete modelComponent;
  }
}

QVector<ModelComponent *> ModelComposition::components() const
{
  return m_modelComponents;
}

const IWorkflowComponent *ModelComposition::workflowComponent() const
{
  return m_workflowComponent;
}

ModelComposition *ModelComposition::clone(QObject *parent)
{
  if(isCloneable())
  {
    ModelComposition *cloneModelComposition = nullptr;

    cloneModelComposition = new ModelComposition(m_disposeComponentOnDelete, QList<IModelComponent*>(), parent);

    HydroCouple::IWorkflowComponent *workflowComponent = nullptr;

    if(m_workflowComponent)
    {
      workflowComponent = m_workflowComponent->componentInfo()->createComponentInstance();
    }

    for(ModelComponent *modelComponent : m_modelComponents)
    {
      ModelComponent *cloneComponent = modelComponent->clone(cloneModelComposition);

      if(workflowComponent)
      {
        workflowComponent->addModelComponent(cloneComponent->modelComponent());
        cloneComponent->modelComponent()->setWorkflow(workflowComponent);
      }
    }

    for(int i = 0; i < m_modelComponents.size() ; i++)
    {
      cloneModelComposition->m_modelComponents[i]->copyConnections(m_modelComponents[i]);
    }

    return cloneModelComposition;
  }

  return nullptr;
}

bool ModelComposition::isCloneable() const
{
  for(ModelComponent *modelComponent : this->m_modelComponents)
  {
    if(dynamic_cast<ICloneableModelComponent*>(modelComponent->modelComponent()) == nullptr)
    {
      return false;
    }
  }

  return true;
}

void ModelComposition::establishConnections()
{
  for(ModelComponent *modelComponent : m_modelComponents)
    modelComponent->establishConnections();
}

void ModelComposition::disestablishConnections()
{
  for(ModelComponent *modelComponent : this->m_modelComponents)
  {
    modelComponent->disestablishConnections();
  }
}

void ModelComposition::reestablishConnections()
{
  for(ModelComponent *modelComponent : this->m_modelComponents)
  {
    modelComponent->reestablishConnections();
  }
}

bool ModelComposition::disposeComponentsOnDelete() const
{
  return m_disposeComponentOnDelete;
}

void ModelComposition::setDisposeComponentsOnDelete(bool disposeComponentsOnDelete)
{
  m_disposeComponentOnDelete = disposeComponentsOnDelete;
}

ModelComponent *ModelComposition::findModelComponent(IModelComponent *modelComponent)
{
  for(ModelComponent *compModelComponent : m_modelComponents)
  {
    if(compModelComponent->modelComponent() == modelComponent)
    {
      return compModelComponent;
    }
  }

  return nullptr;
}

ModelComponent *ModelComposition::findModelComponent(const QString &id)
{
  for(ModelComponent *compModelComponent : this->m_modelComponents)
  {
    if(!compModelComponent->modelComponent()->id().compare(id))
    {
      return compModelComponent;
    }
  }

  return nullptr;
}

void ModelComposition::findComponentsInComposition(const QList<IModelComponent *> &searchComponents)
{
  if(searchComponents.size() && searchComponents[0]->workflow())
  {
    IModelComponent *modelComponent = searchComponents[0];
    m_workflowComponent = modelComponent->workflow();

    for(IModelComponent *modelComponent : m_workflowComponent->modelComponents())
    {
      ModelComponent *modelComp = new ModelComponent(modelComponent, this);
    }
  }
  else
  {
    QSet<IModelComponent*> foundComponents;

    for(IModelComponent *component: searchComponents)
    {
      findComponentsInComposition(component, foundComponents);
    }

    for(IModelComponent *modelComponent : foundComponents)
    {
      if(modelComponent->workflow())
      {
        m_workflowComponent = modelComponent->workflow();
      }

      ModelComponent *modelComp = new ModelComponent(modelComponent, this);
    }
  }

  for(ModelComponent *modelComponent : m_modelComponents)
  {
    modelComponent->establishConnections();
  }
}

void ModelComposition::findComponentsInComposition(IModelComponent *modelComponent, QSet<IModelComponent *> &foundComponents)
{
  if(!foundComponents.contains(modelComponent))
  {
    foundComponents.insert(modelComponent);

    for(IInput *input : modelComponent->inputs())
    {
      IMultiInput *multiInput = nullptr;

      if(input->provider())
      {
        findComponentsInComposition(input->provider()->modelComponent(), foundComponents);
      }
      else if((multiInput = dynamic_cast<IMultiInput*>(input)))
      {
        for(IOutput *output : multiInput->providers())
        {
          findComponentsInComposition(output->modelComponent(), foundComponents);

          for(IOutput *adaptedOutput : output->adaptedOutputs())
          {
            findComponentsInComposition(adaptedOutput->modelComponent(), foundComponents);
          }
        }
      }
    }

    for(IOutput *output : modelComponent->outputs())
    {
      for(IInput *input : output->consumers())
      {
        findComponentsInComposition(input->modelComponent(), foundComponents);
      }

      for(IOutput *adaptedOutput : output->adaptedOutputs())
      {
        findComponentsInComposition(adaptedOutput->modelComponent(), foundComponents);
      }
    }
  }
}
