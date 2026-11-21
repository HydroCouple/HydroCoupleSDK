// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   ugridreader.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  Reads a UGRID mesh out of a NetCDF file into a MeshDefinition.
 *
 * The counterpart the writers lacked: the SDK could produce UGRID files that
 * nothing in the ecosystem could read back, so every consumer — a viewer, a
 * restart, a second model — had to write its own NetCDF walker.
 *
 * Driven by the UGRID conventions rather than by the writer's variable names:
 * the topology variable is found by its \c cf_role, and the coordinate and
 * connectivity variables by the attributes it names. A mesh written by
 * another tool therefore reads as readily as one of ours.
 */

#ifndef HYDROCOUPLESDK_IO_UGRIDREADER_H
#define HYDROCOUPLESDK_IO_UGRIDREADER_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/io/meshdefinition.h"

#include <filesystem>
#include <string>
#include <vector>

namespace HydroCouple::SDK::IO {

/*!
 * \brief The UGRID mesh names a file holds.
 *
 * A file may carry several meshes — a 1-D network and a 2-D floodplain, say.
 *
 * \param[in]  path    File to inspect.
 * \param[out] message Failure description when the result is empty.
 * \return Mesh (topology variable) names, in file order.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::vector<std::string> ugridMeshNames(
    const std::filesystem::path &path, std::string &message);

/*!
 * \brief Reads one UGRID mesh into \p mesh.
 *
 * Honors the conventions' \c start_index (a 1-based file is converted to
 * 0-based here, once, rather than by every caller) and drops the
 * \c _FillValue padding a rectangular connectivity array carries for faces
 * with fewer nodes than the widest one.
 *
 * \param[in]  path     File to read.
 * \param[in]  meshName Mesh to read; empty takes the first.
 * \param[out] mesh     Receives the mesh.
 * \param[out] message  Failure description when returning false.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool readUGRIDMesh(
    const std::filesystem::path &path, const std::string &meshName,
    MeshDefinition &mesh, std::string &message);

/*!
 * \brief A CF vertical coordinate attached to a UGRID mesh.
 *
 * The layered half of a layered UGRID file: a 2-D topology carries the plan
 * view, and this carries the water column over it. Everything is in CF's
 * conventions rather than any one model's, because that is what the file
 * says — \c sigma runs 0 at the surface to -1 at the bed, and \c depth is
 * positive **down**. A consumer whose own sigma runs the other way converts
 * once, on its own side.
 */
struct VerticalCoordinate
{
    /*!
     * \brief Sigma at the layer interfaces: \c layerCount() + 1 values,
     *        0 first (surface) descending to -1 (bed).
     */
    std::vector<double> interfaceSigma;

    //! Sigma at the layer centers, as \c formula_terms names it.
    std::vector<double> layerSigma;

    //! Bed depth per face, positive down.
    std::vector<double> depth;

    //! Water surface elevation per face at the time read, positive up.
    std::vector<double> surface;

    //! The variable \c formula_terms named for the surface elevation.
    std::string surfaceVariable;

    //! Times the surface variable carries; 0 when it is time-independent.
    size_t timeCount = 0;

    /*!
     * \brief Whether the interfaces were derived rather than read.
     *
     * True when the file carried only layer centers. Midpointing them is
     * exact for a uniform distribution and an approximation for a stretched
     * one, so a caller that cares — a volume calculation, as opposed to a
     * picture — can tell the difference.
     */
    bool interfacesAreDerived = false;

    //! Layers implied by the interfaces.
    [[nodiscard]] int layerCount() const noexcept
    {
        return interfaceSigma.size() >= 2
                   ? static_cast<int>(interfaceSigma.size()) - 1
                   : 0;
    }
};

/*!
 * \brief Reads the vertical coordinate of a layered UGRID mesh.
 *
 * Found through the conventions, not through any writer's variable names:
 * the coordinate is the variable whose \c standard_name is an
 * \c ocean_sigma_coordinate, and \c formula_terms names the sigma, surface
 * and depth variables it is built from. Interfaces come from a CF \c bounds
 * attribute when there is one; failing that, from a variable dimensioned one
 * longer than the layers whose values run 0 to -1, which is what an
 * interface sigma *is*; failing that, from the layer centers.
 *
 * \param[in]  path      File to read.
 * \param[in]  meshName  Mesh to read; empty takes the first.
 * \param[in]  timeIndex Which time's surface elevation to take.
 * \param[out] vertical  Receives the coordinate.
 * \param[out] message   Failure description when returning false; also the
 *                       reason when the mesh simply has no vertical
 *                       coordinate, which is not an error in a 2-D file.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool readVerticalCoordinate(
    const std::filesystem::path &path, const std::string &meshName,
    size_t timeIndex, VerticalCoordinate &vertical, std::string &message);

/*!
 * \brief Whether \p path carries a vertical coordinate for \p meshName.
 *
 * Separate from reading it, so a viewer can offer the layered treatment only
 * when it applies instead of attempting it and recovering.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool hasVerticalCoordinate(
    const std::filesystem::path &path, const std::string &meshName);

/*!
 * \brief True when this build can read UGRID files.
 *
 * NetCDF is optional at build time, so a caller can offer the feature only
 * when it exists rather than failing at the point of use. Answered at
 * runtime, so a consumer linking a prebuilt SDK need not know how it was
 * configured.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool ugridReadSupported();

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_UGRIDREADER_H
