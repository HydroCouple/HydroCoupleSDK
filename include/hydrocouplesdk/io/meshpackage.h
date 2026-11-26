// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin
/*!
 * \file   meshpackage.h
 * \author Caleb Buahin
 * \brief  MeshPackage — a surface mesh with its physical metadata, imported
 *         from SWMM 2-D or SMS 2DM text and stored as a manifest plus a
 *         UGRID NetCDF file.
 *
 * A mesh package is what a 2-D model reads its surface from: the geometry
 * (a MeshDefinition), the horizontal and vertical references it is
 * expressed in, per-cell attributes (roughness, initial depth, region
 * tags), boundary edges identified by their endpoints, and the source's
 * identities and unrecognized sections kept as provenance.
 *
 * importSurfaceMesh() reads the source text. writeMeshPackage() stores a
 * package as a \c .mesh.json manifest plus a \c .nc UGRID file and needs an
 * SDK built with USE_NETCDF; readMeshPackage() reads both back and checks
 * the geometry against the manifest's fingerprint. A mesh argument
 * (Spatial::PolyhedralSurfaceArgument) given a \c .json file reads it as a
 * mesh package. The guide chapter "Mesh packages" walks through the
 * workflow.
 *
 * Projected coordinate reference systems are checked with GDAL: an SDK
 * built without USE_GDAL accepts only the horizontal reference "LOCAL".
 *
 * \threadsafety Plain value types and free functions; no shared state.
 */
#ifndef HYDROCOUPLESDK_IO_MESHPACKAGE_H
#define HYDROCOUPLESDK_IO_MESHPACKAGE_H

#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplespatial.h"
#include <nlohmann/json.hpp>
#include <filesystem>

namespace HydroCouple::SDK::IO {

/*!
 * \brief One boundary edge of a mesh package, identified by its endpoints.
 *
 * Endpoint identity survives cell reordering and local-edge conventions.
 * MeshPackage::validate() requires the two nodes to bound exactly one face
 * (an exterior edge), each edge to appear once, and a non-empty type.
 */
struct MeshBoundary
{
    int64_t node0 = -1, node1 = -1;
    std::string type, group;
    std::vector<std::string> parameters; //!< Up to two parameter tokens as written in the source ("*" kept as written).
};
/*!
 * \var MeshBoundary::node0
 * \brief First endpoint: zero-based index into the package's mesh nodes.
 * \var MeshBoundary::node1
 * \brief Second endpoint: zero-based index into the package's mesh nodes.
 * \var MeshBoundary::type
 * \brief Boundary type token from the source (for example "WALL"); required.
 * \var MeshBoundary::group
 * \brief Optional group name; empty when the source gives none or "*".
 */

/*!
 * \brief A versioned UGRID mesh with explicit physical metadata and source
 *        identities.
 *
 * Coordinates, elevations and depths in memory are always meters, positive
 * up (depths positive). Unknown source sections are retained as provenance,
 * never applied as physics. Per-node vectors are parallel to the mesh
 * nodes and per-face vectors to the mesh faces; validate() checks every
 * rule listed below.
 */
struct HYDROCOUPLESDK_EXPORT MeshPackage
{
    MeshDefinition mesh; //!< The geometry; faces and one elevation per node are required. importSurfaceMesh() names it "mesh2d".
    std::string horizontalCrs, verticalDatum, elevationKind;
    std::vector<std::string> nodeIds, faceIds, regionTags;
    std::vector<double> roughness, initialDepth;
    std::vector<MeshBoundary> boundaries; //!< Boundary edges (see MeshBoundary).
    nlohmann::json sourceSections = nlohmann::json::object(); //!< Source sections kept as provenance: section header (for example "[2D_BOUNDARY_CONDITIONS]") to an array of rows, each an array of string tokens.
    nlohmann::json provenance = nlohmann::json::object(); //!< Where the package came from: source file name, fingerprint, format, declared units, source CRS and headers.

