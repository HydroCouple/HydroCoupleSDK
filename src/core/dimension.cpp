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
#include "core/dimension.h"
#include <stdexcept>

using namespace std;

Dimension::Dimension(const QString &id, QObject *parent)
   :Identity(id,parent)
{
}

Dimension::Dimension(const QString &id, const QString &caption, QObject *parent)
   :Identity(id,caption,parent)
{
}

Dimension::~Dimension()
{
}

Dimension *Dimension::copy(QObject *parent)
{
  return copy(this, parent);
}

Dimension *Dimension::copy(const HydroCouple::IDimension *dimension, QObject *parent)
{
  Dimension *oDimension = new Dimension(dimension->id(),parent);
  oDimension->setCaption(dimension->caption());
  oDimension->setDescription(dimension->description());

  return oDimension;
}
