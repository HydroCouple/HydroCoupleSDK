#include "stdafx.h"
#include "core/componentinfo.h"
#include "core/identity.h"
#include <QUuid>

ComponentInfo::ComponentInfo(QObject *parent)
  : Identity("1.0.00000.11","Abstract Component must not be used directly", parent),
    m_iconFilePath(":/HydroCoupleComposer/hydrocouplecomposer"),
    m_vendor("Insuyo LLC"),
    m_license("License Information"),
    m_copyright("Copyright &copy; 2014 Insuyo LLC"),
    m_url("www.insuyo.com"),
    m_email("support@insuyo.com"),
    m_version("1.0.0.1 ")
{
  setId(QUuid::createUuid().toString());
}

QString ComponentInfo::libraryFilePath() const
{
  return m_filePath;
}

void ComponentInfo::setLibraryFilePath(const QString& filePath)
{
  m_filePath = filePath;
  emit propertyChanged("FilePath");
}

QString ComponentInfo::iconFilePath() const
{
  return m_iconFilePath;
}

QString ComponentInfo::vendor() const
{
  return m_vendor;
}

QStringList ComponentInfo::documentation() const
{
  return m_documentation;
}

QString ComponentInfo::license() const
{
  return m_license;
}

QString ComponentInfo::copyright() const
{
  return m_copyright;
}

QString ComponentInfo::url() const
{
  return m_url;
}

QString ComponentInfo::email() const
{
  return m_email;
}

QString ComponentInfo::version() const
{
  return m_version;
}

QString ComponentInfo::category() const
{
  return m_category;
}

bool ComponentInfo::hasValidLicense() const
{
   return true;
}

bool ComponentInfo::validateLicense(const QString& licenseInfo, QString& message)
{
  m_license = licenseInfo;
  message = id() + " | " + version() + "'s license has been validated";
  return true;
}

void ComponentInfo::setIconFilePath(const QString& filepath)
{
  m_iconFilePath = filepath;
  emit propertyChanged("IconFilePath");
}

void ComponentInfo::setVendor(const QString& vendor)
{
  m_vendor = vendor;
  emit propertyChanged("Vendor");
}

void ComponentInfo::setDocumentation(const QStringList &documentation)
{
  m_documentation = documentation;
  emit propertyChanged("Documentation");
}

void ComponentInfo::addDocumentation(const QString &document)
{
  if(!m_documentation.contains(document))
  {
    m_documentation.append(document);
    emit propertyChanged("Documentation");
  }
}

void ComponentInfo::setLicense(const QString & licenseInfo)
{
  m_license = licenseInfo;
  emit propertyChanged("License");
}

void ComponentInfo::setCopyright(const QString& copyright)
{
  m_copyright = copyright;
  emit propertyChanged("Copyright");
}

void ComponentInfo::setUrl(const QString &url)
{
  m_url = url;
  emit propertyChanged("Url");
}

void ComponentInfo::setEmail(const QString& email)
{
  m_email = email;
  emit propertyChanged("Email");
}

void ComponentInfo::setVersion(const QString& version)
{
  m_version = version;
  emit propertyChanged("Version");
}

void ComponentInfo::setCategory(const QString& category)
{
  m_category = category;
  emit propertyChanged("Category");
}





