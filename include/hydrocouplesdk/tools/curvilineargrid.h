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
    std::vector<double> y;

    [[nodiscard]] double &X(int i, int j)
    {
        return x[static_cast<size_t>(j) * ni + i];
    }
    [[nodiscard]] double &Y(int i, int j)
    {
        return y[static_cast<size_t>(j) * ni + i];
    }
    [[nodiscard]] double X(int i, int j) const
    {
        return x[static_cast<size_t>(j) * ni + i];
    }
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
    int                 ni = 0;
    int                 nj = 0;
    int                 nk = 0; //!< Nodes along the extruded direction.
    std::vector<double> x;      //!< Size ni * nj * nk.
    std::vector<double> y;
    std::vector<double> z;
};

/*!
 * \brief Transfinite + elliptic curvilinear grid generator.
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
     * \param[out] out     The generated grid.
     * \param[out] message Failure description on false (size or corner
     *                     mismatches).
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
     *         monitor).
     */
    static double winslowSmooth(StructuredGrid2D &grid, int iterations);

    /*!
     * \brief Minimum corner Jacobian over all cells (> 0 ⇔ every cell is
     *        convex and positively oriented).
     */
    [[nodiscard]] static double minCornerJacobian(
        const StructuredGrid2D &grid);

    /*!
     * \brief Largest deviation of the interior ξ–η angle from 90°
     *        (degrees).
     */
    [[nodiscard]] static double maxOrthogonalityDeviation(
        const StructuredGrid2D &grid);

    /*!
     * \brief Emits the grid as a UGRID-congruent quad mesh
     *        (counter-clockwise faces).
     */
    [[nodiscard]] static IO::MeshDefinition toMeshDefinition(
        const StructuredGrid2D &grid, const std::string &meshName = "mesh");

    /*!
     * \brief Graded extrusion to 3-D: every 2-D node is replicated at the
     *        given z-levels (strictly monotone; grading/stretching is
     *        encoded in the level spacing — combine with
     *        SigmaLayering::stretched() mapped onto an elevation range).
     */
    [[nodiscard]] static bool extrude(const StructuredGrid2D &grid,
                                      std::span<const double> zLevels,
                                      StructuredGrid3D &out,
                                      std::string &message);
};

} // namespace HydroCouple::SDK::Tools

#endif // HYDROCOUPLESDK_TOOLS_CURVILINEARGRID_H
