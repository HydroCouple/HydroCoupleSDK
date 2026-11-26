// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   resultentry.h
 * \author Caleb Buahin
 * \version 2.1.0
 * \brief  ResultEntry — one row of a run's results catalog.
 *
 * A ResultEntry says everything a reader needs to serve a recorded data
 * item's values without opening any model code: which artifact holds it,
 * in what format, under what name, with which element type, shape,
 * dimension names, units, mesh attachment, and time coordinate.
 *
 * The catalog is assembled by the writers themselves (see
 * IOutputWriter::catalog()), because Snapshot's FieldSlice carries only
 * id/kind/shape — units, mesh, and location are writer-side knowledge.
 * Nothing here is inferred at read time: a reader that would have to guess
 * a calendar, a shape, or a variable name instead reports an error.
 *
 * \threadsafety Plain value type.
 */

#ifndef HYDROCOUPLESDK_IO_RESULTENTRY_H
#define HYDROCOUPLESDK_IO_RESULTENTRY_H

#include "hydrocouplesdk/export.h"
#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouple.h"

#include <nlohmann/json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace HydroCouple::SDK::IO {

/*!
 * \brief Manifest specification version written by this SDK ("1.0").
 *
 * RunManifest::fromJson() reads any 1.x manifest.
 */
inline constexpr const char *kRunManifestSchemaVersion = "1.0";

/*!
 * \brief Time coordinate of a recorded variable's leading dimension
 *        (the entry's \c time object).
 *
 * Writers fill it from the snapshots they wrote. A reader that stores no
 * explicit times (CSV is read from its rows instead; GeoPackage uses this)
 * reconstructs a uniform axis from it: count values from start to end.
 */
struct HYDROCOUPLESDK_EXPORT ResultTimeAxis
{
    int64_t     count = 0;               //!< Number of time levels (\c time.count).
    double      start = 0.0;             //!< First time value (\c time.start), in \c units.
    double      end   = 0.0;             //!< Last time value (\c time.end), in \c units.
    std::string units = "julian_day";    //!< SDK convention; ResultsModelComponent also accepts "elapsed_s" and "seconds_since_start" with an epoch.
    std::optional<double> epochJulianDay; //!< Julian day of elapsed time zero (\c time.epoch_julian_day); required for elapsed_s clocks.
};

/*!
 * \brief One recorded data item and where its values live (an entry of the
 *        manifest's \c results[]).
 *
 * Writers produce entries through IOutputWriter::catalog(); RunRecorder
 * attributes them to components and adds metadata; readers and
 * ResultsModelComponent consume them.
 */
struct HYDROCOUPLESDK_EXPORT ResultEntry
{
    std::string componentId; //!< Producing component ("" when unattributed); \c component.
    std::string itemId;      //!< IIdentity::id() of the recorded item; \c item.

    std::string artifact;    //!< File holding the values (manifest-relative); \c artifact.
    std::string format;      //!< "csv", "hdf5_ugrid", "netcdf_ugrid", "geopackage"; selects the reader.
    std::string variable;    //!< Name within the artifact (defaults to itemId); \c variable.

    DataKind             kind = DataKind::Unknown; //!< Element type as stored (Float64 for every writer except CSV, which keeps the captured kind).
    std::vector<int64_t> shape;      //!< Full extent, row-major, time first.
    std::vector<std::string> dimensions; //!< Names parallel to shape.

    std::string units;       //!< Physical units, when known.
    std::string description; //!< Human-readable description.

    std::string                 mesh;     //!< UGRID mesh name, when attached.
    std::optional<MeshLocation> location; //!< Mesh location of the values.

    std::optional<ResultTimeAxis> time; //!< Time axis of dimension 0, if any.

    // Optional explicit metadata; absent for older manifests.
    nlohmann::json geometry; //!< Recorded topology sidecar and optional interface-elevation result; null when absent, otherwise an object.
    nlohmann::json quantity; //!< SI scale/offset, dimensional powers and value kind; null when absent, otherwise an object.
    std::string sha256; //!< Optional artifact hash for drift detection. Stored and round-tripped only: no SDK code computes or checks it.

    /*!
     * \brief Elements described by shape().
     *
     * \return The product of the extents; 1 for an empty shape.
     */
    [[nodiscard]] int64_t elementCount() const noexcept;

    /*!
     * \brief The variable name a reader should look for (variable, or itemId).
     *
     * \return \c variable when set, otherwise \c itemId.
     */
    [[nodiscard]] std::string variableName() const;

    /*!
     * \brief Serializes to the manifest's \c results[] shape.
     *
     * Always writes \c item, \c artifact, \c format, \c kind (via
     * dataKindName()) and \c shape; writes \c component, \c variable,
     * \c dimensions, \c units, \c description, \c mesh, \c location,
     * \c time, \c geometry, \c quantity and \c sha256 only when set.
     *
     * \return The entry object.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /*!
     * \brief Parses one \c results[] entry.
     *
     * Requires string \c item, \c artifact, \c format and \c kind (a
     * fixed-width kind name) and a non-empty \c shape of non-negative
     * integers. \c dimensions, when present and non-empty, must be as long
     * as \c shape. \c geometry and \c quantity, when present, must be
     * objects. \c location must be "node", "edge" or "face". Missing
     * \c time members default to count 0, start 0, end 0 and units
     * "julian_day".
     *
     * A present optional member of the wrong JSON type (a numeric
     * \c component, \c units or \c location, a non-string dimension name,
     * a non-number \c epoch_julian_day) makes nlohmann::json throw
     * \c nlohmann::json::type_error instead of returning false; the
     * exception propagates out of RunManifest::fromJson() and
     * RunManifest::read().
     *
     * \param[in]  json    The entry object.
     * \param[out] entry   Reset first; parsed result on success.
     * \param[out] message Actionable failure description otherwise (for
     *                     example "results[\"flow\"] needs a non-empty
     *                     \"shape\" array"); cleared on success.
     * \return True when the entry is valid.
     */
    [[nodiscard]] static bool fromJson(const nlohmann::json &json,
                                       ResultEntry &entry,
                                       std::string &message);
};

/*!
 * \brief DataKind ↔ manifest spelling ("Float64", "Int32", …).
 *
 * \param[in] kind Element kind.
 * \return "Int8", "UInt8", "Int16", "UInt16", "Int32", "UInt32", "Int64",
 *         "UInt64", "Float32", "Float64" or "Boolean"; "Unknown" for
 *         String, Opaque and Unknown, which no manifest entry may carry.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string dataKindName(DataKind kind);

/*!
 * \brief Parses a manifest DataKind spelling; false when unrecognized.
 *
 * \param[in]  name Spelling, matched exactly (case-sensitive).
 * \param[out] kind Set on success; unchanged otherwise.
 * \return False for any spelling other than the eleven fixed-width names.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool dataKindFromName(
    const std::string &name, DataKind &kind);

/*!
 * \brief MeshLocation ↔ manifest spelling ("node", "edge", "face").
 *
 * \param[in] location Mesh location.
 * \return The lower-case spelling; "node" for a value outside the
 *         enumeration.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string meshLocationName(
    MeshLocation location);

/*!
 * \brief Parses a manifest MeshLocation spelling; false when unrecognized.
 *
 * \param[in]  name     "node", "edge" or "face" (case-sensitive).
 * \param[out] location Set on success; unchanged otherwise.
 * \return False for any other spelling.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool meshLocationFromName(
    const std::string &name, MeshLocation &location);

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_RESULTENTRY_H
