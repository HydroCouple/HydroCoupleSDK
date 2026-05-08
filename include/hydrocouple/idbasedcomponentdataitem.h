// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   idbasedcomponentdataitem.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Typed ID-keyed data-storage mixin for IIdBasedComponentDataItem.
 *
 * Stores a flat std::vector<T> indexed by position in a parallel
 * std::vector<std::string> identifier list. An unordered_map provides
 * O(1) lookup by id string. Concrete classes multiply-inherit from
 * AbstractComponentDataItem and IdBasedComponentDataItem<T>.
 */

#ifndef HYDROCOUPLESDK_IDBASEDCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_IDBASEDCOMPONENTDATAITEM_H

#include "hydrocouplesdk.h"
#include "hydrocouple.h"

#include <algorithm>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace HydroCouple {

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

    // ── Typed get/set by integer index (hydrocouple_variant bridging) ─────

    void getIdValueT(hydrocouple_variant &data, int idIndex,
                      const std::initializer_list<int> & = {}) const
    {
        data = m_values.at(idIndex);
    }

    void getIdValuesT(hydrocouple_variant *data, int idIndex, int count = 1,
                       const std::initializer_list<int> & = {}) const
    {
        for (int i = 0; i < count; ++i)
            data[i] = m_values.at(idIndex + i);
    }

    void setIdValueT(const hydrocouple_variant &data, int idIndex,
                      const std::initializer_list<int> & = {})
    {
        m_values.at(idIndex) = std::get<T>(data);
    }

    void setIdValuesT(const hydrocouple_variant *data, int idIndex, int count = 1,
                       const std::initializer_list<int> & = {})
    {
        for (int i = 0; i < count; ++i)
            m_values.at(idIndex + i) = std::get<T>(data[i]);
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

} // namespace HydroCouple

#endif // HYDROCOUPLESDK_IDBASEDCOMPONENTDATAITEM_H