    /*!
     * \brief Checks the package against every mesh package rule.
     *
     * The horizontal CRS, vertical datum and elevation kind must be declared
     * (see MeshPackage::horizontalCrs, MeshPackage::verticalDatum and
     * MeshPackage::elevationKind); the mesh must pass
     * MeshDefinition::validate(), have faces and one elevation per node;
     * nodeIds, faceIds and regionTags must match the node and face counts,
     * and roughness and initialDepth must be empty or match the face count;
     * node and face ids must be non-empty and unique; coordinates and
     * elevations finite; roughness and depths finite and nonnegative; every
     * face must have at least three distinct nodes; no edge may be shared by
     * more than two faces; boundaries must each name one distinct exterior
     * edge and a type; sourceSections and provenance must be objects.
     *
     * \param[out] message The first rule broken (for example "non-manifold
     *                     mesh edge"); cleared on success.
     * \return True when the package is valid.
     */
    [[nodiscard]] bool validate(std::string &message) const;
};
/*!
 * \var MeshPackage::horizontalCrs
 * \brief Horizontal reference: "LOCAL" for a local Cartesian domain, or a
 *        projected CRS in meters (any form GDAL accepts, such as
 *        "EPSG:25832"; needs an SDK built with USE_GDAL).
 * \var MeshPackage::verticalDatum
 * \brief Name or code of the vertical datum the elevations refer to;
 *        required, and "unknown" is refused.
 * \var MeshPackage::elevationKind
 * \brief What the node elevations are: "bed-elevation" or
 *        "terrain-elevation".
 * \var MeshPackage::nodeIds
 * \brief Source identity of each node (unique, non-empty).
 * \var MeshPackage::faceIds
 * \brief Source identity of each face (unique, non-empty).
 * \var MeshPackage::regionTags
 * \brief Region or material tag of each face; may be empty strings.
 * \var MeshPackage::roughness
 * \brief Roughness of each face as given in the source, not converted;
 *        empty when the source has none (SMS).
 * \var MeshPackage::initialDepth
 * \brief Initial water depth of each face in meters; empty when no cell
 *        gave one.
 */

/*!
 * \brief The declarations importSurfaceMesh() needs and the source's units.
 *
 * Units are explicit at the import boundary; depth units can differ from
 * XY/Z. Each unit is "m", "ft" (0.3048 m) or "us-survey-ft" (1200/3937 m);
 * any other spelling fails the import.
 */
struct MeshImportOptions
{
    std::string horizontalCrs, verticalDatum, elevationKind;
    std::string horizontalUnits = "m", verticalUnits = "m", depthUnits = "m";
};
/*!
 * \var MeshImportOptions::horizontalCrs
 * \brief Becomes MeshPackage::horizontalCrs; with GDAL a projected CRS is
 *        normalized to meters and stored as WKT.
 * \var MeshImportOptions::verticalDatum
 * \brief Becomes MeshPackage::verticalDatum.
 * \var MeshImportOptions::elevationKind
 * \brief Becomes MeshPackage::elevationKind.
 * \var MeshImportOptions::horizontalUnits
 * \brief Units of the source x and y coordinates.
 * \var MeshImportOptions::verticalUnits
 * \brief Units of the source node elevations.
 * \var MeshImportOptions::depthUnits
 * \brief Units of the source initial depths.
 */

/*!
 * \brief Owned spatial reference for geometry adapters, built from a
 *        package's horizontal CRS and vertical datum.
 *
 * Named local datums carry WKT but no fabricated authority code; horizontal
 * and vertical distances are meters. "LOCAL" becomes a LOCAL_CS WKT named
 * "Local Cartesian mesh". A vertical datum that is not an EPSG vertical CRS
 * (or any datum, without GDAL) becomes a VERT_CS WKT carrying the datum's
 * name, with vertical code 0. With GDAL, the horizontal WKT, authority and
 * code come from the CRS, and an "EPSG:" vertical datum supplies its own.
 */
class HYDROCOUPLESDK_EXPORT MeshPackageReference final
    : public HydroCouple::Spatial::ISpatialReferenceSystem
{
public:
    /*!
     * \brief Builds the reference of \p package.
     *
     * Only the horizontal CRS, vertical datum and elevation kind are read.
     *
     * \param[in] package The package whose reference to describe.
     * \throws std::invalid_argument when those three fail the declaration
     *         rule of MeshPackage::validate(), and, with GDAL, when the CRS
     *         cannot be parsed or an EPSG vertical CRS is not in meters.
     */
    explicit MeshPackageReference(const MeshPackage &package);
    //! \return The horizontal authority code; 0 without GDAL or for LOCAL.
    int authSRID() const override { return m_code; }
    //! \return The horizontal authority name; empty without GDAL or for LOCAL.
    const std::string &authName() const override { return m_authority; }
    //! \return The horizontal reference as WKT (the CRS as given, without GDAL).
    const std::string &srText() const override { return m_horizontal; }
    //! \return Always IUnit::DistanceUnits::Meters.
    HydroCouple::IUnit::DistanceUnits distanceUnits() const override;
    //! \return The vertical authority name; empty for a named local datum.
    const std::string &verticalAuthName() const override { return m_verticalAuthority; }
    //! \return The vertical authority code; 0 for a named local datum.
    int verticalAuthSRID() const override { return m_verticalCode; }
    //! \return The vertical reference as WKT.
    const std::string &verticalSrText() const override { return m_vertical; }
    //! \return Always IUnit::DistanceUnits::Meters.
    HydroCouple::IUnit::DistanceUnits verticalDistanceUnits() const override;
private:
    std::string m_authority, m_horizontal, m_verticalAuthority, m_vertical;
    int m_code = 0, m_verticalCode = 0;
};
/*!
 * \var MeshPackageReference::m_authority
 * \brief Horizontal authority name.
 * \var MeshPackageReference::m_horizontal
 * \brief Horizontal WKT.
 * \var MeshPackageReference::m_verticalAuthority
 * \brief Vertical authority name.
 * \var MeshPackageReference::m_vertical
 * \brief Vertical WKT.
 * \var MeshPackageReference::m_code
 * \brief Horizontal authority code.
 * \var MeshPackageReference::m_verticalCode
 * \brief Vertical authority code.
 */

/*!
 * \brief Reads a SWMM section-based .2dm/.inp or an SMS ND/E3T/E4Q file,
 *        recognized by content.
 *
 * Cells remain in source engine order (all triangles, then all quads);
 * quads are never split. No GUI dependency. Geometry beyond the package
 * rules (orientation, cell quality) is checked by the consuming solver.
 *
 * SWMM grammar: rows of \c [2D_VERTICES] are "x y z" and nodes are numbered
 * by position from 0; rows of \c [2D_TRIANGLES] and \c [2D_QUADS] are the
 * node numbers, a roughness, then an optional depth (a token starting with
 * a digit, sign or point) and an optional region tag; every other section
 * is kept in MeshPackage::sourceSections. \c [2D_BOUNDARY_CONDITIONS] rows
 * "cell edge type [p1 [p2 [group]]]" become MeshBoundary entries, where
 * local edge e of a cell with n nodes joins its nodes (e + 1) mod n and
 * (e + 2) mod n. SMS grammar: "ND id x y z", "E3T id n1 n2 n3 [tag]",
 * "E4Q id n1 n2 n3 n4 [tag]", an optional leading "MESH2D"; no roughness
 * or depth. ';' starts a comment outside double quotes; a line
 * ";; UNITS: ..." or ";; SOURCE_CRS: ..." declares metadata that must
 * agree with \p options.
 *
 * \param[in]  path    Source file.
 * \param[in]  options Declared references and source units.
 * \param[out] package The imported package; untouched on failure.
 * \param[out] message Failure description; parse errors read "surface mesh
 *                     line N: ..." (see the guide's diagnostics table);
 *                     cleared on success.
 * \return True when the file was read and the package passes
 *         MeshPackage::validate().
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool importSurfaceMesh(
    const std::filesystem::path &path, const MeshImportOptions &options,
    MeshPackage &package, std::string &message);

/*!
 * \brief Writes a .mesh.json manifest plus companion .nc, refusing
 *        existing outputs.
 *
 * The UGRID file is \p manifest with its last extension replaced by
 * ".nc" (so "site.mesh.json" pairs with "site.mesh.nc"), written by
 * NetCDFUGRIDWriter with no fields. Missing parent directories are created.
 * On failure, any file this call created is removed.
 *
 * \param[in]  manifest Manifest path.
 * \param[in]  package  Package to store; validated first.
 * \param[out] message  Failure description: "mesh packages require an SDK
 *                      built with NetCDF" (SDK built without USE_NETCDF), a
 *                      MeshPackage::validate() message, "mesh package
 *                      outputs already exist; choose a new destination",
 *                      "writing mesh package UGRID failed", "writing mesh
 *                      package manifest failed"; cleared on success.
 * \return True when both files were written.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool writeMeshPackage(
    const std::filesystem::path &manifest, const MeshPackage &package,
    std::string &message);

/*!
 * \brief Reads and validates a manifest and companion UGRID file.
 *        Output is atomic.
 *
 * The manifest must declare schema "hydrocouple-mesh-package", version
 * "1.0", meters and positive up, and carry every key writeMeshPackage()
 * writes. The UGRID file named by its \c mesh key is read relative to the
 * manifest's directory, and the geometry's fingerprint must equal
 * \c geometry_fingerprint.
 *
 * \param[in]  manifest Manifest path.
 * \param[out] package  The package; untouched on failure.
 * \param[out] message  Failure description: a readUGRIDMesh() or
 *                      MeshPackage::validate() message, or "mesh package: "
 *                      followed by the reason ("unsupported mesh package
 *                      schema/version", "mesh geometry fingerprint disagrees
 *                      with manifest", a missing key); cleared on success.
 * \return True when the package was read and is valid.
 */
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool readMeshPackage(
    const std::filesystem::path &manifest, MeshPackage &package,
    std::string &message);

} // namespace HydroCouple::SDK::IO
#endif
