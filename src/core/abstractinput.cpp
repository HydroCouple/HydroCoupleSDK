#include "stdafx.h"
#include "core/abstractinput.h"

using namespace HydroCouple;

AbstractInput::AbstractInput(const QString &id, AbstractModelComponent* modelComponent)
   :AbstractExchangeItem(id,modelComponent), m_provider(nullptr)
{

}

AbstractInput::AbstractInput(const QString &id, const QString &caption, AbstractModelComponent* modelComponent)
   :AbstractExchangeItem(id,caption,modelComponent), m_provider(nullptr)
{

}


AbstractInput::~AbstractInput()
{

}

IOutput* AbstractInput::provider() const
{
   return m_provider;
}

void AbstractInput::setProvider(IOutput *provider)
{
   m_provider = provider;
   emit propertyChanged("Provider");
}

bool AbstractInput::canConsume(IOutput *provider, QString &message) const
{
   message = "";
   return true;
}


//======================================================================================================================================================================

AbstractMultiInput::AbstractMultiInput(const QString &id, AbstractModelComponent *modelComponent)
   :AbstractInput(id, modelComponent)
{

}

AbstractMultiInput::AbstractMultiInput(const QString &id, const QString &caption, AbstractModelComponent *modelComponent)
   :AbstractInput(id,caption, modelComponent)
{

}

AbstractMultiInput::~AbstractMultiInput()
{

}

QList<IOutput*> AbstractMultiInput::providers() const
{
   return m_providers;
}

void AbstractMultiInput::addProvider(IOutput *provider)
{
   if(!m_providers.contains(provider) && provider)
   {
      m_providers.append(provider);
      emit propertyChanged("Providers");
   }
}

bool AbstractMultiInput::removeProvider(IOutput *provider)
{
   if(m_providers.removeAll(provider))
   {
      emit propertyChanged("Providers");
      return true;
   }
   else
   {
      return false;
   }
}
