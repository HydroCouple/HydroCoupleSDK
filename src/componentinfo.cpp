#include "stdafx.h"
#include "componentinfo.h"


ComponentInfo::ComponentInfo(QObject *parent)
   : Identity("1.0.00000.11","Abstract Component must not be used directly", "Abstract Component must not be used directly", parent),
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
   emit propertyChanged("FilePath", m_filePath);
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


