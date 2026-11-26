// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   curvilineargrid.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Boundary-fitted curvilinear grid generation (plan §7b.3).
 *
 * Transfinite interpolation builds a structured grid from four boundary
 * curves; optional Winslow elliptic smoothing improves interior
 * orthogonality and spacing; graded extrusion produces 3-D structured
 * grids with stretching along the vertical parametric direction. Quality
 * metrics (corner Jacobians, orthogonality deviation) are first-class so
 * generators can be asserted against, and toMeshDefinition() emits the
 * UGRID-congruent quad mesh the IO writers consume.
 *
 * Part of the HydroCoupleTools library (BUILD_TOOLS=ON).
 *
 * \threadsafety Stateless static functions; outputs are caller-owned.
 */

#ifndef HYDROCOUPLESDK_TOOLS_CURVILINEARGRID_H
#define HYDROCOUPLESDK_TOOLS_CURVILINEARGRID_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplesdk/spatial/point.h"

#include <span>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Tools {

/*!
 * \brief Structured 2-D grid of ni × nj nodes, node-major [j * ni + i].
 */
struct HYDROCOUPLETOOLS_EXPORT StructuredGrid2D
{
    int                 ni = 0; //!< Nodes along ξ.
    int                 nj = 0; //!< Nodes along η.
    std::vector<double> x;      //!< Size ni * nj.
    std::vector<double> y;      //!< Size ni * nj.

    //! Writable x of node (i, j).
    //! \param[in] i Index along ξ, 0 ≤ i < ni (not checked).
    //! \param[in] j Index along η, 0 ≤ j < nj (not checked).
    //! \return x[j * ni + i].
    [[nodiscard]] double &X(int i, int j)
    {
        return x[static_cast<size_t>(j) * ni + i];
    }
    //! Writable y of node (i, j).
    //! \param[in] i Index along ξ (not checked).
    //! \param[in] j Index along η (not checked).
    //! \return y[j * ni + i].
    [[nodiscard]] double &Y(int i, int j)
    {
        return y[static_cast<size_t>(j) * ni + i];
    }
    //! x of node (i, j).
    //! \param[in] i Index along ξ (not checked).
    //! \param[in] j Index along η (not checked).
    //! \return x[j * ni + i].
    [[nodiscard]] double X(int i, int j) const
    {
        return x[static_cast<size_t>(j) * ni + i];
    }
    //! y of node (i, j).
    //! \param[in] i Index along ξ (not checked).
    //! \param[in] j Index along η (not checked).
    //! \return y[j * ni + i].
    [[nodiscard]] double Y(int i, int j) const
    {
        return y[static_cast<size_t>(j) * ni + i];
    }
};

/*!
 * \brief Structured 3-D grid of ni × nj × nk nodes,
 *        node-major [(k * nj + j) * ni + i].
 */
struct HYDROCOUPLETOOLS_EXPORT StructuredGrid3D
{
    int                 ni = 0; //!< Nodes along ξ.
    int                 nj = 0; //!< Nodes along η.
    int                 nk = 0; //!< Nodes along the extruded direction.
    std::vector<double> x;      //!< Size ni * nj * nk.
    std::vector<double> y;      //!< Size ni * nj * nk.
    std::vector<double> z;      //!< Size ni * nj * nk; constant per level k.
};

/*!
 * \brief Transfinite + elliptic curvilinear grid generator.
 *
 * Typical use: transfinite() from four boundary curves, winslowSmooth()
 * until the returned movement is small, check minCornerJacobian() > 0, then
 * toMeshDefinition() for a 2-D mesh or extrude() for a 3-D grid.
 */
