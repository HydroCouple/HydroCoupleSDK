#include "stdafx.h"
#include "core/abstractcomponentdataitem.h"
#include "core/abstractmodelcomponent.h"

using namespace HydroCouple;

AbstractComponentDataItem::AbstractComponentDataItem(const QString& id, AbstractModelComponent *parentModelComponent)
   : Identity(id,parentModelComponent), m_parentModelComponent(parentModelComponent)
{
}

AbstractComponentDataItem::AbstractComponentDataItem(const QString &id, const QString &caption, AbstractModelComponent *parentModelComponent)
   : Identity(id,caption,parentModelComponent), m_parentModelComponent(parentModelComponent)
{
}

AbstractComponentDataItem::~AbstractComponentDataItem()
{
}

IModelComponent* AbstractComponentDataItem::modelComponent() const
{
   return m_parentModelComponent;
}

void AbstractComponentDataItem::readData(QXmlStreamReader &xmlReader)
{

}

void AbstractComponentDataItem::writeData(QXmlStreamWriter &xmlWriter)
{

}
