#include "stdafx.h"
#include "exchangeitemchangeeventargs.h"
#include "abstractexchangeitem.h"

using namespace HydroCouple;

ExchangeItemEventArgs::ExchangeItemEventArgs(AbstractExchangeItem *exchangeItem)
   :QObject(exchangeItem)
{
   m_exchangeItem = exchangeItem;
}

ExchangeItemEventArgs::ExchangeItemEventArgs(const QString &message, AbstractExchangeItem *exchangeItem)
   :QObject(exchangeItem), m_message(message)
{
   m_exchangeItem = exchangeItem;
}

IExchangeItem* ExchangeItemEventArgs::exchangeItem() const
{
   return m_exchangeItem;
}

QString ExchangeItemEventArgs::message() const
{
   return m_message;
}

void ExchangeItemEventArgs::setMessage(const QString &message)
{
   m_message = message;
}
