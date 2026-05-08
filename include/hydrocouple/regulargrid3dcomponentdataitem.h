// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   regulargrid3dcomponentdataitem.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Component data item for 3D regular grids.
 */

#ifndef HYDROCOUPLESDK_REGULARGRID3DCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_REGULARGRID3DCOMPONENTDATAITEM_H

#include "hydrocouplesdk.h"
#include "abstractcomponentdataitem.h"
#include "regulargrid3d.h"

#include <vector>
#include <memory>

namespace HydroCouple::Spatial {

/*!
 * \brief Data item storing scalar values on a RegularGrid3D.
 *
 * Stores values in K-J-I order (layer-major, then row-major):
 * idx = k*(nx*(ny-1)) + j*nx + i
 *
 * Supports 3D indexing and bulk operations for efficient I/O.
 *
 * \tparam T Data type (double, float, int, etc.)
 */
template <typename T>
class HYDROCOUPLESDK_EXPORT RegularGrid3DComponentDataItem : public AbstractComponentDataItem {
public:
    /*!
     * \brief Grid data location type.
     */
    enum GridDataType {
        CellCentered,  //! Data at cell centers
        NodeCentered   //! Data at grid nodes
    };

    /*!
     * \brief Construct component data item for a RegularGrid3D.
     * \param grid The grid (not owned)
     * \param dataType Location type
     * \param capacity Pre-allocate storage
     */
    RegularGrid3DComponentDataItem(std::shared_ptr<RegularGrid3D> grid,
                                   GridDataType dataType, int capacity = 0)
        : AbstractComponentDataItem(), m_grid(grid), m_dataType(dataType) {
        updateDimensions();
        if (capacity > 0) {
            m_data.reserve(capacity);
        }
    }

    ~RegularGrid3DComponentDataItem() override = default;

    // ── AbstractComponentDataItem interface ────────────────────────────────

    int dimensionCount() const override {
        return 3;  // 3D: X, Y, and Z dimensions
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
        if (indices.size() < 1) return;

        auto it = indices.begin();
        int startIdx = *it;
        int count = (indices.size() > 1) ? *(++it) : m_data.size() - startIdx;

        if (startIdx < 0 || startIdx + count > (int)m_data.size()) return;

        data.resize(count);
        for (int i = 0; i < count; ++i) {
            data[i] = hydrocouple_variant(m_data[startIdx + i]);
        }
    }

    void setValue(const std::initializer_list<int> &indices,
                  const hydrocouple_variant &data) override {
        if (indices.size() != 1) return;
        int idx = *indices.begin();
        if (idx < 0 || idx >= (int)m_data.size()) return;

        if constexpr (std::is_arithmetic_v<T>) {
            if (data.index() == 1) {
                m_data[idx] = static_cast<T>(std::get<double>(data));
            } else if (data.index() == 2) {
                m_data[idx] = static_cast<T>(std::get<int>(data));
            }
        }
    }

    void setValues(const std::initializer_list<int> &indices,
                   const std::vector<hydrocouple_variant> &data) override {
        if (indices.size() < 1 || data.empty()) return;
        auto it = indices.begin();
        int startIdx = *it;
        int count = (indices.size() > 1) ? *(++it) : (int)data.size();

        if (startIdx < 0 || startIdx + count > (int)m_data.size()) return;

        if constexpr (std::is_arithmetic_v<T>) {
            for (int i = 0; i < count; ++i) {
                if (data[i].index() == 1) {
                    m_data[startIdx + i] = static_cast<T>(std::get<double>(data[i]));
                } else if (data[i].index() == 2) {
                    m_data[startIdx + i] = static_cast<T>(std::get<int>(data[i]));
                }
            }
        }
    }

    // ── Grid-specific data access ──────────────────────────────────────────

    /*!
     * \brief Get the grid.
     */
    std::shared_ptr<RegularGrid3D> grid() const { return m_grid; }

    /*!
     * \brief Get grid data type.
     */
    GridDataType gridDataType() const { return m_dataType; }

    /*!
     * \brief Direct access to underlying data array (K-J-I order).
     */
    std::vector<T> &data() { return m_data; }
    const std::vector<T> &data() const { return m_data; }

    /*!
     * \brief Get value at grid position (i, j, k).
     */
    T getValue(int i, int j, int k) const {
        int nx = (m_dataType == CellCentered) ? (m_grid->numXNodes() - 1) : m_grid->numXNodes();
        int ny = (m_dataType == CellCentered) ? (m_grid->numYNodes() - 1) : m_grid->numYNodes();
        int idx = k * (nx * ny) + j * nx + i;
        if (idx < 0 || idx >= (int)m_data.size()) return T{};
        return m_data[idx];
    }

    /*!
     * \brief Set value at grid position (i, j, k).
     */
    void setValue(int i, int j, int k, T value) {
        int nx = (m_dataType == CellCentered) ? (m_grid->numXNodes() - 1) : m_grid->numXNodes();
        int ny = (m_dataType == CellCentered) ? (m_grid->numYNodes() - 1) : m_grid->numYNodes();
        int idx = k * (nx * ny) + j * nx + i;
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

private:
    std::shared_ptr<RegularGrid3D> m_grid;
    GridDataType m_dataType;
    std::vector<T> m_data;

    void updateDimensions() {
        int xCount, yCount, zCount;

        if (m_dataType == CellCentered) {
            xCount = m_grid->numXNodes() - 1;
            yCount = m_grid->numYNodes() - 1;
            zCount = m_grid->numZNodes() - 1;
        } else {
            xCount = m_grid->numXNodes();
            yCount = m_grid->numYNodes();
            zCount = m_grid->numZNodes();
        }

        if (xCount > 0 && yCount > 0 && zCount > 0) {
            m_data.resize(xCount * yCount * zCount, T{});
        }
    }
};

} // namespace HydroCouple::Spatial

#endif // HYDROCOUPLESDK_REGULARGRID3DCOMPONENTDATAITEM_H
