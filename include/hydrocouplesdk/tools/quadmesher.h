// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   quadmesher.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Quad and quad-dominant meshing (plan §7b.2).
 *
 * Structured quad patches come from CurvilinearGridGenerator; this header
 * covers general domains through indirect conversion: greedy pair-merge of
 * a triangulation's adjacent triangles into quads, best-quality pairs
 * first, subject to a quality threshold — leftover triangles remain, so
 * the output is a mixed-element (quad-dominant) UGRID mesh whose padded
 * connectivity the IO writers already handle.
 *
 * Part of the HydroCoupleTools library (BUILD_TOOLS=ON).
 *
 * \threadsafety Stateless static functions; outputs are caller-owned.
 */

#ifndef HYDROCOUPLESDK_TOOLS_QUADMESHER_H
#define HYDROCOUPLESDK_TOOLS_QUADMESHER_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplesdk/tools/meshprogress.h"

#include <string>

namespace HydroCouple::SDK::Tools {

/*!
 * \brief Indirect tri→quad-dominant conversion.
 *
 * Output faces: the merged quads first, best quality first, then the
 * triangles that stayed, in input order. Nodes (x, y, z) and the mesh name
 * are copied unchanged. Two triangles merge only across an edge shared by
 * exactly those two faces and traversed in opposite directions (consistent
 * winding). Pairs of equal quality are taken in an order that depends on
 * the standard library's hash map, so on a regular grid the chosen pairs
 * may differ between platforms. Node indexes must be below 2^32 (edge keys
 * pack two indexes into 64 bits).
 */
class HYDROCOUPLETOOLS_EXPORT QuadMesher
{
public:
    /*!
     * \brief Shape quality of the quad (a, b, c, d) in [0, 1]:
     *        1 − max corner deviation from 90° / 90°, and 0 for
     *        non-convex or degenerate quads.
     *
     * The corners must be listed counter-clockwise: a clockwise quad scores
     * 0. A square scores 1.
     *
     * \param[in] ax Corner a, x.
     * \param[in] ay Corner a, y.
     * \param[in] bx Corner b, x.
     * \param[in] by Corner b, y.
     * \param[in] cx Corner c, x.
     * \param[in] cy Corner c, y.
     * \param[in] dx Corner d, x.
     * \param[in] dy Corner d, y.
     * \return The quality.
     */
    [[nodiscard]] static double quadQuality(double ax, double ay, double bx,
                                            double by, double cx, double cy,
                                            double dx, double dy);

    /*!
     * \brief Merges adjacent triangle pairs into quads, best quality
     *        first.
     * \param[in]  triangleMesh Triangle mesh (all faces 3-node).
     * \param[in]  minQuality   Merge threshold in [0, 1]; pairs whose quad
     *                          quality falls below it stay triangles.
     * \param[out] out          Mixed quad/triangle mesh (same nodes).
     * \param[out] message      Failure description on false: a
     *                          MeshDefinition::validate() message, "input
     *                          mesh has no faces", "face F is not a
     *                          triangle". Cleared on success.
     * \return True on success.
     */
    [[nodiscard]] static bool triToQuadDominant(
        const IO::MeshDefinition &triangleMesh, double minQuality,
        IO::MeshDefinition &out, std::string &message);

    /*!
     * \brief Merges as above, reporting to \p progress as it goes.
     *
     * Every phase of the merge is this library's own loop, so unlike the
     * triangulator a cancellation here lands promptly wherever it is asked
     * for.
     *
     * \param[in]  triangleMesh Triangle mesh (all faces 3-node).
     * \param[in]  minQuality   Merge threshold in [0, 1].
     * \param[out] out          Mixed mesh; left empty if canceled.
     * \param[out] message      Failure or cancellation description
     *                          ("cancelled").
     * \param[in]  progress     Returning false abandons the merge, except at
     *                          the final (1.0, "done") report, whose return
     *                          is ignored.
     * \return True on success; false on failure or cancellation.
     */
    [[nodiscard]] static bool triToQuadDominant(
        const IO::MeshDefinition &triangleMesh, double minQuality,
        IO::MeshDefinition &out, std::string &message,
        const MeshProgress &progress);
};

} // namespace HydroCouple::SDK::Tools

#endif // HYDROCOUPLESDK_TOOLS_QUADMESHER_H
