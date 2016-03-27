#include "stdafx.h"
#include "modelcomponentinfo.h"
#include "abstractmodelcomponent.h"


ModelComponentInfo::ModelComponentInfo(QObject *parent)
   : ComponentInfo(parent)
{

}

HydroCouple::IModelComponent* ModelComponentInfo::createComponentInstance()
{
   AbstractModelComponent* component = new AbstractModelComponent();
   component->setComponentInfo(this);
   return component;
}

bool ModelComponentInfo::validateLicense(QString& message) const
{
   message = m_name + " | " + m_version + "'s license has been validated";
   return true;
}

bool ModelComponentInfo::hasValidLicense() const
{
   return true;
}
