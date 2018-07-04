#include "stdafx.h"
#include "composition/modelcomponent.h"
#include "composition/composition.h"
#include "composition/exchangeitems.h"

using namespace Composition;
using namespace HydroCouple;

ModelComponent::ModelComponent(HydroCouple::IModelComponent *modelComponent, ModelComposition *modelComposition)
  : QObject(modelComposition),
    m_modelComponent(modelComponent),
    m_modelComposition(modelComposition)
{
  m_modelComposition->m_modelComponents.push_back(this);

  QObject *modelComponentObject = dynamic_cast<QObject*>(modelComponent);
  connect(modelComponentObject, SIGNAL(componentStatusChanged(const QSharedPointer<HydroCouple::IComponentStatusChangeEventArgs> &)),
          this, SLOT(onComponentStatusChanged(const QSharedPointer<HydroCouple::IComponentStatusChangeEventArgs> &)));

  createExchangeItems();
}

ModelComponent::~ModelComponent()
{

  deleteExchangeItems();

  if(m_modelComposition->disposeComponentsOnDelete())
  {
    disestablishConnections();
    delete m_modelComponent;
  }

  m_modelComposition->m_modelComponents.removeOne(this);

}

ModelComposition *ModelComponent::modelComposition() const
{
  return m_modelComposition;
}

IModelComponent *ModelComponent::modelComponent() const
{
  return m_modelComponent;
}

QList<Input*> ModelComponent::inputs() const
{
  return m_inputs;
}

Input *ModelComponent::findInput(IInput *input)
{
  for(Input *compInput : m_inputs)
  {
    if(compInput->input() == input)
    {
      return compInput;
    }
  }

  return nullptr;
}

Input *ModelComponent::findInput(const QString &id)
{
  for(Input *compInput : m_inputs)
  {
    if(!compInput->id().compare(id))
    {
      return compInput;
    }
  }

  return nullptr;
}

QList<Output*> ModelComponent::outputs() const
{
  return m_outputs;
}

Output *ModelComponent::findOutput(IOutput *output)
{
  for(Output *compOutput : m_outputs)
  {
    if(compOutput->output() == output)
    {
      return compOutput;
    }
  }

  return nullptr;
}

Output *ModelComponent::findOutput(const QString &id)
{
  for(Output *compOutput : m_outputs)
  {
    if(!compOutput->id().compare(id))
    {
      return compOutput;
    }
  }

  return nullptr;
}

void ModelComponent::establishConnections()
{
  for(Output *output : m_outputs)
    output->establishConnections();
}

void ModelComponent::disestablishConnections()
{
  for(Output *output : m_outputs)
  {
    output->disestablishConnections();
  }
}

void ModelComponent::reestablishConnections()
{
  for(Output *output : m_outputs)
  {
    output->reestablishConnections();
  }
}

ModelComponent *ModelComponent::clone(ModelComposition *parentComposition)
{
  ICloneableModelComponent *cloneModelComponent = dynamic_cast<ICloneableModelComponent*>(m_modelComponent);

  ModelComponent *modelComponent = new ModelComponent(cloneModelComponent->clone(), parentComposition);

  return modelComponent;
}

void ModelComponent::copyConnections(ModelComponent *modelComponent)
{
  for(Output *output : m_outputs)
  {
    Output *parentOutput = modelComponent->findOutput(output->id());

    if(parentOutput)
    {
      output->copyConnections(parentOutput);
    }
  }
}

void ModelComponent::createExchangeItems()
{

  QHash<QString, IInput*> inputsById;

  for(IInput *input : m_modelComponent->inputs())
  {
    inputsById[input->id()] = input;
  }

  QList<Input*> inputsToRemove;

  for(Input *input : m_inputs)
  {
    if(inputsById.contains(input->id()))
    {
      inputsById.remove(input->id());
    }
  }

  for(Input* input : inputsToRemove)
    delete input;

  inputsToRemove.clear();

  for(IInput *input : inputsById)
  {
    if(dynamic_cast<IMultiInput*>(input))
    {
      MultiInput *mInput = new MultiInput(dynamic_cast<IMultiInput*>(input), this);
      m_inputs.push_back(mInput);
    }
    else
    {
      Input *tInput = new Input(input,this);
      m_inputs.push_back(tInput);
    }
  }

  QHash<QString, IOutput*> outputsById;

  for(IOutput *output : m_modelComponent->outputs())
  {
    outputsById[output->id()] = output;
  }

  QList<Output*> outputsToRemove;

  for(Output *output : m_outputs)
  {
    if(outputsById.contains(output->id()))
    {
      outputsById.remove(output->id());
    }
  }

  for(Output *output : outputsToRemove)
    delete output;

  outputsToRemove.clear();

  for(IOutput *output : outputsById)
  {
    Output *tOutput = new Output(output,this);
    m_outputs.push_back(tOutput);
  }

}

void ModelComponent::deleteExchangeItems()
{
  for(Input *input : m_inputs)
  {
    delete input;
  }

  m_inputs.clear();

  for(Output *output : m_outputs)
  {
    delete output;
  }

  m_outputs.clear();

}

void ModelComponent::onComponentStatusChanged(const QSharedPointer<IComponentStatusChangeEventArgs> &statusChangedEvent)
{
  IModelComponent::ComponentStatus status = statusChangedEvent->status();

  switch (status)
  {
    case IModelComponent::Initialized:
      {
        createExchangeItems();
      }
      break;
    default:
      break;
  }
}

