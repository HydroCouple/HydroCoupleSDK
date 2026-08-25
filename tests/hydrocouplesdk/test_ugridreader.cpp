// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin
//
// Tests for readUGRIDMesh: the counterpart the writers lacked. The fixtures
// are written by the SDK's own NetCDFUGRIDWriter, so a round trip proves the
// pair agree — and the index-convention tests are written with the NetCDF C
// API directly, because a file this codebase produced can only ever exercise
// the conventions this codebase chose.

#include <gtest/gtest.h>

#include "hydrocouplesdk/io/netcdfugridwriter.h"
#include "hydrocouplesdk/io/ugridreader.h"

#include <netcdf.h>

#include <filesystem>
#include <string>
#include <vector>

using namespace HydroCouple::SDK::IO;

#ifndef TEST_ARTIFACTS_DIR
#define TEST_ARTIFACTS_DIR "."
#endif

namespace {

std::filesystem::path artifact(const std::string &name)
{
    const std::filesystem::path dir{TEST_ARTIFACTS_DIR};
    std::error_code code;
    std::filesystem::create_directories(dir, code);

    return dir / name;
}

//! A square split into a triangle and a quad, so faces differ in width.
MeshDefinition mixedMesh()
{
    MeshDefinition mesh;
    mesh.meshName = "mixed";
    mesh.nodeX = {0.0, 10.0, 10.0, 0.0, 5.0};
    mesh.nodeY = {0.0, 0.0, 10.0, 10.0, 20.0};
    mesh.nodeZ = {1.0, 2.0, 3.0, 4.0, 5.0};
    mesh.faceNodeOffsets = {0, 4, 7};
    mesh.faceNodes = {0, 1, 2, 3, 3, 2, 4};
    mesh.edgeNodes = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};

    return mesh;
}

} // namespace

TEST(UGRIDReaderTest, ReportsSupportInThisBuild)
{
    // NetCDF is optional at build time, and this test target exists only
    // when it is on.
    EXPECT_TRUE(ugridReadSupported());
}

TEST(UGRIDReaderTest, RoundTripsAMeshThroughTheWriter)
{
    const std::filesystem::path path = artifact("ugrid_roundtrip.nc");
    std::filesystem::remove(path);

    const MeshDefinition original = mixedMesh();

    {
        NetCDFUGRIDWriter writer(path, original);
        ASSERT_EQ(writer.initialize(), 0);
        writer.finalize();
    }

    std::string message;
    MeshDefinition read;

    ASSERT_TRUE(readUGRIDMesh(path, "", read, message)) << message;

    EXPECT_EQ(read.meshName, original.meshName);
    EXPECT_EQ(read.nodeCount(), original.nodeCount());
    EXPECT_EQ(read.faceCount(), original.faceCount());
    EXPECT_EQ(read.edgeCount(), original.edgeCount());

    EXPECT_EQ(read.nodeX, original.nodeX);
    EXPECT_EQ(read.nodeY, original.nodeY);
    EXPECT_EQ(read.nodeZ, original.nodeZ);

    // The connectivity survives the rectangular on-disk form: a triangle
    // stored in a four-wide array comes back three nodes long, not four.
    EXPECT_EQ(read.faceNodeOffsets, original.faceNodeOffsets);
    EXPECT_EQ(read.faceNodes, original.faceNodes);
    EXPECT_EQ(read.edgeNodes, original.edgeNodes);
}

TEST(UGRIDReaderTest, ListsTheMeshesAFileHolds)
{
    const std::filesystem::path path = artifact("ugrid_names.nc");
    std::filesystem::remove(path);

    {
        NetCDFUGRIDWriter writer(path, mixedMesh());
        ASSERT_EQ(writer.initialize(), 0);
        writer.finalize();
    }

    std::string message;
    const std::vector<std::string> names = ugridMeshNames(path, message);

    ASSERT_EQ(names.size(), 1u) << message;
    EXPECT_EQ(names.front(), "mixed");
}

