#include "stdafx.h"
#include "core/abstractexchangeitem.h"
#include "core/exchangeitemchangeeventargs.h"
#include <QSharedPointer>

using namespace HydroCouple;

AbstractExchangeItem::AbstractExchangeItem(const QString& id,
                                           const QList<Dimension*>& dimensions,
                                           ValueDefinition* valueDefinition,
                                           AbstractModelComponent *modelComponent)
   :AbstractComponentDataItem(id,dimensions,valueDefinition,modelComponent)
{

}

AbstractExchangeItem::AbstractExchangeItem(const QString& id,
                                           const QString &caption,
                                           const QList<Dimension*>& dimensions,
                                           ValueDefinition* valueDefinition,
                                           AbstractModelComponent *modelComponent)
   :AbstractComponentDataItem(id, caption, dimensions, valueDefinition, modelComponent)
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
