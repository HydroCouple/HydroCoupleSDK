// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 10: a mesh as a component argument. PolyhedralSurfaceArgument
 *         loaded from a UGRID file, from an \@uri reference and from another
 *         component's output, and the requests it refuses.
 *
 * Writes floodplain.nc in the working directory. Needs an SDK built with
 * NetCDF (see REQUIRES). Exits nonzero when any value is not the one the
 * guide states.
 */

#include "hydrocouplesdk/data/bufferio.h"
#include "hydrocouplesdk/io/meshdefinition.h"
#include "hydrocouplesdk/io/netcdfugridwriter.h"
#include "hydrocouplesdk/spatial/layeredmeshexchangeitems.h"
#include "hydrocouplesdk/spatial/meshadapters.h"
#include "hydrocouplesdk/spatial/polyhedralsurfaceargument.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

using namespace HydroCouple::SDK::Spatial;
namespace HCS = HydroCouple::Spatial;
using HydroCouple::SDK::IO::MeshDefinition;
using InputType = HydroCouple::IArgument::ArgumentInputType;

namespace {

int failures = 0;

void check(bool ok, const char *what)
{
    if (!ok)
    {
        std::printf("FAILED: %s\n", what);
        ++failures;
    }
}

MeshDefinition floodplain()
{
    MeshDefinition mesh;
    mesh.meshName = "floodplain";
    mesh.nodeX = {0.0, 10.0, 10.0, 0.0, 5.0};
    mesh.nodeY = {0.0, 0.0, 10.0, 10.0, 20.0};
    mesh.nodeZ = {1.0, 1.0, 1.5, 1.5, 2.0};
    mesh.faceNodeOffsets = {0, 4, 7};
    mesh.faceNodes = {0, 1, 2, 3, 3, 2, 4};
    return mesh;
}

//! A vertical coordinate with one layer, for the layered output below.
class OneLayer final : public HCS::IVerticalCoordinate
{
public:
    HCS::VerticalCoordinateKind kind() const override { return HCS::VerticalCoordinateKind::ZLevel; }
    int64_t layerCount() const override { return 1; }
    int64_t columnCount() const override { return 2; }
    bool isTimeVarying() const override { return false; }
    uint64_t geometryEpoch() const override { return 0; }
    double interfaceElevation(int64_t, int64_t interfaceIndex) const override
    {
        return interfaceIndex == 0 ? 0.0 : -1.0;
    }
    std::span<const double> interfaceElevations() const override { return m_profile; }

private:
    std::vector<double> m_profile{0.0, -1.0, 0.0, -1.0};
};

} // namespace

