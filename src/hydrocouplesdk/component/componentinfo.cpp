// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "stdafx.h"
#include "hydrocouplesdk/component/componentinfo.h"

namespace HydroCouple::SDK {

ComponentInfo::ComponentInfo(std::string_view id)
    : Identity(id)
{
}

std::string ComponentInfo::libraryFilePath() const
{
    return m_libraryFilePath;
}

void ComponentInfo::setLibraryFilePath(const std::string &filePath)
{
    m_libraryFilePath = filePath;
    emitPropertyChanged("LibraryFilePath");
}

std::string ComponentInfo::iconFilePath() const
{
    return m_iconFilePath;
}

std::string ComponentInfo::developer() const
{
    return m_developer;
}

std::vector<std::string> ComponentInfo::documentation() const
{
    return m_documentation;
}

std::string ComponentInfo::license() const
{
    return m_license;
}

std::string ComponentInfo::copyright() const
{
    return m_copyright;
}

std::string ComponentInfo::url() const
{
    return m_url;
}

std::string ComponentInfo::email() const
{
    return m_email;
}

std::string ComponentInfo::version() const
{
    return m_version;
}

std::set<std::string> ComponentInfo::tags() const
{
    return m_tags;
}

// ── Protected setters ─────────────────────────────────────────────────────

void ComponentInfo::setIconFilePath(std::string_view path)
{
    m_iconFilePath = path;
}

void ComponentInfo::setDeveloper(std::string_view developer)
{
    m_developer = developer;
}

void ComponentInfo::addDocumentation(std::string_view document)
{
    m_documentation.emplace_back(document);
}

void ComponentInfo::setDocumentation(std::vector<std::string> docs)
{
    m_documentation = std::move(docs);
}

void ComponentInfo::setCopyright(std::string_view copyright)
{
    m_copyright = copyright;
}

void ComponentInfo::setUrl(std::string_view url)
{
    m_url = url;
}

void ComponentInfo::setEmail(std::string_view email)
{
    m_email = email;
}

void ComponentInfo::setVersion(std::string_view version)
{
    m_version = version;
}

void ComponentInfo::addTag(std::string_view tag)
{
    m_tags.emplace(tag);
}

void ComponentInfo::setTags(std::set<std::string> tags)
{
    m_tags = std::move(tags);
}

void ComponentInfo::setLicenseText(std::string_view license)
{
    m_license = license;
    emitPropertyChanged("License");
}

} // namespace HydroCouple::SDK
