// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

/*!
 * \file   netcdfugridwriter.h
 * \author Caleb Buahin
 * \version 2.0.0-alpha.1
 * \brief  NetCDFUGRIDWriter — CF-1.11 / UGRID-1.0 output through the
 *         NetCDF-4 C API.
 *
 * Expresses the same UGRID layout as HDF5UGRIDWriter through the netCDF-4
 * data model (the file is HDF5 underneath): mesh-topology dummy variable
 * with \c cf_role, node coordinates, fill-value-padded
 * \c face_node_connectivity, an unlimited \c time dimension, and deflated
 * time-varying field variables tagged \c mesh + \c location.
 *
 * This writer supersedes the legacy \c threadsafenetcdf module: its ~100
 * <tt>\#pragma omp critical</tt> sections existed to serialize concurrent
 * NetCDF calls, a job the single-writer IOThread now performs by design —
 * so the wrappers are retired rather than revived (plan §4.5).
 *
 * The NetCDF C API is fully encapsulated (pimpl); this header is
 * installable without NetCDF present, but the class is compiled into
 * HydroCoupleSDK only when it is built with USE_NETCDF (macro
 * HYDROCOUPLESDK_HAVE_NETCDF).
 *
 * File layout, where MESH stands for the mesh name: global attributes
 * \c Conventions ("CF-1.11 UGRID-1.0"), \c title and \c source; dimensions
 * \c time (unlimited), \c MESH_nNodes, and when present \c MESH_nFaces,
 * \c MESH_maxFaceNodes, \c MESH_nEdges and \c Two; the topology variable
 * \c MESH; \c MESH_node_x, \c MESH_node_y and optional \c MESH_node_z
 * (all listed in \c node_coordinates); \c MESH_face_nodes (int64,
 * \c _FillValue -1, \c start_index 0) and \c MESH_edge_nodes; \c time
 * (double, units "days since -4713-01-01", calendar "julian"); one double
 * variable per registered field over (time, location), shuffled and
 * deflated at level 4. A layered file adds \c MESH_nLayers,
 * \c MESH_nInterfaces, \c MESH_layer_sigma (standard name
 * \c ocean_sigma_coordinate, with \c formula_terms and \c bounds),
 * \c MESH_interface_sigma, \c MESH_depth and \c MESH_eta (time, face).
 *
 * MeshPackage files (writeMeshPackage()) are written by this class with no
 * fields.
 *
 * \threadsafety Driven exclusively by the IOThread (see IOutputWriter).
 * Each library call also holds ioLibraryMutex(), so writers on other
 * IOThreads and readers elsewhere in the process may run at the same time.
 */

#ifndef HYDROCOUPLESDK_IO_NETCDFUGRIDWRITER_H
#define HYDROCOUPLESDK_IO_NETCDFUGRIDWRITER_H

#include "hydrocouplesdk/io/outputwriter.h"
#include "hydrocouplesdk/io/meshdefinition.h"

#include <filesystem>
#include <memory>
#include <string>

namespace HydroCouple::SDK::IO {

/*!
 * \brief CF-1.11 / UGRID-1.0 NetCDF-4 output writer.
 *
 * Fields are registered before initialize(); each snapshot must then carry
 * every registered field (matched by FieldSlice::itemId) with exactly
 * location-count elements. Values of any fixed-width numeric DataKind are
 * converted to double on write through a reused staging buffer. Fields of
 * a snapshot that were not registered are ignored.
 *
 * \threadsafety Driven exclusively by the IOThread (see IOutputWriter).
 * Each library call also holds ioLibraryMutex(), so writers on other
 * IOThreads and readers elsewhere in the process may run at the same time.
 */
class HYDROCOUPLESDK_EXPORT NetCDFUGRIDWriter final : public IOutputWriter
{
public:
    /*!
     * \brief Construct with the output path and the mesh to serialize.
     * \param[in] path Destination .nc file; parents created on initialize(),
     *                 an existing file overwritten (NC_CLOBBER).
     * \param[in] mesh Mesh description (validated in initialize()); copied.
     */
    NetCDFUGRIDWriter(std::filesystem::path path, MeshDefinition mesh);

    //! Closes the file if it is still open (calls finalize()).
    ~NetCDFUGRIDWriter() override;

    //! Deleted: the writer owns a NetCDF file id.
    NetCDFUGRIDWriter(const NetCDFUGRIDWriter &)            = delete;
    //! Deleted: the writer owns a NetCDF file id.
    NetCDFUGRIDWriter &operator=(const NetCDFUGRIDWriter &) = delete;

