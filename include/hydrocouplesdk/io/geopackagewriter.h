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
 * (pimpl); this header is installable without SQLite present.
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
     *                           lower it; ≥ 1).
     */
    explicit GeoPackageWriter(std::filesystem::path path,
                              std::string simulationName = "simulation",
                              int flushRowCount = 5000);

    ~GeoPackageWriter() override;

    GeoPackageWriter(const GeoPackageWriter &)            = delete;
    GeoPackageWriter &operator=(const GeoPackageWriter &) = delete;

    /*!
     * \brief Attaches a mesh whose nodes are written as a point feature
     *        layer (\c mesh_nodes) at initialize(). Optional.
     */
    void setMesh(MeshDefinition mesh);

    /*!
     * \brief Pre-registers a variable with metadata (optional — unknown
     *        fields auto-register with empty metadata on first write).
     */
    void addField(const std::string &itemId, const std::string &units = "",
                  const std::string &description = "");

    // ── IOutputWriter ─────────────────────────────────────────────────────

    [[nodiscard]] std::string name() const override;

    /*!
     * \brief Creates the GeoPackage: conformance tables, SRS rows, result
     *        schema, simulation row, and the optional node feature layer.
     * \return 0 on success; nonzero SQLite/validation error otherwise.
     */
    [[nodiscard]] int initialize() override;

    /*!
     * \brief Buffers one row per element of every field into
     *        \c result_timeseries (transactional flush by row count).
     * \return 0 on success; nonzero on unsupported kinds or SQLite failure.
     */
    [[nodiscard]] int write(const Snapshot &snapshot) override;

    /*!
     * \brief Flushes pending rows, computes \c result_summary aggregates,
     *        and closes the database.
     */
    [[nodiscard]] int finalize() override;

    [[nodiscard]] std::string errorMessage() const override;

    /*!
     * \brief The destination path.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

private:
    struct Impl; //!< Hides all SQLite types from the public surface.
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_GEOPACKAGEWRITER_H
