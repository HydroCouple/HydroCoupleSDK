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
#include "network.h"

#include <vector>
#include <memory>

namespace HydroCouple::Spatial {

/*!
 * \brief Storage mixin for scalar values on a Network.
 *
 * Template stores values indexed by:
 * - **Node data**: m_data[vertexIndex], size = numVertices
 * - **Edge data**: m_data[edgeIndex], size = numEdges
 *
 * Use as a mixin with AbstractComponentDataItem to create concrete data items.
 *
 * \tparam T Data type (double, float, int, etc.)
 */
template <typename T>
class NetworkComponentDataItemStorage {
public:
    /*!
     * \brief Network data location type.
     */
    enum NetworkDataType {
        Node,  //! Data at vertices
        Edge   //! Data at edges
    };

    /*!
     * \brief Initialize storage for a Network.
     * \param network The Network (not owned)
     * \param dataType Location type (Node or Edge)
     * \param capacity Pre-allocate storage
     */
    void initNetworkStorage(Network *network, NetworkDataType dataType, int capacity = 0) {
        m_network = network;
        m_dataType = dataType;
        if (capacity > 0) {
            m_data.reserve(capacity);
        }
        updateDimensions();
    }

    // ── Storage accessors ──────────────────────────────────────────────────

    /*!
     * \brief Direct access to underlying data array.
     */
    std::vector<T> &data() { return m_data; }
    const std::vector<T> &data() const { return m_data; }

    /*!
     * \brief Get the Network.
     */
    Network *network() const { return m_network; }

    /*!
     * \brief Get network data type.
     */
    NetworkDataType networkDataType() const { return m_dataType; }

    /*!
     * \brief Get value at flat index.
     */
    T getValue(int idx) const {
        if (idx < 0 || idx >= (int)m_data.size()) return T{};
        return m_data[idx];
    }

    /*!
     * \brief Set value at flat index.
     */
    void setValue(int idx, T value) {
        if (idx < 0 || idx >= (int)m_data.size()) return;
        m_data[idx] = value;
    }

    /*!
     * \brief Bulk read operation (SIMD-vectorizable).
     */
    void bulkGet(int offset, int count, T *out) const {
        if (offset < 0 || offset + count > (int)m_data.size()) return;
        std::copy(m_data.begin() + offset, m_data.begin() + offset + count, out);
    }

    /*!
     * \brief Bulk write operation (SIMD-vectorizable).
     */
    void bulkSet(int offset, int count, const T *in) {
        if (offset < 0 || offset + count > (int)m_data.size()) return;
        std::copy(in, in + count, m_data.begin() + offset);
    }

protected:
    Network *m_network = nullptr;
    NetworkDataType m_dataType = Node;
    std::vector<T> m_data;

private:
    void updateDimensions() {
        if (!m_network) return;

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
