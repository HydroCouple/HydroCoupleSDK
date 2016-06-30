#include "stdafx.h"
#include "core/abstractadaptedoutputfactory.h"

using namespace HydroCouple;

AbstractAdaptedOutputFactory::AbstractAdaptedOutputFactory(const QString& id,
                                                           QObject* parent)
  :Identity(id,parent)
{

}

AbstractAdaptedOutputFactory::~AbstractAdaptedOutputFactory()
{

}


AbstractAdaptedOutputFactoryComponent::AbstractAdaptedOutputFactoryComponent(const QString &id, QObject *parent)
  : AbstractAdaptedOutputFactory(id,parent),
    m_componentInfo(nullptr)
{

}

AbstractAdaptedOutputFactoryComponent::~AbstractAdaptedOutputFactoryComponent()
{

}

IAdaptedOutputFactoryComponentInfo* AbstractAdaptedOutputFactoryComponent::componentInfo() const
{
  return m_componentInfo;
}

void AbstractAdaptedOutputFactoryComponent::setComponentInfo(AbstractAdaptedOutputFactoryComponentInfo *componentInfo)
{
  m_componentInfo = componentInfo;
  emit propertyChanged("ComponentInfo");
}
