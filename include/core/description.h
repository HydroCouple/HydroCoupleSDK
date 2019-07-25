/*!
 * \file description.h
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

#ifndef DESCRIPTION_H
#define DESCRIPTION_H

#include "hydrocouplesdk.h"
#include "hydrocouple.h"

#include <QObject>
#include <QVariant>

/*!
 * \brief The Description class.
 *
 * \details The Description class is an implementation of the HydroCouple::IDescription class
 * in the HydroCouple interface definitions.
 *
 */
class HYDROCOUPLESDK_EXPORT Description : public QObject,
    public virtual HydroCouple::IDescription
{
    Q_OBJECT

    Q_INTERFACES(HydroCouple::IPropertyChanged
                 HydroCouple::IDescription)

    Q_PROPERTY(QString Caption READ caption WRITE setCaption NOTIFY propertyChanged)
    Q_PROPERTY(QString Description READ description WRITE setDescription NOTIFY propertyChanged)

  public:

    Description(QObject *parent = nullptr);

    Description(const QString &caption, QObject *parent = nullptr);

    virtual ~Description() override;

    QString caption() const override;

    void setCaption(const QString &caption) override;

    QString description() const override;

    void setDescription(const QString &decription) override;

  signals:

    void propertyChanged(const QString& propertyName) override;

  protected:

    QString m_caption, m_description;

};

Q_DECLARE_METATYPE(Description*)

#endif // DESCRIPTION_H
