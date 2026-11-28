// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   outputwriter.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  IOutputWriter — the pluggable format-writer interface of the
 *         HydroCouple::SDK::IO layer.
 *
 * The IO layer follows the proven openswmm.engine design: simulation code
 * captures owned Snapshot value copies and hands them to a single IOThread,
 * which delivers them to one or more IOutputWriter implementations. Format
 * writers therefore never see concurrent calls and never block the
 * simulation thread.
 *
 * Error handling uses \c int return codes (0 = success); a writer reports
 * detail through errorMessage(). The IOThread latches the first nonzero
 * code and stops delivering to the failed writer.
 *
 * The SDK ships four writers: CSVWriter (always built), and
 * GeoPackageWriter, HDF5UGRIDWriter and NetCDFUGRIDWriter (built with
 * USE_GEOPACKAGE, USE_HDF5 and USE_NETCDF). Their error codes do not
 * overlap: CSV 1 to 3, HDF5 10 to 24, GeoPackage 30 to 45, NetCDF 50
 * to 65.
 */

#ifndef HYDROCOUPLESDK_IO_OUTPUTWRITER_H
#define HYDROCOUPLESDK_IO_OUTPUTWRITER_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/io/resultentry.h"
#include "hydrocouplesdk/io/snapshot.h"

#include <string>
#include <vector>

namespace HydroCouple::SDK::IO {

/*!
 * \brief Pluggable output-format writer consumed by the IOThread.
 *
 * Lifecycle (all calls arrive on the writer thread, in order):
 *
 * 1. initialize() — open files, write static metadata. Called once before
 *    any snapshot is delivered.
 * 2. write() — called once per Snapshot, in enqueue order.
 * 3. finalize() — flush and close. Called exactly once during IOThread
 *    shutdown, including after an error.
 *
 * \threadsafety Implementations need no internal locking: the IOThread is
 * the only caller and serializes every method.
 */
class HYDROCOUPLESDK_EXPORT IOutputWriter
{
public:
    /*!
     * \brief Virtual destructor. The SDK's writers close any open file here
     *        too, so a writer destroyed without finalize() still releases it.
     */
    virtual ~IOutputWriter() = default;

    /*!
     * \brief Human-readable writer name for diagnostics (for example "csv").
     *
     * \return The name IOThread puts before the message of a latched error
     *         ("\<name\>: \<message\>"). The SDK writers answer "csv",
     *         "geopackage", "hdf5-ugrid" and "netcdf-ugrid".
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /*!
     * \brief Opens resources and writes static metadata.
     *
     * Called once by IOThread::start(), on the thread that calls start().
     * A writer that fails here receives no write() calls but still gets
     * finalize().
     *
     * \return 0 on success; nonzero error code otherwise.
     */
    [[nodiscard]] virtual int initialize() = 0;

    /*!
     * \brief Persists one snapshot.
     *
     * Called on the IOThread worker thread, once per enqueued snapshot, in
     * enqueue order, until the writer first returns nonzero.
     *
     * \param[in] snapshot Owned value copy of the fields at one time level;
     *                     valid only for the duration of the call.
     * \return 0 on success; nonzero error code otherwise.
     */
    [[nodiscard]] virtual int write(const Snapshot &snapshot) = 0;

    /*!
     * \brief Flushes and closes resources.
     *
     * Called once by IOThread::finish() after the worker has drained the
     * queue and joined, on the thread that calls finish(). Must be safe to
     * call on a writer whose initialize() failed; the SDK writers also
     * tolerate a second call.
     *
     * \return 0 on success; nonzero error code otherwise.
     */
    [[nodiscard]] virtual int finalize() = 0;

    /*!
     * \brief Detail message for the most recent nonzero return code.
     *
     * \return The message; its content is unspecified after a call that
     *         returned 0.
     */
    [[nodiscard]] virtual std::string errorMessage() const = 0;

    /*!
     * \brief Catalog of what this writer produced — one entry per recorded
     *        data item — valid after finalize().
     *
     * A Snapshot's FieldSlice carries only id, kind, and shape; units, mesh
     * attachment, location, and the variable's name inside the artifact are
     * writer-side knowledge. Reporting them here is what lets a RunRecorder
     * assemble a manifest that readers can serve without any model code.
     *
     * The default returns an empty catalog, so a writer that does not
     * describe its artifacts (or a third-party writer predating this
     * method) keeps compiling and simply contributes nothing to the
     * manifest.
     *
     * \return One entry per recorded item. \c artifact holds the file name
     *         only and resolves against the manifest's directory, so write
     *         the manifest in the directory that holds the artifacts.
     */
    [[nodiscard]] virtual std::vector<ResultEntry> catalog() const
    {
        return {};
    }
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_OUTPUTWRITER_H
