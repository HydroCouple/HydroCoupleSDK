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
 * Honours the conventions' \c start_index (a 1-based file is converted to
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
