#include "stdafx.h"
#include "core/dimension.h"
#include <stdexcept>


using namespace std;

Dimension::Dimension(const QString &id, const QString &caption, QObject *parent)
   :Identity(id,caption,parent)
{
}

Dimension::~Dimension()
{
}

