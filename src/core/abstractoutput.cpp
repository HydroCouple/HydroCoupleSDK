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
#include "core/abstractoutput.h"


using namespace HydroCouple;

AbstractOutput::AbstractOutput(const QString& id,
                               const QList<Dimension*>& dimensions,
                               ValueDefinition* valueDefinition,
                               AbstractModelComponent *modelComponent)
  : AbstractExchangeItem(id, dimensions,valueDefinition, modelComponent)
{

}

AbstractOutput::AbstractOutput(const QString& id,
                               const QString& caption,
                               const QList<Dimension*>& dimensions,
                               ValueDefinition* valueDefinition,
                               AbstractModelComponent *modelComponent)
  : AbstractExchangeItem(id, caption , dimensions, valueDefinition, modelComponent)
{

}

AbstractOutput::~AbstractOutput()
{

}

QList<HydroCouple::IInput*> AbstractOutput::consumers() const
{
  return m_consumers.values();
}

void AbstractOutput::addConsumer(HydroCouple::IInput* consumer)
{
  if(!m_consumers.contains(consumer->id()))
  {
    m_consumers[consumer->id()] = consumer;
  }

  emit propertyChanged("Consumers");
}

bool AbstractOutput::removeConsumer(HydroCouple::IInput *consumer)
{
  bool happened = m_consumers.remove(consumer->id());

  emit propertyChanged("Consumers");
  return happened;
}

QList<HydroCouple::IAdaptedOutput*> AbstractOutput::adaptedOutputs()  const
{
  return m_adaptedOutputs.values();
}

void AbstractOutput::addAdaptedOutput(HydroCouple::IAdaptedOutput* adaptedOutput)
{
  if(!m_adaptedOutputs.contains(adaptedOutput->id()))
  {
    m_adaptedOutputs[adaptedOutput->id()] = adaptedOutput;
  }

  emit propertyChanged("Consumers");
}

bool AbstractOutput::removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput)
{
  bool happened = m_adaptedOutputs.remove(adaptedOutput->id());
  emit propertyChanged("Consumers");
  return happened;
}

void AbstractOutput::updateValues(IInput *querySpecifier)
{
  if(modelComponent()->status() != IModelComponent::Updated)
  {
    modelComponent()->update(QList<IOutput*>({this}));
  }

  refreshAdaptedOutputs();
}

void AbstractOutput::refreshAdaptedOutputs()
{
  for(IAdaptedOutput *adaptedOutput : m_adaptedOutputs)
    adaptedOutput->refresh();
}

void AbstractOutput::initializeAdaptedOutputs()
{
  for(IAdaptedOutput* adaptedOutput : m_adaptedOutputs)
    adaptedOutput->initialize();
}