TEST(UGRIDReaderTest, ReadsANodeOnlyMesh)
{
    const std::filesystem::path path = artifact("ugrid_nodes.nc");
    std::filesystem::remove(path);

    MeshDefinition cloud;
    cloud.meshName = "cloud";
    cloud.nodeX = {1.0, 2.0, 3.0};
    cloud.nodeY = {4.0, 5.0, 6.0};

    {
        NetCDFUGRIDWriter writer(path, cloud);
        ASSERT_EQ(writer.initialize(), 0);
        writer.finalize();
    }

    std::string message;
    MeshDefinition read;

    // A node-only mesh is valid — a point cloud, or the nodes of a 1-D
    // network — and must not be mistaken for a malformed one.
    ASSERT_TRUE(readUGRIDMesh(path, "", read, message)) << message;

    EXPECT_EQ(read.nodeCount(), 3);
    EXPECT_EQ(read.faceCount(), 0);
    EXPECT_EQ(read.edgeCount(), 0);
}

TEST(UGRIDReaderTest, ConvertsAOneBasedFileToZeroBased)
{
    // Written by hand with the NetCDF API, because our own writer always
    // emits start_index 0 — a file it produced could never exercise this.
    //
    // Classic netCDF-3, deliberately: it needs no HDF5, so this fixture is
    // written by the same library the reader uses, and it also proves the
    // reader is not tied to the NetCDF-4 files the SDK's writer produces.
    const std::filesystem::path path = artifact("ugrid_one_based.nc");
    std::filesystem::remove(path);

    int ncid = -1;
    ASSERT_EQ(nc_create(path.string().c_str(), NC_CLOBBER, &ncid), NC_NOERR);

    int nodeDim = -1;
    int faceDim = -1;
    int cornerDim = -1;
    ASSERT_EQ(nc_def_dim(ncid, "node", 4, &nodeDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "face", 1, &faceDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "corner", 4, &cornerDim), NC_NOERR);

    int topologyVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "grid", NC_INT, 0, nullptr, &topologyVar),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "cf_role", 13,
                              "mesh_topology"),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "node_coordinates", 13,
                              "node_x node_y"),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "face_node_connectivity", 10,
                              "face_nodes"),
              NC_NOERR);

    int nodeXVar = -1;
    int nodeYVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "node_x", NC_DOUBLE, 1, &nodeDim, &nodeXVar),
              NC_NOERR);
    ASSERT_EQ(nc_def_var(ncid, "node_y", NC_DOUBLE, 1, &nodeDim, &nodeYVar),
              NC_NOERR);

    // NC_INT, not NC_INT64: classic netCDF has no 64-bit integer type, and a
    // reader that only understood the widest one would fail on every file
    // written before NetCDF-4.
    const int faceDims[2] = {faceDim, cornerDim};
    int faceVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "face_nodes", NC_INT, 2, faceDims, &faceVar),
              NC_NOERR);

    const int startIndex = 1;
    ASSERT_EQ(nc_put_att_int(ncid, faceVar, "start_index", NC_INT, 1,
                             &startIndex),
              NC_NOERR);

    const int fill = -999;
    ASSERT_EQ(nc_put_att_int(ncid, faceVar, "_FillValue", NC_INT, 1, &fill),
              NC_NOERR);

    ASSERT_EQ(nc_enddef(ncid), NC_NOERR);

    const double xs[4] = {0.0, 1.0, 1.0, 0.0};
    const double ys[4] = {0.0, 0.0, 1.0, 1.0};
    ASSERT_EQ(nc_put_var_double(ncid, nodeXVar, xs), NC_NOERR);
    ASSERT_EQ(nc_put_var_double(ncid, nodeYVar, ys), NC_NOERR);

    // One triangle in a four-wide array: nodes 1, 2, 3 one-based, then the
    // fill value.
    const int faces[4] = {1, 2, 3, fill};
    ASSERT_EQ(nc_put_var_int(ncid, faceVar, faces), NC_NOERR);

    ASSERT_EQ(nc_close(ncid), NC_NOERR);

    std::string message;
    MeshDefinition read;

    ASSERT_TRUE(readUGRIDMesh(path, "", read, message)) << message;

    EXPECT_EQ(read.meshName, "grid");
    EXPECT_EQ(read.nodeCount(), 4);
    ASSERT_EQ(read.faceCount(), 1);

    // Converted once, here: an off-by-one in connectivity draws a mesh that
    // is subtly wrong everywhere rather than obviously wrong anywhere.
    const std::vector<int64_t> expected = {0, 1, 2};
    EXPECT_EQ(read.faceNodes, expected)
        << "the file's one-based indices were not converted";
    EXPECT_EQ(read.faceNodeOffsets.back(), 3)
        << "the fill padding was read as a node";
}

