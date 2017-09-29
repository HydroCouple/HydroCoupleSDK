#include "stdafx.h"
#include "core/abstractcomponentdataitem.h"
#include "core/abstractmodelcomponent.h"
#include "core/valuedefinition.h"
#include "core/dimension.h"

using namespace HydroCouple;

AbstractComponentDataItem::AbstractComponentDataItem(const QString& id,
                                                     const QList<Dimension*>& dimensions,
                                                     ValueDefinition* valueDefinition,
                                                     AbstractModelComponent *modelComponent)
  : Identity(id,modelComponent),
    m_dimensions(dimensions),
    m_valueDefinition(valueDefinition),
    m_modelComponent(modelComponent)
{
}

AbstractComponentDataItem::AbstractComponentDataItem(const QString& id,
                                                     const QString& caption,
                                                     const QList<Dimension*>& dimensions,
                                                     ValueDefinition* valueDefinition,
                                                     AbstractModelComponent *modelComponent)
  : Identity(id,caption,modelComponent),
    m_dimensions(dimensions),
    m_valueDefinition(valueDefinition),
    m_modelComponent(modelComponent)
{
}

AbstractComponentDataItem::~AbstractComponentDataItem()
{
}

IModelComponent* AbstractComponentDataItem::modelComponent() const
{
  return m_modelComponent;
}

AbstractModelComponent* AbstractComponentDataItem::modelComponentInternal() const
{
  return m_modelComponent;
}

QList<IDimension*> AbstractComponentDataItem::dimensions() const
{
  QList<IDimension*> dimensions;

  for(Dimension* dimension : m_dimensions)
    dimensions.append(dimension);

  return dimensions;
}

IValueDefinition* AbstractComponentDataItem::valueDefinition() const
{
  return m_valueDefinition;
}

QStringList AbstractComponentDataItem::comments() const
{
  return m_comments;
}

void AbstractComponentDataItem::setComments(const QStringList &comments)
{
  m_comments = comments;
  emit propertyChanged("Comments");
}

void AbstractComponentDataItem::addComment(const QString &comment)
{
  m_comments.append(comment);
  emit propertyChanged("Comments");
}

void AbstractComponentDataItem::readData(QXmlStreamReader &xmlReader)
{
  xmlReader.readNext();
}

void AbstractComponentDataItem::writeData(QXmlStreamWriter &xmlWriter) const
{
  xmlWriter.writeTextElement("Empty" ,"");
}

QList<Dimension*> AbstractComponentDataItem::dimensionsInternal() const
{
  return m_dimensions;
}

ValueDefinition* AbstractComponentDataItem::valueDefinitionInternal() const
{
  return m_valueDefinition;
}

