// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   regulargrid2d.h
 * \author Caleb Buahin
 * \version 1.0.0
 * \brief  2D structured regular grid (Cartesian, rectilinear, curvilinear).
 *
 * RegularGrid2D is a value type for a logically rectangular grid of nodes.
 * It does not implement HydroCouple::Spatial::IRegularGrid2D, and the SDK has
 * no adapter to that interface: a component that publishes a grid item
 * (TimeRegularGrid2DComponentDataItem) implements IRegularGrid2D over its own
 * grid or passes no grid.
 */

#ifndef HYDROCOUPLESDK_REGULARGRID2D_H
#define HYDROCOUPLESDK_REGULARGRID2D_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/core/identity.h"
#include "envelope.h"

#include <vector>
#include <memory>

#include "hydrocouplespatial.h"

namespace HydroCouple::SDK::Spatial {

// Interface types from the standard's Spatial namespace.
using namespace HydroCouple::Spatial;

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
 * Here i counts nodes along x (0 to nx - 1) and j along y (0 to ny - 1);
 * cell (i, j) has corners (i, j), (i + 1, j), (i + 1, j + 1) and (i, j + 1).
 * All three types store the full nx·ny coordinate arrays, so a Cartesian grid
 * costs as much memory as a curvilinear one. The type is a label set by the
 * factory function; editing nodes with setNodeX() or setNodeY() does not
 * change it.
 *
 * Grids are made by the static factories cartesian(), rectilinear() and
 * curvilinear(), which return shared pointers because the class, through its
 * Identity base, can be neither copied nor moved. The public default
 * constructor makes an empty 0 × 0 grid that only a subclass can fill.
 *
 * \note envelope() passes its bounds to the Envelope constructor in the
 * wrong order; see envelope().
 *
 * \threadsafety Not thread-safe.
 */
class HYDROCOUPLESDK_EXPORT RegularGrid2D : public Identity {
public:
    /*!
     * \brief Grid type.
     *
     * The SDK's own enumeration; it has the same three values as the
     * standard's HydroCouple::Spatial::RegularGridType.
     */
    enum GridType {
        Cartesian,   //!< Uniform spacing
        Rectilinear, //!< Non-uniform in X or Y (but regular in both)
        Curvilinear  //!< Arbitrary node coordinates
    };

    // ── Constructors ───────────────────────────────────────────────────────

    /*!
     * \brief Construct empty grid.
     *
     * A Cartesian grid of 0 × 0 nodes with a generated UUID id. Use the
     * factory functions for a grid with nodes.
     */
    RegularGrid2D();

    //! Destroys the grid and its coordinate arrays.
    ~RegularGrid2D() override = default;

    // ── Grid construction ──────────────────────────────────────────────────

    /*!
     * \brief Create a Cartesian grid from origin and spacing.
     *
     * Node (i, j) is placed at (x0 + i·dx, y0 + j·dy). Nothing is validated:
     * a zero spacing gives coincident nodes and a negative count fails inside
     * std::vector.
     *
     * \param x0 Origin X coordinate
     * \param y0 Origin Y coordinate (top-left for row-major)
     * \param dx Pixel width (X spacing)
     * \param dy Pixel height (Y spacing, typically negative for top-down)
     * \param nx Number of columns
     * \param ny Number of rows
     * \returns The new grid; never nullptr. \p nx and \p ny are numbers of
     *          nodes (columns and rows of nodes), so the grid has
     *          (nx - 1) × (ny - 1) cells.
     */
    static std::shared_ptr<RegularGrid2D> cartesian(double x0, double y0,
                                                     double dx, double dy,
                                                     int nx, int ny);

    /*!
     * \brief Create a rectilinear grid from X and Y node arrays.
     *
     * Node (i, j) is placed at (nodeX[i], nodeY[j]).
     *
     * \param nodeX Column (X) coordinates; size = nx
     * \param nodeY Row (Y) coordinates; size = ny
     * \param nx Number of columns
     * \param ny Number of rows
     * \returns The new grid, or nullptr when the array sizes are not \p nx
     *          and \p ny. Monotonicity is not checked.
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
     * \returns The new grid holding copies of the arrays, or nullptr when
     *          either array's size is not nx·ny.
     */
    static std::shared_ptr<RegularGrid2D> curvilinear(const std::vector<double> &nodeX,
                                                       const std::vector<double> &nodeY,
                                                       int nx, int ny);

    // ── Grid geometry ──────────────────────────────────────────────────────

    /*!
     * \brief Get grid type.
     * \returns The type the factory function set.
     */
    GridType gridType() const noexcept { return m_gridType; }

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
     * \brief Get total number of nodes.
     * \returns nx·ny.
     */
    int numNodes() const noexcept { return m_nx * m_ny; }

    /*!
     * \brief Get number of cells (in X direction).
     * \returns nx - 1; -1 for an empty grid.
     */
    int numXCells() const noexcept { return m_nx - 1; }

