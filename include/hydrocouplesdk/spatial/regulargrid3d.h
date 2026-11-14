// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   regulargrid3d.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  3D structured regular grid.
 */

#ifndef HYDROCOUPLESDK_REGULARGRID3D_H
#define HYDROCOUPLESDK_REGULARGRID3D_H

#include "hydrocouplesdk/export.h"
#include "regulargrid2d.h"

#include <vector>
#include <memory>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

/*!
 * \brief 3D structured regular grid (layer-stacked 2D grids).
 *
 * A 3D grid is a stack of 2D grids, with nx × ny × nz nodes.
 * Nodes are indexed as: idx = k*(nx*ny) + j*nx + i
 *
 * Cell count = (nx-1) × (ny-1) × (nz-1).
 *
 * \threadsafety Not thread-safe.
 */
class HYDROCOUPLESDK_EXPORT RegularGrid3D : public Identity {
public:
    /*!
     * \brief Construct empty grid.
     */
    RegularGrid3D();

    ~RegularGrid3D() override = default;

    // ── Grid construction ──────────────────────────────────────────────────

    /*!
     * \brief Create a 3D Cartesian grid.
     * \param x0 Origin X coordinate
     * \param y0 Origin Y coordinate
     * \param z0 Origin Z coordinate (top of first layer)
     * \param dx Pixel width (X spacing)
     * \param dy Pixel height (Y spacing, typically negative for top-down)
     * \param dz Layer thickness (Z spacing, typically negative for increasing depth)
     * \param nx Number of columns
     * \param ny Number of rows
     * \param nz Number of layers
     */
    static std::shared_ptr<RegularGrid3D> cartesian(double x0, double y0, double z0,
                                                     double dx, double dy, double dz,
                                                     int nx, int ny, int nz);

    /*!
     * \brief Create a 3D curvilinear grid from arbitrary node coordinates.
     */
    static std::shared_ptr<RegularGrid3D> curvilinear(const std::vector<double> &nodeX,
                                                       const std::vector<double> &nodeY,
                                                       const std::vector<double> &nodeZ,
                                                       int nx, int ny, int nz);

    // ── Grid geometry ──────────────────────────────────────────────────────

    /*!
     * \brief Get number of columns (nodes in X direction).
     */
    int numXNodes() const noexcept { return m_nx; }

    /*!
     * \brief Get number of rows (nodes in Y direction).
     */
    int numYNodes() const noexcept { return m_ny; }

    /*!
     * \brief Get number of layers (nodes in Z direction).
     */
    int numZNodes() const noexcept { return m_nz; }

    /*!
     * \brief Get total number of nodes.
     */
    int numNodes() const noexcept { return m_nx * m_ny * m_nz; }

    /*!
     * \brief Get total number of cells.
     */
    int numCells() const noexcept { return (m_nx - 1) * (m_ny - 1) * (m_nz - 1); }

    // ── Node access ────────────────────────────────────────────────────────

    /*!
     * \brief Get X coordinate of node at (i, j, k).
     */
    double nodeX(int i, int j, int k) const;

    /*!
     * \brief Get Y coordinate of node at (i, j, k).
     */
    double nodeY(int i, int j, int k) const;

    /*!
     * \brief Get Z coordinate of node at (i, j, k).
     */
    double nodeZ(int i, int j, int k) const;

    /*!
     * \brief Set X coordinate of node at (i, j, k).
     */
    void setNodeX(int i, int j, int k, double x);

    /*!
     * \brief Set Y coordinate of node at (i, j, k).
     */
    void setNodeY(int i, int j, int k, double y);

    /*!
     * \brief Set Z coordinate of node at (i, j, k).
     */
    void setNodeZ(int i, int j, int k, double z);

    // ── Statistics ─────────────────────────────────────────────────────────

    /*!
     * \brief Check if grid is empty.
     */
    bool isEmpty() const noexcept { return m_nx == 0 || m_ny == 0 || m_nz == 0; }

    /*!
     * \brief Get all node X coordinates.
     */
    const std::vector<double> &nodeXData() const noexcept { return m_nodeX; }

    /*!
     * \brief Get all node Y coordinates.
     */
    const std::vector<double> &nodeYData() const noexcept { return m_nodeY; }

    /*!
     * \brief Get all node Z coordinates.
     */
    const std::vector<double> &nodeZData() const noexcept { return m_nodeZ; }

protected:
    int m_nx = 0;
    int m_ny = 0;
    int m_nz = 0;
    std::vector<double> m_nodeX;  // size = nx * ny * nz (k-j-i order)
    std::vector<double> m_nodeY;  // size = nx * ny * nz
    std::vector<double> m_nodeZ;  // size = nx * ny * nz
};

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_REGULARGRID3D_H
