// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   sigmagrid.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Terrain-following sigma grid generation (plan §7b.4).
 *
 * Builds vertical layer structures whose interfaces follow terrain or
 * bathymetry: SigmaLayering produces normalized interface distributions
 * (uniform, Song–Haidvogel-style theta stretching, or user-supplied),
 * SigmaGridGenerator::build() maps them onto per-column surface/bed
 * elevations with minimum-thickness enforcement, and extrude() turns any
 * horizontal MeshDefinition (tri, quad, curvilinear — the generator does
 * not care) into a layered 3-D mesh of prisms/hexahedra whose nodes
 * follow the terrain. SigmaGrid and LayeredMesh are plain arrays (node
 * coordinates and CSR cell→node connectivity in the UGRID style); no IO
 * writer takes them directly, so a caller writing a layered mesh copies
 * the arrays into the structure its writer expects.
 *
 * Coordinates: \f$ \sigma \in [0, 1] \f$ runs from the surface
 * (\f$ \sigma = 0 \f$) to the bed (\f$ \sigma = 1 \f$). For a column with
 * surface \f$ \eta \f$ and bed \f$ z_b \f$, depth \f$ D = \eta - z_b \f$,
 * the nominal interface elevations are \f$ z_k = \eta - \sigma_k D \f$.
 * Elevations are in the caller's length unit (meters, say).
 *
 * Part of the HydroCoupleTools library (BUILD_TOOLS=ON); link the
 * HydroCoupleTools target.
 *
 * \threadsafety Stateless static functions; outputs are caller-owned.
 */

#ifndef HYDROCOUPLESDK_TOOLS_SIGMAGRID_H
#define HYDROCOUPLESDK_TOOLS_SIGMAGRID_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/io/meshdefinition.h"

#include <span>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Tools {

/*!
 * \brief Normalized sigma interface distributions on [0, 1]
 *        (0 = surface, 1 = bed; layerCount + 1 strictly increasing values).
 */
class HYDROCOUPLETOOLS_EXPORT SigmaLayering
{
public:
    /*!
     * \brief Equal-thickness interfaces.
     * \param[in] layerCount Number of layers (≥ 1).
     * \return k / layerCount for k = 0..layerCount (last value exactly 1),
     *         or an empty vector when \p layerCount < 1.
     */
    [[nodiscard]] static std::vector<double> uniform(int layerCount);

    /*!
     * \brief Song–Haidvogel-style hyperbolic stretching.
     *
     * With \f$ s_k = k / L \f$ the interfaces are
     * \f[ C(s) = (1 - b)\,\frac{\sinh(\theta s)}{\sinh\theta}
     *     + b\,\frac{\tanh(\theta (s - \tfrac12)) + \tanh(\theta/2)}
     *              {2 \tanh(\theta/2)}, \f]
     * with the first and last values pinned to exactly 0 and 1.
     *
     * \param[in] layerCount Number of layers (≥ 1).
     * \param[in] theta      Stretching intensity (> 0; larger = stronger
     *                       clustering). Values ≤ 0 fall back to uniform.
     * \param[in] b          Surface/bottom blend, clamped to [0, 1]: 0
     *                       clusters interfaces (thin layers) toward the
     *                       surface only; 1 gives the symmetric tanh profile,
     *                       which clusters toward the surface and the bed
     *                       equally; values between blend the two.
     * \return layerCount + 1 values, or an empty vector when
     *         \p layerCount < 1.
     */
    [[nodiscard]] static std::vector<double> stretched(int layerCount,
                                                       double theta,
                                                       double b = 0.0);

    /*!
     * \brief Checks a distribution: ≥ 2 values, first 0, last 1, strictly
     *        increasing.
     * \param[in]  interfaces Distribution to check.
     * \param[out] message    "sigma distribution needs at least 2
     *                        interfaces", "sigma distribution must span
     *                        [0, 1] exactly", or "sigma distribution must be
     *                        strictly increasing (violated at interface K)";
     *                        cleared on success.
     * \return True when the distribution is valid.
     */
    [[nodiscard]] static bool validate(std::span<const double> interfaces,
                                       std::string &message);
};

/*!
 * \brief Per-column terrain-following interface elevations.
 *
 * interfaceZ is column-major: interfaceZ[column * (layerCount + 1) + k],
 * k = 0 at the surface descending to k = layerCount at the bed; strictly
 * decreasing within every column.
 */
struct HYDROCOUPLETOOLS_EXPORT SigmaGrid
{
    int64_t             columnCount = 0; //!< Number of columns (one per horizontal node when extruding).
    int                 layerCount  = 0; //!< Layers per column.
    std::vector<double> interfaceZ;      //!< columnCount × (layerCount + 1) elevations, column-major.

    /*!
     * \brief Interface elevation k of \p column.
     * \param[in] column Column, 0 ≤ column < columnCount (not checked).
     * \param[in] k      Interface, 0 (surface) to layerCount (bed).
     * \return The elevation.
     */
    [[nodiscard]] double z(int64_t column, int k) const
    {
        return interfaceZ[static_cast<size_t>(column) *
                              static_cast<size_t>(layerCount + 1) +
                          static_cast<size_t>(k)];
    }

