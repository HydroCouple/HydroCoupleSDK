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
 *
 * \c index is the element's row-major position within the field. Values
 * and times are written with 17 significant digits, so a double read back
 * by the CSV result reader equals the one written. Item ids are written
 * unquoted: an id containing a comma or a line break makes the file
 * unreadable by the CSV result reader.
 */

#ifndef HYDROCOUPLESDK_IO_CSVWRITER_H
#define HYDROCOUPLESDK_IO_CSVWRITER_H

#include "hydrocouplesdk/io/outputwriter.h"

#include <filesystem>
#include <unordered_map>
#include <fstream>

namespace HydroCouple::SDK::IO {

/*!
 * \brief Long-format CSV output writer.
 *
 * Numeric fields are written element-by-element with full double
 * precision; Boolean fields as 0/1. Rows appear in capture order within a
 * snapshot and snapshot order across time. Fields need no registration:
 * every field of every snapshot is written. 64-bit integers above 2^53 lose
 * precision, because every value is converted to double.
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

    /*!
     * \brief Declares the units of the snapshot time coordinate before
     *        starting the IO thread.
     *
     * Recorded as \c time.units of every catalog entry; the CSV file itself
     * does not carry it. ResultsModelComponent accepts "julian_day" (the
     * default), and "elapsed_s" or "seconds_since_start" when the run
     * manifest supplies an epoch; it refuses other units.
     *
     * \param[in] units Time units spelling.
     */
    void setTimeUnits(std::string units) { m_timeUnits = std::move(units); }

    //! Closes the stream if it is still open.
    ~CSVWriter() override = default;

    // ── IOutputWriter ─────────────────────────────────────────────────────

    /*!
     * \brief The writer name used in IOThread error messages.
     *
     * \return "csv".
     */
    [[nodiscard]] std::string name() const override;

    /*!
     * \brief Creates parent directories, opens the file, writes the header.
     *
     * An existing file is truncated. The header is "time,item,index,value".
     *
     * \return 0 on success; 1 when the file cannot be opened ("cannot open
     *         '\<path\>' for writing").
     */
    [[nodiscard]] int initialize() override;

    /*!
     * \brief Appends one row per element of every field in \p snapshot.
     *
     * Also records the time bounds and, the first time an item appears,
     * its element shape and kind for catalog(); later snapshots only count
     * time levels.
     *
     * \param[in] snapshot The time level to write.
     * \return 0 on success; 2 on a stream failure (including a write
     *         without a successful initialize()); 3 for an unsupported
     *         DataKind ("unsupported DataKind for field '\<id\>'").
     */
    [[nodiscard]] int write(const Snapshot &snapshot) override;

    /*!
     * \brief Flushes and closes the stream.
     *
     * Does nothing when the stream is not open.
     *
     * \return 0 on success; 2 on a stream failure.
     */
    [[nodiscard]] int finalize() override;

    /*!
     * \brief Detail message of the last failure.
     *
     * \return The message set by the call that returned nonzero; empty when
     *         none failed.
     */
    [[nodiscard]] std::string errorMessage() const override;

    /*!
     * \brief Catalog of the items written, one entry per field.
     *
     * The CSV is long format (\c time,item,index,value), so each entry's
     * shape is { timeLevels, elementsPerLevel } — both accumulated while
     * writing, since a long-format file states neither in its header.
     *
     * For a field of shape { n } the entry's shape is { levels, n } with
     * dimensions { "time", "element" }; for a field of rank r > 1 the
     * dimensions are { "time", "dim0", ..., "dim(r-1)" }. \c kind is the
     * captured kind, \c variable is the item id, \c artifact the file name,
     * and \c time carries the first and last snapshot times and the units
     * of setTimeUnits(). Entries carry no component, units or mesh;
     * RunRecorder adds what it can.
     *
     * \return One entry per item, in order of first appearance; empty until
     *         something is written.
     */
    [[nodiscard]] std::vector<ResultEntry> catalog() const override;

    /*!
     * \brief The destination path.
     *
     * \return The path given to the constructor.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept
    {
        return m_path;
    }

private:
    /*!
     * \brief Element value as double for any fixed-width DataKind.
     *
     * \param[in]  field The slice holding the value.
     * \param[in]  index Row-major element index, in [0, elementCount()).
     * \param[out] value The element converted to double (Boolean as 0 or 1).
     * \return false for unsupported kinds.
     */
    static bool elementAsDouble(const FieldSlice &field, int64_t index,
                                double &value);

    /*!
     * \brief Per-item extent observed while writing (catalog support).
     */
    struct ItemExtent
    {
        int64_t              timeLevels = 0; //!< Snapshots that carried the item.
        std::vector<int64_t> elementShape; //!< Per-level shape.
        DataKind             kind = DataKind::Unknown; //!< Kind at first appearance.
    };

    std::filesystem::path m_path;      //!< Destination file.
    std::ofstream         m_stream;    //!< Open between initialize() and finalize().
    std::string           m_error;     //!< Last failure message.
    std::string           m_timeUnits = "julian_day"; //!< Catalog time units.

    std::vector<std::string>                     m_itemOrder; //!< Item ids in order of first appearance.
    std::unordered_map<std::string, ItemExtent>  m_extents;   //!< Observed extent per item id.
    bool    m_haveTimes = false; //!< Whether a snapshot has been written.
    double  m_firstTime = 0.0;   //!< Time of the first snapshot written.
    double  m_lastTime  = 0.0;   //!< Time of the last snapshot written.
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_CSVWRITER_H
