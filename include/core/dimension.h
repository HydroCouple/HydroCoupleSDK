/*!
 * \file dimension.h
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

#ifndef DIMENSION_H
#define DIMENSION_H

#include "identity.h"

/*!
 * \brief The Dimension class
 */
class HYDROCOUPLESDK_EXPORT Dimension : public Identity,
    public virtual HydroCouple::IDimension
{
    Q_OBJECT
    Q_INTERFACES(HydroCouple::IDimension)

  public:

    Dimension(const QString& id, QObject* parent = nullptr);

    Dimension(const QString& id, const QString& caption, QObject* parent = nullptr);

    virtual ~Dimension();

    Dimension *copy(QObject *parent);

    static Dimension *copy(const HydroCouple::IDimension *dimension, QObject *parent);

  signals:

    void propertyChanged(const QString& propertyName) override;

  private:
    QString m_name;
};

Q_DECLARE_METATYPE(Dimension*)


#endif // DIMENSION_H

