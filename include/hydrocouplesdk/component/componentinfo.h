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
 * documentation citations, version string, URL, email, icon path and
 * classification tags. A host reads it to list a component library before
 * creating any instance. Subclasses (AbstractModelComponentInfo) extend
 * this with factory capabilities; the interface forbids implementing
 * IComponentInfo on its own.
 *
 * Inherits from Identity, so every ComponentInfo carries an id: give it a
 * stable, reverse-domain id (for example "org.example.reservoir"), because
 * hosts and composition documents identify the component type by it. The
 * default constructor generates a random UUID, which changes every run.
 *
 * The setters other than setLibraryFilePath() are protected: a subclass
 * fills the metadata in its constructor. Only setLibraryFilePath() and
 * setLicenseText() fire propertyChanged.
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
     * \param[in] id  Unique identifier string of the component type; copied.
     *                Empty is replaced by a generated UUID v4.
     */
    explicit ComponentInfo(std::string_view id);

    //! Destroys the info. Instances it created are not affected.
    ~ComponentInfo() override = default;

    // ── IComponentInfo ────────────────────────────────────────────────────

    /*!
     * \brief Returns the path to the shared library from which this component
     *        was loaded.
     * \return The path a host stored with setLibraryFilePath(); empty until
     *         then. The SDK never sets it itself.
     */
    [[nodiscard]] std::string libraryFilePath() const override;

    /*!
     * \brief Sets the library file path.
     * \details Called by a host after it loads the library. Fires
     *          propertyChanged("LibraryFilePath").
     * \param[in] filePath  Absolute or relative path to the shared library;
     *                      copied, not checked.
     */
    void setLibraryFilePath(const std::string &filePath) override;

    /*!
     * \brief Returns the path (relative to the library) to the component icon.
     * \return The path set with setIconFilePath(); empty when none.
     */
    [[nodiscard]] std::string iconFilePath() const override;

    /*!
     * \brief Returns the developer / vendor name.
     * \return The name set with setDeveloper(); empty when none.
     */
    [[nodiscard]] std::string developer() const override;

    /*!
     * \brief Returns the list of documentation citations for this component.
     * \return A copy of the citations, in the order they were added.
     */
    [[nodiscard]] std::vector<std::string> documentation() const override;

    /*!
     * \brief Returns the license text or SPDX identifier.
     * \return The text set with setLicenseText(); empty when none.
     */
    [[nodiscard]] std::string license() const override;

    /*!
     * \brief Returns the copyright notice string.
     * \return The notice set with setCopyright(); empty when none.
     */
    [[nodiscard]] std::string copyright() const override;

    /*!
     * \brief Returns the developer's home-page URL.
     * \return The URL set with setUrl(); empty when none.
     */
    [[nodiscard]] std::string url() const override;

    /*!
     * \brief Returns the developer's contact email.
     * \return The address set with setEmail(); empty when none.
     */
    [[nodiscard]] std::string email() const override;

    /*!
     * \brief Returns the component version string (for example "1.0.0").
     * \return The version set with setVersion(); empty when none.
     */
    [[nodiscard]] std::string version() const override;

    /*!
     * \brief Returns the set of classification tags for this component.
     * \details Examples: "Hydrology", "Groundwater", "Finite Volume".
     * \return A copy of the tags, sorted (a \c std::set).
     */
    [[nodiscard]] std::set<std::string> tags() const override;

protected:
    /*!
     * \brief Sets the icon file path (called by subclass constructors).
     * \param[in] path  Path relative to the component library; copied.
     */
    void setIconFilePath(std::string_view path);

    /*!
     * \brief Sets the developer / vendor name.
     * \param[in] developer  Name; copied.
     */
    void setDeveloper(std::string_view developer);

    /*!
     * \brief Appends a documentation citation.
     * \param[in] document  Citation text; copied.
     */
    void addDocumentation(std::string_view document);

    /*!
     * \brief Replaces the entire documentation list.
     * \param[in] docs  New citations; moved in.
     */
    void setDocumentation(std::vector<std::string> docs);

    /*!
     * \brief Sets the copyright notice.
     * \param[in] copyright  Notice text; copied.
     */
    void setCopyright(std::string_view copyright);

    /*!
     * \brief Sets the developer URL.
     * \param[in] url  URL; copied, not checked.
     */
    void setUrl(std::string_view url);

    /*!
     * \brief Sets the developer email.
     * \param[in] email  Address; copied, not checked.
     */
    void setEmail(std::string_view email);

    /*!
     * \brief Sets the version string.
     * \param[in] version  Version text (for example "1.2.0"); copied.
     */
    void setVersion(std::string_view version);

    /*!
     * \brief Adds a single classification tag.
     * \details Adding a tag already present does nothing.
     * \param[in] tag  Tag text; copied.
     */
    void addTag(std::string_view tag);

    /*!
     * \brief Replaces all classification tags.
     * \param[in] tags  New tags; moved in.
     */
    void setTags(std::set<std::string> tags);

    /*!
     * \brief Sets the license text returned by license().
     * \details Fires propertyChanged("License"). There is no other license
     *          setter.
     * \param[in] license  License text or SPDX identifier (for example
     *                     "MIT"); copied.
     */
    void setLicenseText(std::string_view license);

private:
    std::string            m_libraryFilePath; //!< Set by the host.
    std::string            m_iconFilePath;    //!< Relative to the library.
    std::string            m_developer;       //!< Developer or vendor.
    std::string            m_license;         //!< License text or SPDX id.
    std::string            m_copyright;       //!< Copyright notice.
    std::string            m_url;             //!< Developer URL.
    std::string            m_email;           //!< Developer email.
    std::string            m_version;         //!< Component version.
    std::vector<std::string> m_documentation; //!< Citations, in order.
    std::set<std::string>  m_tags;            //!< Classification tags.
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_COMPONENTINFO_H
