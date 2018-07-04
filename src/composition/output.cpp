#include "stdafx.h"
#include "composition/exchangeitems.h"
#include "composition/modelcomponent.h"
#include "composition/composition.h"

using namespace Composition;
using namespace HydroCouple;

Output::Output(IOutput *output, ModelComponent *modelComponent)
  : ExchangeItem(output->id(), modelComponent)
{
  establishSignalSlotConnections();
}

Output::~Output()
{
  for(Input *consumer : m_consumers)
  {
    if(dynamic_cast<MultiInput*>(consumer))
    {
      dynamic_cast<MultiInput*>(consumer)->removeProvider(this);
    }
    else
    {
      consumer->setProvider(nullptr);
    }
  }

  while (m_adaptedOutputs.size())
  {
    AdaptedOutput *adaptedOutput = m_adaptedOutputs.first();
    m_adaptedOutputs.removeFirst();
    delete adaptedOutput;
  }
}

IExchangeItem *Output::exchangeItem() const
{
  return output();
}

IOutput *Output::output() const
{
  for(IOutput *out : m_modelComponent->modelComponent()->outputs())
  {
    if(!out->id().compare(m_id))
    {
      return out;
    }
  }

  return nullptr;
}

QList<Input*> Output::consumers() const
{
  return m_consumers;
}

void Output::addConsumer(Input *input)
{
  if(!m_consumers.contains(input))
  {
    m_consumers.push_back(input);
  }
}

void Output::removeConsumer(Input *input)
{
  m_consumers.removeOne(input);
}

QList<AdaptedOutput*> Output::adaptedOutputs() const
{
  return m_adaptedOutputs;
}

void Output::addAdaptedOutput(AdaptedOutput *adaptedOutput)
{
  if(!m_adaptedOutputs.contains(adaptedOutput))
  {
    m_adaptedOutputs.push_back(adaptedOutput);
  }
}

void Output::removeAdaptedOutput(AdaptedOutput *adaptedOutput)
{
  m_adaptedOutputs.removeOne(adaptedOutput);
}

bool Output::createConnection(Input *input)
{
  QString message;

  if(input->input() && output() && input->input()->canConsume(output(), message))
  {
    addConsumer(input);

    if(dynamic_cast<MultiInput*>(input))
    {
      dynamic_cast<MultiInput*>(input)->addProvider(this);
    }
    else
    {
      input->setProvider(this);
    }

    return true;
  }

  return false;
}

bool Output::insertAdaptedOutput(IIdentity *identity, IAdaptedOutputFactory *adaptedOutputFactory, Input *consumer)
{


  return false;
}

void Output::establishConnections()
{
  IOutput *thisOutput = output();

  if(thisOutput)
  {
    for(IInput *consumer : thisOutput->consumers())
    {
      ModelComponent *consumerModelComponent =  m_modelComponent->modelComposition()->findModelComponent(consumer->modelComponent());

      if(consumerModelComponent)
      {
        Input *compConsumer = consumerModelComponent->findInput(consumer);

        if(compConsumer)
        {
          addConsumer(compConsumer);

          if(dynamic_cast<MultiInput*>(compConsumer))
          {
            MultiInput *compMConsumer = dynamic_cast<MultiInput*>(compConsumer);
            compMConsumer->addProvider(this);
          }
          else
          {
            compConsumer->setProvider(this);
          }
        }
      }
    }

    //set adapated Outputs
    for(IAdaptedOutput *adapted : thisOutput->adaptedOutputs())
    {
      IInput *input = nullptr;
      Input *compInput = nullptr;

      if(adapted->consumers().size())
      {
        input = adapted->consumers()[0];

        ModelComponent *inputModelComponent =  m_modelComponent->modelComposition()->findModelComponent(input->modelComponent());
        compInput = inputModelComponent->findInput(input);
      }

      AdaptedOutput *adaptedOutput = new AdaptedOutput(adapted, compInput, this, m_modelComponent);
      addAdaptedOutput(adaptedOutput);
    }
  }
}

