/*!
 * \file idbasedcomponentdataitem.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \description
 * \license
 * This file and its associated files, and libraries are free software.
 * You can redistribute it and/or modify it under the terms of the
 * Lesser GNU General Public License as published by the Free Software Foundation;
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

#ifndef IDBASEDEXCHANGEITEM_H
#define IDBASEDEXCHANGEITEM_H

#include "componentdataitem.h"

#include <QDateTime>

template<class T>
class HYDROCOUPLESDK_EXPORT IdBasedComponentDataItem :
    public ComponentDataItem1D<T>
{
  public:

    IdBasedComponentDataItem(const QString &id,
                             const QStringList& identifiers,
                             const T& defaultValue);

    virtual ~IdBasedComponentDataItem();

    bool addIdentifier(const QString &identifier);

    void addIdentifiers(const QList<QString> &identifiers);

    bool removeIdentifier(const QString &identifier);

    bool containsIdentifier(const QString& identifier) const;

    T &operator[](const QString &id);

    const T &operator [](const QString &id) const;

  protected:

    QStringList identifiersInternal() const;

    void clearIdentifiers();

  private:

    QStringList  m_identifiers;
};

#endif // IDBASEDEXCHANGEITEM_H
