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
 * \brief Manifest specification version written and read by this SDK.
 */
inline constexpr const char *kRunManifestSchemaVersion = "1.0";

/*!
 * \brief Time coordinate of a recorded variable's leading dimension.
 */
struct HYDROCOUPLESDK_EXPORT ResultTimeAxis
{
    int64_t     count = 0;               //!< Number of time levels.
    double      start = 0.0;             //!< First time value.
    double      end   = 0.0;             //!< Last time value.
    std::string units = "julian_day";    //!< SDK convention.
};

/*!
 * \brief One recorded data item and where its values live.
 */
struct HYDROCOUPLESDK_EXPORT ResultEntry
{
    std::string componentId; //!< Producing component ("" when unattributed).
    std::string itemId;      //!< IIdentity::id() of the recorded item.

    std::string artifact;    //!< File holding the values (manifest-relative).
    std::string format;      //!< "csv", "hdf5_ugrid", "netcdf_ugrid", "geopackage".
    std::string variable;    //!< Name within the artifact (defaults to itemId).

    DataKind             kind = DataKind::Unknown; //!< Element type.
    std::vector<int64_t> shape;      //!< Full extent, row-major, time first.
    std::vector<std::string> dimensions; //!< Names parallel to shape.

    std::string units;       //!< Physical units, when known.
    std::string description; //!< Human-readable description.

    std::string                 mesh;     //!< UGRID mesh name, when attached.
    std::optional<MeshLocation> location; //!< Mesh location of the values.

    std::optional<ResultTimeAxis> time; //!< Time axis of dimension 0, if any.

    std::string sha256; //!< Optional artifact hash for drift detection.

    /*!
     * \brief Elements described by shape().
     */
    [[nodiscard]] int64_t elementCount() const noexcept;

    /*!
     * \brief The variable name a reader should look for (variable, or itemId).
     */
    [[nodiscard]] std::string variableName() const;

    /*!
     * \brief Serializes to the manifest's \c results[] shape.
     */
    [[nodiscard]] nlohmann::json toJson() const;

    /*!
     * \brief Parses one \c results[] entry.
     * \param[in]  json    The entry object.
     * \param[out] entry   Parsed result on success.
     * \param[out] message Actionable failure description otherwise.
     */
    [[nodiscard]] static bool fromJson(const nlohmann::json &json,
                                       ResultEntry &entry,
                                       std::string &message);
};

/*!
 * \brief DataKind ↔ manifest spelling ("Float64", "Int32", …).
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string dataKindName(DataKind kind);

/*!
 * \brief Parses a manifest DataKind spelling; false when unrecognized.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool dataKindFromName(
    const std::string &name, DataKind &kind);

/*!
 * \brief MeshLocation ↔ manifest spelling ("node", "edge", "face").
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT std::string meshLocationName(
    MeshLocation location);

/*!
 * \brief Parses a manifest MeshLocation spelling; false when unrecognized.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool meshLocationFromName(
    const std::string &name, MeshLocation &location);

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_RESULTENTRY_H
