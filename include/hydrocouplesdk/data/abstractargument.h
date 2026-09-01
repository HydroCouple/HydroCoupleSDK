// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractargument.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Abstract base implementation of HydroCouple::IArgument.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTARGUMENT_H
#define HYDROCOUPLESDK_ABSTRACTARGUMENT_H

#include "hydrocouplesdk/data/abstractcomponentdataitem.h"
#include "hydrocouple.h"

#include <string>
#include <typeinfo>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Abstract base class for HydroCouple::IArgument.
 *
 * Implements argument metadata (optional flag, read-only flag, file filters,
 * current input type) and delegates data storage to the subclass.
 *
 * Subclasses must implement:
 * - toString() — return the current argument value as a string
 * - saveData() — persist associated files (if any)
 * - initialize(string, ArgumentInputType, string&) — parse and store a value
 * - initialize(IComponentDataItem, string&) — copy from another data item
 * - dimensionLength() — required by IComponentDataItem
 * - getValue / getValues / setValue / setValues — typed data access
 *
 * File-filter registration
 * ------------------------
 * Call addFileFilter("CSV Files (*.csv)") in the subclass constructor to
 * declare which file types the argument can read.
 *
 * Valid component-data-item types
 * --------------------------------
 * Call addValidComponentDataItemType<MyType>() to declare which concrete
 * IComponentDataItem types can be passed to initialize(IComponentDataItem).
 *
 * \threadsafety Not thread-safe on its own. Argument mutation is expected
 * to happen from a single thread before component initialization.
 */
class HYDROCOUPLESDK_EXPORT AbstractArgument
    : public AbstractComponentDataItem,
      public virtual HydroCouple::IArgument
{
public:
    /*!
     * \brief Construct with identifier, dimensions, value definition, and
     *        owning model component.
     */
    AbstractArgument(std::string_view id,
                     const std::vector<Dimension *> &dimensions,
                     ValueDefinition *valueDefinition,
                     HydroCouple::IModelComponent *modelComponent);

    /*!
     * \brief Construct with identifier, caption, dimensions, value definition,
     *        and owning model component.
     */
    AbstractArgument(std::string_view id,
                     std::string_view caption,
                     const std::vector<Dimension *> &dimensions,
                     ValueDefinition *valueDefinition,
                     HydroCouple::IModelComponent *modelComponent);

    ~AbstractArgument() override = default;

    // ── IArgument ─────────────────────────────────────────────────────────

    /*!
     * \brief Returns true if the argument is optional (a missing value is acceptable).
     */
    [[nodiscard]] bool isOptional() const override;

    /*!
     * \brief Returns true if the argument value may not be changed by the user.
     */
    [[nodiscard]] bool isReadOnly() const override;

    /*!
     * \brief Returns a string / JSON representation of the current argument value.
     *
     * Pure virtual — subclasses serialise their typed value.
     */
    [[nodiscard]] std::string toString() const override = 0;

    /*!
     * \brief Writes data to associated files (if any).
     *
     * The default implementation does nothing. Subclasses that maintain
     * external file state (e.g. CSV, NetCDF) override this.
     */
    void saveData() override;

    /*!
     * \brief Returns file-type filter strings for this argument.
     * \details Format: "Label (*.ext1 *.ext2)" — e.g. "CSV Files (*.csv)".
     */
    [[nodiscard]] std::vector<std::string> fileFilters() const override;

    /*!
     * \brief Returns the set of valid IComponentDataItem types for initialize().
     */
    [[nodiscard]] std::vector<const std::type_info *>
    validComponentDataItemTypes() const override;

    /*!
     * \brief Returns true if \p argType is a valid input type for this argument.
     */
    [[nodiscard]] bool isValidArgType(
        HydroCouple::IArgument::ArgumentInputType argType) const override;

    /*!
     * \brief Returns the most recently used ArgumentInputType.
     */
    [[nodiscard]] HydroCouple::IArgument::ArgumentInputType
    currentArgumentInputType() const override;

    /*!
     * \brief Initialises the argument from a string, JSON, XML, file path, or URL.
     *
     * Pure virtual — subclasses parse the value according to \p argType.
     *
     * \param[in]  value    String representation (depends on argType).
     * \param[in]  argType  How to interpret \p value.
     * \param[out] message  Error or status message.
     * \return true on success.
     */
    bool initialize(const std::string &value,
                    HydroCouple::IArgument::ArgumentInputType argType,
                    std::string &message) override = 0;

    /*!
     * \brief Copies values from a compatible IComponentDataItem.
     *
     * Pure virtual — subclasses perform the typed copy.
     *
     * \param[in]  componentDataItem  Source data item.
     * \param[out] message            Error or status message.
     * \return true on success.
     */
    bool initialize(const HydroCouple::IComponentDataItem &componentDataItem,
                    std::string &message) override = 0;

    /*!
     * \brief Serialises the current argument value to the requested
     *        representation (v2 write-side counterpart of initialize()).
     *
     * The base implementation supports String (via toString()) and JSON
     * (via writeData()); other representations return false. Subclasses
     * override to add formats such as YAML.
     *
     * \param[in]  argType  Requested representation.
     * \param[out] value    Serialized representation on success.
     * \param[out] message  Failure description on failure.
     * \return true on success.
     */
    bool serialize(HydroCouple::IArgument::ArgumentInputType argType,
                   std::string &value, std::string &message) const override;

    /*!
     * \brief The URI these values were read from, empty when they were not.
     */
    [[nodiscard]] const std::string &sourceUri() const;

    /*!
     * \brief Reads the base fields, and the source URI beside them.
     */
    void readData(const nlohmann::json &j) override;

    /*!
     * \brief Writes the base fields, and the source URI when there is one.
     *
     * IArgument::serialize() asks for exactly this: "the serialized form
     * should carry an external binary payload reference (URI ...)". A local
     * file is still inlined as values -- the document can be moved next to
     * it, and it always has been -- but a resource on a service cannot be
     * got back from the document, so where it came from is recorded.
     */
    [[nodiscard]] nlohmann::json writeData() const override;

    // ── Setters (for subclass constructors) ──────────────────────────────

    /*!
     * \brief Sets whether this argument is optional.
     */
    void setIsOptional(bool isOptional);

    /*!
     * \brief Sets whether this argument is read-only.
     */
    void setIsReadOnly(bool isReadOnly);

    /*!
     * \brief Adds a file-type filter string.
     * \param filter  e.g. "CSV Files (*.csv)".
     */
    void addFileFilter(const std::string &filter);

    /*!
     * \brief Registers a type_info as a valid ComponentDataItem type.
     * \tparam ItemType  The concrete IComponentDataItem subtype to register.
     */
    template<typename ItemType>
    void addValidComponentDataItemType()
    {
        m_validTypes.push_back(&typeid(ItemType));
    }

protected:
    /*!
     * \brief Updates the current input type (call from initialize() overrides).
     */
    void setCurrentArgumentInputType(
        HydroCouple::IArgument::ArgumentInputType argType);

    /*!
     * \brief Records where these values came from; empty forgets it.
     */
    void setSourceUri(const std::string &uri);

private:
    bool m_isOptional        = false;
    bool m_isReadOnly        = false;
    HydroCouple::IArgument::ArgumentInputType m_currentArgType =
        HydroCouple::IArgument::ArgumentInputType::String;

    std::string                          m_sourceUri;
    std::vector<std::string>             m_fileFilters;
    std::vector<const std::type_info *>  m_validTypes;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTARGUMENT_H
