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
#include "regulargrid3d.h"

#include <vector>
#include <memory>

namespace HydroCouple::Spatial {

/*!
 * \brief Storage mixin for scalar values on a RegularGrid3D.
 *
 * Stores values in K-J-I order (layer-major, then row-major):
 * idx = k*(nx*(ny-1)) + j*nx + i
 *
 * Use as a mixin with AbstractComponentDataItem to create concrete data items.
 *
 * \tparam T Data type (double, float, int, etc.)
 */
template <typename T>
class RegularGrid3DComponentDataItemStorage {
public:
    /*!
     * \brief Grid data location type.
     */
    enum GridDataType {
        CellCentered,  //! Data at cell centers
        NodeCentered   //! Data at grid nodes
    };

    /*!
     * \brief Initialize storage for a RegularGrid3D.
     * \param grid The grid (not owned)
     * \param dataType Location type
     * \param capacity Pre-allocate storage
     */
    void initGrid3DStorage(std::shared_ptr<RegularGrid3D> grid,
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
     * \brief Direct access to underlying data array (K-J-I order).
     */
    std::vector<T> &data() { return m_data; }
    const std::vector<T> &data() const { return m_data; }

    /*!
     * \brief Get the grid.
     */
    std::shared_ptr<RegularGrid3D> grid() const { return m_grid; }

    /*!
     * \brief Get grid data type.
     */
    GridDataType gridDataType() const { return m_dataType; }

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

protected:
    std::shared_ptr<RegularGrid3D> m_grid;
    GridDataType m_dataType = CellCentered;
    std::vector<T> m_data;

private:
    void updateDimensions() {
        if (!m_grid) return;

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
