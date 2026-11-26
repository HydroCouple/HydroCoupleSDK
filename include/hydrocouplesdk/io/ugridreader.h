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
 *
 * Every function needs an SDK built with USE_NETCDF. Without it they fail
 * with "This build has no NetCDF support, so UGRID files cannot be read."
 * (hasVerticalCoordinate() answers false); ugridReadSupported() says which
 * build this is.
 *
 * \threadsafety Each call opens and closes the file itself. Concurrent
 * calls are as safe as the NetCDF library build in use.
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
 * A mesh is a variable whose \c cf_role attribute is "mesh_topology".
 *
 * \param[in]  path    File to inspect.
 * \param[out] message Failure description when the result is empty ("The
 *                     file could not be opened as NetCDF: ...", "The file
 *                     holds no UGRID mesh topology variable."). Not cleared
 *                     on success.
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
 * Coordinates come from the first two variables \c node_coordinates names;
 * elevations from a \c node_z or \c node_elevation attribute, or else a
 * third \c node_coordinates entry. Connectivity may be stored (entity,
 * node) or, when \c face_dimension or \c edge_dimension says so,
 * transposed. Padding must be trailing; \c start_index must be 0 or 1; the
 * fill value defaults to -1. The mesh name becomes MeshDefinition::meshName
 * and the result must pass MeshDefinition::validate().
 *
 * \param[in]  path     File to read.
 * \param[in]  meshName Mesh to read; empty takes the first.
 * \param[out] mesh     Receives the mesh; reset before reading, so partly
 *                      filled on failure.
 * \param[out] message  Failure description when returning false (for
 *                      example "The file holds no mesh named ..." or
 *                      "Invalid or non-trailing connectivity padding in
 *                      face N"); cleared by MeshDefinition::validate() on
 *                      success.
 * \return True when the mesh was read and is valid.
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
 *
 * The elevation of sigma level \f$\sigma\f$ over a face is
 * \f$z = \eta + \sigma\,(h + \eta)\f$, with surface \f$\eta\f$ and depth
 * \f$h\f$ (the CF ocean_sigma_coordinate formula). Values are in the
 * file's units, meters for NetCDFUGRIDWriter files. Filled by
 * readVerticalCoordinate(); written by
 * NetCDFUGRIDWriter::setVerticalCoordinate().
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

    //! Times the surface variable carries; 0 when it is time-independent (a rank-1 surface).
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

    //! Layers implied by the interfaces. \return interfaceSigma.size() - 1, or 0 for fewer than two interfaces.
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
 * \p meshName is only checked to exist: the first variable in the file
 * whose standard name is \c ocean_sigma_coordinate or
 * \c ocean_s_coordinate and that has \c formula_terms is used, whichever
 * mesh it belongs to. The surface is one face-length slice of a
 * (time, face) variable, or the whole of a rank-1 one.
 *
 * \param[in]  path      File to read.
 * \param[in]  meshName  Mesh to read; empty takes the first.
 * \param[in]  timeIndex Which time's surface elevation to take (zero-based);
 *                       an index past the last time takes the last.
 * \param[out] vertical  Receives the coordinate; reset first.
 * \param[out] message   Failure description when returning false; also the
 *                       reason when the mesh simply has no vertical
 *                       coordinate, which is not an error in a 2-D file
 *                       ("The file has no CF vertical coordinate; its mesh
 *                       is two-dimensional."). Not cleared on success.
 * \return True when the coordinate was read, with as many surface values
 *         as depths.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool readVerticalCoordinate(
    const std::filesystem::path &path, const std::string &meshName,
    size_t timeIndex, VerticalCoordinate &vertical, std::string &message);

/*!
 * \brief Whether \p path carries a vertical coordinate for \p meshName.
 *
 * Separate from reading it, so a viewer can offer the layered treatment only
 * when it applies instead of attempting it and recovering.
 *
 * \param[in] path     File to inspect.
 * \param[in] meshName Mesh name; empty takes the first.
 * \return True when readVerticalCoordinate() succeeds for time 0 (it does
 *         the full read).
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
 *
 * \return True when the SDK was built with USE_NETCDF.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool ugridReadSupported();

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_UGRIDREADER_H
