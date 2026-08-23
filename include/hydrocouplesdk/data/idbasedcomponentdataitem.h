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

template<typename T>
class IdBasedComponentDataItem
{
public:
    explicit IdBasedComponentDataItem(const std::vector<std::string> &identifiers = {},
                                       const T &defaultValue = T{})
        : m_defaultValue(defaultValue)
    {
        m_identifiers = identifiers;
        m_values.assign(identifiers.size(), defaultValue);
        for (int i = 0; i < static_cast<int>(identifiers.size()); ++i)
            m_idIndex[identifiers[i]] = i;
    }

    virtual ~IdBasedComponentDataItem() = default;

    // ── Identifier queries ────────────────────────────────────────────────

    [[nodiscard]] std::vector<std::string> identifiers() const { return m_identifiers; }

    [[nodiscard]] int indexOf(const std::string &id) const
    {
        auto it = m_idIndex.find(id);
        return it != m_idIndex.end() ? it->second : -1;
    }

    [[nodiscard]] bool hasIdentifier(const std::string &id) const
    {
        return m_idIndex.count(id) > 0;
    }

    [[nodiscard]] int length() const noexcept { return (int)m_identifiers.size(); }

    // ── v2 typed hyperslab data plane ────────────────────────────────────

    /*!
     * \brief The DataKind of the stored element type.
     */
    [[nodiscard]] static constexpr DataKind storageKind()
    {
        return Helpers::DataKindOf<T>::kind;
    }

    /*!
     * \brief The item's shape: { identifier count }. The identifier
     *        dimension is dimension 0 per the v2 standard.
     */
    [[nodiscard]] std::vector<int64_t> storageShape() const
    {
        return {static_cast<int64_t>(m_identifiers.size())};
    }

    /*!
     * \brief Copies a hyperslab of the storage into \p destination
     *        (IComponentDataItem::getValuesInto semantics).
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

    T &operator[](int i)             { return m_values.at(i); }
    const T &operator[](int i) const { return m_values.at(i); }
    T &valueAt(const std::string &id)             { return m_values.at(m_idIndex.at(id)); }
    const T &valueAt(const std::string &id) const { return m_values.at(m_idIndex.at(id)); }

    [[nodiscard]] T defaultValue() const { return m_defaultValue; }
    void setDefaultValue(const T &v) { m_defaultValue = v; }

    void resetDataArray() { std::fill(m_values.begin(), m_values.end(), m_defaultValue); }

    // ── Raw / SIMD-eligible bulk access ──────────────────────────────────

    [[nodiscard]] const std::vector<T> &rawData() const noexcept { return m_values; }
    [[nodiscard]] std::vector<T>       &rawData()       noexcept { return m_values; }

    void bulkGet(int offset, int count, T *out) const noexcept
    {
        std::copy(m_values.begin() + offset, m_values.begin() + offset + count, out);
    }

    void bulkSet(int offset, int count, const T *in) noexcept
    {
        std::copy(in, in + count, m_values.begin() + offset);
    }

    // ── Identifier mutation ───────────────────────────────────────────────

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

    void addIdentifier(const std::string &id, const T &value = T{})
    {
        if (m_idIndex.count(id))
            throw std::invalid_argument("Duplicate id: " + id);
        m_idIndex[id] = (int)m_identifiers.size();
        m_identifiers.push_back(id);
        m_values.push_back(value);
    }

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

    void clearIdentifiers()
    {
        m_identifiers.clear();
        m_values.clear();
        m_idIndex.clear();
    }

private:
    std::vector<std::string>             m_identifiers;
    std::vector<T>                       m_values;
    std::unordered_map<std::string, int> m_idIndex;
    T                                    m_defaultValue;
};

} // namespace HydroCouple::SDK

#endif // HYDROCOUPLESDK_IDBASEDCOMPONENTDATAITEM_H
