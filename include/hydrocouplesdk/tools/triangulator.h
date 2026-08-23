// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   triangulator.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Constrained Delaunay triangulation (plan §7b.1) over the CDT
 *         library (MIT).
 *
 * Triangulates a boundary polygon with optional holes, constraint
 * segments, and interior (Steiner) points. Boundary and constraint edges
 * may be densified to a maximum edge length before triangulation — the
 * practical refinement control for coupled-model meshes. Output is a
 * UGRID-congruent MeshDefinition of counter-clockwise triangles, directly
 * consumable by the IO writers, the mesh data items, and the quad-dominant
 * mesher.
 *
 * \threadsafety Stateless static function; outputs are caller-owned.
 */

#ifndef HYDROCOUPLESDK_TOOLS_TRIANGULATOR_H
#define HYDROCOUPLESDK_TOOLS_TRIANGULATOR_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplesdk/spatial/point.h"

#include <array>
#include <string>
#include <vector>

namespace HydroCouple::SDK::Tools {

/*!
 * \brief Input domain for constrained Delaunay triangulation.
 */
struct HYDROCOUPLETOOLS_EXPORT TriangulationInput
{
    /*!
     * Outer boundary ring, counter-clockwise, without repeating the first
     * point. Required.
     */
    std::vector<Spatial::Point> boundary;

    /*!
     * Hole rings (any orientation, first point not repeated). Triangles
     * inside a hole are removed.
     */
    std::vector<std::vector<Spatial::Point>> holes;

    /*!
     * Constraint segments the triangulation must conform to (breaklines,
     * channels, levees).
     */
    std::vector<std::array<Spatial::Point, 2>> constraintSegments;

    /*!
     * Optional interior Steiner points (density control).
     */
    std::vector<Spatial::Point> interiorPoints;

    /*!
     * When > 0, boundary/hole/constraint edges longer than this are split
     * before triangulation.
     */
    double maxEdgeLength = 0.0;
};

/*!
 * \brief Constrained Delaunay triangulator (CDT-backed).
 */
class HYDROCOUPLETOOLS_EXPORT Triangulator
{
public:
    /*!
     * \brief Triangulates \p input into counter-clockwise triangles.
     * \param[in]  input   Domain description (boundary required).
     * \param[out] out     Triangle mesh (3-node CSR faces).
     * \param[out] message Failure description on false.
     */
    [[nodiscard]] static bool triangulate(const TriangulationInput &input,
                                          IO::MeshDefinition &out,
                                          std::string &message);
};

} // namespace HydroCouple::SDK::Tools

#endif // HYDROCOUPLESDK_TOOLS_TRIANGULATOR_H
