// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   csvwriter.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  CSVWriter — the reference IOutputWriter implementation.
 *
 * Emits snapshots in long (tidy) format, one value per row:
 *
 * \code
 * time,item,index,value
 * 100.0,flow,0,1.25
 * 100.0,flow,1,2.5
 * \endcode
 *
 * The long format handles any field shape and mixed item sets per snapshot
 * without schema negotiation, loads directly into pandas/R, and doubles as
 * the test workhorse for the IO layer.
 */

#ifndef HYDROCOUPLESDK_IO_CSVWRITER_H
#define HYDROCOUPLESDK_IO_CSVWRITER_H

#include "hydrocouplesdk/io/outputwriter.h"

#include <filesystem>
#include <fstream>

namespace HydroCouple::SDK::IO {

/*!
 * \brief Long-format CSV output writer.
 *
 * Numeric fields are written element-by-element with full double
 * precision; Boolean fields as 0/1. Rows appear in capture order within a
 * snapshot and snapshot order across time.
 *
 * \threadsafety Driven exclusively by the IOThread; no internal locking
 * (see IOutputWriter).
 */
class HYDROCOUPLESDK_EXPORT CSVWriter final : public IOutputWriter
{
public:
    /*!
     * \brief Construct with the output file path.
     * \param[in] path Destination CSV file; parent directories are created
     *                 by initialize().
     */
    explicit CSVWriter(std::filesystem::path path);

    ~CSVWriter() override = default;

    // ── IOutputWriter ─────────────────────────────────────────────────────

    [[nodiscard]] std::string name() const override;

    /*!
     * \brief Creates parent directories, opens the file, writes the header.
     * \return 0 on success; 1 when the file cannot be opened.
     */
    [[nodiscard]] int initialize() override;

    /*!
     * \brief Appends one row per element of every field in \p snapshot.
     * \return 0 on success; 2 on a stream failure; 3 for an unsupported
     *         DataKind.
     */
    [[nodiscard]] int write(const Snapshot &snapshot) override;

    /*!
     * \brief Flushes and closes the stream.
     * \return 0 on success; 2 on a stream failure.
     */
    [[nodiscard]] int finalize() override;

    [[nodiscard]] std::string errorMessage() const override;

    /*!
     * \brief The destination path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
        return m_path;
    }

private:
    /*!
     * \brief Element value as double for any fixed-width DataKind.
     * \return false for unsupported kinds.
     */
    static bool elementAsDouble(const FieldSlice &field, int64_t index,
                                double &value);

    std::filesystem::path m_path;
    std::ofstream         m_stream;
    std::string           m_error;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_CSVWRITER_H