TEST(UGRIDReaderTest, ExplainsWhatItCannotRead)
{
    std::string message;
    MeshDefinition mesh;

    EXPECT_FALSE(readUGRIDMesh(artifact("does_not_exist.nc"), "", mesh,
                               message));
    EXPECT_FALSE(message.empty()) << "a failure with no explanation";

    // A file with no topology variable is not a UGRID file, and saying so is
    // more useful than returning an empty mesh.
    const std::filesystem::path bare = artifact("ugrid_bare.nc");
    std::filesystem::remove(bare);

    int ncid = -1;
    ASSERT_EQ(nc_create(bare.string().c_str(), NC_CLOBBER, &ncid), NC_NOERR);
    ASSERT_EQ(nc_close(ncid), NC_NOERR);

    message.clear();
    EXPECT_FALSE(readUGRIDMesh(bare, "", mesh, message));
    EXPECT_FALSE(message.empty());

    // Naming a mesh the file does not hold must not silently read another.
    message.clear();
    const std::filesystem::path path = artifact("ugrid_roundtrip.nc");

    if (std::filesystem::exists(path))
    {
        EXPECT_FALSE(readUGRIDMesh(path, "no_such_mesh", mesh, message));
        EXPECT_FALSE(message.empty());
    }
}

// ── Vertical coordinate (layered UGRID) ──────────────────────────────────

