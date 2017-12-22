#include "stdafx.h"
#include "core/abstractmodelcomponentinfo.h"
#include "core/abstractmodelcomponent.h"
#include "core/abstractadaptedoutputfactory.h"
#include <QUuid>

using namespace HydroCouple;

AbstractModelComponentInfo::AbstractModelComponentInfo(QObject *parent)
   : ComponentInfo(parent)
{
}

AbstractModelComponentInfo::~AbstractModelComponentInfo()
{
  qDeleteAll(m_adaptedOutputFactories.values());
  m_adaptedOutputFactories.clear();
}

QList<IAdaptedOutputFactory*> AbstractModelComponentInfo::adaptedOutputFactories() const
{

  QList<IAdaptedOutputFactory*> factories;

  for(AbstractAdaptedOutputFactory* factory : m_adaptedOutputFactories)
  {
    factories.append(factory);
  }

  return factories;
}


QHash<QString,AbstractAdaptedOutputFactory*> AbstractModelComponentInfo::adaptedOutputFactoriesInternal() const
{
  return m_adaptedOutputFactories;
}

void AbstractModelComponentInfo::addAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory)
{
  if(m_adaptedOutputFactories.find(adaptedOutputFactory->id()) == m_adaptedOutputFactories.end())
  {
    m_adaptedOutputFactories[adaptedOutputFactory->id()] = adaptedOutputFactory;
    emit propertyChanged("AdaptedOutputFactories");
  }
}

bool AbstractModelComponentInfo::removeAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory)
{
  if(m_adaptedOutputFactories.contains(adaptedOutputFactory->id()) == true)
  {
    m_adaptedOutputFactories.remove(adaptedOutputFactory->id());
    emit propertyChanged("AdaptedOutputFactories");
    return true;
  }

  return false;
}

void AbstractModelComponentInfo::clearAdaptedOutputFactories()
{
  qDeleteAll(m_adaptedOutputFactories);
  m_adaptedOutputFactories.clear();

  emit propertyChanged("AdaptedOutputFactories");
}