void Output::disestablishConnections()
{
  IOutput *thisOutput = output();

  if(thisOutput)
  {
    for(IInput *consumer : thisOutput->consumers())
    {
      thisOutput->removeConsumer(consumer);

      if(dynamic_cast<IMultiInput*>(consumer))
      {
        dynamic_cast<IMultiInput*>(consumer)->removeProvider(thisOutput);
      }
      else
      {
        consumer->setProvider(nullptr);
      }

    }

    for(IAdaptedOutput *adapted : thisOutput->adaptedOutputs())
    {
      adapted->removeAdaptedOutput(adapted);
    }
  }
}

void Output::reestablishConnections()
{
  IOutput *thisOutput = output();

  if(thisOutput)
  {
    for(Input *consumer : m_consumers)
    {
      if(consumer->input())
      {
        thisOutput->addConsumer(consumer->input());

        if(dynamic_cast<MultiInput*>(consumer))
        {
          dynamic_cast<MultiInput*>(consumer)->multiInput()->addProvider(thisOutput);
        }
        else
        {
          consumer->input()->setProvider(thisOutput);
        }
      }
    }

    for(AdaptedOutput *adaptedOutput : m_adaptedOutputs)
    {
      if(adaptedOutput->adaptedOutput())
      {
        thisOutput->addAdaptedOutput(adaptedOutput->adaptedOutput());
      }
    }
  }
}

void Output::copyConnections(Output *output)
{
  for(Input *consumer : output->m_consumers)
  {
    ModelComponent *consumerComponent = modelComponent()->modelComposition()->findModelComponent(consumer->modelComponent()->modelComponent()->id());
    Input *corressConsumer = consumerComponent->findInput(consumer->id());

    if(corressConsumer)
    {
      addConsumer(corressConsumer);

      if(dynamic_cast<MultiInput*>(corressConsumer))
      {
        dynamic_cast<MultiInput*>(corressConsumer)->addProvider(this);
      }
      else
      {
        corressConsumer->setProvider(this);
      }
    }
  }

  for(AdaptedOutput *adaptedOutput : m_adaptedOutputs)
  {
    AdaptedOutput *tempAdaptedOutput = adaptedOutput->clone(output, this->modelComponent());
    addAdaptedOutput(tempAdaptedOutput);
  }
}


AdaptedOutput::AdaptedOutput(IAdaptedOutput *adaptedOutput,
                             Input *input, Output *adaptee,
                             ModelComponent *modelComponent)
  : Output(adaptedOutput, modelComponent),
    m_adaptedOutput(adaptedOutput),
    m_input(input),
    m_adaptee(adaptee),
    m_adaptedOutputFactory(adaptedOutput->adaptedOutputFactory())
{
  establishConnections();
}

AdaptedOutput::~AdaptedOutput()
{
  AdaptedOutput *adaptedOutput = nullptr;

  if((adaptedOutput = dynamic_cast<AdaptedOutput*>(m_adaptee)))
  {
    if(adaptedOutput->consumers().size() == 0 && adaptedOutputs().size() <= 1)
    {
      delete adaptedOutput;
    }
  }
}

HydroCouple::IAdaptedOutputFactory *AdaptedOutput::adaptedOutputFactory() const
{
  return m_adaptedOutputFactory;
}

HydroCouple::IOutput *AdaptedOutput::output() const
{
  return nullptr;
}

HydroCouple::IAdaptedOutput *AdaptedOutput::adaptedOutput() const
{
  return nullptr;
}

Output *AdaptedOutput::adaptee() const
{

  return nullptr;
}


AdaptedOutput *AdaptedOutput::clone(Output *output, ModelComponent *modelComponent)
{

  if(m_adaptee == output)
  {


  }
  else
  {

  }

  return nullptr;
}
