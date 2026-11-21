// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   triangulator.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Constrained Delaunay triangulation (plan §7b.1) over MeshKit's
 *         constrained Delaunay kernel (Apache-2.0), the one the OpenSWMM GUI
 *         meshes with.
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
#include "hydrocouplesdk/tools/meshprogress.h"
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
     * channels, levees). Segments that cross each other, or a ring, are
     * split at the crossing, which becomes a node.
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
 * \brief Constrained Delaunay triangulator (MeshKit-backed).
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

    /*!
     * \brief Triangulates \p input, reporting to \p progress as it goes.
     *
     * The same triangulation as the three-argument form, watched. An empty
     * \p progress is exactly the unwatched call, and a \p progress that
     * always returns true changes nothing about the mesh — observing a run
     * must not alter it.
     *
     * \param[in]  input    Domain description (boundary required).
     * \param[out] out      Triangle mesh; left empty if canceled.
     * \param[out] message  Failure or cancellation description on false.
     * \param[in]  progress Called at each phase and through the loops this
     *                      library runs itself; returning false abandons
     *                      the run. See MeshProgress for what can and
     *                      cannot be interrupted.
     */
    [[nodiscard]] static bool triangulate(const TriangulationInput &input,
                                          IO::MeshDefinition &out,
                                          std::string &message,
                                          const MeshProgress &progress);
};

} // namespace HydroCouple::SDK::Tools

#endif // HYDROCOUPLESDK_TOOLS_TRIANGULATOR_H
