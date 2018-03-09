/*!
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

#include "stdafx.h"
#include "core/abstractmodelcomponentinfo.h"
#include "core/abstractmodelcomponent.h"
#include "core/abstractadaptedoutputfactory.h"
#include <QUuid>

using namespace HydroCouple;

AbstractModelComponentInfo::AbstractModelComponentInfo(QObject *parent)
   : ComponentInfo(parent)
{
}

AbstractModelComponentInfo::~AbstractModelComponentInfo()
{
  qDeleteAll(m_adaptedOutputFactories.values());
  m_adaptedOutputFactories.clear();
}

QList<IAdaptedOutputFactory*> AbstractModelComponentInfo::adaptedOutputFactories() const
{

  QList<IAdaptedOutputFactory*> factories;

  for(AbstractAdaptedOutputFactory* factory : m_adaptedOutputFactories)
  {
    factories.append(factory);
  }

  return factories;
}


QHash<QString,AbstractAdaptedOutputFactory*> AbstractModelComponentInfo::adaptedOutputFactoriesInternal() const
{
  return m_adaptedOutputFactories;
}

void AbstractModelComponentInfo::addAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory)
{
  if(m_adaptedOutputFactories.find(adaptedOutputFactory->id()) == m_adaptedOutputFactories.end())
  {
    m_adaptedOutputFactories[adaptedOutputFactory->id()] = adaptedOutputFactory;
    emit propertyChanged("AdaptedOutputFactories");
  }
}

bool AbstractModelComponentInfo::removeAdaptedOutputFactory(AbstractAdaptedOutputFactory* adaptedOutputFactory)
{
  if(m_adaptedOutputFactories.contains(adaptedOutputFactory->id()) == true)
  {
    m_adaptedOutputFactories.remove(adaptedOutputFactory->id());
    emit propertyChanged("AdaptedOutputFactories");
    return true;
  }

  return false;
}

void AbstractModelComponentInfo::clearAdaptedOutputFactories()
{
  qDeleteAll(m_adaptedOutputFactories);
  m_adaptedOutputFactories.clear();

  emit propertyChanged("AdaptedOutputFactories");
}
