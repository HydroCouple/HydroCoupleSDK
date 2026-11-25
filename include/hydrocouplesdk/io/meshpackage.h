// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin
#ifndef HYDROCOUPLESDK_IO_MESHPACKAGE_H
#define HYDROCOUPLESDK_IO_MESHPACKAGE_H

#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplespatial.h"
#include <nlohmann/json.hpp>
#include <filesystem>

namespace HydroCouple::SDK::IO {

//! Endpoint identity survives cell reordering and local-edge conventions.
struct MeshBoundary
{
    int64_t node0 = -1, node1 = -1;
    std::string type, group;
    std::vector<std::string> parameters;
};

//! A versioned UGRID mesh with explicit physical metadata and source identities.
//! Coordinates, elevations and depths in memory are always metres, positive up.
//! Unknown source sections are retained as provenance, never applied as physics.
struct HYDROCOUPLESDK_EXPORT MeshPackage
{
    MeshDefinition mesh;
    std::string horizontalCrs, verticalDatum, elevationKind;
    std::vector<std::string> nodeIds, faceIds, regionTags;
    std::vector<double> roughness, initialDepth;
    std::vector<MeshBoundary> boundaries;
    nlohmann::json sourceSections = nlohmann::json::object();
    nlohmann::json provenance = nlohmann::json::object();

    [[nodiscard]] bool validate(std::string &message) const;
};

//! Units are explicit at the import boundary; depth units can differ from XY/Z.
struct MeshImportOptions
{
    std::string horizontalCrs, verticalDatum, elevationKind;
    std::string horizontalUnits = "m", verticalUnits = "m", depthUnits = "m";
};

//! Owned reference for geometry adapters. Named local datums carry WKT but no
//! fabricated authority code; horizontal and vertical distances are metres.
class HYDROCOUPLESDK_EXPORT MeshPackageReference final
    : public HydroCouple::Spatial::ISpatialReferenceSystem
{
public:
    explicit MeshPackageReference(const MeshPackage &package);
    int authSRID() const override { return m_code; }
    const std::string &authName() const override { return m_authority; }
    const std::string &srText() const override { return m_horizontal; }
    HydroCouple::IUnit::DistanceUnits distanceUnits() const override;
    const std::string &verticalAuthName() const override { return m_verticalAuthority; }
    int verticalAuthSRID() const override { return m_verticalCode; }
    const std::string &verticalSrText() const override { return m_vertical; }
    HydroCouple::IUnit::DistanceUnits verticalDistanceUnits() const override;
private:
    std::string m_authority, m_horizontal, m_verticalAuthority, m_vertical;
    int m_code = 0, m_verticalCode = 0;
};

//! Read SWMM section-based .2dm/.inp or SMS ND/E3T/E4Q by content.
//! Cells remain in source engine order (triangles then quads); never split quads.
//! No GUI dependency. Geometry validation is performed by the consuming solver.
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool importSurfaceMesh(
    const std::filesystem::path &path, const MeshImportOptions &options,
    MeshPackage &package, std::string &message);

//! Write a .mesh.json manifest plus companion .nc, refusing existing outputs.
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool writeMeshPackage(
    const std::filesystem::path &manifest, const MeshPackage &package,
    std::string &message);

//! Read and validate a manifest and companion UGRID file. Output is atomic.
[[nodiscard]] HYDROCOUPLESDK_EXPORT bool readMeshPackage(
    const std::filesystem::path &manifest, MeshPackage &package,
    std::string &message);

} // namespace HydroCouple::SDK::IO
#endif
