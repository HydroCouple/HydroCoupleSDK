// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   resultreader.h
 * \author Caleb Buahin
 * \version 2.1.0
 * \brief  IResultReader — serves recorded values from a run's artifacts.
 *
 * A reader answers hyperslab requests against one artifact, guided
 * entirely by the ResultEntry that describes it: nothing about shape,
 * kind, variable name, or time coordinate is inferred from the file. That
 * is what lets a run be reopened by tools that have never seen the model
 * that produced it.
 *
 * Readers are selected by the entry's \c format through the registry
 * below. CSV is always available; the HDF5, NetCDF, and GeoPackage
 * readers are compiled under the same feature gates as their writers, so
 * an SDK built without HDF5 reports a clear "no reader for format" error
 * rather than failing to link.
 *
 * \threadsafety One reader instance belongs to one thread. Distinct
 * readers over distinct artifacts may run concurrently.
 */

#ifndef HYDROCOUPLESDK_IO_RESULTREADER_H
#define HYDROCOUPLESDK_IO_RESULTREADER_H

#include "hydrocouplesdk/io/resultentry.h"

#include <filesystem>
#include <memory>
#include <span>
#include <string>

namespace HydroCouple::SDK::IO {

/*!
 * \brief Reads recorded values out of one run artifact.
 *
 * Obtain one from createResultReader(), open() it on the artifact path
 * (RunManifest::artifactPath()), then read() hyperslabs described by the
 * artifact's catalog entries. ResultsModelComponent does exactly this, one
 * reader per artifact. The SDK's readers serve values as doubles and
 * convert them into a destination of kind Float64, Float32, Int32 or
 * Int64; since every writer except CSV records Float64, request the
 * entry's kind.
 */
class HYDROCOUPLESDK_EXPORT IResultReader
{
public:
    //! Closes the artifact if it is open.
    virtual ~IResultReader() = default;

    /*!
     * \brief Format token this reader serves ("csv", "hdf5_ugrid", …).
     *
     * \return "csv", "hdf5_ugrid", "netcdf_ugrid" or "geopackage".
     */
    [[nodiscard]] virtual std::string format() const = 0;

    /*!
     * \brief Opens an artifact for reading.
     *
     * Call once per reader. The CSV reader reads and indexes the whole file
     * here (a malformed number makes std::stod throw); the HDF5, NetCDF and
     * GeoPackage readers open the file read-only and read on demand.
     *
     * \param[in]  path    Artifact file (already resolved against the
     *                     manifest directory).
     * \param[out] message Failure description when returning false (for
     *                     example "cannot open result artifact \"...\"");
     *                     cleared on success.
     * \return True when the artifact is open.
     */
    [[nodiscard]] virtual bool open(const std::filesystem::path &path,
                                    std::string &message) = 0;

    /*!
     * \brief Copies a hyperslab of a recorded variable into \p into.
     *
     * The selection is validated against the entry first: start and count
     * must have the entry's rank, stay within its shape and cover as many
     * elements as the destination holds. Values are written contiguously in
     * row-major selection order (destination strides are ignored).
     *
     * \param[in]  entry   Catalog entry describing the variable.
     * \param[in]  start   Per-dimension origin, parallel to entry.shape.
     * \param[in]  count   Per-dimension extent, parallel to entry.shape.
     * \param[in]  into    Destination descriptor; its kind must match the
     *                     entry's kind and its element count the slab's.
     *                     Host memory, of kind Float64, Float32, Int32 or
     *                     Int64.
     * \param[out] message Failure description when returning false ("result
     *                     read of ...: selection out of range on dimension
     *                     N", "... destination kind ... differs from
     *                     recorded kind ...", a missing variable, or a
     *                     library failure).
     * \return True when the slab was copied.
     */
    [[nodiscard]] virtual bool read(const ResultEntry &entry,
                                    std::span<const int64_t> start,
                                    std::span<const int64_t> count,
                                    const BufferDescriptor &into,
                                    std::string &message) = 0;

    /*!
     * \brief Time coordinate values of a recorded variable, when it has a
     *        time dimension.
     *
     * The default reconstructs a uniform axis from the entry's recorded
     * bounds; readers whose artifacts store the coordinate explicitly
     * (HDF5, NetCDF) override to return the stored values. The CSV reader
     * returns the distinct times of the item's rows; the GeoPackage reader
     * uses the default.
     *
     * The default axis holds \c time.count values, the i-th being
     * \f$t_0 + i\,(t_1 - t_0)/(n - 1)\f$ for start \f$t_0\f$, end
     * \f$t_1\f$ and count \f$n > 1\f$ (just \f$t_0\f$ when
     * \f$n = 1\f$).
     *
     * \param[in]  entry   Catalog entry describing the variable.
     * \param[out] times   The time values, in the entry's time units.
     * \param[out] message Failure description when returning false ("result
     *                     \"...\" records no time axis" for the default);
     *                     cleared on success.
     * \return True when \p times was filled.
     */
    [[nodiscard]] virtual bool readTimes(const ResultEntry &entry,
                                         std::vector<double> &times,
                                         std::string &message);

    /*!
     * \brief The mesh an entry's values are attached to, when the artifact
     *        carries one.
     *
     * The catalog already names the attachment — \c mesh and \c location —
     * so that a reader would not have to infer it; this is what reads it
     * back. Without it a reopened run can be plotted and cannot be mapped,
     * and every consumer that wants the geometry has to open the artifact a
     * second time and pair the two halves itself.
     *
     * The default reports that this artifact carries no geometry, and says
     * so with a reason rather than an error: a CSV of values is a complete
     * and valid recording of values.
     *
     * The HDF5 and NetCDF readers read the mesh the entry's \c mesh names
     * with readUGRIDMesh() (so both need NetCDF support) and refuse an entry
     * that names none. The GeoPackage reader returns the \c mesh_nodes
     * points as a node-only mesh, named after the entry's mesh or "mesh".
     *
     * \param[in]  entry   Catalog entry describing the variable.
     * \param[out] mesh    Receives the mesh when one is read.
     * \param[out] message Why there is no mesh, or what went wrong.
     * \return True when \p mesh was filled.
     */
    [[nodiscard]] virtual bool readMesh(const ResultEntry &entry,
                                        MeshDefinition &mesh,
                                        std::string &message);
};

/*!
 * \brief Creates the reader that serves a given format.
 *
 * "csv" is always available; "hdf5_ugrid", "netcdf_ugrid" and "geopackage"
 * need an SDK built with USE_HDF5, USE_NETCDF and USE_GEOPACKAGE.
 *
 * \param[in]  format  Format token from a ResultEntry.
 * \param[out] message Explains an unsupported or unbuilt format: "this SDK
 *                     build has no reader for result format \"...\" —
 *                     rebuild with the matching feature enabled" or
 *                     "unknown result format \"...\""; cleared on success.
 * \return The reader (unopened, owned by the caller), or nullptr when the
 *         format has no reader in this build (the message then names the
 *         missing feature).
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::unique_ptr<IResultReader>
createResultReader(const std::string &format, std::string &message);

/*!
 * \brief True when this build can read the given format.
 *
 * \param[in] format Format token.
 * \return True when createResultReader() would return a reader.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool resultFormatSupported(
    const std::string &format);

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_RESULTREADER_H
