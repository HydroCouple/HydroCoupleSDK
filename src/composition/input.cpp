#include "stdafx.h"
#include "composition/exchangeitems.h"
#include "composition/modelcomponent.h"

using namespace Composition;
using namespace HydroCouple;

Input::Input(IInput *input, ModelComponent *modelComponent)
  : ExchangeItem(input->id(), modelComponent),
    m_provider(nullptr)
{
  establishSignalSlotConnections();
}

Input::~Input()
{
  //remove provider
  if(m_provider)
  {
    m_provider->removeConsumer(this);
    m_provider = nullptr;
  }

  //delete if provide is adaptedOutput and is first input
}

IExchangeItem *Input::exchangeItem() const
{
  return input();
}

IInput *Input::input() const
{
  for(IInput *inp : m_modelComponent->modelComponent()->inputs())
  {
    if(!inp->id().compare(m_id))
    {
      return inp;
    }
  }

  return nullptr;
}

Output *Input::provider() const
{
  return m_provider;
}

void Input::setProvider(Output *provider)
{
  m_provider = provider;
}



MultiInput::MultiInput(IMultiInput *input, ModelComponent *modelComponent)
  : Input(input, modelComponent)
{
  establishSignalSlotConnections();
}

MultiInput::~MultiInput()
{
  //remove provider
  for(Output *provider : m_providers)
  {
    provider->removeConsumer(this);
  }

  m_providers.clear();

  //delete if provide is adaptedOutput and is first input
}

IMultiInput *MultiInput::multiInput() const
{
  IInput *inp = input();

  if(inp)
    return dynamic_cast<IMultiInput*>(inp);
  else
    return nullptr;
}

QList<Output*> MultiInput::providers() const
{
  return m_providers;
}


void MultiInput::addProvider(Output *provider)
{
  m_providers.append(provider);
}

void MultiInput::removeProvider(Output *provider)
{
  if(m_providers.contains(provider))
  {
    m_providers.removeOne(provider);
  }
}


