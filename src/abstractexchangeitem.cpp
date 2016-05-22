#include "stdafx.h"
#include "abstractexchangeitem.h"

using namespace HydroCouple;

AbstractExchangeItem::AbstractExchangeItem(const QString &id, AbstractModelComponent *parentModelComponent)
   :AbstractComponentDataItem(id,parentModelComponent)
{

}

AbstractExchangeItem::AbstractExchangeItem(const QString &id, const QString &caption, AbstractModelComponent *parentModelComponent)
   :AbstractComponentDataItem(id, caption, parentModelComponent)
{

}

AbstractExchangeItem::~AbstractExchangeItem()
{

}
