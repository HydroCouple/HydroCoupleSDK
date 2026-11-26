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
 * Lifecycle
 * ---------
 * A component creates its arguments in its constructor (or
 * createArguments()), so a host can set them while the component is still
 * Created; the component reads them in initialize(). Nothing in this class
 * enforces isReadOnly(): the flag is metadata for hosts and editors.
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
     * \details The role starts as Configuration, the argument is required
     *          and writable, and the current input type is String.
     * \param id              Argument identifier, unique within the component;
     *                        hosts and composition documents name the
     *                        argument by it. Empty generates a UUID.
     * \param dimensions      Axes in storage order (borrowed; list copied).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Owning component (borrowed; may be nullptr).
     */
    AbstractArgument(std::string_view id,
                     const std::vector<Dimension *> &dimensions,
                     ValueDefinition *valueDefinition,
                     HydroCouple::IModelComponent *modelComponent);

    /*!
     * \brief Construct with identifier, caption, dimensions, value definition,
     *        and owning model component.
     * \param id              Argument identifier; empty generates a UUID.
     * \param caption         Human-readable caption.
     * \param dimensions      Axes in storage order (borrowed; list copied).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Owning component (borrowed; may be nullptr).
     */
    AbstractArgument(std::string_view id,
                     std::string_view caption,
                     const std::vector<Dimension *> &dimensions,
                     ValueDefinition *valueDefinition,
                     HydroCouple::IModelComponent *modelComponent);

    //! Destroys the argument; borrowed dimensions and value definition are not touched.
    ~AbstractArgument() override = default;

    // ── IArgument ─────────────────────────────────────────────────────────

    /*!
     * \brief What this argument is to the model (configuration, parameter,
     *        initial condition, forcing, geometry). Defaults to Configuration.
     * \return The role set by setRole(), or Configuration.
     */
    [[nodiscard]] HydroCouple::IArgument::ArgumentRole role() const override;

    /*!
     * \brief Returns true if the argument is optional (a missing value is acceptable).
     * \return The flag set by setIsOptional(); false by default.
     */
    [[nodiscard]] bool isOptional() const override;

    /*!
     * \brief Returns true if the argument value may not be changed by the user.
     * \return The flag set by setIsReadOnly(); false by default. The SDK does
     *         not refuse initialize() or setValuesFrom() on a read-only
     *         argument; hosts are expected to honor the flag.
     */
    [[nodiscard]] bool isReadOnly() const override;

    /*!
     * \brief Returns a string / JSON representation of the current argument value.
     *
     * Pure virtual — subclasses serialize their typed value.
     *
     * \return The value as text. The SDK's typed arguments return compact
     *         JSON: an array (Argument1D), an array of rows (Argument2D), an
     *         object keyed by identifier (IdBasedArgument), or an object
     *         with "times" and "values" (TimeSeriesArgumentDouble).
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
     * \return A copy of the filters added with addFileFilter(), in order;
     *         empty when none were added.
     */
    [[nodiscard]] std::vector<std::string> fileFilters() const override;

    /*!
     * \brief Returns the set of valid IComponentDataItem types for initialize().
     * \return The type_info pointers registered with
     *         addValidComponentDataItemType(), in order (static storage;
     *         never deleted).
     */
    [[nodiscard]] std::vector<const std::type_info *>
    validComponentDataItemTypes() const override;

    /*!
     * \brief Returns true if \p argType is a valid input type for this argument.
     * \details The base answer: String and JSON are always valid; File, XML
     *          and URL only when at least one file filter was added;
     *          MEMORY_OBJECT only when at least one valid item type was
     *          registered; YAML never. This is metadata only: the typed
     *          arguments' initialize() accepts File and URL whether or not a
     *          filter was added.
     * \param[in] argType Representation to query.
     * \return true when the representation is advertised.
     */
    [[nodiscard]] bool isValidArgType(
        HydroCouple::IArgument::ArgumentInputType argType) const override;

    /*!
     * \brief Returns the most recently used ArgumentInputType.
     * \return The type recorded by the last successful string initialize();
     *         String before any. Copying from a data item does not change it.
     */
    [[nodiscard]] HydroCouple::IArgument::ArgumentInputType
    currentArgumentInputType() const override;

    /*!
     * \brief Initializes the argument from a string, JSON, XML, file path, or URL.
     *
     * Pure virtual — subclasses parse the value according to \p argType.
     *
     * \details The SDK's typed arguments share one reading: File and URL read
     *          the text a path or URI names through IO::readReference() (a
     *          remote scheme needs the host's IO::UriResolver); any other
     *          type treats \p value as the text itself, and String and JSON
     *          are both parsed as JSON. A JSON object carrying a non-empty
     *          URI under the key named by IO::kUriRefKey is read from that
     *          URI instead.
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
     * \brief Serializes the current argument value to the requested
     *        representation (v2 write-side counterpart of initialize()).
     *
     * The base implementation supports String (via toString()) and JSON
     * (via writeData()); other representations return false. Subclasses
     * override to add formats such as YAML.
     *
     * \details JSON is writeData() pretty-printed with two-space indents. Any
     *          other type sets \p message to "unsupported serialization
     *          format for this argument".
     * \param[in]  argType  Requested representation.
     * \param[out] value    Serialized representation on success.
     * \param[out] message  Failure description on failure.
     * \return true on success.
     */
    bool serialize(HydroCouple::IArgument::ArgumentInputType argType,
                   std::string &value, std::string &message) const override;

    /*!
     * \brief The URI these values were read from, empty when they were not.
     * \return A reference valid until the next initialize() or readData().
     *         Only remote references are recorded: a local path or a
     *         \c file: URI leaves it empty, because those values are written
     *         inline.
     */
    [[nodiscard]] const std::string &sourceUri() const;

    /*!
     * \brief Reads the base fields, and the source URI beside them.
     * \details Calls AbstractComponentDataItem::readData() and then sets the
     *          source URI from the string under IO::kUriRefKey, or clears it
     *          when the key is absent or not a string. It does not fetch the
     *          URI.
     * \param[in] j JSON object as written by writeData().
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
     *
     * \return The base object with the source URI under IO::kUriRefKey when
     *         sourceUri() is not empty.
     */
    [[nodiscard]] nlohmann::json writeData() const override;

    // ── Setters (for subclass constructors) ──────────────────────────────

    /*!
     * \brief Sets the argument's role.
     * \details Does not fire propertyChanged.
     * \param[in] role What the argument is to the model; calibration and
     *                 assimilation tools select arguments by it.
     */
    void setRole(HydroCouple::IArgument::ArgumentRole role);

    /*!
     * \brief Sets whether this argument is optional.
     * \details Fires propertyChanged("IsOptional").
     * \param[in] isOptional true when a missing value is acceptable.
     */
    void setIsOptional(bool isOptional);

    /*!
     * \brief Sets whether this argument is read-only.
     * \details Fires propertyChanged("IsReadOnly").
     * \param[in] isReadOnly true when hosts must not change the value.
     */
    void setIsReadOnly(bool isReadOnly);

    /*!
     * \brief Adds a file-type filter string.
     * \details Fires propertyChanged("FileFilters"). Duplicates are kept.
     *          Adding any filter makes isValidArgType() true for File, XML
     *          and URL.
     * \param filter  e.g. "CSV Files (*.csv)".
     */
    void addFileFilter(const std::string &filter);

    /*!
     * \brief Registers a type_info as a valid ComponentDataItem type.
     * \details Makes isValidArgType(MEMORY_OBJECT) true. It does not restrict
     *          what initialize(IComponentDataItem) accepts; the typed
     *          arguments check the source's shape and kind themselves.
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
     * \param[in] argType Representation the value was last read from.
     */
    void setCurrentArgumentInputType(
        HydroCouple::IArgument::ArgumentInputType argType);

    /*!
     * \brief Records where these values came from; empty forgets it.
     * \param[in] uri Remote URI the values were read from, or empty.
     */
    void setSourceUri(const std::string &uri);

private:
    //! What the argument is to the model; Configuration by default.
    HydroCouple::IArgument::ArgumentRole m_role =
        HydroCouple::IArgument::ArgumentRole::Configuration;
    bool m_isOptional        = false; //!< Whether a missing value is acceptable.
    bool m_isReadOnly        = false; //!< Whether hosts must leave the value alone.
    //! Representation of the last successful string initialize().
    HydroCouple::IArgument::ArgumentInputType m_currentArgType =
        HydroCouple::IArgument::ArgumentInputType::String;

    std::string                          m_sourceUri;   //!< Remote URI of the values, or empty.
    std::vector<std::string>             m_fileFilters; //!< File filters, in order added.
    std::vector<const std::type_info *>  m_validTypes;  //!< Registered source item types.
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTARGUMENT_H