namespace {

/*!
 * \brief Writes a layered UGRID fixture: one quad, N layers, M times.
 *
 * Hand-built with the NetCDF API in classic netCDF-3, so the fixture needs
 * no HDF5 and is not tied to any one writer's output. \p interfaceVarName
 * empty omits the interface variable, which is how the derived-interface
 * fallback is exercised.
 */
void writeLayeredFixture(const std::filesystem::path &path,
                         const std::vector<double> &layerSigma,
                         const std::vector<double> &interfaceSigma,
                         const std::string &interfaceVarName,
                         const std::vector<double> &etaPerTime,
                         double depth, bool useBounds = false,
                         bool withDecoys = false)
{
    std::filesystem::remove(path);

    int ncid = -1;
    ASSERT_EQ(nc_create(path.string().c_str(), NC_CLOBBER, &ncid), NC_NOERR);

    int nodeDim = -1, faceDim = -1, cornerDim = -1, layerDim = -1;
    int interfaceDim = -1, timeDim = -1, boundsDim = -1;
    ASSERT_EQ(nc_def_dim(ncid, "node", 4, &nodeDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "face", 1, &faceDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "corner", 4, &cornerDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "layer", layerSigma.size(), &layerDim),
              NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "interface", interfaceSigma.size(),
                         &interfaceDim),
              NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "time", etaPerTime.size(), &timeDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "two", 2, &boundsDim), NC_NOERR);

    // Defined before anything real, because the reader scans by variable id
    // — definition order — so a decoy appended afterwards would never be
    // reached first and would prove nothing.
    int decoyTermsVar = -1;
    int decoyInterfaceVar = -1;

    if (withDecoys)
    {
        ASSERT_EQ(nc_def_var(ncid, "decoy_coordinate", NC_DOUBLE, 0, nullptr,
                             &decoyTermsVar),
                  NC_NOERR);
        ASSERT_EQ(nc_put_att_text(ncid, decoyTermsVar, "standard_name", 32,
                                  "atmosphere_hybrid_sigma_pressure"),
                  NC_NOERR);
        const std::string decoyTerms = "sigma: nonsense eta: nonsense "
                                       "depth: nonsense";
        ASSERT_EQ(nc_put_att_text(ncid, decoyTermsVar, "formula_terms",
                                  decoyTerms.size(), decoyTerms.c_str()),
                  NC_NOERR);

        ASSERT_EQ(nc_def_var(ncid, "decoy_interface_area", NC_DOUBLE, 1,
                             &interfaceDim, &decoyInterfaceVar),
                  NC_NOERR);
    }

    int topologyVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "mesh2d", NC_INT, 0, nullptr, &topologyVar),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "cf_role", 13,
                              "mesh_topology"),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "node_coordinates", 13,
                              "node_x node_y"),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "face_node_connectivity", 10,
                              "face_nodes"),
              NC_NOERR);

    int nodeXVar = -1, nodeYVar = -1, faceVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "node_x", NC_DOUBLE, 1, &nodeDim, &nodeXVar),
              NC_NOERR);
    ASSERT_EQ(nc_def_var(ncid, "node_y", NC_DOUBLE, 1, &nodeDim, &nodeYVar),
              NC_NOERR);

    const int faceDims[2] = {faceDim, cornerDim};
    ASSERT_EQ(nc_def_var(ncid, "face_nodes", NC_INT, 2, faceDims, &faceVar),
              NC_NOERR);

    // Vertical coordinate, named exactly as CF says rather than as any one
    // model names it.
    int sigmaVar = -1, depthVar = -1, etaVar = -1, interfaceVar = -1;
    int boundsVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "layer_sigma", NC_DOUBLE, 1, &layerDim,
                         &sigmaVar),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, sigmaVar, "standard_name", 22,
                              "ocean_sigma_coordinate"),
              NC_NOERR);

    const std::string formula = "sigma: layer_sigma eta: water_level "
                                "depth: bed_depth";
    ASSERT_EQ(nc_put_att_text(ncid, sigmaVar, "formula_terms", formula.size(),
                              formula.c_str()),
              NC_NOERR);

    if (useBounds)
    {
        const int boundsDims[2] = {layerDim, boundsDim};
        ASSERT_EQ(nc_def_var(ncid, "layer_bounds", NC_DOUBLE, 2, boundsDims,
                             &boundsVar),
                  NC_NOERR);
        ASSERT_EQ(nc_put_att_text(ncid, sigmaVar, "bounds", 12,
                                  "layer_bounds"),
                  NC_NOERR);
    }
    else if (!interfaceVarName.empty())
    {
        ASSERT_EQ(nc_def_var(ncid, interfaceVarName.c_str(), NC_DOUBLE, 1,
                             &interfaceDim, &interfaceVar),
                  NC_NOERR);
    }

    ASSERT_EQ(nc_def_var(ncid, "bed_depth", NC_DOUBLE, 1, &faceDim, &depthVar),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, depthVar, "positive", 4, "down"), NC_NOERR);

    const int etaDims[2] = {timeDim, faceDim};
    ASSERT_EQ(nc_def_var(ncid, "water_level", NC_DOUBLE, 2, etaDims, &etaVar),
              NC_NOERR);

    ASSERT_EQ(nc_enddef(ncid), NC_NOERR);

    const double xs[4] = {0.0, 1.0, 1.0, 0.0};
    const double ys[4] = {0.0, 0.0, 1.0, 1.0};
    const int faces[4] = {0, 1, 2, 3};
    ASSERT_EQ(nc_put_var_double(ncid, nodeXVar, xs), NC_NOERR);
    ASSERT_EQ(nc_put_var_double(ncid, nodeYVar, ys), NC_NOERR);
    ASSERT_EQ(nc_put_var_int(ncid, faceVar, faces), NC_NOERR);
    if (withDecoys)
    {
        // Same length as the interfaces, and not a sigma: areas, which a
        // layered file plausibly carries.
        std::vector<double> areas(interfaceSigma.size(), 0.0);

        for (size_t index = 0; index < areas.size(); ++index)
        {
            areas[index] = 100.0 - 10.0 * double(index);
        }

        ASSERT_EQ(nc_put_var_double(ncid, decoyInterfaceVar, areas.data()),
                  NC_NOERR);
    }

    ASSERT_EQ(nc_put_var_double(ncid, sigmaVar, layerSigma.data()), NC_NOERR);
    ASSERT_EQ(nc_put_var_double(ncid, depthVar, &depth), NC_NOERR);
    ASSERT_EQ(nc_put_var_double(ncid, etaVar, etaPerTime.data()), NC_NOERR);

    if (useBounds)
    {
        std::vector<double> bounds;

        for (size_t layer = 0; layer < layerSigma.size(); ++layer)
        {
            bounds.push_back(interfaceSigma[layer]);
            bounds.push_back(interfaceSigma[layer + 1]);
        }

        ASSERT_EQ(nc_put_var_double(ncid, boundsVar, bounds.data()), NC_NOERR);
    }
    else if (interfaceVar >= 0)
    {
        ASSERT_EQ(nc_put_var_double(ncid, interfaceVar, interfaceSigma.data()),
                  NC_NOERR);
    }

    ASSERT_EQ(nc_close(ncid), NC_NOERR);
}

}   // namespace

