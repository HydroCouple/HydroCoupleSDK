// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   meshdefinition.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  MeshDefinition and MeshLocation — the UGRID-congruent mesh
 *         vocabulary shared by the HDF5, NetCDF, and GeoPackage writers.
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
 * \threadsafety Immutable after construction by convention.
 */
struct HYDROCOUPLESDK_EXPORT MeshDefinition
{
    std::string meshName = "mesh";  //!< UGRID mesh variable name.

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

    std::vector<std::array<int64_t, 2>> edgeNodes; //!< Optional edge endpoints.

    [[nodiscard]] int64_t nodeCount() const noexcept
    {
        return static_cast<int64_t>(nodeX.size());
    }
    [[nodiscard]] int64_t faceCount() const noexcept
    {
        return faceNodeOffsets.empty()
                   ? 0
                   : static_cast<int64_t>(faceNodeOffsets.size()) - 1;
    }
    [[nodiscard]] int64_t edgeCount() const noexcept
    {
        return static_cast<int64_t>(edgeNodes.size());
    }

    /*!
     * \brief Widest face (columns of the padded connectivity matrix).
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
     * \param[out] message Failure description when returning false.
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
