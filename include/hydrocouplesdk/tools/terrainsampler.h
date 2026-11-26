// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   terrainsampler.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Terrain sampling from scattered points (plan §7b.5).
 *
 * Inverse-distance-weighted interpolation of elevations from scattered
 * (x, y, z) samples onto arbitrary query locations — the mesh generators'
 * bridge from survey/DEM points to node elevations and sigma-grid bed
 * columns. Query points that coincide with a sample take its exact value.
 * Raster-backed sampling (GDAL) stays with the gated raster module;
 * natural-neighbor interpolation is a planned refinement on top of the
 * Triangulator.
 *
 * Part of the HydroCoupleTools library (BUILD_TOOLS=ON).
 *
 * \threadsafety Stateless static function.
 */

#ifndef HYDROCOUPLESDK_TOOLS_TERRAINSAMPLER_H
#define HYDROCOUPLESDK_TOOLS_TERRAINSAMPLER_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/spatial/point.h"

#include <span>
#include <string>

namespace HydroCouple::SDK::Tools {

/*!
 * \brief Scattered-point elevation interpolator.
 *
 * Every query visits every sample (no spatial index), so the cost is
 * queries × samples.
 */
class HYDROCOUPLETOOLS_EXPORT TerrainSampler
{
public:
    /*!
     * \brief Inverse-distance-weighted interpolation.
     *
     * \f[ z(q) = \frac{\sum_i w_i z_i}{\sum_i w_i}, \qquad
     *     w_i = d(q, p_i)^{-\mathrm{power}}, \f]
     * over the samples within \p searchRadius. A query exactly on a sample
     * takes that sample's z (the first such sample in order).
     *
     * \param[in]  samples      Scattered (x, y, z) samples (≥ 1).
     * \param[in]  power        IDW exponent (> 0; 2 is customary).
     * \param[in]  queryX       Query x coordinates.
     * \param[in]  queryY       Query y coordinates (same length).
     * \param[out] elevationOut Interpolated z per query (same length).
     * \param[in]  searchRadius Only samples within this distance
     *                          contribute; 0 uses every sample. Queries
     *                          with no sample in radius fail.
     * \param[out] message      Failure description on false: "no samples",
     *                          "IDW power must be positive", "query and
     *                          output extents differ", or "query Q has no
     *                          sample within the search radius" (earlier
     *                          queries are already written). Cleared on
     *                          success.
     * \return True when every query was interpolated.
     */
    [[nodiscard]] static bool idw(std::span<const Spatial::Point> samples,
                                  double power,
                                  std::span<const double> queryX,
                                  std::span<const double> queryY,
                                  std::span<double> elevationOut,
                                  std::string &message,
                                  double searchRadius = 0.0);
};

} // namespace HydroCouple::SDK::Tools

#endif // HYDROCOUPLESDK_TOOLS_TERRAINSAMPLER_H