TEST(UGRIDReaderTest, IgnoresDecoysDefinedBeforeTheRealCoordinate)
{
    // Two traps in one file, both defined ahead of the real variables so the
    // reader meets them first.
    //
    // The first carries formula_terms but is a different parametric
    // coordinate: CF uses that attribute for several, so the standard name
    // is what says which one this is.
    //
    // The second is exactly as long as the interfaces but holds areas.
    // Identifying the interface sigma by its length alone would take it.
    const std::filesystem::path path = artifact("ugrid_layered_decoys.nc");

    const std::vector<double> interfaces = {0.0, -0.4, -1.0};

    writeLayeredFixture(path, {-0.2, -0.7}, interfaces, "interface_sigma",
                        {3.0}, 6.0, false, true);

    VerticalCoordinate vertical;
    std::string message;

    ASSERT_TRUE(readVerticalCoordinate(path, "", 0, vertical, message))
        << message;

    EXPECT_EQ(vertical.surfaceVariable, "water_level")
        << "the decoy's formula_terms was followed";
    EXPECT_EQ(vertical.interfaceSigma, interfaces)
        << "a same-length variable that is not a sigma was taken for one";
    EXPECT_DOUBLE_EQ(vertical.surface.front(), 3.0);
}

TEST(UGRIDReaderTest, RefusesASurfaceElevationOfTheWrongLength)
{
    // eta must be per face. A file that dimensioned it on nodes reads
    // perfectly well and describes a different mesh, so the lengths are
    // checked rather than assumed to agree.
    const std::filesystem::path path = artifact("ugrid_layered_badeta.nc");
    std::filesystem::remove(path);

    int ncid = -1;
    ASSERT_EQ(nc_create(path.string().c_str(), NC_CLOBBER, &ncid), NC_NOERR);

    int nodeDim = -1, faceDim = -1, cornerDim = -1, layerDim = -1;
    int timeDim = -1;
    ASSERT_EQ(nc_def_dim(ncid, "node", 4, &nodeDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "face", 1, &faceDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "corner", 4, &cornerDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "layer", 1, &layerDim), NC_NOERR);
    ASSERT_EQ(nc_def_dim(ncid, "time", 1, &timeDim), NC_NOERR);

    int topologyVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "mesh2d", NC_INT, 0, nullptr, &topologyVar),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "cf_role", 13,
                              "mesh_topology"),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "node_coordinates", 13,
                              "node_x node_y"),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, topologyVar, "face_node_connectivity", 10,
                              "face_nodes"),
              NC_NOERR);

    int nodeXVar = -1, nodeYVar = -1, faceVar = -1, sigmaVar = -1;
    int depthVar = -1, etaVar = -1;
    ASSERT_EQ(nc_def_var(ncid, "node_x", NC_DOUBLE, 1, &nodeDim, &nodeXVar),
              NC_NOERR);
    ASSERT_EQ(nc_def_var(ncid, "node_y", NC_DOUBLE, 1, &nodeDim, &nodeYVar),
              NC_NOERR);
    const int faceDims[2] = {faceDim, cornerDim};
    ASSERT_EQ(nc_def_var(ncid, "face_nodes", NC_INT, 2, faceDims, &faceVar),
              NC_NOERR);
    ASSERT_EQ(nc_def_var(ncid, "layer_sigma", NC_DOUBLE, 1, &layerDim,
                         &sigmaVar),
              NC_NOERR);
    ASSERT_EQ(nc_put_att_text(ncid, sigmaVar, "standard_name", 22,
                              "ocean_sigma_coordinate"),
              NC_NOERR);
    const std::string formula = "sigma: layer_sigma eta: water_level "
                                "depth: bed_depth";
    ASSERT_EQ(nc_put_att_text(ncid, sigmaVar, "formula_terms", formula.size(),
                              formula.c_str()),
              NC_NOERR);
    ASSERT_EQ(nc_def_var(ncid, "bed_depth", NC_DOUBLE, 1, &faceDim, &depthVar),
              NC_NOERR);

    // The mistake: eta on nodes rather than faces.
    const int etaDims[2] = {timeDim, nodeDim};
    ASSERT_EQ(nc_def_var(ncid, "water_level", NC_DOUBLE, 2, etaDims, &etaVar),
              NC_NOERR);
    ASSERT_EQ(nc_enddef(ncid), NC_NOERR);

    const double xs[4] = {0.0, 1.0, 1.0, 0.0};
    const double ys[4] = {0.0, 0.0, 1.0, 1.0};
    const int faces[4] = {0, 1, 2, 3};
    const double sigma = -0.5;
    const double depth = 4.0;
    const double etas[4] = {0.0, 0.0, 0.0, 0.0};
    ASSERT_EQ(nc_put_var_double(ncid, nodeXVar, xs), NC_NOERR);
    ASSERT_EQ(nc_put_var_double(ncid, nodeYVar, ys), NC_NOERR);
    ASSERT_EQ(nc_put_var_int(ncid, faceVar, faces), NC_NOERR);
    ASSERT_EQ(nc_put_var_double(ncid, sigmaVar, &sigma), NC_NOERR);
    ASSERT_EQ(nc_put_var_double(ncid, depthVar, &depth), NC_NOERR);
    ASSERT_EQ(nc_put_var_double(ncid, etaVar, etas), NC_NOERR);
    ASSERT_EQ(nc_close(ncid), NC_NOERR);

    VerticalCoordinate vertical;
    std::string message;

    EXPECT_FALSE(readVerticalCoordinate(path, "", 0, vertical, message));
    EXPECT_NE(message.find("surface elevations"), std::string::npos)
        << message;
}

