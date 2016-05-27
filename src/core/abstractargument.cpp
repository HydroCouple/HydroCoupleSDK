#include "stdafx.h"
#include "core/abstractargument.h"
#include "core/abstractmodelcomponent.h"
#include "core/dimension.h"
#include "core/abstractcomponentdataitem.h"

using namespace HydroCouple;

AbstractArgument::AbstractArgument(const QString &id, AbstractModelComponent *parentModelComponent)
   :AbstractComponentDataItem(id,parentModelComponent),
     m_isOptional(false),  m_isReadOnly(false), m_canReadFromFile(false),
     m_canReadFromString(true), m_argumentIOType(ArgumentIOType::String)

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

QStringList AbstractArgument::inputFileTypeFilters() const
{
   return m_inputFileTypeFilters;
}

bool AbstractArgument::canReadFromFile() const
{
   return m_canReadFromFile;
}

bool AbstractArgument::canReadFromString() const
{
   return m_canReadFromString;
}

HydroCouple::ArgumentIOType AbstractArgument::currentArgumentIOType() const
{
   return m_argumentIOType;
}

void AbstractArgument::addInputFileTypeFilter(const QString &extension)
{
   if(!m_inputFileTypeFilters.contains(extension))
   {
      m_inputFileTypeFilters.append(extension);
      emit propertyChanged("InputFileTypeFilters");
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
