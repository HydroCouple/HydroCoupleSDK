#include "stdafx.h"
#include "core/dimension.h"
#include <stdexcept>


using namespace std;

Dimension::Dimension(const QString &id, QObject *parent)
   :Identity(id,parent)
{
}

Dimension::Dimension(const QString &id, const QString &caption, QObject *parent)
   :Identity(id,caption,parent)
{
}

Dimension::~Dimension()
{
}

Dimension *Dimension::copy(QObject *parent)
{
  return copy(this, parent);
}

Dimension *Dimension::copy(const HydroCouple::IDimension *dimension, QObject *parent)
{
  Dimension *oDimension = new Dimension(dimension->id(),parent);
  oDimension->setCaption(dimension->caption());
  oDimension->setDescription(dimension->description());

  return oDimension;
}
