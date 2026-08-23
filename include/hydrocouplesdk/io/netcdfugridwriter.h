// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   netcdfugridwriter.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  NetCDFUGRIDWriter — CF-1.11 / UGRID-1.0 output through the
 *         NetCDF-4 C API.
 *
 * Expresses the same UGRID layout as HDF5UGRIDWriter through the netCDF-4
 * data model (the file is HDF5 underneath): mesh-topology dummy variable
 * with \c cf_role, node coordinates, fill-value-padded
 * \c face_node_connectivity, an unlimited \c time dimension, and deflated
 * time-varying field variables tagged \c mesh + \c location.
 *
 * This writer supersedes the legacy \c threadsafenetcdf module: its ~100
 * <tt>\#pragma omp critical</tt> sections existed to serialize concurrent
 * NetCDF calls, a job the single-writer IOThread now performs by design —
 * so the wrappers are retired rather than revived (plan §4.5).
 *
 * The NetCDF C API is fully encapsulated (pimpl); this header is
 * installable without NetCDF present.
 *
 * \threadsafety Driven exclusively by the IOThread (see IOutputWriter).
 */

#ifndef HYDROCOUPLESDK_IO_NETCDFUGRIDWRITER_H
#define HYDROCOUPLESDK_IO_NETCDFUGRIDWRITER_H

#include "hydrocouplesdk/io/outputwriter.h"
#include "hydrocouplesdk/io/meshdefinition.h"

#include <filesystem>
#include <memory>
#include <string>

namespace HydroCouple::SDK::IO {

/*!
 * \brief CF-1.11 / UGRID-1.0 NetCDF-4 output writer.
 *
 * Fields are registered before initialize(); each snapshot must then carry
 * every registered field (matched by FieldSlice::itemId) with exactly
 * location-count elements. Values of any fixed-width numeric DataKind are
 * converted to double on write through a reused staging buffer.
 *
 * \threadsafety Driven exclusively by the IOThread (see IOutputWriter).
 */
class HYDROCOUPLESDK_EXPORT NetCDFUGRIDWriter final : public IOutputWriter
{
public:
    /*!
     * \brief Construct with the output path and the mesh to serialize.
     * \param[in] path Destination .nc file; parents created on initialize().
     * \param[in] mesh Mesh description (validated in initialize()).
     */
    NetCDFUGRIDWriter(std::filesystem::path path, MeshDefinition mesh);

    ~NetCDFUGRIDWriter() override;

    NetCDFUGRIDWriter(const NetCDFUGRIDWriter &)            = delete;
    NetCDFUGRIDWriter &operator=(const NetCDFUGRIDWriter &) = delete;

    /*!
     * \brief Registers a time-varying field before initialize().
     * \param[in] itemId       FieldSlice::itemId to match in snapshots
     *                         (also the variable name).
     * \param[in] location     UGRID location the values live on.
     * \param[in] units        CF \c units attribute ("" omits it).
     * \param[in] standardName CF \c standard_name attribute ("" omits it).
     */
    void addField(const std::string &itemId, MeshLocation location,
                  const std::string &units = "",
                  const std::string &standardName = "");

    // ── IOutputWriter ─────────────────────────────────────────────────────

    [[nodiscard]] std::string name() const override;

    /*!
     * \brief Validates the mesh, creates the NetCDF-4 file, and defines
     *        conventions, topology, coordinates, connectivity, and fields.
     * \return 0 on success; nonzero NetCDF/validation error otherwise.
     */
    [[nodiscard]] int initialize() override;

    /*!
     * \brief Appends one time level to \c time and every field variable.
     * \return 0 on success; nonzero on missing/mis-sized fields or NetCDF
     *         failure.
     */
    [[nodiscard]] int write(const Snapshot &snapshot) override;

    /*!
     * \brief Syncs and closes the file.
     */
    [[nodiscard]] int finalize() override;

    [[nodiscard]] std::string errorMessage() const override;

    /*!
     * \brief The destination path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

private:
    struct Impl; //!< Hides all NetCDF C-API types from the public surface.
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_NETCDFUGRIDWRITER_H
