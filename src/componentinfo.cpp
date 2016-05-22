#include "stdafx.h"
#include "componentinfo.h"
#include "identity.h"

ComponentInfo::ComponentInfo(QObject *parent)
   : Identity("1.0.00000.11","Abstract Component must not be used directly", parent),
     m_name("Abstract Component"),
     m_iconFilePath(":/HydroCoupleComposer/hydrocouplecomposer"),
     m_vendor("Insuyo LLC"),
     m_license("License Information"),
     m_copyright("Copyright &copy; 2014 Insuyo LLC"),
     m_url("www.insuyo.com"),
     m_email("support@insuyo.com"),
     m_version("1.0.00000.11 "),
     m_category("\\Hydrology")
{
   m_publications.append("tet");
}

QString ComponentInfo::name() const
{
   return m_name;
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

QStringList ComponentInfo::publications() const
{
   return m_publications;
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

void ComponentInfo::setName(const QString& name)
{
   m_name = name;
   emit propertyChanged("Name");
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

void ComponentInfo::setPublications(const QStringList & publications)
{
   m_publications = publications;
   emit propertyChanged("Publications");
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





