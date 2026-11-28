// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   regulargrid3d.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  3D structured regular grid.
 *
 * RegularGrid3D is a value type for a logically box-shaped grid of nodes. It
 * does not implement HydroCouple::Spatial::IRegularGrid3D and the SDK has no
 * adapter to that interface (see regulargrid2d.h).
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
 * Every node stores its own x, y and z, so, unlike the standard's
 * IRegularGrid3D (whose plan coordinates are the same in every layer), a
 * RegularGrid3D can hold a grid that is curvilinear in three dimensions.
 * The class is not derived from RegularGrid2D and has no grid-type label,
 * envelope or cell geometry. Grids are made by the static factories, which
 * return shared pointers because the class cannot be copied or moved.
 *
 * \threadsafety Not thread-safe.
 */
class HYDROCOUPLESDK_EXPORT RegularGrid3D : public Identity {
public:
    /*!
     * \brief Construct empty grid.
     *
     * A grid of 0 × 0 × 0 nodes with a generated UUID id; use the factory
     * functions for a grid with nodes.
     */
    RegularGrid3D();

    //! Destroys the grid and its coordinate arrays.
    ~RegularGrid3D() override = default;

    // ── Grid construction ──────────────────────────────────────────────────

    /*!
     * \brief Create a 3D Cartesian grid.
     *
     * Node (i, j, k) is placed at (x0 + i·dx, y0 + j·dy, z0 + k·dz), so layer
     * k = 0 is at z0. Nothing is validated.
     *
     * \param x0 Origin X coordinate
     * \param y0 Origin Y coordinate
     * \param z0 Origin Z coordinate (top of first layer)
     * \param dx Pixel width (X spacing)
     * \param dy Pixel height (Y spacing, typically negative for top-down)
     * \param dz Layer thickness (Z spacing, typically negative for increasing depth)
     * \param nx Number of columns
     * \param ny Number of rows
     * \param nz Number of layers
     * \returns The new grid; never nullptr. The counts are numbers of nodes,
     *          so the grid has (nx - 1)·(ny - 1)·(nz - 1) cells.
     */
    static std::shared_ptr<RegularGrid3D> cartesian(double x0, double y0, double z0,
                                                     double dx, double dy, double dz,
                                                     int nx, int ny, int nz);

    /*!
     * \brief Create a 3D curvilinear grid from arbitrary node coordinates.
     * \param[in] nodeX x of every node, nx·ny·nz values, node (i, j, k) at
     *                  k·nx·ny + j·nx + i.
     * \param[in] nodeY y of every node, in the same order.
     * \param[in] nodeZ z of every node, in the same order.
     * \param[in] nx    Number of nodes along x.
     * \param[in] ny    Number of nodes along y.
     * \param[in] nz    Number of nodes along z (layers of nodes).
     * \returns The new grid holding copies of the arrays, or nullptr when any
     *          array's size is not nx·ny·nz.
     */
    static std::shared_ptr<RegularGrid3D> curvilinear(const std::vector<double> &nodeX,
                                                       const std::vector<double> &nodeY,
                                                       const std::vector<double> &nodeZ,
                                                       int nx, int ny, int nz);

    // ── Grid geometry ──────────────────────────────────────────────────────

    /*!
     * \brief Get number of columns (nodes in X direction).
     * \returns nx.
     */
    int numXNodes() const noexcept { return m_nx; }

    /*!
     * \brief Get number of rows (nodes in Y direction).
     * \returns ny.
     */
    int numYNodes() const noexcept { return m_ny; }

    /*!
     * \brief Get number of layers (nodes in Z direction).
     * \returns nz.
     */
    int numZNodes() const noexcept { return m_nz; }

    /*!
     * \brief Get total number of nodes.
     * \returns nx·ny·nz.
     */
    int numNodes() const noexcept { return m_nx * m_ny * m_nz; }

    /*!
     * \brief Get total number of cells.
     * \returns (nx - 1)·(ny - 1)·(nz - 1); meaningless (−1) for an empty grid.
     */
    int numCells() const noexcept { return (m_nx - 1) * (m_ny - 1) * (m_nz - 1); }

