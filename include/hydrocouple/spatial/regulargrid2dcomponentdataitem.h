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
#include "regulargrid2d.h"

#include <vector>
#include <memory>

namespace HydroCouple::Spatial {

/*!
 * \brief Storage mixin for scalar values on a RegularGrid2D.
 *
 * Stores values in row-major layout:
 * - **Cell data**: m_data[j*(nx-1) + i], size = (nx-1) × (ny-1)
 * - **Node data**: m_data[j*nx + i], size = nx × ny
 *
 * Use as a mixin with AbstractComponentDataItem to create concrete data items.
 *
 * \tparam T Data type (double, float, int, etc.)
 */
template <typename T>
class RegularGrid2DComponentDataItemStorage {
public:
    /*!
     * \brief Grid data location type.
     */
    enum GridDataType {
        CellCentered,  //! Data at cell centers
        NodeCentered   //! Data at grid nodes
    };

    /*!
     * \brief Initialize storage for a RegularGrid2D.
     * \param grid The grid (not owned)
     * \param dataType Location type (CellCentered or NodeCentered)
     * \param capacity Pre-allocate storage
     */
    void initGrid2DStorage(std::shared_ptr<RegularGrid2D> grid,
                          GridDataType dataType, int capacity = 0) {
        m_grid = grid;
        m_dataType = dataType;
        if (capacity > 0) {
            m_data.reserve(capacity);
        }
        updateDimensions();
    }

    // ── Storage accessors ──────────────────────────────────────────────────

    /*!
     * \brief Direct access to underlying data array (row-major layout).
     */
    std::vector<T> &data() { return m_data; }
    const std::vector<T> &data() const { return m_data; }

    /*!
     * \brief Get the grid.
     */
    std::shared_ptr<RegularGrid2D> grid() const { return m_grid; }

    /*!
     * \brief Get grid data type.
     */
    GridDataType gridDataType() const { return m_dataType; }

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

protected:
    std::shared_ptr<RegularGrid2D> m_grid;
    GridDataType m_dataType = CellCentered;
    std::vector<T> m_data;

private:
    void updateDimensions() {
        if (!m_grid) return;

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
