// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Amoa Buahin <caleb.buahin@gmail.com>

/*!
 * \file   regulargrid2d.h
 * \author Caleb Amoa Buahin <caleb.buahin@gmail.com>
 * \version 1.0.0
 * \brief  2D structured regular grid (Cartesian, rectilinear, curvilinear).
 */

#ifndef HYDROCOUPLESDK_REGULARGRID2D_H
#define HYDROCOUPLESDK_REGULARGRID2D_H

#include "hydrocouplesdk.h"
#include "hydrocouple/identity.h"
#include "envelope.h"

#include <vector>
#include <memory>

namespace HydroCouple::Spatial {

/*!
 * \brief 2D structured regular grid.
 *
 * A grid of nodes arranged in a regular nx × ny array.
 * Supports three grid types:
 *
 * - **Cartesian**: uniform spacing, defined by origin, pixel width, pixel height
 * - **Rectilinear**: non-uniform spacing in each dimension independently
 * - **Curvilinear**: arbitrary node coordinates (fully general)
 *
 * Node (i, j) is accessed as: m_nodeX[j * nx + i], m_nodeY[j * nx + i]
 * Cells are formed between nodes; cell count = (nx-1) × (ny-1).
 *
 * \threadsafety Not thread-safe.
 */
class HYDROCOUPLESDK_EXPORT RegularGrid2D : public Identity {
public:
    /*!
     * \brief Grid type.
     */
    enum GridType {
        Cartesian,   //! Uniform spacing
        Rectilinear, //! Non-uniform in X or Y (but regular in both)
        Curvilinear  //! Arbitrary node coordinates
    };

    // ── Constructors ───────────────────────────────────────────────────────

    /*!
     * \brief Construct empty grid.
     */
    RegularGrid2D();

    ~RegularGrid2D() override = default;

    // ── Grid construction ──────────────────────────────────────────────────

    /*!
     * \brief Create a Cartesian grid from origin and spacing.
     * \param x0 Origin X coordinate
     * \param y0 Origin Y coordinate (top-left for row-major)
     * \param dx Pixel width (X spacing)
     * \param dy Pixel height (Y spacing, typically negative for top-down)
     * \param nx Number of columns
     * \param ny Number of rows
     */
    static std::shared_ptr<RegularGrid2D> cartesian(double x0, double y0,
                                                     double dx, double dy,
                                                     int nx, int ny);

    /*!
     * \brief Create a rectilinear grid from X and Y node arrays.
     * \param nodeX Column (X) coordinates; size = nx
     * \param nodeY Row (Y) coordinates; size = ny
     * \param nx Number of columns
     * \param ny Number of rows
     */
    static std::shared_ptr<RegularGrid2D> rectilinear(const std::vector<double> &nodeX,
                                                       const std::vector<double> &nodeY,
                                                       int nx, int ny);

    /*!
     * \brief Create a curvilinear grid from arbitrary node coordinates.
     * \param nodeX X coordinates of all nodes; size = nx * ny (row-major)
     * \param nodeY Y coordinates of all nodes; size = nx * ny
     * \param nx Number of columns
     * \param ny Number of rows
     */
    static std::shared_ptr<RegularGrid2D> curvilinear(const std::vector<double> &nodeX,
                                                       const std::vector<double> &nodeY,
                                                       int nx, int ny);

    // ── Grid geometry ──────────────────────────────────────────────────────

    /*!
     * \brief Get grid type.
     */
    GridType gridType() const noexcept { return m_gridType; }

    /*!
     * \brief Get number of columns (nodes in X direction).
     */
    int numXNodes() const noexcept { return m_nx; }

    /*!
     * \brief Get number of rows (nodes in Y direction).
     */
    int numYNodes() const noexcept { return m_ny; }

    /*!
     * \brief Get total number of nodes.
     */
    int numNodes() const noexcept { return m_nx * m_ny; }

    /*!
     * \brief Get number of cells (in X direction).
     */
    int numXCells() const noexcept { return m_nx - 1; }

    /*!
     * \brief Get number of cells (in Y direction).
     */
    int numYCells() const noexcept { return m_ny - 1; }

    /*!
     * \brief Get total number of cells.
     */
    int numCells() const noexcept { return (m_nx - 1) * (m_ny - 1); }

    // ── Node access ────────────────────────────────────────────────────────

    /*!
     * \brief Get X coordinate of node at (i, j).
     */
    double nodeX(int i, int j) const;

    /*!
     * \brief Get Y coordinate of node at (i, j).
     */
    double nodeY(int i, int j) const;

    /*!
     * \brief Set X coordinate of node at (i, j).
     */
    void setNodeX(int i, int j, double x);

    /*!
     * \brief Set Y coordinate of node at (i, j).
     */
    void setNodeY(int i, int j, double y);

    // ── Cell geometry ──────────────────────────────────────────────────────

    /*!
     * \brief Get cell center (i, j).
     * Returns the average of the 4 corner nodes.
     */
    void cellCenter(int i, int j, double &x, double &y) const;

    /*!
     * \brief Get cell area using the shoelace formula.
     */
    double cellArea(int i, int j) const;

    // ── Statistics ─────────────────────────────────────────────────────────

    /*!
     * \brief Get bounding envelope.
     */
    Envelope envelope() const;

    /*!
     * \brief Check if grid is empty.
     */
    bool isEmpty() const noexcept { return m_nx == 0 || m_ny == 0; }

    /*!
     * \brief Get all node X coordinates (row-major layout).
     */
    const std::vector<double> &nodeXData() const noexcept { return m_nodeX; }

    /*!
     * \brief Get all node Y coordinates (row-major layout).
     */
    const std::vector<double> &nodeYData() const noexcept { return m_nodeY; }

protected:
    GridType m_gridType = Cartesian;
    int m_nx = 0;
    int m_ny = 0;
    std::vector<double> m_nodeX;  // size = nx * ny (row-major [j*nx + i])
    std::vector<double> m_nodeY;  // size = nx * ny
};

} // namespace HydroCouple::Spatial

#endif // HYDROCOUPLESDK_REGULARGRID2D_H