int main()
{
    // A UGRID file to read, written with the SDK's NetCDF writer.
    const std::filesystem::path file = "floodplain.nc";
    std::filesystem::remove(file);
    {
        HydroCouple::SDK::IO::NetCDFUGRIDWriter writer(file, floodplain());
        if (writer.initialize() != 0 || writer.finalize() != 0)
        {
            std::printf("FAILED: writing %s: %s\n", file.string().c_str(),
                        writer.errorMessage().c_str());
            return 1;
        }
    }

    //! [file]
    PolyhedralSurfaceArgument mesh("mesh", nullptr);
    std::string message;
    const bool loaded = mesh.initialize(file.string(), InputType::File, message);

    // A composition document stores the reference, never the mesh.
    std::string document;
    const bool written = mesh.serialize(InputType::JSON, document, message);
    //! [file]

    const HCS::IPolyhedralSurface *surface = mesh.polyhedralSurface();
    std::printf("from file: loaded %d, %s; JSON %s; surface %lld patches, view %lld faces "
                "and %lld edges\n",
                loaded, mesh.toString().c_str(), written ? document.c_str() : message.c_str(),
                static_cast<long long>(surface->patchCount()),
                static_cast<long long>(surface->meshView()->faceCount()),
                static_cast<long long>(surface->meshView()->edgeCount()));
    check(loaded && mesh.mesh().faceCount() == 2, "UGRID file read");
    check(written && document == "{\"@uri\":\"floodplain.nc\"}", "serialized as a reference");
    check(surface->meshView()->faceCount() == 2, "the surface keeps the connectivity");

    //! [data_plane]
    // A generic host sees node coordinates: { node, x/y/z }.
    std::array<double, 3> node4{};
    const std::array<int64_t, 2> start{4, 0}, count{1, 3};
    const std::array<int64_t, 1> bufferShape{3};
    const bool read = mesh.getValuesInto(
        HydroCouple::SDK::detail::hostDescriptor(node4.data(), HydroCouple::DataKind::Float64,
                                                 bufferShape),
        start, count, &message);
    //! [data_plane]
    std::printf("data plane: shape {%lld, %lld}, location Node %d, type Vector %d, "
                "node 4 = (%.1f, %.1f, %.1f)\n",
                static_cast<long long>(mesh.shape()[0]), static_cast<long long>(mesh.shape()[1]),
                mesh.location() == HCS::MeshLocation::Node,
                mesh.meshDataType() == HCS::SpatialDataType::Vector, node4[0], node4[1],
                node4[2]);
    check(read && node4[1] == 20.0 && node4[2] == 2.0, "node 4 coordinates");

    //! [uri]
    PolyhedralSurfaceArgument fromUri("mesh", nullptr);
    const bool resolved =
        fromUri.initialize("{\"@uri\": \"floodplain.nc\"}", InputType::JSON, message);
    //! [uri]
    std::printf("from @uri: %d, %s\n", resolved, fromUri.toString().c_str());
    check(resolved && fromUri.mesh().nodeCount() == 5, "@uri reference followed");

    //! [from_output]
    // A component output that carries a surface: the argument copies its
    // mesh view, connectivity included.
    PolyhedralSurfaceAdapter generated(polyhedralSurfaceFromMesh(floodplain()), floodplain());
    OneLayer layer;
    LayeredMeshOutputDouble output("generated_mesh", &generated, &layer,
                                   HCS::MeshLocation::Face, HCS::SpatialDataType::Scalar, 2, 1,
                                   nullptr, nullptr);
    PolyhedralSurfaceArgument fromOutput("mesh", nullptr);
    const bool adopted = fromOutput.initialize(
        static_cast<const HydroCouple::IComponentDataItem &>(output), message);
    std::string again;
    const bool reserialized = fromOutput.serialize(InputType::JSON, again, message);
    //! [from_output]
    std::printf("from an output: adopted %d, %s, %zu edges copied; serialize %d: \"%s\"\n",
                adopted, fromOutput.toString().c_str(), fromOutput.mesh().edgeNodes.size(),
                reserialized, message.c_str());
    check(adopted && fromOutput.mesh().faceCount() == 2, "mesh adopted from an output");
    check(!reserialized && message.find("@from") != std::string::npos,
          "a mesh from an output has no file to reference");

    //! [refusals]
    PolyhedralSurfaceArgument refusals("mesh", nullptr);
    std::string inlineWhy, remoteWhy, stringWhy, writeWhy;
    const bool inlined = refusals.initialize("{\"nodeX\": [0, 1], \"nodeY\": [0, 1]}",
                                             InputType::JSON, inlineWhy);
    const bool remote = refusals.initialize("{\"@uri\": \"https://example.org/mesh.nc\"}",
                                            InputType::JSON, remoteWhy);
    const bool asString = refusals.initialize("floodplain.nc", InputType::String, stringWhy);
    const std::array<double, 3> poke{0.0, 0.0, 0.0};
    const bool poked = mesh.setValuesFrom(
        HydroCouple::SDK::detail::hostDescriptor(const_cast<double *>(poke.data()),
                                                 HydroCouple::DataKind::Float64, bufferShape),
        start, count, &writeWhy);
    //! [refusals]
    std::printf("inline payload %d: %s\nremote uri %d: %s\nString input %d: %s\n"
                "setValuesFrom %d: %s\n",
                inlined, inlineWhy.c_str(), remote, remoteWhy.c_str(), asString,
                stringWhy.c_str(), poked, writeWhy.c_str());
    check(!inlined && !remote && !asString && !poked, "refusals");
    check(!refusals.hasMesh() && refusals.polyhedralSurface() == nullptr,
          "no mesh is not an empty surface");

    std::printf("%s\n", failures == 0 ? "all checks passed" : "checks FAILED");
    return failures == 0 ? 0 : 1;
}