TEST(UGRIDReaderTest, ReadsACfVerticalCoordinate)
{
    const std::filesystem::path path = artifact("ugrid_layered.nc");

    // Deliberately non-uniform, so midpointing the centres would give a
    // different answer than reading the interfaces.
    const std::vector<double> interfaces = {0.0, -0.1, -0.4, -1.0};
    const std::vector<double> centres = {-0.05, -0.25, -0.7};

    writeLayeredFixture(path, centres, interfaces, "interface_sigma",
                        {2.0, 3.0}, 10.0);

    VerticalCoordinate vertical;
    std::string message;

    ASSERT_TRUE(readVerticalCoordinate(path, "", 0, vertical, message))
        << message;

    EXPECT_EQ(vertical.layerCount(), 3);
    EXPECT_EQ(vertical.interfaceSigma, interfaces)
        << "the interfaces were not the ones the file holds";
    EXPECT_FALSE(vertical.interfacesAreDerived);
    EXPECT_EQ(vertical.surfaceVariable, "water_level");
    EXPECT_EQ(vertical.timeCount, 2u);

    ASSERT_EQ(vertical.depth.size(), 1u);
    EXPECT_DOUBLE_EQ(vertical.depth.front(), 10.0);
    ASSERT_EQ(vertical.surface.size(), 1u);
    EXPECT_DOUBLE_EQ(vertical.surface.front(), 2.0);
}

