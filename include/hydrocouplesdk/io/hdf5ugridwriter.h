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
 * HDF5 present, but the class is compiled into HydroCoupleSDK only when it
 * is built with USE_HDF5 (macro HYDROCOUPLESDK_HAVE_HDF5).
 *
 * The file is plain HDF5 with the UGRID attributes; the NetCDF library can
 * open it, which is how the HDF5 result reader reads its mesh back (so
 * reading the mesh also needs an SDK built with USE_NETCDF).
 * Unlike NetCDFUGRIDWriter, the \c node_coordinates attribute names only
 * the x and y datasets, even when a \c _node_z dataset is written.
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
 * per-snapshot allocation after the first). Fields of a snapshot that were
 * not registered are ignored.
 *
 * \threadsafety Driven exclusively by the IOThread (see IOutputWriter).
 */
class HYDROCOUPLESDK_EXPORT HDF5UGRIDWriter final : public IOutputWriter
{
public:
    /*!
     * \brief Construct with the output path and the mesh to serialize.
     * \param[in] path Destination .h5/.nc file; parents created on
     *                 initialize(), an existing file truncated.
     * \param[in] mesh Mesh description (validated in initialize()); copied.
     */
    HDF5UGRIDWriter(std::filesystem::path path, MeshDefinition mesh);

    //! Closes every open handle (calls finalize()).
    ~HDF5UGRIDWriter() override;

    //! Deleted: the writer owns HDF5 handles.
    HDF5UGRIDWriter(const HDF5UGRIDWriter &)            = delete;
    //! Deleted: the writer owns HDF5 handles.
    HDF5UGRIDWriter &operator=(const HDF5UGRIDWriter &) = delete;

    /*!
     * \brief Registers a time-varying field before initialize().
     * \param[in] itemId       FieldSlice::itemId to match in snapshots
     *                         (also the dataset name).
     * \param[in] location     UGRID location the values live on.
     * \param[in] units        CF \c units attribute ("" omits it); also the
     *                         catalog entry's units.
     * \param[in] standardName CF \c standard_name attribute ("" omits it);
     *                         also the catalog entry's description.
     *
     * Registering after initialize() adds a field that has no dataset, and
     * the next write() fails on it.
     */
    void addField(const std::string &itemId, MeshLocation location,
                  const std::string &units = "",
                  const std::string &standardName = "");

    // ── IOutputWriter ─────────────────────────────────────────────────────

    /*!
     * \brief The writer name used in IOThread error messages.
     *
     * \return "hdf5-ugrid".
     */
    [[nodiscard]] std::string name() const override;

    /*!
     * \brief Validates the mesh, creates the file, writes conventions,
     *        topology, coordinates, connectivity, and field datasets.
     * \return 0 on success; nonzero HDF5/validation error otherwise: 10 the
     *         mesh fails MeshDefinition::validate(), 11 the file cannot be
     *         created, 12 to 15 a topology, coordinate, connectivity or time
     *         dataset cannot be created, 16 a field is registered on a
     *         location the mesh has none of, 17 a field dataset cannot be
     *         created.
     */
    [[nodiscard]] int initialize() override;

    /*!
     * \brief Appends one time level: extends /time and every field dataset
     *        by one row and writes the snapshot's values.
     *
     * \param[in] snapshot The time level; must carry every registered field.
     * \return 0 on success; nonzero on missing/mis-sized fields or HDF5
     *         failure: 20 /time cannot be extended, 21 a registered field is
     *         missing, 22 a field's element count differs from its
     *         location count, 23 a field has an unsupported kind, 24 a field
     *         dataset cannot be extended. The time level counts toward the
     *         catalog even when the write fails.
     */
    [[nodiscard]] int write(const Snapshot &snapshot) override;

    /*!
     * \brief Flushes and closes every HDF5 handle.
     *
     * \return Always 0; HDF5 close errors are not reported.
     */
    [[nodiscard]] int finalize() override;

    /*!
     * \brief Detail message of the last failure.
     *
     * \return The message naming the dataset or field that failed.
     */
    [[nodiscard]] std::string errorMessage() const override;

    /*!
     * \brief Catalog of the datasets written: one entry per field, shaped
     *        { time, locationCount } with the mesh name and location the
     *        field is attached to.
     *
     * \return Entries in registration order with format "hdf5_ugrid", kind
     *         Float64, dimensions { "time", location }, the field's units
     *         and standard name (as description), and the recorded time
     *         bounds in units "julian_day".
     */
    [[nodiscard]] std::vector<ResultEntry> catalog() const override;

    /*!
     * \brief The destination path.
     *
     * \return The path given to the constructor.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

private:
    struct Impl; //!< Hides all HDF5 C-API types from the public surface.
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_HDF5UGRIDWRITER_H