    // ── Node access ────────────────────────────────────────────────────────

    /*!
     * \brief Get X coordinate of node at (i, j, k).
     * \param[in] i Node index along x, 0 to nx - 1.
     * \param[in] j Node index along y, 0 to ny - 1.
     * \param[in] k Node index along z, 0 to nz - 1.
     * \returns The x coordinate; 0 when (i, j, k) is out of range.
     */
    double nodeX(int i, int j, int k) const;

    /*!
     * \brief Get Y coordinate of node at (i, j, k).
     * \param[in] i Node index along x.
     * \param[in] j Node index along y.
     * \param[in] k Node index along z.
     * \returns The y coordinate; 0 when (i, j, k) is out of range.
     */
    double nodeY(int i, int j, int k) const;

    /*!
     * \brief Get Z coordinate of node at (i, j, k).
     * \param[in] i Node index along x.
     * \param[in] j Node index along y.
     * \param[in] k Node index along z.
     * \returns The z coordinate; 0 when (i, j, k) is out of range.
     */
    double nodeZ(int i, int j, int k) const;

    /*!
     * \brief Set X coordinate of node at (i, j, k).
     * \param[in] i Node index along x; out of range does nothing.
     * \param[in] j Node index along y; out of range does nothing.
     * \param[in] k Node index along z; out of range does nothing.
     * \param[in] x The new x coordinate.
     */
    void setNodeX(int i, int j, int k, double x);

    /*!
     * \brief Set Y coordinate of node at (i, j, k).
     * \param[in] i Node index along x; out of range does nothing.
     * \param[in] j Node index along y; out of range does nothing.
     * \param[in] k Node index along z; out of range does nothing.
     * \param[in] y The new y coordinate.
     */
    void setNodeY(int i, int j, int k, double y);

    /*!
     * \brief Set Z coordinate of node at (i, j, k).
     * \param[in] i Node index along x; out of range does nothing.
     * \param[in] j Node index along y; out of range does nothing.
     * \param[in] k Node index along z; out of range does nothing.
     * \param[in] z The new z coordinate.
     */
    void setNodeZ(int i, int j, int k, double z);

    // ── Statistics ─────────────────────────────────────────────────────────

    /*!
     * \brief Check if grid is empty.
     * \returns True when nx, ny or nz is 0.
     */
    bool isEmpty() const noexcept { return m_nx == 0 || m_ny == 0 || m_nz == 0; }

    /*!
     * \brief Get all node X coordinates.
     * \returns nx·ny·nz values, node (i, j, k) at k·nx·ny + j·nx + i.
     */
    const std::vector<double> &nodeXData() const noexcept { return m_nodeX; }

    /*!
     * \brief Get all node Y coordinates.
     * \returns nx·ny·nz values in the order of nodeXData().
     */
    const std::vector<double> &nodeYData() const noexcept { return m_nodeY; }

    /*!
     * \brief Get all node Z coordinates.
     * \returns nx·ny·nz values in the order of nodeXData().
     */
    const std::vector<double> &nodeZData() const noexcept { return m_nodeZ; }

protected:
    int m_nx = 0; //!< Number of nodes along x.
    int m_ny = 0; //!< Number of nodes along y.
    int m_nz = 0; //!< Number of nodes along z.
    std::vector<double> m_nodeX;  // size = nx * ny * nz (k-j-i order)
    std::vector<double> m_nodeY;  // size = nx * ny * nz
    std::vector<double> m_nodeZ;  // size = nx * ny * nz
};

/*! \var std::vector<double> HydroCouple::SDK::Spatial::RegularGrid3D::m_nodeX
 *  \brief x of every node, nx·ny·nz values in k-j-i order.
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::RegularGrid3D::m_nodeY
 *  \brief y of every node, in the order of m_nodeX.
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::RegularGrid3D::m_nodeZ
 *  \brief z of every node, in the order of m_nodeX.
 */

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_REGULARGRID3D_H
