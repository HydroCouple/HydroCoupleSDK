// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   componentinfo.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Concrete implementation of HydroCouple::IComponentInfo.
 */

#ifndef HYDROCOUPLESDK_COMPONENTINFO_H
#define HYDROCOUPLESDK_COMPONENTINFO_H

#include "hydrocouplesdk/core/identity.h"
#include "hydrocouple.h"

#include <set>
#include <string>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Concrete implementation of HydroCouple::IComponentInfo.
 *
 * Stores all metadata about a component: developer name, license,
 * documentation citations, version string, URL, email, and classification
 * tags. Subclasses (e.g. AbstractModelComponentInfo) extend this with
 * factory capabilities.
 *
 * Inherits from Identity so every ComponentInfo carries a stable UUID.
 *
 * \threadsafety Not thread-safe on its own. Metadata is normally written
 * once at construction time and read-only thereafter.
 */
class HYDROCOUPLESDK_EXPORT ComponentInfo
    : public Identity,
      public virtual HydroCouple::IComponentInfo
{
public:
    /*!
     * \brief Construct with an auto-generated UUID.
     */
    ComponentInfo() = default;

    /*!
     * \brief Construct with an explicit identifier.
     * \param id  Unique identifier string.
     */
    explicit ComponentInfo(std::string_view id);

    ~ComponentInfo() override = default;

    // ── IComponentInfo ────────────────────────────────────────────────────

    /*!
     * \brief Returns the path to the shared library from which this component
     *        was loaded.
     */
    [[nodiscard]] std::string libraryFilePath() const override;

    /*!
     * \brief Sets the library file path.
     * \param filePath  Absolute or relative path to the shared library.
     */
    void setLibraryFilePath(const std::string &filePath) override;

    /*!
     * \brief Returns the path (relative to the library) to the component icon.
     */
    [[nodiscard]] std::string iconFilePath() const override;

    /*!
     * \brief Returns the developer / vendor name.
     */
    [[nodiscard]] std::string developer() const override;

    /*!
     * \brief Returns the list of documentation citations for this component.
     */
    [[nodiscard]] std::vector<std::string> documentation() const override;

    /*!
     * \brief Returns the license text or SPDX identifier.
     */
    [[nodiscard]] std::string license() const override;

    /*!
     * \brief Returns the copyright notice string.
     */
    [[nodiscard]] std::string copyright() const override;

    /*!
     * \brief Returns the developer's home-page URL.
     */
    [[nodiscard]] std::string url() const override;

    /*!
     * \brief Returns the developer's contact email.
     */
    [[nodiscard]] std::string email() const override;

    /*!
     * \brief Returns the component version string (for example "1.0.0").
     */
    [[nodiscard]] std::string version() const override;

    /*!
     * \brief Returns the set of classification tags for this component.
     * \details Examples: "Hydrology", "Groundwater", "Finite Volume".
     */
    [[nodiscard]] std::set<std::string> tags() const override;

protected:
    /*!
     * \brief Sets the icon file path (called by subclass constructors).
     */
    void setIconFilePath(std::string_view path);

    /*!
     * \brief Sets the developer / vendor name.
     */
    void setDeveloper(std::string_view developer);

    /*!
     * \brief Appends a documentation citation.
     */
    void addDocumentation(std::string_view document);

    /*!
     * \brief Replaces the entire documentation list.
     */
    void setDocumentation(std::vector<std::string> docs);

    /*!
     * \brief Sets the copyright notice.
     */
    void setCopyright(std::string_view copyright);

    /*!
     * \brief Sets the developer URL.
     */
    void setUrl(std::string_view url);

    /*!
     * \brief Sets the developer email.
     */
    void setEmail(std::string_view email);

    /*!
     * \brief Sets the version string.
     */
    void setVersion(std::string_view version);

    /*!
     * \brief Adds a single classification tag.
     */
    void addTag(std::string_view tag);

    /*!
     * \brief Replaces all classification tags.
     */
    void setTags(std::set<std::string> tags);

    /*!
     * \brief Sets the license text (also accessible via setLicense).
     */
    void setLicenseText(std::string_view license);

private:
    std::string            m_libraryFilePath;
    std::string            m_iconFilePath;
    std::string            m_developer;
    std::string            m_license;
    std::string            m_copyright;
    std::string            m_url;
    std::string            m_email;
    std::string            m_version;
    std::vector<std::string> m_documentation;
    std::set<std::string>  m_tags;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_COMPONENTINFO_H
