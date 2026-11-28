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
 * Output node order: the distinct input points in first-occurrence order
 * (boundary ring with its densification points, holes, constraint
 * segments, interior points), then any constraint crossing points. The
 * mesh is named "triangulation" and has no nodeZ; sample elevations with
 * TerrainSampler. The densification never adds interior points: control
 * interior size with \c interiorPoints.
 *
 * Part of the HydroCoupleTools library (BUILD_TOOLS=ON). MeshKit is linked
 * privately into it, so consumers do not need MeshKit.
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
     * point. Required (at least 3 points). Triangles are kept when their
     * centroid is inside the ring by the crossing-number rule.
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
     * before triangulation (into ceil(length / maxEdgeLength) equal parts),
     * in the coordinates' length unit.
     */
    double maxEdgeLength = 0.0;
};

/*!
 * \brief Constrained Delaunay triangulator (MeshKit-backed).
 *
 * Failure messages: "boundary ring needs at least 3 points", "hole ring
 * needs at least 3 points", "triangulation failed: ..." (MeshKit's reason,
 * or a constraint that could not be recovered), "triangulation produced no
 * triangles", and "cancelled" when a MeshProgress callback stops the run.
 */
class HYDROCOUPLETOOLS_EXPORT Triangulator
{
public:
    /*!
     * \brief Triangulates \p input into counter-clockwise triangles.
     * \param[in]  input   Domain description (boundary required).
     * \param[out] out     Triangle mesh (3-node CSR faces).
     * \param[out] message Failure description on false; cleared on success.
     * \return True when \p out holds at least one triangle.
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
     *                      cannot be interrupted. The final call, (1.0,
     *                      "done"), is a report: its return value is
     *                      ignored and the mesh is kept.
     * \return True when \p out holds at least one triangle; false on
     *         failure or cancellation.
     */
    [[nodiscard]] static bool triangulate(const TriangulationInput &input,
                                          IO::MeshDefinition &out,
                                          std::string &message,
                                          const MeshProgress &progress);
};

} // namespace HydroCouple::SDK::Tools

#endif // HYDROCOUPLESDK_TOOLS_TRIANGULATOR_H
