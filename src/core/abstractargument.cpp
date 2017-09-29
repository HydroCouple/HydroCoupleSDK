#include "stdafx.h"
#include "core/abstractargument.h"
#include "core/abstractmodelcomponent.h"
#include "core/dimension.h"
#include "core/abstractcomponentdataitem.h"

using namespace HydroCouple;

AbstractArgument::AbstractArgument(const QString& id,
                                   const QList<Dimension*>& dimensions,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent *modelComponent)
  : AbstractComponentDataItem(id, dimensions, valueDefinition, modelComponent),
    m_isOptional(false),
    m_isReadOnly(false),
    m_canReadFromFile(false),
    m_canReadFromString(true),
    m_argumentIOType(ArgumentIOType::String)
{
  m_fileFilters.append("All files (*.*)");
}

AbstractArgument::AbstractArgument(const QString& id,
                                   const QString& caption,
                                   const QList<Dimension*>& dimensions,
                                   ValueDefinition* valueDefinition,
                                   AbstractModelComponent *modelComponent)
  : AbstractComponentDataItem(id, caption, dimensions, valueDefinition, modelComponent),
    m_isOptional(false),
    m_isReadOnly(false),
    m_canReadFromFile(false),
    m_canReadFromString(true),
    m_argumentIOType(ArgumentIOType::String)
{

}

AbstractArgument::~AbstractArgument()
{

}

bool AbstractArgument::isOptional() const
{
  return m_isOptional;
}

bool AbstractArgument::isReadOnly() const
{
  return m_isReadOnly;
}

QStringList AbstractArgument::fileFilters() const
{
  return m_fileFilters;
}

bool AbstractArgument::canReadFromFile() const
{
  return m_canReadFromFile;
}

bool AbstractArgument::canReadFromString() const
{
  return m_canReadFromString;
}

IArgument::ArgumentIOType AbstractArgument::currentArgumentIOType() const
{
  return m_argumentIOType;
}

AbstractArgument *AbstractArgument::clone()  const
{
  return nullptr;
}

void AbstractArgument::addFileFilter(const QString &extension)
{
  if(!m_fileFilters.contains(extension))
  {
    m_fileFilters.append(extension);
    emit propertyChanged("FileFilters");
  }
}

void AbstractArgument::setIsOptional(bool isOptional)
{
  m_isOptional = isOptional;
  emit propertyChanged("Optional");
}

void AbstractArgument::setIsReadOnly(bool isReadOnly)
{
  m_isReadOnly = isReadOnly;
  emit propertyChanged("ReadOnly");
}

void AbstractArgument::setArgumentIOType(ArgumentIOType argumentIOType)
{
  m_argumentIOType = argumentIOType;
}

void AbstractArgument::setCanReadFromFile(bool canReadFromFile)
{
  m_canReadFromFile = canReadFromFile;
}

void AbstractArgument::setCanReadFromString(bool canReadFromString)
{
  m_canReadFromString = canReadFromString;
}
