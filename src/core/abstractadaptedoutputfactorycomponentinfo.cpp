#include "stdafx.h"
#include "core/abstractadaptedoutputfactorycomponentinfo.h"

AbstractAdaptedOutputFactoryComponentInfo::AbstractAdaptedOutputFactoryComponentInfo(QObject *parent)
  :ComponentInfo(parent)
{

}

AbstractAdaptedOutputFactoryComponentInfo::~AbstractAdaptedOutputFactoryComponentInfo()
{

}

bool AbstractAdaptedOutputFactoryComponentInfo::validateLicense(QString& message) const
{
   message = id() + " | " + version() + "'s license has been validated";
   return true;
}

bool AbstractAdaptedOutputFactoryComponentInfo::hasValidLicense() const
{
   return true;
}
