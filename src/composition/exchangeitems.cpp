#include "stdafx.h"
#include "composition/exchangeitems.h"
#include "composition/modelcomponent.h"

using namespace Composition;
using namespace HydroCouple;

ExchangeItem::ExchangeItem(const QString &id, ModelComponent *modelComponent)
  : QObject(modelComponent),
    m_id(id),
    m_modelComponent(modelComponent)
{
}

ExchangeItem::~ExchangeItem()
{
}

QString ExchangeItem::id() const
{
  return m_id;
}

ModelComponent *ExchangeItem::modelComponent() const
{
  return m_modelComponent;
}

void ExchangeItem::establishSignalSlotConnections()
{
  if(exchangeItem())
  {
    QObject *exchangeItemObject = dynamic_cast<QObject*>(exchangeItem());

    if(exchangeItemObject)
    {
      connect(exchangeItemObject, SIGNAL(propertyChanged(const QString &)), this, SLOT(onPropertyChanged(const QString&)));
    }
  }
}

void ExchangeItem::onPropertyChanged(const QString &propertyName)
{
  if(!propertyName.compare("Id", Qt::CaseInsensitive))
  {
    m_id = exchangeItem()->id();
  }
}