    /*!
     * \brief Thickness of layer k of \p column (positive).
     * \param[in] column Column (not checked).
     * \param[in] k      Layer, 0 (top) to layerCount - 1 (bottom).
     * \return z(column, k) - z(column, k + 1).
     */
    [[nodiscard]] double thickness(int64_t column, int k) const
    {
        return z(column, k) - z(column, k + 1);
    }
};

/*!
 * \brief Layered 3-D mesh produced by extruding a horizontal mesh through
 *        a SigmaGrid.
 *
 * Nodes are interface-major: node (k, n) of interface k over horizontal
 * node n has index k * nodesPerLayer + n. Cells are layer-major over the
 * horizontal faces: cell (k, f) is the prism/hexahedron between interfaces
 * k and k+1 over face f, with the top face's nodes listed first and the
 * bottom face's nodes (same order) after them.
 */
struct HYDROCOUPLETOOLS_EXPORT LayeredMesh
{
    int     layerCount    = 0; //!< Layers (cells per horizontal face).
    int64_t nodesPerLayer = 0; //!< Horizontal nodes (nodes per interface).

    std::vector<double> nodeX; //!< Size (layerCount+1) * nodesPerLayer.
    std::vector<double> nodeY; //!< Same size as nodeX.
    std::vector<double> nodeZ; //!< Terrain-following elevations.

    std::vector<int64_t> cellNodeOffsets; //!< CSR cell→node connectivity.
    std::vector<int64_t> cellNodes;       //!< Concatenated node indexes: top face, then bottom face.

    //! Total nodes.
    //! \return nodeX.size().
    [[nodiscard]] int64_t nodeCount() const noexcept
    {
        return static_cast<int64_t>(nodeX.size());
    }

    //! Total cells.
    //! \return layerCount × the horizontal face count (0 when empty).
    [[nodiscard]] int64_t cellCount() const noexcept
    {
        return cellNodeOffsets.empty()
                   ? 0
                   : static_cast<int64_t>(cellNodeOffsets.size()) - 1;
    }
};

/*!
 * \brief Terrain-following sigma grid builder and extruder.
 *
 * Typical use: sample bed elevations at the horizontal mesh's nodes
 * (TerrainSampler::idw), build() a SigmaGrid with one column per node,
 * then extrude() the mesh through it.
 */
class HYDROCOUPLETOOLS_EXPORT SigmaGridGenerator
{
public:
    /*!
     * \brief Maps a normalized distribution onto per-column elevations.
     *
     * Layer thicknesses are the sigma increments scaled by the column
     * depth. Minimum thickness is enforced by lifting thin layers to
     * \p minLayerThickness and shrinking the surplus of thicker layers
     * proportionally; columns too shallow to hold
     * layerCount × minLayerThickness collapse to equal thickness
     * depth / layerCount. Interfaces are strictly monotone whenever the
     * column has positive depth.
     *
     * \param[in]  bedElevation      Bed/terrain elevation per column.
     * \param[in]  surfaceElevation  Surface elevation per column
     *                               (must exceed the bed).
     * \param[in]  sigmaInterfaces   Normalized distribution
     *                               (see SigmaLayering).
     * \param[in]  minLayerThickness Minimum layer thickness (≥ 0).
     * \param[out] out               The generated grid (partly filled on
     *                               failure).
     * \param[out] message           Failure description on false: "bed and
     *                               surface elevation counts differ", a
     *                               SigmaLayering::validate() message,
     *                               "minimum layer thickness must be
     *                               non-negative", or "column C has
     *                               non-positive depth (surface S, bed B)".
     *                               Cleared on success.
     * \return True when every column was built.
     */
    [[nodiscard]] static bool build(std::span<const double> bedElevation,
                                    std::span<const double> surfaceElevation,
                                    std::span<const double> sigmaInterfaces,
                                    double minLayerThickness, SigmaGrid &out,
                                    std::string &message);

    /*!
     * \brief Extrudes a horizontal mesh through a sigma grid into layered
     *        prisms/hexahedra (triangles → wedges, quads → hexahedra;
     *        any polygonal face extrudes to the matching prism).
     *
     * \param[in]  horizontal Horizontal mesh with faces; sigma columns
     *                        must equal its node count.
     * \param[in]  sigma      Per-node interface elevations.
     * \param[out] out        The layered 3-D mesh.
     * \param[out] message    Failure description on false: a
     *                        MeshDefinition::validate() message, "horizontal
     *                        mesh has no faces to extrude", or "sigma grid
     *                        has C columns; horizontal mesh has N nodes".
     *                        Cleared on success.
     * \return True when the mesh was extruded. Node x and y are copied from
     *         the horizontal mesh; its nodeZ is ignored.
     */
    [[nodiscard]] static bool extrude(const IO::MeshDefinition &horizontal,
                                      const SigmaGrid &sigma,
                                      LayeredMesh &out, std::string &message);
};

} // namespace HydroCouple::SDK::Tools

#endif // HYDROCOUPLESDK_TOOLS_SIGMAGRID_H
