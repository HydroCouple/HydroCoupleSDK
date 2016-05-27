#include "stdafx.h"
#include "core/description.h"

Description::Description(QObject *parent)
   : QObject(parent)
{

}

Description::Description(const QString &caption,  QObject *parent)
   : QObject(parent), m_caption(caption)
{

}

Description::~Description()
{

}

QString Description::caption() const
{
   return m_caption;
}

void Description::setCaption(const QString &caption)
{
   m_caption = caption;
   emit propertyChanged("Caption");
}

QString Description::description() const
{
   return m_description;
}

void Description::setDescription(const QString &description)
{
   m_description = description;
   emit propertyChanged("Description");
}