class HYDROCOUPLETOOLS_EXPORT CurvilinearGridGenerator
{
public:
    /*!
     * \brief Transfinite interpolation from four boundary curves.
     *
     * \param[in]  south   ni points along η = 0 (west→east).
     * \param[in]  north   ni points along η = 1 (west→east).
     * \param[in]  west    nj points along ξ = 0 (south→north).
     * \param[in]  east    nj points along ξ = 1 (south→north).
     * Corners must agree within 1e-9 in x and y: south.front() with
     * west.front(), south.back() with east.front(), north.front() with
     * west.back(), north.back() with east.back(). The grid is
     * \f$ x(\xi,\eta) = (1-\eta) S(\xi) + \eta N(\xi) + (1-\xi) W(\eta) + \xi E(\eta) \f$
     * minus the bilinear blend of the four corners, at
     * \f$ \xi = i/(n_i - 1) \f$, \f$ \eta = j/(n_j - 1) \f$: point i of a curve
     * sits at that ξ, so the boundary points' spacing carries into the
     * interior.
     *
     * \param[out] out     The generated grid.
     * \param[out] message Failure description on false (size or corner
     *                     mismatches): "boundary curves need at least 2
     *                     points each", "opposite boundary curves must have
     *                     equal point counts", "boundary curves do not meet
     *                     at consistent corners". Cleared on success.
     * \return True when the grid was built.
     */
    [[nodiscard]] static bool transfinite(
        std::span<const Spatial::Point> south,
        std::span<const Spatial::Point> north,
        std::span<const Spatial::Point> west,
        std::span<const Spatial::Point> east, StructuredGrid2D &out,
        std::string &message);

    /*!
     * \brief Winslow elliptic smoothing of the interior nodes
     *        (boundary nodes are fixed).
     * \param[in,out] grid       The grid to smooth.
     * \param[in]     iterations Gauss–Seidel sweeps (≥ 0).
     * \return Maximum node movement of the final sweep (convergence
     *         monitor), in coordinate units; 0 when \p iterations is 0.
     */
    static double winslowSmooth(StructuredGrid2D &grid, int iterations);

    /*!
     * \brief Minimum corner Jacobian over all cells (> 0 ⇔ every cell is
     *        convex and positively oriented).
     *
     * At each corner of each cell (corners taken counter-clockwise from
     * node (i, j)) the cross product of the edges to the next and previous
     * corners.
     *
     * \param[in] grid The grid.
     * \return The minimum, in squared coordinate units; the largest double
     *         when the grid has no cells.
     */
    [[nodiscard]] static double minCornerJacobian(
        const StructuredGrid2D &grid);

    /*!
     * \brief Largest deviation of the interior ξ–η angle from 90°
     *        (degrees).
     *
     * Measured at interior nodes between the central-difference ξ and η
     * directions; boundary nodes are not measured.
     *
     * \param[in] grid The grid.
     * \return Degrees in [0, 90]; 0 when the grid has no interior node.
     */
    [[nodiscard]] static double maxOrthogonalityDeviation(
        const StructuredGrid2D &grid);

    /*!
     * \brief Emits the grid as a UGRID-congruent quad mesh
     *        (counter-clockwise faces).
     *
     * Nodes keep the grid's order (node j * ni + i); face (i, j) lists nodes
     * (i, j), (i+1, j), (i+1, j+1), (i, j+1), which is counter-clockwise when
     * ξ runs east and η north. No edges or nodeZ are set.
     *
     * \param[in] grid     The grid.
     * \param[in] meshName UGRID mesh variable name.
     * \return The mesh, with (ni - 1)(nj - 1) faces.
     */
    [[nodiscard]] static IO::MeshDefinition toMeshDefinition(
        const StructuredGrid2D &grid, const std::string &meshName = "mesh");

    /*!
     * \brief Graded extrusion to 3-D: every 2-D node is replicated at the
     *        given z-levels (strictly monotone; grading/stretching is
     *        encoded in the level spacing — combine with
     *        SigmaLayering::stretched() mapped onto an elevation range).
     *
     * The levels are flat (z does not follow terrain); for terrain-following
     * layers use SigmaGridGenerator.
     *
     * \param[in]  grid    The 2-D grid (at least 2 × 2 nodes).
     * \param[in]  zLevels Elevations, increasing or decreasing, no repeats.
     * \param[out] out     The 3-D grid, node (k * nj + j) * ni + i.
     * \param[out] message "2-D grid is empty", "extrusion needs at least 2
     *                     z-levels", or "z-levels must be strictly
     *                     monotone"; cleared on success.
     * \return True when extruded.
     */
    [[nodiscard]] static bool extrude(const StructuredGrid2D &grid,
                                      std::span<const double> zLevels,
                                      StructuredGrid3D &out,
                                      std::string &message);
};

} // namespace HydroCouple::SDK::Tools

#endif // HYDROCOUPLESDK_TOOLS_CURVILINEARGRID_H
