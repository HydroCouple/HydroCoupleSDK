#include "stdafx.h"
#include "identity.h"

Identity::Identity(const QString &id, QObject *parent)
   : Description(parent) , m_id(id)
{

}

Identity::Identity(const QString &id, const QString &caption, QObject *parent)
   : Description(caption, parent) , m_id(id)
{

}

QString Identity::id() const
{
   return m_id;
}

void Identity::setId(const QString& id)
{
   m_id = id;
   emit propertyChanged("Id");
}
