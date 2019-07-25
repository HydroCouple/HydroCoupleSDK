/*!
 * \file  abstractadaptedoutput.h
 * \author  Caleb Amoa Buahin <caleb.buahin@gmail.com>
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

#ifndef ABSTRACTEXCHANGEITEM_H
#define ABSTRACTEXCHANGEITEM_H

#include "hydrocouple.h"
#include "hydrocouplesdk.h"
#include "abstractcomponentdataitem.h"

class AbstractModelComponent;

class HYDROCOUPLESDK_EXPORT AbstractExchangeItem : public AbstractComponentDataItem,
    public virtual HydroCouple::IExchangeItem
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IExchangeItem)

  public:
    AbstractExchangeItem(const QString& id,
                         const QList<Dimension*>& dimensions,
                         ValueDefinition* valueDefinition,
                         AbstractModelComponent* modelComponent);

    AbstractExchangeItem(const QString& id,
                         const QString& caption,
                         const QList<Dimension*>& dimensions,
                         ValueDefinition* valueDefinition,
                         AbstractModelComponent *modelComponent);

    virtual ~AbstractExchangeItem() override;

  protected:

    void emitMessage(const QString &message);

  signals:

    void itemChanged(const QSharedPointer<HydroCouple::IExchangeItemChangeEventArgs> &statusChangedEvent) override;

};

Q_DECLARE_METATYPE(AbstractExchangeItem*)

#endif // ABSTRACTEXCHANGEITEM_H

