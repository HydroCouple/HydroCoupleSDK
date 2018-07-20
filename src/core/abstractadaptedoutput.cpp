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
#include "core/abstractargument.h"
#include "core/abstractadaptedoutput.h"
#include "core/abstractadaptedoutputfactory.h"
#include "core/dimension.h"
#include "core/valuedefinition.h"


using namespace HydroCouple;

AbstractAdaptedOutput::AbstractAdaptedOutput(const QString& id,
                                             const QList<Dimension*>& dimensions,
                                             ValueDefinition* valueDefinition,
                                             HydroCouple::IOutput* adaptee,
                                             AbstractAdaptedOutputFactory* adaptedOutputFactory)
  : Identity(id,adaptedOutputFactory),
    m_dimensions(dimensions),
    m_valueDefinition(valueDefinition),
    m_adaptedOutputFactory(adaptedOutputFactory),
    m_adaptee(adaptee)
{
}

IModelComponent* AbstractAdaptedOutput::modelComponent() const
{
  if(adaptee())
  {
    return adaptee()->modelComponent();
  }
  else
  {
    return nullptr;
  }
}

QList<IDimension*> AbstractAdaptedOutput::dimensions() const
{
  QList<IDimension*> tdimensions;

  for(Dimension* dim : m_dimensions)
  {
    tdimensions.append(dim);
  }

  return tdimensions;
}

IValueDefinition* AbstractAdaptedOutput::valueDefinition() const
{
  return m_valueDefinition;
}

QStringList AbstractAdaptedOutput::comments() const
{
  return m_comments;
}

void AbstractAdaptedOutput::setComments(const QStringList& comments)
{
  m_comments = comments;
  emit propertyChanged("Comments");
}

void AbstractAdaptedOutput::addComment(const QString& comment)
{
  if(!m_comments.contains(comment, Qt::CaseInsensitive))
  {
    m_comments.append(comment);
    emit propertyChanged("Comments");
  }
}

QList<HydroCouple::IInput*> AbstractAdaptedOutput::consumers() const
{
  return m_consumers.values();
}

void AbstractAdaptedOutput::addConsumer(HydroCouple::IInput* consumer)
{
  if(!m_consumers.contains(consumer->id()))
    m_consumers[consumer->id()] = consumer;

  emit propertyChanged("Consumers");
}

bool AbstractAdaptedOutput::removeConsumer(HydroCouple::IInput *consumer)
{
  bool happened = m_consumers.remove(consumer->id());

  emit propertyChanged("Consumers");
  return happened;
}

QList<HydroCouple::IAdaptedOutput*> AbstractAdaptedOutput::adaptedOutputs()  const
{
  return m_adaptedOutputs.values();
}

void AbstractAdaptedOutput::addAdaptedOutput(HydroCouple::IAdaptedOutput* adaptedOutput)
{
  if(!m_adaptedOutputs.contains(adaptedOutput->id()))
    m_adaptedOutputs[adaptedOutput->id()] = adaptedOutput;

  emit propertyChanged("Consumers");
}

bool AbstractAdaptedOutput::removeAdaptedOutput(HydroCouple::IAdaptedOutput *adaptedOutput)
{
  bool happened = m_adaptedOutputs.remove(adaptedOutput->id());
  emit propertyChanged("Consumers");
  return happened;
}

IAdaptedOutputFactory* AbstractAdaptedOutput::adaptedOutputFactory() const
{
  return m_adaptedOutputFactory;
}

QList<IArgument*> AbstractAdaptedOutput::arguments() const
{
  QList<IArgument*> arguments;

  for(AbstractArgument *argument : m_arguments.values())
  {
    arguments.append(argument);
  }

  return arguments;
}

IOutput* AbstractAdaptedOutput::adaptee() const
{
  return m_adaptee;
}

IInput* AbstractAdaptedOutput::input() const
{
  return m_input;
}

void AbstractAdaptedOutput::setInput(IInput* input)
{
  m_input = input;
  emit propertyChanged("Input");
}

void AbstractAdaptedOutput::refreshAdaptedOutputs()
{
  for(IAdaptedOutput *adaptedOutput : m_adaptedOutputs)
    adaptedOutput->refresh();
}

void AbstractAdaptedOutput::initializeAdaptedOutputs()
{
  for(IAdaptedOutput* adaptedOutput : m_adaptedOutputs)
  {
    adaptedOutput->initialize();
  }
}

QList<Dimension*> AbstractAdaptedOutput::dimensionsInternal() const
{
  return m_dimensions;
}

ValueDefinition* AbstractAdaptedOutput::valueDefinitionInternal() const
{
  return m_valueDefinition;
}

AbstractAdaptedOutputFactory* AbstractAdaptedOutput::adaptedOutputFactoryInternal() const
{
  return m_adaptedOutputFactory;
}

void AbstractAdaptedOutput::addArgument(AbstractArgument *argument)
{
  if(!m_arguments.contains(argument->id()))
  {
    m_arguments[argument->id()] = argument;
    emit propertyChanged("Arguments");
  }
}

bool AbstractAdaptedOutput::removeArgument(AbstractArgument *argument)
{
  if(m_arguments.contains(argument->id()))
  {
    m_arguments.remove(argument->id());
    delete argument;
    emit propertyChanged("Arguments");
    return true;
  }

  return false;
}

void AbstractAdaptedOutput::clearArguments()
{
  qDeleteAll(m_arguments.values());
  m_arguments.clear();

  emit propertyChanged("Arguments");
}

QHash<QString,AbstractArgument*> AbstractAdaptedOutput::argumentsInternal() const
{
  return m_arguments;
}