    /*!
     * \brief Get number of cells (in Y direction).
     * \returns ny - 1; -1 for an empty grid.
     */
    int numYCells() const noexcept { return m_ny - 1; }

    /*!
     * \brief Get total number of cells.
     * \returns (nx - 1)·(ny - 1); note that an empty 0 × 0 grid reports 1.
     */
    int numCells() const noexcept { return (m_nx - 1) * (m_ny - 1); }

    // ── Node access ────────────────────────────────────────────────────────

    /*!
     * \brief Get X coordinate of node at (i, j).
     * \param[in] i Node index along x, 0 to nx - 1.
     * \param[in] j Node index along y, 0 to ny - 1.
     * \returns The x coordinate; 0 when (i, j) is out of range.
     */
    double nodeX(int i, int j) const;

    /*!
     * \brief Get Y coordinate of node at (i, j).
     * \param[in] i Node index along x, 0 to nx - 1.
     * \param[in] j Node index along y, 0 to ny - 1.
     * \returns The y coordinate; 0 when (i, j) is out of range.
     */
    double nodeY(int i, int j) const;

    /*!
     * \brief Set X coordinate of node at (i, j).
     * \param[in] i Node index along x; out of range does nothing.
     * \param[in] j Node index along y; out of range does nothing.
     * \param[in] x The new x coordinate.
     */
    void setNodeX(int i, int j, double x);

    /*!
     * \brief Set Y coordinate of node at (i, j).
     * \param[in] i Node index along x; out of range does nothing.
     * \param[in] j Node index along y; out of range does nothing.
     * \param[in] y The new y coordinate.
     */
    void setNodeY(int i, int j, double y);

    // ── Cell geometry ──────────────────────────────────────────────────────

    /*!
     * \brief Get cell center (i, j).
     * Returns the average of the 4 corner nodes.
     * \param[in]  i Cell index along x, 0 to nx - 2.
     * \param[in]  j Cell index along y, 0 to ny - 2.
     * \param[out] x Receives the mean x of the four corners; 0 when (i, j)
     *               is out of range.
     * \param[out] y Receives the mean y of the four corners; 0 when (i, j)
     *               is out of range.
     */
    void cellCenter(int i, int j, double &x, double &y) const;

    /*!
     * \brief Get cell area using the shoelace formula.
     *
     * Sums the signed areas of the triangles (i, j)–(i+1, j)–(i, j+1) and
     * (i+1, j)–(i+1, j+1)–(i, j+1), which is exact for any simple
     * quadrilateral cell.
     *
     * \param[in] i Cell index along x, 0 to nx - 2.
     * \param[in] j Cell index along y, 0 to ny - 2.
     * \returns The plan area in squared coordinate units, never negative; 0
     *          when (i, j) is out of range.
     */
    double cellArea(int i, int j) const;

    // ── Statistics ─────────────────────────────────────────────────────────

    /*!
     * \brief Get bounding envelope.
     *
     * Finds the minimum and maximum node x and y, then passes them to the
     * two-dimensional Envelope constructor as (minX, maxX, minY, maxY) where
     * it expects (minX, minY, maxX, maxY). The result's minY therefore holds
     * the largest x and its maxX the smallest y; read the extent from
     * nodeXData() and nodeYData() until this is fixed.
     *
     * \returns The envelope as described, z bounds 0; an empty Envelope for
     *          an empty grid.
     */
    Envelope envelope() const;

    /*!
     * \brief Check if grid is empty.
     * \returns True when nx or ny is 0.
     */
    bool isEmpty() const noexcept { return m_nx == 0 || m_ny == 0; }

    /*!
     * \brief Get all node X coordinates (row-major layout).
     * \returns nx·ny values, node (i, j) at index j·nx + i; valid while the
     *          grid exists.
     */
    const std::vector<double> &nodeXData() const noexcept { return m_nodeX; }

    /*!
     * \brief Get all node Y coordinates (row-major layout).
     * \returns nx·ny values in the same order as nodeXData().
     */
    const std::vector<double> &nodeYData() const noexcept { return m_nodeY; }

protected:
    GridType m_gridType = Cartesian; //!< The label the factory set.
    int m_nx = 0;                    //!< Number of nodes along x.
    int m_ny = 0;                    //!< Number of nodes along y.
    std::vector<double> m_nodeX;  // size = nx * ny (row-major [j*nx + i])
    std::vector<double> m_nodeY;  // size = nx * ny
};

/*! \var std::vector<double> HydroCouple::SDK::Spatial::RegularGrid2D::m_nodeX
 *  \brief x of every node, nx·ny values, node (i, j) at j·nx + i.
 */
/*! \var std::vector<double> HydroCouple::SDK::Spatial::RegularGrid2D::m_nodeY
 *  \brief y of every node, in the same order as m_nodeX.
 */

} // namespace HydroCouple::SDK::Spatial

#endif // HYDROCOUPLESDK_REGULARGRID2D_H
