// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   geopackagewriter.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  GeoPackageWriter — OGC GeoPackage result persistence following
 *         the openswmm.engine convention.
 *
 * Produces a standard GeoPackage (SQLite) containing:
 *
 * - the \c gpkg_* conformance tables (\c gpkg_spatial_ref_sys,
 *   \c gpkg_contents, \c gpkg_geometry_columns) with the mandatory SRS
 *   rows (-1 undefined cartesian, 0 undefined geographic, 4326 WGS-84);
 * - an optional \c mesh_nodes feature layer (GeoPackageBinary-encoded WKB
 *   points) when a MeshDefinition is attached;
 * - the engine's result schema: \c simulations, \c variables,
 *   \c result_timeseries(simulation_id, variable_id, object_index,
 *   elapsed_time, value), and \c result_summary aggregates computed at
 *   finalize().
 *
 * Efficiency follows the engine's lessons: an internal RAII kit
 * (Db/Stmt/Transaction with rollback on error), prepared statements,
 * buffered inserts flushed in transactions (~5000 rows by default), and
 * \c PRAGMA synchronous=NORMAL. The SQLite C API is fully encapsulated
 * (pimpl); this header is installable without SQLite present, but the
 * class is compiled into HydroCoupleSDK only when it is built with
 * USE_GEOPACKAGE (macro HYDROCOUPLESDK_HAVE_GEOPACKAGE).
 *
 * The \c elapsed_time column holds Snapshot::time() as given (a Julian day
 * by SDK convention), not seconds since the start.
 *
 * \threadsafety Driven exclusively by the IOThread (see IOutputWriter).
 */

#ifndef HYDROCOUPLESDK_IO_GEOPACKAGEWRITER_H
#define HYDROCOUPLESDK_IO_GEOPACKAGEWRITER_H

#include "hydrocouplesdk/io/outputwriter.h"
#include "hydrocouplesdk/io/meshdefinition.h"

#include <filesystem>
#include <memory>
#include <string>

namespace HydroCouple::SDK::IO {

/*!
 * \brief OGC GeoPackage output writer (results + optional node features).
 *
 * Fields are auto-registered in \c variables on first appearance in a
 * snapshot; addField() beforehand enriches them with units and a
 * description. Values of any fixed-width numeric DataKind are stored as
 * REAL; \c object_index is the element's index within its field (the mesh
 * node/face ordinal for mesh-located fields).
 *
 * \c variables.item_id is unique, so two items with the same id share one
 * variable row. catalog() names the mesh \c meshName of the writer's
 * MeshDefinition even when setMesh() was never called, so its entries then
 * carry the default name "mesh".
 *
 * \threadsafety Driven exclusively by the IOThread (see IOutputWriter).
 */
class HYDROCOUPLESDK_EXPORT GeoPackageWriter final : public IOutputWriter
{
public:
    /*!
     * \brief Construct with the destination and simulation name.
     * \param[in] path           Destination .gpkg file (parents created on
     *                           initialize()).
     * \param[in] simulationName Row inserted into \c simulations.
     * \param[in] flushRowCount  Buffered-insert transaction size (tests may
     *                           lower it; ≥ 1). Values below 1 are stored
     *                           as 1.
     */
    explicit GeoPackageWriter(std::filesystem::path path,
                              std::string simulationName = "simulation",
                              int flushRowCount = 5000);

    //! Commits pending rows and closes the database (calls finalize()).
    ~GeoPackageWriter() override;

    //! Deleted: the writer owns an SQLite connection.
    GeoPackageWriter(const GeoPackageWriter &)            = delete;
    //! Deleted: the writer owns an SQLite connection.
    GeoPackageWriter &operator=(const GeoPackageWriter &) = delete;

    /*!
     * \brief Attaches a mesh whose nodes are written as a point feature
     *        layer (\c mesh_nodes) at initialize(). Optional.
     *
     * Only the node x and y coordinates are stored (srs_id -1, undefined
     * Cartesian); faces, edges and elevations are not.
     *
     * \param[in] mesh Mesh, validated in initialize(); copied.
     */
    void setMesh(MeshDefinition mesh);

    /*!
     * \brief Pre-registers a variable with metadata (optional — unknown
     *        fields auto-register with empty metadata on first write).
     *
     * The \c variables row is inserted when the item first appears in a
     * snapshot, so a field that never appears gets no row and no catalog
     * entry. Calling it again for the same id before that replaces the
     * metadata.
     *
     * \param[in] itemId      FieldSlice::itemId of the field.
     * \param[in] units       Written to \c variables.units and the catalog.
     * \param[in] description Written to \c variables.description and the
     *                        catalog.
     */
    void addField(const std::string &itemId, const std::string &units = "",
                  const std::string &description = "");

    // ── IOutputWriter ─────────────────────────────────────────────────────

    /*!
     * \brief The writer name used in IOThread error messages.
     *
     * \return "geopackage".
     */
    [[nodiscard]] std::string name() const override;

    /*!
     * \brief Creates the GeoPackage: conformance tables, SRS rows, result
     *        schema, simulation row, and the optional node feature layer.
     *
     * An existing file at the path is deleted first. Sets the GeoPackage
     * application id and user version 10300 (GeoPackage 1.3).
     *
     * \return 0 on success; nonzero SQLite/validation error otherwise: 30
     *         the attached mesh fails MeshDefinition::validate(), 31 the
     *         database cannot be opened, 32 the pragmas, 33 the conformance
     *         tables, 34 the result tables, 35 the simulation row, 36 to 38
     *         the node feature layer, 39 preparing the insert or beginning
     *         the first transaction.
     */
    [[nodiscard]] int initialize() override;

    /*!
     * \brief Buffers one row per element of every field into
     *        \c result_timeseries (transactional flush by row count).
     *
     * \param[in] snapshot The time level to write.
     * \return 0 on success; nonzero on unsupported kinds or SQLite failure:
     *         40 inserting a \c variables row, 41 an unsupported kind, 42
     *         inserting a value row, 43 committing a full transaction.
     */
    [[nodiscard]] int write(const Snapshot &snapshot) override;

    /*!
     * \brief Flushes pending rows, computes \c result_summary aggregates,
     *        and closes the database.
     *
     * \c result_summary gets one row per (simulation, variable) with the
     * count, minimum, maximum and mean of every value written. Does nothing
     * when no database is open.
     *
     * \return 0 on success; 44 when the last commit fails, 45 when the
     *         summary fails. The database is closed in every case.
     */
    [[nodiscard]] int finalize() override;

    /*!
     * \brief Detail message of the last failure.
     *
     * \return The SQLite message, or a message naming the field.
     */
    [[nodiscard]] std::string errorMessage() const override;

    /*!
     * \brief Catalog of the variables written to \c result_timeseries.
     *
     * Rows are long format (simulation, variable, object_index,
     * elapsed_time, value), so each entry records the { time, object }
     * shape observed while writing — a reader reconstructs the array from
     * it rather than inferring one from the table.
     *
     * \return Entries in order of first appearance with format
     *         "geopackage", kind Float64, shape { levels, elements of the
     *         first snapshot }, dimensions { "time", "object" }, the
     *         addField() metadata, the mesh name (see the class
     *         description) and the time bounds in units "julian_day".
     */
    [[nodiscard]] std::vector<ResultEntry> catalog() const override;

    /*!
     * \brief The destination path.
     *
     * \return The path given to the constructor.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

private:
    struct Impl; //!< Hides all SQLite types from the public surface.
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_GEOPACKAGEWRITER_H
