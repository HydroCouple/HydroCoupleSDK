// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   networkcomponentdataitem.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Component data item for directed networks.
 */

#ifndef HYDROCOUPLESDK_NETWORKCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_NETWORKCOMPONENTDATAITEM_H

#include "hydrocouplesdk.h"
#include "abstractcomponentdataitem.h"
#include "network.h"

#include <vector>
#include <memory>

namespace HydroCouple::Spatial {

/*!
 * \brief Data item storing scalar values on a Network.
 *
 * Template stores values indexed by:
 * - **Node data**: m_data[vertexIndex], size = numVertices
 * - **Edge data**: m_data[edgeIndex], size = numEdges
 *
 * Useful for flow values on edges, concentrations at nodes, etc.
 *
 * \tparam T Data type (double, float, int, etc.)
 */
template <typename T>
class HYDROCOUPLESDK_EXPORT NetworkComponentDataItem : public AbstractComponentDataItem {
public:
    /*!
     * \brief Network data location type.
     */
    enum NetworkDataType {
        Node,  //! Data at vertices
        Edge   //! Data at edges
    };

    /*!
     * \brief Construct component data item for a Network.
     * \param network The Network (not owned)
     * \param dataType Location type (Node or Edge)
     * \param capacity Pre-allocate storage
     */
    NetworkComponentDataItem(Network *network, NetworkDataType dataType, int capacity = 0)
        : AbstractComponentDataItem(), m_network(network), m_dataType(dataType) {
        updateDimensions();
        if (capacity > 0) {
            m_data.reserve(capacity);
        }
    }

    ~NetworkComponentDataItem() override = default;

    // ── AbstractComponentDataItem interface ────────────────────────────────

    int dimensionCount() const override {
        return 1;
    }

    HydroCouple::IDimension *dimension(int index) const override {
        return nullptr;  // TODO: implement proper Dimension in C++20 namespace
    }

    void getValue(const std::initializer_list<int> &indices, hydrocouple_variant &data) const override {
        if (indices.size() != 1) return;
        int idx = *indices.begin();
        if (idx < 0 || idx >= (int)m_data.size()) return;
        data = hydrocouple_variant(m_data[idx]);
    }

    void getValues(const std::initializer_list<int> &indices,
                   std::vector<hydrocouple_variant> &data) const override {
        data.clear();
        if (indices.size() != 1) return;

        int idx = *indices.begin();
        int count = dimension(0)->dimensionLength();
        if (idx < 0 || idx + count > (int)m_data.size()) return;

        data.resize(count);
        for (int i = 0; i < count; ++i) {
            data[i] = hydrocouple_variant(m_data[idx + i]);
        }
    }

    void setValue(const std::initializer_list<int> &indices,
                  const hydrocouple_variant &data) override {
        if (indices.size() != 1) return;
        int idx = *indices.begin();
        if (idx < 0 || idx >= (int)m_data.size()) return;

        if constexpr (std::is_arithmetic_v<T>) {
            if (data.index() == 1) {  // double
                m_data[idx] = static_cast<T>(std::get<double>(data));
            } else if (data.index() == 2) {  // int
                m_data[idx] = static_cast<T>(std::get<int>(data));
            }
        }
    }

    void setValues(const std::initializer_list<int> &indices,
                   const std::vector<hydrocouple_variant> &data) override {
        if (indices.size() != 1 || data.empty()) return;
        int idx = *indices.begin();
        int count = std::min((int)data.size(), dimension(0)->dimensionLength());

        if (idx < 0 || idx + count > (int)m_data.size()) return;

        if constexpr (std::is_arithmetic_v<T>) {
            for (int i = 0; i < count; ++i) {
                if (data[i].index() == 1) {
                    m_data[idx + i] = static_cast<T>(std::get<double>(data[i]));
                } else if (data[i].index() == 2) {
                    m_data[idx + i] = static_cast<T>(std::get<int>(data[i]));
                }
            }
        }
    }

    // ── Network-specific data access ───────────────────────────────────────

    /*!
     * \brief Get the Network.
     */
    Network *network() const { return m_network; }

    /*!
     * \brief Get network data type.
     */
    NetworkDataType networkDataType() const { return m_dataType; }

    /*!
     * \brief Direct access to underlying data array.
     */
    std::vector<T> &data() { return m_data; }
    const std::vector<T> &data() const { return m_data; }

    /*!
     * \brief Bulk read operation.
     */
    void bulkGet(int offset, int count, T *out) const {
        if (offset < 0 || offset + count > (int)m_data.size()) return;
        std::copy(m_data.begin() + offset, m_data.begin() + offset + count, out);
    }

    /*!
     * \brief Bulk write operation.
     */
    void bulkSet(int offset, int count, const T *in) {
        if (offset < 0 || offset + count > (int)m_data.size()) return;
        std::copy(in, in + count, m_data.begin() + offset);
    }

private:
    Network *m_network;
    NetworkDataType m_dataType;
    std::vector<T> m_data;

    void updateDimensions() {
        int elementCount = 0;

        switch (m_dataType) {
            case Node:
                elementCount = m_network->vertexCount();
                break;
            case Edge:
                elementCount = m_network->edgeCount();
                break;
        }

        if (elementCount > 0) {
            m_data.resize(elementCount, T{});
        }
    }
};

} // namespace HydroCouple::Spatial

#endif // HYDROCOUPLESDK_NETWORKCOMPONENTDATAITEM_H
