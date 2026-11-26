// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Chapter 14 example: a mesh package round trip. Imports site.2dm (the file
// of the ch14_mesh_import example), writes it as a manifest plus a UGRID
// NetCDF file, refuses to overwrite it, reads it back and compares every
// field, detects a replaced UGRID file, hands the package to a mesh argument,
// and adds a CF sigma coordinate to the same mesh. Needs an SDK built with
// USE_NETCDF; without it writeMeshPackage() fails with "mesh packages require
// an SDK built with NetCDF". Returns nonzero on any difference.

#include "hydrocouplesdk/io/meshpackage.h"
#include "hydrocouplesdk/io/netcdfugridwriter.h"
#include "hydrocouplesdk/io/ugridreader.h"
#include "hydrocouplesdk/spatial/polyhedralsurfaceargument.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

using namespace HydroCouple::SDK::IO;
using AT = HydroCouple::IArgument::ArgumentInputType;

namespace {

int failures = 0;

void check(bool condition, const std::string &what)
{
    if (!condition)
    {
        std::cout << "CHECK FAILED: " << what << '\n';
        ++failures;
    }
}

} // namespace

int main()
{
    std::filesystem::remove_all("package");
    std::filesystem::remove_all("tampered");

    MeshImportOptions options;
    options.horizontalCrs = "LOCAL";
    options.verticalDatum = "site-benchmark-1";
    options.elevationKind = "bed-elevation";
    MeshPackage package;
    std::string message;
    if (!importSurfaceMesh(std::string(DOCS_EXAMPLE_SOURCE_DIR) +
                               "/../ch14_mesh_import/site.2dm",
                           options, package, message))
    {
        std::cout << "import failed: " << message << '\n';
        return 1;
    }

    //! [write]
    const std::filesystem::path manifest = "package/site.mesh.json";
    if (!writeMeshPackage(manifest, package, message))
    {
        std::cout << "write failed: " << message << '\n';
        return 1;
    }
    std::vector<std::string> files;
    for (const auto &file : std::filesystem::directory_iterator("package"))
        files.push_back(file.path().filename().string());
    std::sort(files.begin(), files.end());
    for (const std::string &file : files)
        std::cout << "wrote " << file << '\n';

    // A package is never overwritten in place.
    const bool again = writeMeshPackage(manifest, package, message);
    std::cout << "second write: " << message << '\n';
    //! [write]
    check(std::filesystem::exists("package/site.mesh.nc"),
          "the UGRID file sits beside the manifest");
    check(!again && message.find("already exist") != std::string::npos,
          "existing outputs are refused");

    //! [read]
    MeshPackage restored;
    if (!readMeshPackage(manifest, restored, message))
    {
        std::cout << "read failed: " << message << '\n';
        return 1;
    }
    const bool same =
        restored.mesh.nodeX == package.mesh.nodeX &&
        restored.mesh.nodeZ == package.mesh.nodeZ &&
        restored.mesh.faceNodes == package.mesh.faceNodes &&
        restored.mesh.faceNodeOffsets == package.mesh.faceNodeOffsets &&
        restored.nodeIds == package.nodeIds &&
        restored.faceIds == package.faceIds &&
        restored.regionTags == package.regionTags &&
        restored.roughness == package.roughness &&
        restored.initialDepth == package.initialDepth &&
        restored.boundaries.size() == package.boundaries.size() &&
        restored.sourceSections == package.sourceSections &&
        restored.provenance == package.provenance;
    std::cout << "read back " << restored.mesh.nodeCount() << " nodes, "
              << restored.mesh.faceCount() << " faces, "
              << restored.boundaries.size() << " boundaries: "
              << (same ? "identical to what was written" : "DIFFERENT") << '\n';

    const std::vector<std::string> meshes =
        ugridMeshNames("package/site.mesh.nc", message);
    std::cout << "site.mesh.nc holds mesh \"" << meshes.at(0) << "\"\n";
    //! [read]
    check(same, "the round trip keeps every field");

    //! [tamper]
    // Replace the UGRID file with one whose first node is 10 cm higher.
    std::filesystem::create_directories("tampered");
    std::filesystem::copy_file(manifest, "tampered/site.mesh.json");
    MeshDefinition moved = package.mesh;
    moved.nodeZ[0] += 0.1;
    NetCDFUGRIDWriter replacement("tampered/site.mesh.nc", moved);
    (void)replacement.initialize();
    (void)replacement.finalize();

    MeshPackage refused;
    const bool tamperedRead =
        readMeshPackage("tampered/site.mesh.json", refused, message);
    std::cout << "replaced UGRID file: " << message << '\n';
    //! [tamper]
    check(!tamperedRead && message.find("fingerprint") != std::string::npos,
          "a changed geometry is detected");

    //! [argument]
    // A mesh argument given a .json file reads it as a mesh package; in a
    // composition the payload is {"@uri": "package/site.mesh.json"}.
    HydroCouple::SDK::Spatial::PolyhedralSurfaceArgument mesh("mesh", nullptr);
    const std::string payload =
        nlohmann::json{{"@uri", manifest.string()}}.dump();
    if (!mesh.initialize(payload, AT::JSON, message))
    {
        std::cout << "argument: " << message << '\n';
        return 1;
    }
    std::cout << "argument: " << mesh.toString() << ", package datum "
              << mesh.meshPackage()->verticalDatum << '\n';
    //! [argument]
    check(mesh.meshPackage().has_value() &&
              mesh.meshPackage()->boundaries.size() == 2,
          "the argument keeps the package");

    //! [vertical]
    // Two equal layers over each face. Depth is positive down; the surface
    // is given for two time levels (faces values per level).
    const std::vector<double> interfaces = {0.0, -0.5, -1.0};
    const std::vector<double> depth      = {2.1, 1.95, 1.9};
    const std::vector<double> surface    = {0.0, 0.0, 0.0,   // level 0
                                            0.4, 0.3, 0.2};  // level 1
    NetCDFUGRIDWriter layered("package/site_layered.nc", package.mesh);
    layered.setVerticalCoordinate(interfaces, depth, surface);
    if (layered.initialize() != 0 || layered.finalize() != 0)
    {
        std::cout << "layered file: " << layered.errorMessage() << '\n';
        return 1;
    }

    VerticalCoordinate vertical;
    if (!readVerticalCoordinate("package/site_layered.nc", "", 1, vertical,
                                message))
    {
        std::cout << message << '\n';
        return 1;
    }
    std::cout << vertical.layerCount() << " layers, layer sigma "
              << vertical.layerSigma[0] << " and " << vertical.layerSigma[1]
              << ", " << vertical.timeCount << " surface times, surface "
              << vertical.surfaceVariable << '\n';
    // CF ocean_sigma_coordinate: z = eta + sigma * (depth + eta).
    const double eta = vertical.surface[0];
    const double h   = vertical.depth[0];
    std::cout << "face 0 at time 1: interfaces at";
    for (double sigma : vertical.interfaceSigma)
        std::cout << ' ' << eta + sigma * (h + eta);
    std::cout << " m\n";

    std::cout << "site.mesh.nc has a vertical coordinate: "
              << (hasVerticalCoordinate("package/site.mesh.nc", "") ? "yes" : "no")
              << '\n';
    //! [vertical]
    check(vertical.layerCount() == 2 && !vertical.interfacesAreDerived,
          "interfaces read, not derived");
    check(vertical.timeCount == 2 && vertical.surface[0] == 0.4,
          "the surface of the time asked for");

    std::cout << (failures == 0 ? "all checks passed" : "checks failed")
              << '\n';
    return failures == 0 ? 0 : 1;
}
