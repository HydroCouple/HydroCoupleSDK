#include "stdafx.h"
#include "idbasedtimeexchangeitem.h"


IdBasedTimeOutputDouble::IdBasedTimeOutputDouble(const QString &id, const QStringList &identifiers,
                                     const QList<HTime *> &times, Dimension *identifierDimension,
                                     Dimension *timeDimension, ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent)
   : AbstractOutput(id,parentModelComponent) , IdBasedTimeComponentDataItem<double>(identifiers,times,identifierDimension,timeDimension,valueDefinition)
{

}

IdBasedTimeOutputDouble::~IdBasedTimeOutputDouble()
{

}


IdBasedTimeInputDouble::IdBasedTimeInputDouble(const QString &id, const QStringList &identifiers,
                                     const QList<HTime *> &times, Dimension *identifierDimension,
                                     Dimension *timeDimension, ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent)
   : AbstractInput(id,parentModelComponent), IdBasedTimeComponentDataItem<double>(identifiers,times,identifierDimension,timeDimension,valueDefinition)
{

}

IdBasedTimeInputDouble::~IdBasedTimeInputDouble()
{

}

IdBasedTimeMultiInputDouble::IdBasedTimeMultiInputDouble(const QString &id, const QStringList &identifiers,
                                     const QList<HTime *> &times, Dimension *identifierDimension,
                                     Dimension *timeDimension, ValueDefinition *valueDefinition, AbstractModelComponent *parentModelComponent)
   : AbstractMultiInput(id,parentModelComponent), IdBasedTimeComponentDataItem<double>(identifiers,times,identifierDimension,timeDimension,valueDefinition)
{

}

IdBasedTimeMultiInputDouble::~IdBasedTimeMultiInputDouble()
{

}
