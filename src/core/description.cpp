/*!
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


#include "stdafx.h"
#include "core/description.h"

Description::Description(QObject *parent)
   : QObject(parent)
{

}

Description::Description(const QString &caption,  QObject *parent)
   : QObject(parent), m_caption(caption)
{

}

Description::~Description()
{

}

QString Description::caption() const
{
   return m_caption;
}

void Description::setCaption(const QString &caption)
{
   m_caption = caption;
   emit propertyChanged("Caption");
}

QString Description::description() const
{
   return m_description;
}

void Description::setDescription(const QString &description)
{
   m_description = description;
   emit propertyChanged("Description");
}
