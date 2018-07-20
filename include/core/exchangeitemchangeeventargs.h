/*!
 * \file exchangeitemchangeeventargs.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU Lesser General Public License as published by the Free Software Foundation;
 * either version 3 of the License, or (at your option) any later version.
 * This file and its associated files is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 * \copyright Copyright 2014-2018, Caleb Buahin, All rights reserved.
 * \date 2014-2018
 * \pre
 * \bug
 * \warning
 * \todo
 */

#ifndef EXCHANGEITEMCHANGEEVENTARGS_H
#define EXCHANGEITEMCHANGEEVENTARGS_H

#include "hydrocouple.h"
#include "hydrocouplesdk.h"

class AbstractExchangeItem;

class HYDROCOUPLESDK_EXPORT ExchangeItemEventArgs : public QObject,
    public virtual HydroCouple::IExchangeItemChangeEventArgs
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IExchangeItemChangeEventArgs)
    Q_PROPERTY(HydroCouple::IExchangeItem exchangeItem READ exchangeItem)
    Q_PROPERTY(QString Message READ message  WRITE setMessage)

  public:
    ExchangeItemEventArgs(AbstractExchangeItem* exchangeItem);

    ExchangeItemEventArgs(const QString &message, AbstractExchangeItem* exchangeItem);

    virtual ~ExchangeItemEventArgs(){}

    HydroCouple::IExchangeItem* exchangeItem() const override;

    QString message() const override;

    void setMessage(const QString &message);

  private:
    AbstractExchangeItem* m_exchangeItem;
    QString m_message;

};

Q_DECLARE_METATYPE(ExchangeItemEventArgs*)
#endif // EXCHANGEITEMCHANGEEVENTARGS_H