    /*!
     * \brief Registers a time-varying field before initialize().
     * \param[in] itemId       FieldSlice::itemId to match in snapshots
     *                         (also the variable name).
     * \param[in] location     UGRID location the values live on.
     * \param[in] units        CF \c units attribute ("" omits it); also the
     *                         catalog entry's units.
     * \param[in] standardName CF \c standard_name attribute ("" omits it);
     *                         also the catalog entry's description.
     *
     * Registering after initialize() adds a field with no variable, and the
     * next write() fails on it.
     */
    void addField(const std::string &itemId, MeshLocation location,
                  const std::string &units = "",
                  const std::string &standardName = "");

    // ── IOutputWriter ─────────────────────────────────────────────────────

    /*!
     * \brief Gives the file a CF vertical coordinate.
     *
     * Turns a 2-D mesh file into a layered one: the topology stays
     * two-dimensional, as the conventions intend, and the water column is
     * described by a sigma coordinate over it. Without this the file is
     * plain 2-D, which is what most meshes are.
     *
     * The arguments are in CF's conventions, matching what readVerticalCoordinate()
     * returns: sigma runs 0 at the surface to -1 at the bed, and depth is
     * positive **down**. Both are written verbatim, together with the
     * \c formula_terms that let any CF-aware reader rebuild elevations.
     *
     * Must be called before initialize(). The file is layered only when
     * \p interfaceSigma holds at least two values and \p depth is not
     * empty; the mesh must then have faces. No value is checked: the sigma
     * order and range and the lengths are written as given. Layer centers
     * are written as the midpoints of consecutive interfaces.
     *
     * \param[in] interfaceSigma Sigma at the layer interfaces, 0 to -1
     *                           (layers + 1 values).
     * \param[in] depth Bed depth per face, positive down, in meters (one
     *                  value per face).
     * \param[in] surface Water surface elevation per face, positive up, in
     *                    meters. Several time levels may be given as one run
     *                    of \c faces values per level; a moving surface is
     *                    what makes sigma layers real elevations at each step
     *                    rather than only at the first. A length that is not
     *                    a whole number of levels writes no surface.
     */
    void setVerticalCoordinate(std::vector<double> interfaceSigma,
                               std::vector<double> depth,
                               std::vector<double> surface);

    /*!
     * \brief The writer name used in IOThread error messages.
     *
     * \return "netcdf-ugrid".
     */
    [[nodiscard]] std::string name() const override;

    /*!
     * \brief Validates the mesh, creates the NetCDF-4 file, and defines
     *        conventions, topology, coordinates, connectivity, and fields.
     * \return 0 on success; nonzero NetCDF/validation error otherwise: 50
     *         the mesh fails MeshDefinition::validate(), 51 nc_create, 52
     *         global attributes, 53 a dimension, 54 the topology variable,
     *         55 a coordinate variable, 56 connectivity or vertical
     *         coordinate variables, 57 the time variable, 58 a field (also
     *         when it is on a location the mesh has none of), 59 leaving
     *         define mode or writing the static values. The NetCDF message
     *         is in errorMessage().
     */
    [[nodiscard]] int initialize() override;

    /*!
     * \brief Appends one time level to \c time and every field variable.
     *
     * \param[in] snapshot The time level; must carry every registered field.
     * \return 0 on success; nonzero on missing/mis-sized fields or NetCDF
     *         failure: 60 the time value, 61 a registered field is missing,
     *         62 a field's element count differs from its location count, 63
     *         an unsupported kind, 64 a field value write. The level counts
     *         toward the catalog even when the write fails.
     */
    [[nodiscard]] int write(const Snapshot &snapshot) override;

    /*!
     * \brief Syncs and closes the file.
     *
     * Does nothing when the file is not open.
     *
     * \return 0 on success; 65 when nc_close fails.
     */
    [[nodiscard]] int finalize() override;

    /*!
     * \brief Detail message of the last failure.
     *
     * \return For a NetCDF failure, the failing call followed by
     *         nc_strerror(); otherwise a message naming the field.
     */
    [[nodiscard]] std::string errorMessage() const override;

    /*!
     * \brief Catalog of the variables written: one entry per field, shaped
     *        { time, locationCount }, carrying mesh name and location.
     *
     * \return Entries in registration order with format "netcdf_ugrid",
     *         kind Float64, dimensions { "time", location }, the field's
     *         units and standard name (as description), and the recorded
     *         time bounds in units "julian_day".
     */
    [[nodiscard]] std::vector<ResultEntry> catalog() const override;

    /*!
     * \brief The destination path.
     *
     * \return The path given to the constructor.
     */
    [[nodiscard]] const std::filesystem::path &path() const noexcept;

private:
    struct Impl; //!< Hides all NetCDF C-API types from the public surface.
    std::unique_ptr<Impl> m_impl;
};

} // namespace HydroCouple::SDK::IO

#endif // HYDROCOUPLESDK_IO_NETCDFUGRIDWRITER_H
