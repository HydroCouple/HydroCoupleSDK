/*! \file   identity.h
 *  \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 *  \version   1.0.0.0
 *  \section   Description
 *  This header file is part of the HydroCoupleSDK library. It is an implementation
 * of the IIdentity interface of the HydroCouple interface definitions.
 *  \section License
 *  identity.h, associated files and libraries are free software;
 *  you can redistribute it and/or modify it under the terms of the
 *  Lesser GNU General Public License as published by the Free Software Foundation;
 *  either version 3 of the License, or (at your option) any later version.
 *  hydrocouple.h its associated files is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.(see <http://www.gnu.org/licenses/> for details)
 *  \date 2014-2016
 *  \pre
 *  \bug
 *  \warning
 *  \todo
 */

#ifndef IDENTITY_H
#define IDENTITY_H

#include "description.h"
#include <QUuid>

/*!
 * \brief The Identity class is an implementation of the HydroCouple::IIdentity of the HydroCouple interface definitions.
 */
class HYDROCOUPLESDK_EXPORT Identity : public Description,
    public virtual HydroCouple::IIdentity
{

    Q_OBJECT

    Q_INTERFACES(HydroCouple::IIdentity)
    Q_PROPERTY(QString Id READ id NOTIFY propertyChanged)

  public:
    Identity(const QString& id = QUuid::createUuid().toString() , QObject* parent = nullptr);

    Identity(const QString& id = QUuid::createUuid().toString(), const QString& caption = "", QObject* parent = nullptr);

    virtual ~Identity();

    QString id() const override;

  signals:
    void propertyChanged(const QString& propertyName) override;

  protected:
    void setId(const QString& id);

  private:
    QString m_id;

};

Q_DECLARE_METATYPE(Identity*)

#endif // IDENTITY_H
