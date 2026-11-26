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
 *
 * \details AbstractComponentDataItem::emitValueChanged() builds one of these
 * for every value-changed notification and hands it to the slots as a
 * \c std::shared_ptr. The \c start and \c count vectors are copies taken when
 * the event is built and have one entry per dimension of the item's shape()
 * when the emitter knows the region (setValuesFrom() reports exactly the box
 * it wrote). Both are empty when the emitter does not say which values
 * changed (AbstractExchangeItem::emitMessage() emits that form); a slot must
 * then treat the whole item as changed.
 *
 * \threadsafety Immutable after construction.
 */
class HYDROCOUPLESDK_EXPORT ComponentDataItemValueChanged
    : public HydroCouple::IComponentDataItemValueChanged
{
public:
    /*!
     * \brief Builds the event for \p item and the changed box.
     * \param[in] item   The item whose values changed; borrowed, not owned,
     *                   and may be nullptr (the event then names no item).
     * \param[in] start  First changed index in each dimension; empty when the
     *                   region is not known.
     * \param[in] count  Changed extent in each dimension, parallel to
     *                   \p start; empty when the region is not known.
     */
    ComponentDataItemValueChanged(HydroCouple::IComponentDataItem *item,
                                  std::vector<int64_t> start = {},
                                  std::vector<int64_t> count = {});

    //! Destroys the event; the item it names is not touched.
    ~ComponentDataItemValueChanged() override = default;

    /*!
     * \brief The item whose values changed.
     * \return The pointer given at construction (non-owning; may be nullptr).
     *         It is valid only while that item lives, so a slot that keeps
     *         the event must not use it after the item is destroyed.
     */
    HydroCouple::IComponentDataItem *componentDataItem() const override;

    /*!
     * \brief First changed index in each dimension.
     * \return A copy of the start vector; empty when the region is not known.
     */
    std::vector<int64_t> start() const override;

    /*!
     * \brief Changed extent in each dimension.
     * \return A copy of the count vector, parallel to start(); empty when the
     *         region is not known.
     */
    std::vector<int64_t> count() const override;

private:
    HydroCouple::IComponentDataItem *m_item;  //!< Item that changed (borrowed).
    std::vector<int64_t>             m_start; //!< First changed index per dimension.
    std::vector<int64_t>             m_count; //!< Changed extent per dimension.
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
 * Ownership
 * ---------
 * Nothing passed to the constructors is owned. The Dimension objects, the
 * ValueDefinition and the model component must outlive the item; typical
 * owners are the component (for its own items) or the item subclass itself
 * (IdBasedInput keeps its identifier dimension as a member). A Quantity does
 * not own its Unit either, so whoever creates the value definition keeps both.
 *
 * Typical use: a component author does not derive from this class directly
 * but from AbstractArgument, AbstractInput or AbstractOutput, or uses one of
 * the ready typed items (Argument1D, Input1D, Output1D, the 2-D and id-based
 * variants and the time-series items), which add storage and implement the
 * five data-plane members.
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
     * \details An empty \p id is replaced by a generated UUID (Identity).
     *          The caption starts empty. The dimension list is copied; the
     *          pointers in it are not checked and may include nullptr, which
     *          dimensions() then returns as given. Each axis of shape() must
     *          have one entry, in storage order, for a consumer to read the
     *          dimension roles correctly.
     */
    AbstractComponentDataItem(std::string_view id,
                               const std::vector<Dimension *> &dimensions,
                               ValueDefinition *valueDefinition,
                               HydroCouple::IModelComponent *modelComponent);

    /*!
     * \brief Construct with identifier, caption, dimensions, value definition,
     *        and owning model component.
     * \param id              Identifier, unique within the owning component.
     * \param caption         Human-readable caption.
     * \param dimensions      Ordered list of Dimension pointers (borrowed,
     *                        copied as a list).
     * \param valueDefinition Value definition (borrowed; may be nullptr).
     * \param modelComponent  Owning component (borrowed; may be nullptr).
     */
    AbstractComponentDataItem(std::string_view id,
                               std::string_view caption,
                               const std::vector<Dimension *> &dimensions,
                               ValueDefinition *valueDefinition,
                               HydroCouple::IModelComponent *modelComponent);

    //! Destroys the item; dimensions, value definition and component are not touched.
    ~AbstractComponentDataItem() override = default;

    // ── IComponentDataItem ────────────────────────────────────────────────

    /*!
     * \brief Returns the owning model component (may be nullptr).
     * \return The component given at construction (non-owning). For an
     *         output it is the component that produces the values; for an
     *         adapted output it is the adaptee's component.
     */
    [[nodiscard]] HydroCouple::IModelComponent *modelComponent() const override;

    /*!
     * \brief Returns the list of dimensions as IIDimension pointers.
     * \return The constructor's dimension list, in order, as interface
     *         pointers (non-owning observers valid while the dimensions
     *         live). An item built with an empty list returns an empty
     *         vector even when its shape() has a rank.
     */
    [[nodiscard]] std::vector<HydroCouple::IDimension *> dimensions() const override;

    /*!
     * \brief Returns the value definition for this data item.
     * \return The value definition given at construction (non-owning); a
     *         Quantity for numbers with units, a Quality for categories.
     *         nullptr when the item was built without one, as the SDK's
     *         adapted outputs are.
     */
    [[nodiscard]] HydroCouple::IValueDefinition *valueDefinition() const override;

    // ── v2 typed hyperslab data plane — pure virtual ──────────────────────

    /*!
     * \brief The extent of each dimension of the item's index space.
     * Pure virtual — implemented by typed subclasses (typically by
     * delegating to their storage mixin's storageShape()).
     * \return One extent per dimension, in storage order. A dynamic
     *         dimension reports its current extent, so the result can change
     *         between calls (a time series grows by one row per time step).
     */
    [[nodiscard]] std::vector<int64_t> shape() const override = 0;

    /*!
     * \brief The element type of the item's values.
     * Pure virtual — implemented by typed subclasses.
     * \return The DataKind every buffer passed to getValuesInto() and
     *         setValuesFrom() must carry; the typed SDK items return
     *         Helpers::DataKindOf of their element type (Int32 for
     *         \c int, Float64 for \c double, String for \c std::string).
     */
    [[nodiscard]] DataKind dataKind() const override = 0;

    /*!
     * \brief Copies a hyperslab of this item's values into \p destination.
     * Pure virtual — implemented by typed subclasses.
     * \details The selection is the box from \p start to
     *          \p start + \p count in every dimension of shape(). The SDK's
     *          typed items implement the contract through
     *          detail::readHyperslab(), which refuses, with these messages:
     *          a kind other than dataKind() ("kind mismatch"); a memory space
     *          other than Host ("unsupported memory space"); a start or count
     *          whose length differs from the rank ("selection rank
     *          mismatch"); a negative start or count, or a box past the end
     *          of a dimension ("selection out of bounds"); a destination
     *          whose element count differs from the product of \p count
     *          ("buffer element count mismatch"). A failed read leaves the
     *          destination untouched. The SDK items do not queue an error on
     *          the owning component when a read fails.
     * \param[in]  destination Describes the caller's buffer: its kind, shape
     *                         (any rank, as long as the element count
     *                         matches), optional byte strides, and host
     *                         memory. The caller owns the memory.
     * \param[in]  start       First index of the selection in each dimension;
     *                         one entry per dimension of shape().
     * \param[in]  count       Extent of the selection in each dimension; one
     *                         entry per dimension of shape(). A zero entry
     *                         selects nothing and succeeds.
     * \param[out] message     Receives the reason when the call returns false;
     *                         may be nullptr. Left unchanged on success.
     * \return true when the values were copied.
     */
    [[nodiscard]] bool getValuesInto(const BufferDescriptor &destination,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) const override = 0;

    /*!
     * \brief Copies values from \p source into a hyperslab of this item.
     * Pure virtual — implemented by typed subclasses.
     * \details The selection and compatibility rules, and the failure
     *          messages, are those of getValuesInto(). The SDK's writable
     *          items fire the value-changed signal with \p start and \p count
     *          after a successful write and do not fire it on failure.
     *          Adapted outputs and other derived views refuse every write.
     * \param[in]  source  Describes the caller's buffer holding the values;
     *                     the caller owns the memory.
     * \param[in]  start   First index of the selection in each dimension.
     * \param[in]  count   Extent of the selection in each dimension.
     * \param[out] message Receives the reason when the call returns false;
     *                     may be nullptr.
     * \return true when the values were stored.
     */
    [[nodiscard]] bool setValuesFrom(const BufferDescriptor &source,
                                     std::span<const int64_t> start,
                                     std::span<const int64_t> count,
                                     std::string *message = nullptr) override = 0;

    // ── ISignal<const shared_ptr<IComponentDataItemValueChanged>&> ────────

    /*!
     * \brief Connect a slot to receive value-changed notifications.
     * \details The signal holds the slot through a \c std::weak_ptr: the
     *          caller keeps the \c shared_ptr alive for as long as it wants
     *          notifications, and a slot whose last owner releases it is
     *          dropped at the next emission. Connecting the same slot twice
     *          has no further effect; a null slot is ignored. The sender a
     *          slot receives is the item's internal signal object, not the
     *          item, so slots identify the item through
     *          IComponentDataItemValueChanged::componentDataItem().
     * \param[in] slot Slot to call on every value change; may be null.
     */
    void connect(const std::shared_ptr<
        ISlot<const std::shared_ptr<IComponentDataItemValueChanged> &>> &slot) override;

    /*!
     * \brief Disconnect a previously connected value-changed slot.
     * \details A null slot, or one that is not connected, is ignored. Expired
     *          slots are removed at the same time.
     * \param[in] slot Slot to remove.
     */
    void disconnect(const std::shared_ptr<
        ISlot<const std::shared_ptr<IComponentDataItemValueChanged> &>> &slot) override;

    /*!
     * \brief Suppress or restore both the propertyChanged and valueChanged signals.
     *
     * Overrides Description::blockSignals so that a single call gates
     * both the metadata-changed and data-changed signals.
     *
     * \details While blocked, values and properties still change; the
     *          notifications are dropped, not queued.
     * \param block  true to suppress; false to restore.
     */
    void blockSignals(bool block) override;

    // ── Comments (non-interface extension) ───────────────────────────────

    /*!
     * \brief Returns the list of free-form comment strings attached to this item.
     * \return A copy of the comments, in the order they were added.
     */
    [[nodiscard]] std::vector<std::string> comments() const;

    /*!
     * \brief Replaces all comments and fires propertyChanged("Comments").
     * \param[in] comments The new comment list (copied).
     */
    void setComments(const std::vector<std::string> &comments);

    /*!
     * \brief Appends a single comment.
     * \details Fires propertyChanged("Comments").
     * \param[in] comment Comment text to append (copied).
     */
    void addComment(const std::string &comment);

    // ── Serialization ─────────────────────────────────────────────────────

    /*!
     * \brief Deserializes caption, description, and comments from a JSON object.
     * \details Each of the keys "caption", "description" and "comments" is
     *          applied only when present; absent keys leave the current value.
     *          Setting the caption or description fires propertyChanged;
     *          replacing the comments does not. A key holding the wrong JSON
     *          type throws nlohmann::json::type_error. Subclasses call this
     *          first and then read their values.
     * \param j  JSON object (may be {}).
     */
    virtual void readData(const nlohmann::json &j);

    /*!
     * \brief Serializes caption, description, and comments to a JSON object.
     * \return JSON object containing at least "caption" and "comments".
     *         The base writes "caption", "description" and "comments" (an
     *         array, possibly empty); subclasses add their values.
     */
    [[nodiscard]] virtual nlohmann::json writeData() const;

protected:
    // ── Internal typed accessors (for subclasses) ─────────────────────────

    //! The dimensions as SDK Dimension pointers (a copy of the list; non-owning).
    //! \return The constructor's dimension list.
    [[nodiscard]] std::vector<Dimension *>       dimensionsInternal()     const;
    //! The value definition as the SDK type (non-owning; may be nullptr).
    //! \return The constructor's value definition.
    [[nodiscard]] ValueDefinition               *valueDefinitionInternal() const;
    //! The owning component (non-owning; may be nullptr); same as modelComponent().
    //! \return The constructor's model component.
    [[nodiscard]] HydroCouple::IModelComponent  *modelComponentInternal()  const;

    /*!
     * \brief Fires the valueChanged signal with the given event args.
     * \details Calls every connected slot on the calling thread, unless
     *          signals are blocked.
     * \param[in] args Event to hand to every slot; shared with them.
     */
    void emitValueChanged(
        const std::shared_ptr<IComponentDataItemValueChanged> &args);

    /*!
     * \brief Convenience overload: constructs the event args inline.
     * \details Builds a ComponentDataItemValueChanged naming this item and
     *          fires it. Typed subclasses call it after every successful
     *          write with the written box.
     * \param start  First changed index in each dimension.
     * \param count  Changed extent in each dimension.
     */
    void emitValueChanged(std::vector<int64_t> start = {},
                          std::vector<int64_t> count = {});

    /*!
     * \brief Implements ISignal::emit() for the value-changed signal.
     * \details Required by the interface; the same as
     *          emitValueChanged(args). Subclasses call emitValueChanged().
     * \param[in] args Event to hand to every slot.
     */
    void emit(const std::shared_ptr<IComponentDataItemValueChanged> &args) override;

private:
    std::vector<Dimension *>          m_dimensions;      //!< Axes in storage order (borrowed).
    ValueDefinition                  *m_valueDefinition; //!< Value definition (borrowed; may be nullptr).
    HydroCouple::IModelComponent     *m_modelComponent;  //!< Owning component (borrowed; may be nullptr).
    std::vector<std::string>   m_comments;               //!< Free-form comments, in order added.

    //! The value-changed signal behind connect(), disconnect() and emitValueChanged().
    Signal<const std::shared_ptr<IComponentDataItemValueChanged> &> m_valueChanged;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_ABSTRACTCOMPONENTDATAITEM_H
