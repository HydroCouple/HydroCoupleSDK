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
    virtual ~IOutputWriter() = default;

    /*!
     * \brief Human-readable writer name for diagnostics (for example "csv").
     */
    [[nodiscard]] virtual std::string name() const = 0;

    /*!
     * \brief Opens resources and writes static metadata.
     * \return 0 on success; nonzero error code otherwise.
     */
    [[nodiscard]] virtual int initialize() = 0;

    /*!
     * \brief Persists one snapshot.
     * \param[in] snapshot Owned value copy of the fields at one time level.
     * \return 0 on success; nonzero error code otherwise.
     */
    [[nodiscard]] virtual int write(const Snapshot &snapshot) = 0;

    /*!
     * \brief Flushes and closes resources.
     * \return 0 on success; nonzero error code otherwise.
     */
    [[nodiscard]] virtual int finalize() = 0;

    /*!
     * \brief Detail message for the most recent nonzero return code.
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
     */
    [[nodiscard]] virtual std::vector<ResultEntry> catalog() const
    {
        return {};
    }
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_OUTPUTWRITER_H
