#include "stdafx.h"
#include "modelcomponentinfo.h"
#include "abstractmodelcomponent.h"
#include <QUuid>


ModelComponentInfo::ModelComponentInfo(QObject *parent)
   : ComponentInfo(parent)
{

}

bool ModelComponentInfo::validateLicense(QString& message) const
{
   message = name() + " | " + version() + "'s license has been validated";
   return true;
}

bool ModelComponentInfo::hasValidLicense() const
{
   return true;
}
