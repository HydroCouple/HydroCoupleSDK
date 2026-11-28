// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Chapter 14 example: import a surface mesh into a MeshPackage. Reads a SWMM
// 2-D section file (site.2dm) and an SMS 2DM file (site_sms.2dm) from this
// directory, prints what the package holds, converts a file authored in feet,
// shows the messages of the refusals the chapter lists, and builds the
// package's spatial reference. Needs no NetCDF: writing the package is the
// next example (ch14_mesh_package). Returns nonzero when anything differs
// from what the guide states.

#include "hydrocouplesdk/io/meshpackage.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace HydroCouple::SDK::IO;

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

std::string source(const char *name)
{
    return std::string(DOCS_EXAMPLE_SOURCE_DIR) + "/" + name;
}

//! Writes \p text to \p name in the working directory and returns the path.
std::string scratch(const std::string &name, const std::string &text)
{
    std::ofstream(name) << text;
    return name;
}

} // namespace

int main()
{
    //! [import]
    MeshImportOptions options;
    options.horizontalCrs = "LOCAL";            // a local Cartesian domain
    options.verticalDatum = "site-benchmark-1"; // what z = 0 means
    options.elevationKind = "bed-elevation";    // z is the channel bed
    // horizontalUnits, verticalUnits and depthUnits default to "m".

    MeshPackage package;
    std::string message;
    if (!importSurfaceMesh(source("site.2dm"), options, package, message))
    {
        std::cout << "import failed: " << message << '\n';
        return 1;
    }
    //! [import]

    //! [inspect]
    const MeshDefinition &mesh = package.mesh;
    std::cout << package.provenance["format"].get<std::string>() << " mesh \""
              << mesh.meshName << "\": " << mesh.nodeCount() << " nodes, "
              << mesh.faceCount() << " faces\n";
    for (int64_t face = 0; face < mesh.faceCount(); ++face)
    {
        const size_t f = static_cast<size_t>(face);
        std::cout << "  face " << package.faceIds[f] << " ("
                  << package.regionTags[f] << "): nodes";
        for (int64_t k = mesh.faceNodeOffsets[f]; k < mesh.faceNodeOffsets[f + 1]; ++k)
            std::cout << ' ' << mesh.faceNodes[static_cast<size_t>(k)];
        std::cout << ", roughness " << package.roughness[f]
                  << ", initial depth " << package.initialDepth[f] << " m\n";
    }
    for (const MeshBoundary &boundary : package.boundaries)
    {
        std::cout << "  boundary " << boundary.type << " on edge "
                  << boundary.node0 << "-" << boundary.node1;
        if (!boundary.group.empty())
            std::cout << ", group " << boundary.group;
        std::cout << ", parameters";
        for (const std::string &parameter : boundary.parameters)
            std::cout << ' ' << parameter;
        std::cout << '\n';
    }
    for (const auto &[section, rows] : package.sourceSections.items())
        std::cout << "  kept section " << section << " (" << rows.size()
                  << " rows)\n";
    std::cout << "  fingerprint " << package.provenance["source_fingerprint"]
              << ", headers " << package.provenance["source_headers"].dump()
              << '\n';
    //! [inspect]

    check(mesh.nodeCount() == 6 && mesh.faceCount() == 3, "6 nodes, 3 faces");
    check(mesh.faceNodeOffsets == std::vector<int64_t>{0, 3, 6, 10},
          "triangles first, then the quad");
    check(package.regionTags[2] == "floodplain", "the quad's tag");
    check(package.boundaries.size() == 2 && package.boundaries[0].node0 == 3 &&
              package.boundaries[0].node1 == 0,
          "local edge 2 of the quad joins nodes 3 and 0");
    check(package.sourceSections.contains("[2D_OPTIONS]"),
          "unknown sections are kept");

    //! [sms]
    MeshPackage sms;
    if (!importSurfaceMesh(source("site_sms.2dm"), options, sms, message))
    {
        std::cout << "SMS import failed: " << message << '\n';
        return 1;
    }
    std::cout << sms.provenance["format"].get<std::string>() << ": face ids";
    for (const std::string &id : sms.faceIds)
        std::cout << ' ' << id;
    std::cout << ", node ids";
    for (const std::string &id : sms.nodeIds)
        std::cout << ' ' << id;
    std::cout << ", roughness values " << sms.roughness.size() << '\n';
    //! [sms]
    check(sms.faceIds == std::vector<std::string>{"9", "7"},
          "SMS ids are kept, triangles first");

    //! [units]
    // The same geometry authored in feet: coordinates, elevations and depths
    // are converted to meters at the boundary.
    const std::string feet = scratch("site_feet.2dm",
        "[2D_VERTICES]\n0 0 -6.5\n32.8 0 -6.5\n0 32.8 -6.0\n"
        "[2D_TRIANGLES]\n0 1 2 0.03 1.0\n");
    MeshImportOptions feetOptions = options;
    feetOptions.horizontalUnits = "ft";
    feetOptions.verticalUnits   = "us-survey-ft";
    feetOptions.depthUnits      = "ft";
    MeshPackage converted;
    if (!importSurfaceMesh(feet, feetOptions, converted, message))
    {
        std::cout << message << '\n';
        return 1;
    }
    std::cout << "feet: x " << converted.mesh.nodeX[1] << " m, z "
              << converted.mesh.nodeZ[0] << " m, depth "
              << converted.initialDepth[0] << " m\n";
    //! [units]
    check(std::abs(converted.mesh.nodeX[1] - 32.8 * 0.3048) < 1e-12,
          "international feet");
    check(std::abs(converted.mesh.nodeZ[0] + 6.5 * 1200.0 / 3937.0) < 1e-12,
          "US survey feet");

    //! [refusals]
    struct Refusal
    {
        std::string what;
        std::string text;     // file content
        MeshImportOptions how;
        std::string expected; // fragment of the message
    };
    MeshImportOptions noDatum = options;
    noDatum.verticalDatum.clear();
    MeshImportOptions projected = options;
    projected.horizontalCrs = "EPSG:25832";
    MeshImportOptions wrongUnits = options;
    wrongUnits.horizontalUnits = "ft";
    const std::string triangle = "[2D_VERTICES]\n0 0 1\n1 0 1\n0 1 1\n"
                                 "[2D_TRIANGLES]\n";
    const std::vector<Refusal> refusals = {
        {"no vertical datum", triangle + "0 1 2 0.03\n", noDatum,
         "declare horizontal CRS, vertical datum and elevation kind"},
        {"a projected CRS", triangle + "0 1 2 0.03\n", projected,
         "requires a GDAL-enabled SDK"},
        {"a header that disagrees", ";; UNITS: m\n" + triangle + "0 1 2 0.03\n",
         wrongUnits, "source UNITS header disagrees"},
        {"a repeated node", triangle + "0 1 1 0.03\n", options,
         "mesh face repeats a node"},
        {"an unknown node", triangle + "0 1 7 0.03\n", options,
         "cell references unknown node id"},
        {"a negative depth", triangle + "0 1 2 0.03 -0.1\n", options,
         "negative roughness or depth"},
        {"no roughness", triangle + "0 1 2\n", options,
         "cell needs vertex IDs and SWMM roughness"},
        {"an interior boundary edge",
         "[2D_VERTICES]\n0 0 1\n1 0 1\n1 1 1\n0 1 1\n[2D_TRIANGLES]\n"
         "0 1 2 0.03\n0 2 3 0.03\n[2D_BOUNDARY_CONDITIONS]\n0 1 WALL\n",
         options, "boundary must identify one unique exterior edge"},
    };
    int index = 0;
    for (const Refusal &refusal : refusals)
    {
        MeshPackage refused;
        const std::string path =
            scratch("refused_" + std::to_string(index++) + ".2dm", refusal.text);
        const bool imported = importSurfaceMesh(path, refusal.how, refused, message);
        std::cout << refusal.what << ": " << message << '\n';
        check(!imported && message.find(refusal.expected) != std::string::npos,
              refusal.what);
    }
    //! [refusals]

    //! [reference]
    const MeshPackageReference reference(package);
    std::cout << "horizontal: " << reference.srText() << '\n'
              << "vertical:   " << reference.verticalSrText() << '\n'
              << "codes " << reference.authSRID() << "/"
              << reference.verticalAuthSRID() << '\n';
    //! [reference]
    check(reference.srText().rfind("LOCAL_CS", 0) == 0, "LOCAL becomes LOCAL_CS");
    check(reference.verticalSrText().find("site-benchmark-1") != std::string::npos,
          "the datum is named in the vertical WKT");

    std::cout << (failures == 0 ? "all checks passed" : "checks failed")
              << '\n';
    return failures == 0 ? 0 : 1;
}
