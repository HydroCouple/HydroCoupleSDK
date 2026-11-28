// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   meshdefinition.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  MeshDefinition and MeshLocation — the UGRID-congruent mesh
 *         vocabulary shared by the HDF5, NetCDF, and GeoPackage writers.
 *
 * Also the form in which the UGRID reader (readUGRIDMesh()), the result
 * readers (IResultReader::readMesh()) and mesh packages (MeshPackage) hand
 * a mesh over. Indexes are zero-based.
 */

#ifndef HYDROCOUPLESDK_IO_MESHDEFINITION_H
#define HYDROCOUPLESDK_IO_MESHDEFINITION_H

#include "hydrocouplesdk/export.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace HydroCouple::SDK::IO {

/*!
 * \brief UGRID-congruent structure-of-arrays mesh description.
 *
 * Deliberately mirrors the standard's IMeshView vocabulary (node coordinate
 * spans, CSR face→node connectivity) so mesh views serialize without
 * transformation. Faces and edges are optional: a node-only "mesh" (point
 * cloud / 1-D network nodes) is valid.
 *
 * Coordinates carry no units of their own; a MeshPackage holds them in
 * meters. Face node order is kept as given: validate() does not check
 * orientation, convexity or a minimum node count per face.
 *
 * \threadsafety Immutable after construction by convention.
 */
struct HYDROCOUPLESDK_EXPORT MeshDefinition
{
    std::string meshName = "mesh";  //!< UGRID mesh variable name; prefixes every variable a UGRID writer creates.

    std::vector<double> nodeX;      //!< Node x coordinates (required).
    std::vector<double> nodeY;      //!< Node y coordinates (required, same length).
    std::vector<double> nodeZ;      //!< Optional node elevations.

    /*!
     * CSR face→node connectivity: face f uses node indexes
     * faceNodes[faceNodeOffsets[f] .. faceNodeOffsets[f+1]). Empty when the
     * mesh has no faces.
     */
    std::vector<int64_t> faceNodeOffsets; //!< Size faceCount()+1 when non-empty.
    std::vector<int64_t> faceNodes;       //!< Concatenated node indexes.

    std::vector<std::array<int64_t, 2>> edgeNodes; //!< Optional edge endpoints (two node indexes per edge).

    /*!
     * \brief Number of nodes.
     *
     * \return The length of nodeX.
     */
    [[nodiscard]] int64_t nodeCount() const noexcept
    {
        return static_cast<int64_t>(nodeX.size());
    }
    /*!
     * \brief Number of faces.
     *
     * \return faceNodeOffsets.size() - 1, or 0 when faceNodeOffsets is
     *         empty.
     */
    [[nodiscard]] int64_t faceCount() const noexcept
    {
        return faceNodeOffsets.empty()
                   ? 0
                   : static_cast<int64_t>(faceNodeOffsets.size()) - 1;
    }
    /*!
     * \brief Number of edges.
     *
     * \return The length of edgeNodes.
     */
    [[nodiscard]] int64_t edgeCount() const noexcept
    {
        return static_cast<int64_t>(edgeNodes.size());
    }

    /*!
     * \brief Widest face (columns of the padded connectivity matrix).
     *
     * \return The largest node count of any face; 0 when there are no faces.
     */
    [[nodiscard]] int64_t maxNodesPerFace() const noexcept
    {
        int64_t widest = 0;
        for (size_t f = 0; f + 1 < faceNodeOffsets.size(); ++f)
            widest = std::max(widest,
                              faceNodeOffsets[f + 1] - faceNodeOffsets[f]);
        return widest;
    }

    /*!
     * \brief Structural validation (coordinate lengths, CSR monotonicity,
     *        index bounds).
     *
     * Requires non-empty nodeX and nodeY of equal length, nodeZ empty or of
     * the same length, faceNodeOffsets (when present) starting at 0, ending
     * at faceNodes.size() and non-decreasing, and every face and edge node
     * index in [0, nodeCount()).
     *
     * \param[out] message Failure description when returning false (each
     *                     begins "MeshDefinition: "); cleared on success.
     * \return True when the mesh is structurally valid.
     */
    [[nodiscard]] bool validate(std::string &message) const;
};

/*!
 * \brief UGRID data location of a field.
 */
enum class MeshLocation
{
    Node = 0, //!< One value per mesh node.
    Edge = 1, //!< One value per mesh edge.
    Face = 2  //!< One value per mesh face.
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_MESHDEFINITION_H
