// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   idbasedcomponentdataitem.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Typed ID-keyed data-storage mixin for IIdBasedComponentDataItem.
 *
 * Stores a flat std::vector<T> indexed by position in a parallel
 * std::vector<std::string> identifier list. An unordered_map provides
 * O(1) lookup by id string. Concrete classes multiply-inherit from
 * AbstractComponentDataItem and IdBasedComponentDataItem<T>.
 *
 * Implements the v2 typed hyperslab data plane over the flat storage; the
 * identifier dimension is dimension 0 of the item's shape.
 */

#ifndef HYDROCOUPLESDK_IDBASEDCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_IDBASEDCOMPONENTDATAITEM_H

#include "hydrocouplesdk/export.h"
#include "hydrocouple.h"
#include "hydrocouplesdk/data/bufferio.h"

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace HydroCouple::SDK {

/*!
 * \brief Typed storage of one value per string identifier.
 *
 * \details A storage mixin like ComponentDataItem1D: it is not an
 * IComponentDataItem itself. IdBasedArgument, IdBasedInput and IdBasedOutput
 * combine it with an SDK base class and IIdBasedComponentDataItem. Position
 * i in the identifier list holds value i, and shape() is { count }. Lookup
 * by identifier goes through a hash map kept in step with the list.
 *
 * Identifiers are expected to be unique. addIdentifier() refuses a
 * duplicate; the constructor and setIdentifiers() do not check, and with a
 * duplicate the lookup map points at the last occurrence.
 *
 * Writes through operator[], valueAt(), rawData() and bulkSet() fire no
 * signal; the classes built on this mixin fire value-changed from
 * setValuesFrom() and their own readers.
 *
 * \tparam T Element type (int, double, std::string, ...).
 * \threadsafety Not thread-safe.
 */
template<typename T>
class IdBasedComponentDataItem
{
public:
    /*!
     * \brief Construct with identifiers, each value set to \p defaultValue.
     * \param[in] identifiers  Identifiers in dimension-0 order (copied).
     * \param[in] defaultValue Initial value of every slot and the fill value
     *                         for identifiers added later by setIdentifiers().
     */
    explicit IdBasedComponentDataItem(const std::vector<std::string> &identifiers = {},
                                       const T &defaultValue = T{})
        : m_defaultValue(defaultValue)
    {
        m_identifiers = identifiers;
        m_values.assign(identifiers.size(), defaultValue);
        for (int i = 0; i < static_cast<int>(identifiers.size()); ++i)
            m_idIndex[identifiers[i]] = i;
    }

    //! Destroys the storage.
    virtual ~IdBasedComponentDataItem() = default;

    // ── Identifier queries ────────────────────────────────────────────────

    //! The identifiers in dimension-0 order.
    //! \return A copy of the list.
    [[nodiscard]] std::vector<std::string> identifiers() const { return m_identifiers; }

    /*!
     * \brief Position of an identifier.
     * \param[in] id Identifier to find.
     * \return Its zero-based index, or -1 when absent.
     */
    [[nodiscard]] int indexOf(const std::string &id) const
    {
        auto it = m_idIndex.find(id);
        return it != m_idIndex.end() ? it->second : -1;
    }

    /*!
     * \brief Whether an identifier is present.
     * \param[in] id Identifier to find.
     * \return true when \p id has a slot.
     */
    [[nodiscard]] bool hasIdentifier(const std::string &id) const
    {
        return m_idIndex.count(id) > 0;
    }

    //! Number of identifiers (and values).
    //! \return The identifier count.
    [[nodiscard]] int length() const noexcept { return (int)m_identifiers.size(); }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The DataKind of the stored element type.
     * \return Helpers::DataKindOf of \c T.
     */
    [[nodiscard]] static constexpr DataKind storageKind()
    {
        return Helpers::DataKindOf<T>::kind;
    }

    /*!
     * \brief The item's shape: { identifier count }. The identifier
     *        dimension is dimension 0 per the v2 standard.
     * \return A one-element vector holding length().
     */
    [[nodiscard]] std::vector<int64_t> storageShape() const
    {
        return {static_cast<int64_t>(m_identifiers.size())};
    }

    /*!
     * \brief Copies a hyperslab of the storage into \p destination
     *        (IComponentDataItem::getValuesInto semantics).
     * \details The selection is by position: start {i} and count {n} read
     *          the values of identifiers i to i + n - 1.
     * \param[in]  destination Caller's host buffer of kind storageKind().
     * \param[in]  start       First position (one entry).
     * \param[in]  count       Number of values (one entry).
     * \param[out] message     Failure description; may be nullptr.
     * \return true when the values were copied.
     */
    [[nodiscard]] bool getSlab(const BufferDescriptor &destination,
                               std::span<const int64_t> start,
                               std::span<const int64_t> count,
                               std::string *message = nullptr) const
    {
        const int64_t shape0 = static_cast<int64_t>(m_identifiers.size());
        return detail::readHyperslab<T>(m_values.data(),
                                        std::span<const int64_t>(&shape0, 1),
                                        destination, start, count, message);
    }

    /*!
     * \brief Copies values from \p source into a hyperslab of the storage
     *        (IComponentDataItem::setValuesFrom semantics).
     * \details Fires nothing; the identifier list does not change.
     * \param[in]  source  Caller's host buffer of kind storageKind().
     * \param[in]  start   First position (one entry).
     * \param[in]  count   Number of values (one entry).
     * \param[out] message Failure description; may be nullptr.
     * \return true when the values were stored.
     */
    [[nodiscard]] bool setSlab(const BufferDescriptor &source,
                               std::span<const int64_t> start,
                               std::span<const int64_t> count,
                               std::string *message = nullptr)
    {
        const int64_t shape0 = static_cast<int64_t>(m_identifiers.size());
        return detail::writeHyperslab<T>(m_values.data(),
                                         std::span<const int64_t>(&shape0, 1),
                                         source, start, count, message);
    }

    // ── Direct typed access ───────────────────────────────────────────────

    //! Value at a position (bounds-checked).
    //! \param[in] i Zero-based position.
    //! \return A reference into the storage.
    //! \exception std::out_of_range when \p i is outside [0, length()).
    T &operator[](int i)             { return m_values.at(i); }
    //! Value at a position (bounds-checked, const).
    //! \param[in] i Zero-based position.
    //! \return A const reference into the storage.
    //! \exception std::out_of_range when \p i is outside [0, length()).
    const T &operator[](int i) const { return m_values.at(i); }
    //! Value of an identifier.
    //! \param[in] id Identifier.
    //! \return A reference into the storage.
    //! \exception std::out_of_range when \p id is absent.
    T &valueAt(const std::string &id)             { return m_values.at(m_idIndex.at(id)); }
    //! Value of an identifier (const).
    //! \param[in] id Identifier.
    //! \return A const reference into the storage.
    //! \exception std::out_of_range when \p id is absent.
    const T &valueAt(const std::string &id) const { return m_values.at(m_idIndex.at(id)); }

    //! The fill value for new identifiers and resetDataArray().
    //! \return A copy of the default value.
    [[nodiscard]] T defaultValue() const { return m_defaultValue; }
    //! Changes the default value; existing values are not changed.
    //! \param[in] v New default.
    void setDefaultValue(const T &v) { m_defaultValue = v; }

    //! Sets every value to the default.
    void resetDataArray() { std::fill(m_values.begin(), m_values.end(), m_defaultValue); }

    // ── Raw / SIMD-eligible bulk access ──────────────────────────────────

    //! The values in identifier order.
    //! \return A const reference to the vector.
    [[nodiscard]] const std::vector<T> &rawData() const noexcept { return m_values; }
    //! The values in identifier order, for direct writes (no signal).
    //! \return A mutable reference; do not change its size.
    [[nodiscard]] std::vector<T>       &rawData()       noexcept { return m_values; }

    /*!
     * \brief Copies \p count values starting at position \p offset into \p out.
     * \param[in]  offset First position; not bounds-checked.
     * \param[in]  count  Number of values.
     * \param[out] out    Buffer of at least \p count elements.
     */
    void bulkGet(int offset, int count, T *out) const noexcept
    {
        std::copy(m_values.begin() + offset, m_values.begin() + offset + count, out);
    }

    /*!
     * \brief Copies \p count values from \p in starting at position \p offset.
     * \param[in] offset First position; not bounds-checked.
     * \param[in] count  Number of values.
     * \param[in] in     Buffer of at least \p count elements.
     */
    void bulkSet(int offset, int count, const T *in) noexcept
    {
        std::copy(in, in + count, m_values.begin() + offset);
    }

    // ── Identifier mutation ───────────────────────────────────────────────

    /*!
     * \brief Replaces the identifier list, keeping the values of
     *        identifiers that remain.
     * \details An identifier in both lists keeps its value at its new
     *          position; a new one gets the default value. Duplicates are
     *          not checked.
     * \param[in] identifiers The new list, in dimension-0 order.
     */
    void setIdentifiers(const std::vector<std::string> &identifiers)
    {
        std::vector<T> newValues(identifiers.size(), m_defaultValue);
        std::unordered_map<std::string, int> newIndex;
        for (int i = 0; i < (int)identifiers.size(); ++i) {
            newIndex[identifiers[i]] = i;
            auto it = m_idIndex.find(identifiers[i]);
            if (it != m_idIndex.end())
                newValues[i] = m_values[it->second];
        }
        m_identifiers = identifiers;
        m_values      = std::move(newValues);
        m_idIndex     = std::move(newIndex);
    }

    /*!
     * \brief Appends an identifier with its value.
     * \param[in] id    New identifier.
     * \param[in] value Its value (\c T{} by default, not defaultValue()).
     * \exception std::invalid_argument ("Duplicate id: " and the id) when \p id is
     *            already present.
     */
    void addIdentifier(const std::string &id, const T &value = T{})
    {
        if (m_idIndex.count(id))
            throw std::invalid_argument("Duplicate id: " + id);
        m_idIndex[id] = (int)m_identifiers.size();
        m_identifiers.push_back(id);
        m_values.push_back(value);
    }

    /*!
     * \brief Removes an identifier and its value; later positions shift down.
     * \param[in] id Identifier to remove.
     * \return false when \p id is absent.
     */
    bool removeIdentifier(const std::string &id)
    {
        auto it = m_idIndex.find(id);
        if (it == m_idIndex.end()) return false;
        int idx = it->second;
        m_identifiers.erase(m_identifiers.begin() + idx);
        m_values.erase(m_values.begin() + idx);
        m_idIndex.erase(it);
        // Re-index entries after the removed one
        for (auto &[k, v] : m_idIndex)
            if (v > idx) --v;
        return true;
    }

    //! Removes every identifier and value.
    void clearIdentifiers()
    {
        m_identifiers.clear();
        m_values.clear();
        m_idIndex.clear();
    }

private:
    std::vector<std::string>             m_identifiers;  //!< Identifiers in dimension-0 order.
    std::vector<T>                       m_values;       //!< One value per identifier, same order.
    std::unordered_map<std::string, int> m_idIndex;      //!< Identifier to position.
    T                                    m_defaultValue; //!< Fill value for new identifiers.
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_IDBASEDCOMPONENTDATAITEM_H
