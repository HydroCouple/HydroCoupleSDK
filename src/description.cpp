#include "stdafx.h"
#include "description.h"


Description::Description(QObject *parent)
   : QObject(parent)
{

}

Description::Description(const QString & caption, const QString & description, QObject *parent)
   : QObject(parent), m_caption(caption), m_description(description)
{

}


QString Description::getCaption() const
{
   return m_caption;
}

void Description::setCaption(const QString& caption)
{
   m_caption = caption;
   emit propertyChanged("Caption", m_caption);
}

QString Description::getDescription() const
{
   return m_description;
}

void Description::setDescription(const QString& description)
{
   m_description = description;
   emit propertyChanged("Description", m_description);
}
