// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   tincomponentdataitem.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Component data item for triangulated irregular networks (TIN).
 */

#ifndef HYDROCOUPLESDK_TINCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_TINCOMPONENTDATAITEM_H

#include "hydrocouplesdk.h"
#include "tin.h"

#include <vector>
#include <memory>

namespace HydroCouple::Spatial {

/*!
 * \brief Storage mixin for scalar values on a TIN mesh.
 *
 * Template provides typed storage indexed by mesh location:
 * - **Node data**: m_data[vertexIndex], size = numVertices
 * - **Edge data**: m_data[edgeIndex], size = numEdges
 * - **Cell data**: m_data[cellIndex], size = numTriangles
 *
 * Use as a mixin with AbstractComponentDataItem to create concrete data items.
 * Bulk operations are SIMD-vectorizable via contiguous memory access.
 *
 * \tparam T Data type (double, float, int, etc.)
 */
template <typename T>
class TINComponentDataItemStorage {
public:
    /*!
     * \brief Mesh data location type.
     */
    enum MeshDataType {
        Node,  //! Data at vertices
        Edge,  //! Data at edges
        Cell   //! Data at triangle cells
    };

    /*!
     * \brief Initialize storage for a TIN.
     * \param tin The TIN mesh (not owned)
     * \param dataType Location type (Node, Edge, or Cell)
     * \param capacity Pre-allocate storage
     */
    void initTINStorage(HCTIN *tin, MeshDataType dataType, int capacity = 0) {
        m_tin = tin;
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
     * \brief Get the TIN mesh.
     */
    HCTIN *tin() const { return m_tin; }

    /*!
     * \brief Get mesh data type.
     */
    MeshDataType meshDataType() const { return m_dataType; }

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
    HCTIN *m_tin = nullptr;
    MeshDataType m_dataType = Node;
    std::vector<T> m_data;

private:
    void updateDimensions() {
        if (!m_tin) return;

        int elementCount = 0;
        switch (m_dataType) {
            case Node:
                elementCount = m_tin->numVertices();
                break;
            case Edge:
                elementCount = m_tin->numEdges();
                break;
            case Cell:
                elementCount = m_tin->numTriangles();
                break;
        }

        if (elementCount > 0) {
            m_data.resize(elementCount, T{});
        }
    }
};

} // namespace HydroCouple::Spatial

#endif // HYDROCOUPLESDK_TINCOMPONENTDATAITEM_H
