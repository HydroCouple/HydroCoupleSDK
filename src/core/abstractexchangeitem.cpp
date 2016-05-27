#include "stdafx.h"
#include "core/abstractexchangeitem.h"
#include "core/exchangeitemchangeeventargs.h"
#include <QSharedPointer>

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

void AbstractExchangeItem::emitMessage(const QString &message)
{
   QSharedPointer<IExchangeItemChangeEventArgs> arg(new ExchangeItemEventArgs(message , this));
   emit itemChanged(arg);
}