TEST(UGRIDReaderTest, TakesTheSurfaceElevationOfTheTimeAsked)
{
    // A layered mesh moves: the water surface is what makes sigma layers
    // real elevations, so reading time 0 for every frame freezes the mesh
    // while the values on it change.
    const std::filesystem::path path = artifact("ugrid_layered_times.nc");

    writeLayeredFixture(path, {-0.5}, {0.0, -1.0}, "interface_sigma",
                        {1.0, 5.0, 9.0}, 4.0);

    VerticalCoordinate vertical;
    std::string message;

    ASSERT_TRUE(readVerticalCoordinate(path, "", 2, vertical, message))
        << message;
    EXPECT_DOUBLE_EQ(vertical.surface.front(), 9.0);

    ASSERT_TRUE(readVerticalCoordinate(path, "", 1, vertical, message))
        << message;
    EXPECT_DOUBLE_EQ(vertical.surface.front(), 5.0);

    // Past the end clamps to the latest, so a viewer following a run still
    // being written gets the newest there is rather than an error.
    ASSERT_TRUE(readVerticalCoordinate(path, "", 99, vertical, message))
        << message;
    EXPECT_DOUBLE_EQ(vertical.surface.front(), 9.0);
}

TEST(UGRIDReaderTest, FindsInterfacesThroughCfBounds)
{
    // The standard way to say where a layer begins and ends. Preferred over
    // the property search, since it is unambiguous.
    const std::filesystem::path path = artifact("ugrid_layered_bounds.nc");

    const std::vector<double> interfaces = {0.0, -0.2, -1.0};

    writeLayeredFixture(path, {-0.1, -0.6}, interfaces, "", {0.0}, 8.0, true);

    VerticalCoordinate vertical;
    std::string message;

    ASSERT_TRUE(readVerticalCoordinate(path, "", 0, vertical, message))
        << message;

    EXPECT_EQ(vertical.interfaceSigma, interfaces);
    EXPECT_FALSE(vertical.interfacesAreDerived);
}

TEST(UGRIDReaderTest, FindsInterfacesByShapeWhateverTheyAreCalled)
{
    // FVQual calls it mesh2d_interface_sigma; another model will call it
    // something else. What identifies it is what it *is* — one longer than
    // the layers, running 0 to -1 — not the name a writer happened to pick.
    const std::filesystem::path path = artifact("ugrid_layered_oddname.nc");

    const std::vector<double> interfaces = {0.0, -0.3, -1.0};

    writeLayeredFixture(path, {-0.15, -0.65}, interfaces,
                        "zeta_at_w_locations", {0.0}, 8.0);

    VerticalCoordinate vertical;
    std::string message;

    ASSERT_TRUE(readVerticalCoordinate(path, "", 0, vertical, message))
        << message;

    EXPECT_EQ(vertical.interfaceSigma, interfaces);
    EXPECT_FALSE(vertical.interfacesAreDerived);
}

TEST(UGRIDReaderTest, DerivesInterfacesWhenTheFileHasNoneAndSaysSo)
{
    // Midpointing is exact for a uniform distribution and an approximation
    // otherwise, so a caller computing volumes has to be able to tell the
    // difference from a caller drawing a picture.
    const std::filesystem::path path = artifact("ugrid_layered_derived.nc");

    writeLayeredFixture(path, {-0.25, -0.75}, {}, "", {0.0}, 8.0);

    VerticalCoordinate vertical;
    std::string message;

    ASSERT_TRUE(readVerticalCoordinate(path, "", 0, vertical, message))
        << message;

    EXPECT_TRUE(vertical.interfacesAreDerived);

    const std::vector<double> expected = {0.0, -0.5, -1.0};
    ASSERT_EQ(vertical.interfaceSigma.size(), expected.size());

    for (size_t index = 0; index < expected.size(); ++index)
    {
        EXPECT_NEAR(vertical.interfaceSigma[index], expected[index], 1.0e-12);
    }
}

