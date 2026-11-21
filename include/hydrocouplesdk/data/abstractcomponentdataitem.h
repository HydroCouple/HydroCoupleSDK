// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   abstractcomponentdataitem.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  Abstract base implementation of HydroCouple::IComponentDataItem.
 */

#ifndef HYDROCOUPLESDK_ABSTRACTCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_ABSTRACTCOMPONENTDATAITEM_H

#include "hydrocouplesdk/core/identity.h"
#include "hydrocouplesdk/core/signal.h"
#include "hydrocouple.h"

#include <memory>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>

namespace HydroCouple::SDK {

class Dimension;
class ValueDefinition;

// ── ComponentDataItemValueChanged ─────────────────────────────────────────

/*!
 * \brief Concrete implementation of IComponentDataItemValueChanged.
 *
 * Carries a pointer back to the data item that changed plus the start/count
 * hyperslab describing the affected region.
 */
class HYDROCOUPLESDK_EXPORT ComponentDataItemValueChanged
    : public HydroCouple::IComponentDataItemValueChanged
{
public:
    ComponentDataItemValueChanged(HydroCouple::IComponentDataItem *item,
                                  std::vector<int64_t> start = {},
                                  std::vector<int64_t> count = {});

    ~ComponentDataItemValueChanged() override = default;

    HydroCouple::IComponentDataItem *componentDataItem() const override;

    /*! \brief First changed index in each dimension. */
    std::vector<int64_t> start() const override;

    /*! \brief Changed extent in each dimension. */
    std::vector<int64_t> count() const override;

private:
    HydroCouple::IComponentDataItem *m_item;
    std::vector<int64_t>             m_start;
    std::vector<int64_t>             m_count;
};


// ── AbstractComponentDataItem ─────────────────────────────────────────────

/*!
 * \brief Abstract base class for HydroCouple::IComponentDataItem.
 *
 * Implements the bookkeeping common to all data items (dimensions, value
 * definition, owning model component, comments, and the valueChanged
 * signal). Subclasses provide typed storage and implement the pure-virtual
 * getValue / getValues / setValue / setValues / dimensionLength methods.
 *
 * Signal design
 * -------------
 * AbstractComponentDataItem holds two signals:
 * - \c m_propertyChanged (Signal<std::string>) — inherited via Description;
 *   fires when metadata (caption, description, comments) changes.
 * - \c m_valueChanged (Signal<...>) — fires when element data change;
 *   implements ISignal<const shared_ptr<IComponentDataItemValueChanged>&>.
 *
 * Both signals share the same blockSignals(bool) gate.
 *
 * Serialization
 * -------------
 * readData() / writeData() use nlohmann::json for argument persistence.
 * The default implementations read/write caption and comments.
 * Subclasses may extend them for typed value serialization.
 *
 * \threadsafety Not thread-safe on its own. The signals are individually
 * thread-safe (std::shared_mutex), but the data arrays are not.
 */
class HYDROCOUPLESDK_EXPORT AbstractComponentDataItem
    : public Identity,
      public virtual HydroCouple::IComponentDataItem
{
public:
    /*!
     * \brief Construct with identifier, dimension list, value definition,
     *        and owning model component.
     * \param id              Unique identifier string.
     * \param dimensions      Ordered list of Dimension pointers (borrowed).
     * \param valueDefinition Value definition (borrowed).
     * \param modelComponent  Owning component (may be nullptr for standalone items).
     */
    AbstractComponentDataItem(std::string_view id,
                               const std::vector<Dimension *> &dimensions,
                               ValueDefinition *valueDefinition,
                               HydroCouple::IModelComponent *modelComponent);

    /*!
     * \brief Construct with identifier, caption, dimensions, value definition,
     *        and owning model component.
     */
    AbstractComponentDataItem(std::string_view id,
                               std::string_view caption,
                               const std::vector<Dimension *> &dimensions,
                               ValueDefinition *valueDefinition,
                               HydroCouple::IModelComponent *modelComponent);

    ~AbstractComponentDataItem() override = default;

    // ── IComponentDataItem ────────────────────────────────────────────────

    /*!
     * \brief Returns the owning model component (may be nullptr).
     */
    [[nodiscard]] HydroCouple::IModelComponent *modelComponent() const override;

    /*!
     * \brief Returns the list of dimensions as IIDimension pointers.
     */
    [[nodiscard]] std::vector<HydroCouple::IDimension *> dimensions() const override;

    /*!
     * \brief Returns the value definition for this data item.
     */
    [[nodiscard]] HydroCouple::IValueDefinition *valueDefinition() const override;

    // ── v2 typed hyperslab data plane — pure virtual ──────────────────────

    /*!
     * \brief The extent of each dimension of the item's index space.
     * Pure virtual — implemented by typed subclasses (typically by
     * delegating to their storage mixin's storageShape()).
     */
    [[nodiscard]] std::vector<int64_t> shape() const override = 0;

    /*!
     * \brief The element type of the item's values.
     * Pure virtual — implemented by typed subclasses.
     */
    [[nodiscard]] DataKind dataKind() const override = 0;

    /*!
     * \brief Copies a hyperslab of this item's values into \p destination.
     * Pure virtual — implemented by typed subclasses.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override = 0;

    /*!
     * \brief Copies values from \p source into a hyperslab of this item.
     * Pure virtual — implemented by typed subclasses.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override = 0;

    // ── ISignal<const shared_ptr<IComponentDataItemValueChanged>&> ────────

    /*!
     * \brief Connect a slot to receive value-changed notifications.
     */
    void connect(const std::shared_ptr<
        ISlot<const std::shared_ptr<IComponentDataItemValueChanged> &>> &slot) override;

    /*!
     * \brief Disconnect a previously connected value-changed slot.
     */
    void disconnect(const std::shared_ptr<
        ISlot<const std::shared_ptr<IComponentDataItemValueChanged> &>> &slot) override;

    /*!
     * \brief Suppress or restore both the propertyChanged and valueChanged signals.
     *
     * Overrides Description::blockSignals so that a single call gates
     * both the metadata-changed and data-changed signals.
     *
     * \param block  true to suppress; false to restore.
     */
    void blockSignals(bool block) override;

    // ── Comments (non-interface extension) ───────────────────────────────

    /*!
     * \brief Returns the list of free-form comment strings attached to this item.
     */
    [[nodiscard]] std::vector<std::string> comments() const;

    /*!
     * \brief Replaces all comments and fires propertyChanged("Comments").
     */
    void setComments(const std::vector<std::string> &comments);

    /*!
     * \brief Appends a single comment.
     */
    void addComment(const std::string &comment);

    // ── Serialization ─────────────────────────────────────────────────────

    /*!
     * \brief Deserializes caption, description, and comments from a JSON object.
     * \param j  JSON object (may be {}).
     */
    virtual void readData(const nlohmann::json &j);

    /*!
     * \brief Serializes caption, description, and comments to a JSON object.
     * \return JSON object containing at least "caption" and "comments".
     */
    [[nodiscard]] virtual nlohmann::json writeData() const;

protected:
    // ── Internal typed accessors (for subclasses) ─────────────────────────

    [[nodiscard]] std::vector<Dimension *>       dimensionsInternal()     const;
    [[nodiscard]] ValueDefinition               *valueDefinitionInternal() const;
    [[nodiscard]] HydroCouple::IModelComponent  *modelComponentInternal()  const;

    /*!
     * \brief Fires the valueChanged signal with the given event args.
     */
    void emitValueChanged(
        const std::shared_ptr<IComponentDataItemValueChanged> &args);

    /*!
     * \brief Convenience overload: constructs the event args inline.
     * \param start  First changed index in each dimension.
     * \param count  Changed extent in each dimension.
     */
    void emitValueChanged(std::vector<int64_t> start = {},
                          std::vector<int64_t> count = {});

    // ISignal<...>::emit — required pure-virtual override
    void emit(const std::shared_ptr<IComponentDataItemValueChanged> &args) override;

private:
    std::vector<Dimension *>          m_dimensions;
    ValueDefinition                  *m_valueDefinition;
    HydroCouple::IModelComponent     *m_modelComponent;
    std::vector<std::string>   m_comments;

    Signal<const std::shared_ptr<IComponentDataItemValueChanged> &> m_valueChanged;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTCOMPONENTDATAITEM_H
