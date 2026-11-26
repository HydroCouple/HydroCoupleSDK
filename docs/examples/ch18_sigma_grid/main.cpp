// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Caleb Buahin

/*!
 * \file   main.cpp
 * \brief  Chapter 18: a terrain-following sigma grid from HydroCoupleTools.
 *         Triangulate a basin, sample its bed from survey points, layer it
 *         with a stretched sigma distribution and a minimum thickness, and
 *         extrude the triangles into prisms; every step is checked.
 */

#include "hydrocouplesdk/tools/sigmagrid.h"
#include "hydrocouplesdk/tools/terrainsampler.h"
#include "hydrocouplesdk/tools/triangulator.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace HydroCouple::SDK;
using namespace HydroCouple::SDK::Tools;
using HydroCouple::SDK::Spatial::Point;

namespace {

int fail(const std::string &what)
{
    std::fprintf(stderr, "FAILED: %s\n", what.c_str());
    return 1;
}

void printLayers(const char *name, const std::vector<double> &sigma)
{
    std::printf("%-22s", name);
    for (size_t k = 1; k < sigma.size(); ++k)
        std::printf(" %.4f", sigma[k] - sigma[k - 1]);
    std::printf("\n");
}

} // namespace

int main()
{
    //! [layering]
    // Normalized interfaces on [0, 1], 0 at the surface; printed as layer
    // thicknesses (fractions of the depth), surface layer first.
    const std::vector<double> uniform    = SigmaLayering::uniform(6);
    const std::vector<double> surface    = SigmaLayering::stretched(6, 3.0, 0.0);
    const std::vector<double> bothEnds   = SigmaLayering::stretched(6, 3.0, 1.0);
    const std::vector<double> blended    = SigmaLayering::stretched(6, 3.0, 0.5);
    printLayers("uniform(6)", uniform);
    printLayers("stretched(6, 3, 0)", surface);
    printLayers("stretched(6, 3, 1)", bothEnds);
    printLayers("stretched(6, 3, 0.5)", blended);

    std::string message;
    for (const auto *sigma : {&uniform, &surface, &bothEnds, &blended})
        if (!SigmaLayering::validate(*sigma, message))
            return fail(message);
    // b = 0 thins the surface layer; b = 1 thins the surface and bed layers alike.
    const double top = bothEnds[1] - bothEnds[0], bottom = bothEnds[6] - bothEnds[5];
    if (!(surface[1] - surface[0] < surface[6] - surface[5]) ||
        std::abs(top - bottom) > 1e-12)
        return fail("stretching profile");
    if (SigmaLayering::validate(std::vector<double>{0.0, 0.6, 0.4, 1.0}, message))
        return fail("a decreasing distribution should be refused");
    std::printf("validate {0, 0.6, 0.4, 1}: %s\n", message.c_str());
    //! [layering]

    //! [mesh]
    // A 100 m x 60 m basin, boundary edges no longer than 20 m.
    TriangulationInput basin;
    basin.boundary      = {{0, 0}, {100, 0}, {100, 60}, {0, 60}};
    basin.maxEdgeLength = 20.0;
    basin.interiorPoints = {{50, 30}, {25, 30}, {75, 30}};
    IO::MeshDefinition horizontal;
    if (!Triangulator::triangulate(basin, horizontal, message))
        return fail(message);
    std::printf("triangulation: %lld nodes, %lld triangles\n",
                static_cast<long long>(horizontal.nodeCount()),
                static_cast<long long>(horizontal.faceCount()));
    //! [mesh]

    //! [bed]
    // Bed elevations (m) at the mesh nodes from five survey points; the
    // water surface is at 0 m everywhere.
    const std::vector<Point> survey{
        {0, 0, -1.0}, {100, 0, -6.0}, {100, 60, -9.0}, {0, 60, -2.0}, {50, 30, -8.0}};
    std::vector<double> bed(static_cast<size_t>(horizontal.nodeCount()));
    if (!TerrainSampler::idw(survey, 2.0, horizontal.nodeX, horizontal.nodeY, bed, message))
        return fail(message);
    const std::vector<double> water(bed.size(), 0.0);
    //! [bed]

    //! [build]
    // Five stretched layers per column, none thinner than 0.5 m where the
    // column is deep enough to allow it.
    const double minimum = 0.5;
    const std::vector<double> sigma = SigmaLayering::stretched(5, 3.0, 0.0);
    SigmaGrid grid;
    if (!SigmaGridGenerator::build(bed, water, sigma, minimum, grid, message))
        return fail(message);

    const auto deepest = std::min_element(bed.begin(), bed.end()) - bed.begin();
    const auto shallowest = std::max_element(bed.begin(), bed.end()) - bed.begin();
    for (const int64_t column : {static_cast<int64_t>(deepest), static_cast<int64_t>(shallowest)})
    {
        std::printf("column %3lld, bed %6.3f m: thicknesses", static_cast<long long>(column),
                    bed[static_cast<size_t>(column)]);
        for (int k = 0; k < grid.layerCount; ++k)
            std::printf(" %.3f", grid.thickness(column, k));
        std::printf("\n");
    }
    for (int64_t column = 0; column < grid.columnCount; ++column)
    {
        const double depth = water[static_cast<size_t>(column)] - bed[static_cast<size_t>(column)];
        if (grid.z(column, 0) != 0.0 || grid.z(column, grid.layerCount) != bed[column])
            return fail("interfaces are pinned to the surface and the bed");
        for (int k = 0; k < grid.layerCount; ++k)
        {
            const double h = grid.thickness(column, k);
            const double floor = depth >= grid.layerCount * minimum ? minimum
                                                                    : depth / grid.layerCount;
            if (!(h > 0.0) || h < floor - 1e-12)
                return fail("layer thickness");
        }
    }
    //! [build]

    //! [extrude]
    LayeredMesh volume;
    if (!SigmaGridGenerator::extrude(horizontal, grid, volume, message))
        return fail(message);
    std::printf("layered mesh: %lld nodes, %lld prisms (%d layers x %lld triangles)\n",
                static_cast<long long>(volume.nodeCount()),
                static_cast<long long>(volume.cellCount()), volume.layerCount,
                static_cast<long long>(horizontal.faceCount()));
    if (volume.cellCount() != grid.layerCount * horizontal.faceCount() ||
        volume.nodeCount() != (grid.layerCount + 1) * horizontal.nodeCount())
        return fail("extruded counts");
    // Each prism lists its top triangle, then the same triangle one interface down.
    for (int64_t cell = 0; cell < volume.cellCount(); ++cell)
    {
        const int64_t begin = volume.cellNodeOffsets[cell], end = volume.cellNodeOffsets[cell + 1];
        const int64_t half = (end - begin) / 2;
        for (int64_t i = 0; i < half; ++i)
            if (!(volume.nodeZ[volume.cellNodes[begin + i]] >
                  volume.nodeZ[volume.cellNodes[begin + half + i]]))
                return fail("a prism's top is not above its bottom");
    }
    //! [extrude]

    //! [errors]
    // A column whose bed is above the water surface has no depth to layer.
    const std::vector<double> dryBed{-2.0, 0.5};
    const std::vector<double> dryWater{0.0, 0.0};
    if (SigmaGridGenerator::build(dryBed, dryWater, sigma, minimum, grid, message))
        return fail("a dry column should be refused");
    std::printf("dry column: %s\n", message.c_str());
    //! [errors]
    return 0;
}
