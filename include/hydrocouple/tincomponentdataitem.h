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
#include "abstractcomponentdataitem.h"
#include "tin.h"

#include <vector>
#include <memory>
#include <cstring>

namespace HydroCouple::Spatial {

/*!
 * \brief Data item storing scalar values on a TIN mesh.
 *
 * Template stores values indexed by mesh location:
 * - **Node data**: m_data[vertexIndex], size = numVertices
 * - **Edge data**: m_data[edgeIndex], size = numEdges
 * - **Cell data**: m_data[cellIndex], size = numTriangles
 *
 * All getValue/getValues calls use flat array indexing.
 * Bulk operations are SIMD-vectorizable via contiguous memory access.
 *
 * \tparam T Data type (double, float, int, etc.)
 */
template <typename T>
class HYDROCOUPLESDK_EXPORT TINComponentDataItem : public AbstractComponentDataItem {
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
     * \brief Construct component data item for a TIN.
     * \param tin The TIN mesh (not owned)
     * \param dataType Location type (Node, Edge, or Cell)
     * \param capacity Pre-allocate storage
     */
    TINComponentDataItem(HCTIN *tin, MeshDataType dataType, int capacity = 0)
        : AbstractComponentDataItem(), m_tin(tin), m_dataType(dataType) {
        updateDimensions();
        if (capacity > 0) {
            m_data.reserve(capacity);
        }
    }

    ~TINComponentDataItem() override = default;

    // ── AbstractComponentDataItem interface ────────────────────────────────

    int dimensionCount() const override {
        return 1;  // Single dimension: element index
    }

    HydroCouple::IDimension *dimension(int index) const override {
        // TODO: implement proper Dimension class in C++20 namespace
        return nullptr;
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

    // ── TIN-specific data access ───────────────────────────────────────────

    /*!
     * \brief Get the TIN mesh.
     */
    HCTIN *tin() const { return m_tin; }

    /*!
     * \brief Get mesh data type.
     */
    MeshDataType meshDataType() const { return m_dataType; }

    /*!
     * \brief Direct access to underlying data array.
     */
    std::vector<T> &data() { return m_data; }
    const std::vector<T> &data() const { return m_data; }

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

private:
    HCTIN *m_tin;
    MeshDataType m_dataType;
    std::vector<T> m_data;

    void updateDimensions() {
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
