// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   hdf5ugridwriter.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  HDF5UGRIDWriter — CF-1.11 / UGRID-1.0 output on thread-safe HDF5.
 *
 * Ports the proven openswmm.engine Default2DOutputPlugin pattern to the SDK
 * IO layer: a mesh-topology dummy dataset with \c cf_role, node-coordinate
 * and \c face_node_connectivity datasets written once at initialize(), an
 * unlimited time dimension with chunking + deflate, and extend-and-write
 * hyperslab appends per snapshot. Every field dataset is tagged with
 * \c mesh and \c location (node|edge|face) attributes so UGRID-aware tools
 * (xarray + xugrid, QGIS/MDAL, ugrid-checks) load the file directly.
 *
 * Thread safety comes from the single-writer IOThread — the writer itself
 * performs no locking and must only be driven from one thread. The HDF5 C
 * API is fully encapsulated (pimpl); this header is installable without
 * HDF5 present.
 */

#ifndef HYDROCOUPLESDK_IO_HDF5UGRIDWRITER_H
#define HYDROCOUPLESDK_IO_HDF5UGRIDWRITER_H

#include "hydrocouplesdk/io/outputwriter.h"
#include "hydrocouplesdk/io/meshdefinition.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace HydroCouple::SDK::IO {

/*!
 * \brief CF-1.11 / UGRID-1.0 HDF5 output writer.
 *
 * Fields are registered before initialize(); each snapshot must then carry
 * every registered field (matched by FieldSlice::itemId) with exactly
 * location-count elements. Values of any fixed-width numeric DataKind are
 * converted to double on write through a reused staging buffer (no
 * per-snapshot allocation after the first).
 *
 * \threadsafety Driven exclusively by the IOThread (see IOutputWriter).
 */
class HYDROCOUPLESDK_EXPORT HDF5UGRIDWriter final : public IOutputWriter
{
public:
    /*!
     * \brief Construct with the output path and the mesh to serialize.
     * \param[in] path Destination .h5/.nc file; parents created on
     *                 initialize().
     * \param[in] mesh Mesh description (validated in initialize()).
     */
    HDF5UGRIDWriter(std::filesystem::path path, MeshDefinition mesh);

    ~HDF5UGRIDWriter() override;

    HDF5UGRIDWriter(const HDF5UGRIDWriter &)            = delete;
    HDF5UGRIDWriter &operator=(const HDF5UGRIDWriter &) = delete;

    /*!
     * \brief Registers a time-varying field before initialize().
     * \param[in] itemId       FieldSlice::itemId to match in snapshots
     *                         (also the dataset name).
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
     * \brief Validates the mesh, creates the file, writes conventions,
     *        topology, coordinates, connectivity, and field datasets.
     * \return 0 on success; nonzero HDF5/validation error otherwise.
     */
    [[nodiscard]] int initialize() override;

    /*!
     * \brief Appends one time level: extends /time and every field dataset
     *        by one row and writes the snapshot's values.
     * \return 0 on success; nonzero on missing/mis-sized fields or HDF5
     *         failure.
     */
    [[nodiscard]] int write(const Snapshot &snapshot) override;

    /*!
     * \brief Flushes and closes every HDF5 handle.
     */
    [[nodiscard]] int finalize() override;

    [[nodiscard]] std::string errorMessage() const override;

    /*!
     * \brief The destination path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

private:
    struct Impl; //!< Hides all HDF5 C-API types from the public surface.
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_HDF5UGRIDWRITER_H
