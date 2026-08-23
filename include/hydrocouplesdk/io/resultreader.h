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
 */
class HYDROCOUPLESDK_EXPORT IResultReader
{
public:
    virtual ~IResultReader() = default;

    /*!
     * \brief Format token this reader serves ("csv", "hdf5_ugrid", …).
     */
    [[nodiscard]] virtual std::string format() const = 0;

    /*!
     * \brief Opens an artifact for reading.
     * \param[in]  path    Artifact file (already resolved against the
     *                     manifest directory).
     * \param[out] message Failure description when returning false.
     */
    [[nodiscard]] virtual bool open(const std::filesystem::path &path,
                                    std::string &message) = 0;

    /*!
     * \brief Copies a hyperslab of a recorded variable into \p into.
     *
     * \param[in]  entry   Catalog entry describing the variable.
     * \param[in]  start   Per-dimension origin, parallel to entry.shape.
     * \param[in]  count   Per-dimension extent, parallel to entry.shape.
     * \param[in]  into    Destination descriptor; its kind must match the
     *                     entry's kind and its element count the slab's.
     * \param[out] message Failure description when returning false.
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
     * (HDF5, NetCDF) override to return the stored values.
     */
    [[nodiscard]] virtual bool readTimes(const ResultEntry &entry,
                                         std::vector<double> &times,
                                         std::string &message);
};

/*!
 * \brief Creates the reader that serves a given format.
 *
 * \param[in]  format  Format token from a ResultEntry.
 * \param[out] message Explains an unsupported or unbuilt format.
 * \return The reader, or nullptr when the format has no reader in this
 *         build (the message then names the missing feature).
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::unique_ptr<IResultReader>
createResultReader(const std::string &format, std::string &message);

/*!
 * \brief True when this build can read the given format.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool resultFormatSupported(
    const std::string &format);

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_RESULTREADER_H
