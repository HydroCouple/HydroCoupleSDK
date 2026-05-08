// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   regulargrid2dcomponentdataitem.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  Component data item for 2D regular grids.
 */

#ifndef HYDROCOUPLESDK_REGULARGRID2DCOMPONENTDATAITEM_H
#define HYDROCOUPLESDK_REGULARGRID2DCOMPONENTDATAITEM_H

#include "hydrocouplesdk.h"
#include "abstractcomponentdataitem.h"
#include "regulargrid2d.h"

#include <vector>
#include <memory>

namespace HydroCouple::Spatial {

/*!
 * \brief Data item storing scalar values on a RegularGrid2D.
 *
 * Stores values in row-major layout:
 * - **Cell data**: m_data[j*(nx-1) + i], size = (nx-1) × (ny-1)
 * - **Node data**: m_data[j*nx + i], size = nx × ny
 *
 * Supports 2D indexing via getValue(j*nx + i) or direct (i,j) access.
 * Contiguous memory layout enables bulk SIMD operations.
 *
 * \tparam T Data type (double, float, int, etc.)
 */
template <typename T>
class HYDROCOUPLESDK_EXPORT RegularGrid2DComponentDataItem : public AbstractComponentDataItem {
public:
    /*!
     * \brief Grid data location type.
     */
    enum GridDataType {
        CellCentered,  //! Data at cell centers
        NodeCentered   //! Data at grid nodes
    };

    /*!
     * \brief Construct component data item for a RegularGrid2D.
     * \param grid The grid (not owned)
     * \param dataType Location type (CellCentered or NodeCentered)
     * \param capacity Pre-allocate storage
     */
    RegularGrid2DComponentDataItem(std::shared_ptr<RegularGrid2D> grid,
                                   GridDataType dataType, int capacity = 0)
        : AbstractComponentDataItem(), m_grid(grid), m_dataType(dataType) {
        updateDimensions();
        if (capacity > 0) {
            m_data.reserve(capacity);
        }
    }

    ~RegularGrid2DComponentDataItem() override = default;

    // ── AbstractComponentDataItem interface ────────────────────────────────

    int dimensionCount() const override {
        return 2;  // 2D: X and Y dimensions
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
        if (indices.size() != 2) return;

        auto it = indices.begin();
        int startIdx = *it;
        ++it;
        int count = *it;

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
            if (data.index() == 1) {  // double
                m_data[idx] = static_cast<T>(std::get<double>(data));
            } else if (data.index() == 2) {  // int
                m_data[idx] = static_cast<T>(std::get<int>(data));
            }
        }
    }

    void setValues(const std::initializer_list<int> &indices,
                   const std::vector<hydrocouple_variant> &data) override {
        if (indices.size() != 2 || data.empty()) return;
        auto it = indices.begin();
        int startIdx = *it;
        ++it;
        int count = *it;

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
    std::shared_ptr<RegularGrid2D> grid() const { return m_grid; }

    /*!
     * \brief Get grid data type.
     */
    GridDataType gridDataType() const { return m_dataType; }

    /*!
     * \brief Direct access to underlying data array (row-major layout).
     */
    std::vector<T> &data() { return m_data; }
    const std::vector<T> &data() const { return m_data; }

    /*!
     * \brief Get value at grid position (i, j).
     */
    T getValue(int i, int j) const {
        int stride = (m_dataType == CellCentered) ? (m_grid->numXNodes() - 1) : m_grid->numXNodes();
        int idx = j * stride + i;
        if (idx < 0 || idx >= (int)m_data.size()) return T{};
        return m_data[idx];
    }

    /*!
     * \brief Set value at grid position (i, j).
     */
    void setValue(int i, int j, T value) {
        int stride = (m_dataType == CellCentered) ? (m_grid->numXNodes() - 1) : m_grid->numXNodes();
        int idx = j * stride + i;
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
    std::shared_ptr<RegularGrid2D> m_grid;
    GridDataType m_dataType;
    std::vector<T> m_data;

    void updateDimensions() {
        int xCount, yCount;

        if (m_dataType == CellCentered) {
            xCount = m_grid->numXNodes() - 1;
            yCount = m_grid->numYNodes() - 1;
        } else {
            xCount = m_grid->numXNodes();
            yCount = m_grid->numYNodes();
        }

        if (xCount > 0 && yCount > 0) {
            m_data.resize(xCount * yCount, T{});
        }
    }
};

} // namespace HydroCouple::Spatial

#endif // HYDROCOUPLESDK_REGULARGRID2DCOMPONENTDATAITEM_H
