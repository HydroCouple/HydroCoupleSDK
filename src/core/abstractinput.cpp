#include "stdafx.h"
#include "core/abstractinput.h"

using namespace HydroCouple;

AbstractInput::AbstractInput(const QString& id,
                             const QList<Dimension*>& dimensions,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent* modelComponent)
  :AbstractExchangeItem(id,dimensions,valueDefinition,modelComponent),
    m_provider(nullptr)
{

}

AbstractInput::AbstractInput(const QString& id,
                             const QString& caption,
                             const QList<Dimension*>& dimensions,
                             ValueDefinition* valueDefinition,
                             AbstractModelComponent* modelComponent)
  :AbstractExchangeItem(id,caption,dimensions,valueDefinition,modelComponent),
    m_provider(nullptr)
{

}

AbstractInput::~AbstractInput()
{

}

IOutput* AbstractInput::provider() const
{
  return m_provider;
}

bool AbstractInput::setProvider(IOutput *provider)
{
  QString message ="";

  if(canConsume(provider, message))
  {
    m_provider = provider;
    emit propertyChanged("Provider");
    return true;
  }

  return false;
}

bool AbstractInput::canConsume(IOutput *provider, QString &message) const
{
  message = "Not implemented ";
  return true;
}


//======================================================================================================================================================================

AbstractMultiInput::AbstractMultiInput(const QString& id,
                                       const QList<Dimension*>& dimensions,
                                       ValueDefinition* valueDefinition,
                                       AbstractModelComponent* modelComponent)
  :AbstractInput(id, dimensions, valueDefinition, modelComponent)
{

}

AbstractMultiInput::AbstractMultiInput(const QString& id,
                                       const QString& caption,
                                       const QList<Dimension*>& dimensions,
                                       ValueDefinition* valueDefinition,
                                       AbstractModelComponent* modelComponent)
  :AbstractInput(id,caption,dimensions,valueDefinition,modelComponent)
{

}

AbstractMultiInput::~AbstractMultiInput()
{

}

QList<IOutput*> AbstractMultiInput::providers() const
{
  return m_providers;
}

bool AbstractMultiInput::addProvider(IOutput *provider)
{
  QString message = "";

  if(provider  && !m_providers.contains(provider) && canConsume(provider, message))
  {
    m_providers.append(provider);
    emit propertyChanged("Providers");
    return true;
  }

  return false;
}

bool AbstractMultiInput::removeProvider(IOutput *provider)
{
  if(m_providers.removeOne(provider))
  {
    emit propertyChanged("Providers");
    return true;
  }
  else
  {
    return false;
  }
}
