#include "stdafx.h"
#include "spatial/spatialreferencesystem.h"
#include <ogr_spatialref.h>

SpatialReferenceSystem::SpatialReferenceSystem(const QString &caption, const QString& srText, QObject *parent)
  :Description(caption , parent)
{
   m_srs = new OGRSpatialReference(srText.toStdString().c_str());
   m_srs->AutoIdentifyEPSG();
}

SpatialReferenceSystem::SpatialReferenceSystem(const QString &caption, const QString &authName, int authSRID, QObject *parent)
  :Description(caption , parent)
{
   m_srs = new OGRSpatialReference();
   m_srs->SetAuthority(nullptr , authName.toStdString().c_str() , authSRID);
}

SpatialReferenceSystem::~SpatialReferenceSystem()
{
  OGRSpatialReference::DestroySpatialReference(m_srs);
}

int SpatialReferenceSystem::authSRID() const
{
  QString code =  QString(m_srs->GetAuthorityCode(nullptr));
  return code.toInt();
}

QString SpatialReferenceSystem::authName() const
{
  QString authName(m_srs->GetAuthorityName(nullptr));
  return authName;
}

void SpatialReferenceSystem::setAuth(const QString &name, int authSRID)
{
  m_srs->SetAuthority(nullptr,name.toStdString().c_str(),authSRID);
  emit propertyChanged("AuthSRID");
  emit propertyChanged("AuthName");
  emit propertyChanged("WellKnownText");
}

QString SpatialReferenceSystem::srText() const
{
  char** name = nullptr;
  m_srs->exportToWkt(name);
  QString srsWkt(*name);
  OGRFree(name);
  return srsWkt ;
}

void SpatialReferenceSystem::setSrText(const QString &srtext)
{
  char* name = new char[srtext.length()  +1];
  strcpy(name, srtext.toStdString().c_str());

  m_srs->importFromWkt(&name);

  emit propertyChanged("AuthSRID");
  emit propertyChanged("AuthName");
  emit propertyChanged("WellKnownText");
}