TEST(UGRIDReaderTest, SaysWhenAMeshIsSimplyTwoDimensional)
{
    // Not an error: most UGRID files have no water column, and a viewer has
    // to be able to tell "no vertical coordinate" from "unreadable".
    const std::filesystem::path path = artifact("ugrid_roundtrip.nc");

    EXPECT_FALSE(hasVerticalCoordinate(path, ""));

    VerticalCoordinate vertical;
    std::string message;

    EXPECT_FALSE(readVerticalCoordinate(path, "", 0, vertical, message));
    EXPECT_NE(message.find("two-dimensional"), std::string::npos) << message;
}

TEST(UGRIDReaderTest, RoundTripsAVerticalCoordinateThroughTheWriter)
{
    // The pair must agree, and only writing one and reading it back shows
    // that. This is the same asymmetry the node_z fix closed: the reader
    // could read vertical coordinates the writer could not produce, so
    // nothing exercised the two against each other.
    const std::filesystem::path path = artifact("ugrid_layered_roundtrip.nc");
    std::filesystem::remove(path);

    MeshDefinition mesh;
    mesh.meshName = "lake";
    mesh.nodeX = {0.0, 1.0, 1.0, 0.0};
    mesh.nodeY = {0.0, 0.0, 1.0, 1.0};
    mesh.faceNodeOffsets = {0, 4};
    mesh.faceNodes = {0, 1, 2, 3};

    const std::vector<double> interfaces = {0.0, -0.15, -0.6, -1.0};

    {
        NetCDFUGRIDWriter writer(path.string(), mesh);
        writer.setVerticalCoordinate(interfaces, {12.0}, {1.5});

        ASSERT_EQ(writer.initialize(), 0) << writer.errorMessage();
        ASSERT_EQ(writer.finalize(), 0) << writer.errorMessage();
    }

    EXPECT_TRUE(hasVerticalCoordinate(path, "lake"));

    VerticalCoordinate vertical;
    std::string message;

    ASSERT_TRUE(readVerticalCoordinate(path, "lake", 0, vertical, message))
        << message;

    EXPECT_EQ(vertical.layerCount(), 3);
    EXPECT_FALSE(vertical.interfacesAreDerived);

    ASSERT_EQ(vertical.interfaceSigma.size(), interfaces.size());

    for (size_t index = 0; index < interfaces.size(); ++index)
    {
        EXPECT_NEAR(vertical.interfaceSigma[index], interfaces[index], 1.0e-12)
            << "interface " << index;
    }

    ASSERT_EQ(vertical.depth.size(), 1u);
    EXPECT_DOUBLE_EQ(vertical.depth.front(), 12.0);
    ASSERT_EQ(vertical.surface.size(), 1u);
    EXPECT_DOUBLE_EQ(vertical.surface.front(), 1.5);

    // The centres must be the midpoints the bounds attribute claims they
    // are; a reader labelling layers by them would otherwise place every
    // label at the wrong depth.
    ASSERT_EQ(vertical.layerSigma.size(), interfaces.size() - 1);

    for (size_t layer = 0; layer + 1 < interfaces.size(); ++layer)
    {
        EXPECT_NEAR(vertical.layerSigma[layer],
                    0.5 * (interfaces[layer] + interfaces[layer + 1]), 1.0e-12)
            << "layer " << layer << " centre is not between its bounds";
    }

    // The elevations the coordinate describes: surface at 1.5, bed at -12,
    // a 13.5 m column. Checked here rather than left to the consumer,
    // because the sign conventions are the whole reason this is easy to get
    // wrong.
    const double column = vertical.depth.front() + vertical.surface.front();

    EXPECT_NEAR(vertical.surface.front() + vertical.interfaceSigma.back() * column,
                -12.0, 1.0e-12);
    EXPECT_NEAR(vertical.surface.front() + vertical.interfaceSigma.front() * column,
                1.5, 1.0e-12);
}
