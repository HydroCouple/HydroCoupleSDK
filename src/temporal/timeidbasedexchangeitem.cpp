#include "stdafx.h"
#include "temporal/timeidbasedexchangeitem.h"

using namespace Temporal;

TimeIdBasedOutputDouble::TimeIdBasedOutputDouble(const QString &id, const QStringList &identifiers,
                                     const QList<Time *> &times, Dimension *identifierDimension,
                                     Dimension *timeDimension, ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent)
   : AbstractOutput(id,parentModelComponent) , TimeIdBasedComponentDataItem<double>(identifiers,times,identifierDimension,timeDimension,valueDefinition)
{

}

TimeIdBasedOutputDouble::~TimeIdBasedOutputDouble()
{

}


TimeIdBasedInputDouble::TimeIdBasedInputDouble(const QString &id, const QStringList &identifiers,
                                     const QList<Time *> &times, Dimension *identifierDimension,
                                     Dimension *timeDimension, ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent)
   : AbstractInput(id,parentModelComponent), TimeIdBasedComponentDataItem<double>(identifiers,times,identifierDimension,timeDimension,valueDefinition)
{

}

TimeIdBasedInputDouble::~TimeIdBasedInputDouble()
{

}

TimeIdBasedMultiInputDouble::TimeIdBasedMultiInputDouble(const QString &id, const QStringList &identifiers,
                                     const QList<Time *> &times, Dimension *identifierDimension,
                                     Dimension *timeDimension, ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent)
   : AbstractMultiInput(id,parentModelComponent), TimeIdBasedComponentDataItem<double>(identifiers,times,identifierDimension,timeDimension,valueDefinition)
{

}

TimeIdBasedMultiInputDouble::~TimeIdBasedMultiInputDouble()
{

}
