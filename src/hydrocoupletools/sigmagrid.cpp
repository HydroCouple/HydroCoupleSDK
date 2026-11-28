// SPDX-License-Identifier: MIT
// Copyright (c) 2014-2026 Caleb Buahin

#include "hydrocouplesdk/tools/sigmagrid.h"

#include <algorithm>
#include <cmath>

namespace HydroCouple::SDK::Tools {

// ── SigmaLayering ─────────────────────────────────────────────────────────

std::vector<double> SigmaLayering::uniform(int layerCount)
{
    if (layerCount < 1)
        return {};
    std::vector<double> interfaces(static_cast<size_t>(layerCount) + 1);
    for (int k = 0; k <= layerCount; ++k)
        interfaces[static_cast<size_t>(k)] =
            static_cast<double>(k) / layerCount;
    interfaces.back() = 1.0;
    return interfaces;
}

std::vector<double> SigmaLayering::stretched(int layerCount, double theta,
                                             double b)
{
    if (layerCount < 1)
        return {};
    if (theta <= 0.0)
        return uniform(layerCount);
    b = std::clamp(b, 0.0, 1.0);

    // Song–Haidvogel-style C(s) on s in [0, 1] (0 = surface, 1 = bed):
    //   C(s) = (1 - b) sinh(θ s)/sinh θ
    //        + b [tanh(θ (s - 1/2)) + tanh(θ/2)] / (2 tanh(θ/2))
    // C(0) = 0, C(1) = 1, strictly increasing for θ > 0, b in [0, 1].
    std::vector<double> interfaces(static_cast<size_t>(layerCount) + 1);
    const double sinhTheta     = std::sinh(theta);
    const double tanhHalfTheta = std::tanh(theta / 2.0);
    for (int k = 0; k <= layerCount; ++k)
    {
        const double s = static_cast<double>(k) / layerCount;
        const double surfaceTerm = std::sinh(theta * s) / sinhTheta;
        const double bedTerm =
            (std::tanh(theta * (s - 0.5)) + tanhHalfTheta) /
            (2.0 * tanhHalfTheta);
        interfaces[static_cast<size_t>(k)] =
            (1.0 - b) * surfaceTerm + b * bedTerm;
    }
    interfaces.front() = 0.0;
    interfaces.back()  = 1.0;
    return interfaces;
}

bool SigmaLayering::validate(std::span<const double> interfaces,
                             std::string &message)
{
    if (interfaces.size() < 2)
    {
        message = "sigma distribution needs at least 2 interfaces";
        return false;
    }
    if (interfaces.front() != 0.0 || interfaces.back() != 1.0)
    {
        message = "sigma distribution must span [0, 1] exactly";
        return false;
    }
    for (size_t k = 1; k < interfaces.size(); ++k)
    {
        if (interfaces[k] <= interfaces[k - 1])
        {
            message = "sigma distribution must be strictly increasing "
                      "(violated at interface " +
                      std::to_string(k) + ")";
            return false;
        }
    }
    message.clear();
    return true;
}

// ── SigmaGridGenerator ────────────────────────────────────────────────────

bool SigmaGridGenerator::build(std::span<const double> bedElevation,
                               std::span<const double> surfaceElevation,
                               std::span<const double> sigmaInterfaces,
                               double minLayerThickness, SigmaGrid &out,
                               std::string &message)
{
    if (bedElevation.size() != surfaceElevation.size())
    {
        message = "bed and surface elevation counts differ";
        return false;
    }
    if (!SigmaLayering::validate(sigmaInterfaces, message))
        return false;
    if (minLayerThickness < 0.0)
    {
        message = "minimum layer thickness must be non-negative";
        return false;
    }

    const int64_t columns = static_cast<int64_t>(bedElevation.size());
    const int     layers  = static_cast<int>(sigmaInterfaces.size()) - 1;

    out.columnCount = columns;
    out.layerCount  = layers;
    out.interfaceZ.assign(
        static_cast<size_t>(columns) * static_cast<size_t>(layers + 1), 0.0);

    std::vector<double> thickness(static_cast<size_t>(layers));

    for (int64_t column = 0; column < columns; ++column)
    {
        const double surface = surfaceElevation[static_cast<size_t>(column)];
        const double bed     = bedElevation[static_cast<size_t>(column)];
        const double depth   = surface - bed;
        if (depth <= 0.0)
        {
            message = "column " + std::to_string(column) +
                      " has non-positive depth (surface " +
                      std::to_string(surface) + ", bed " +
                      std::to_string(bed) + ")";
            return false;
        }

        // Nominal thicknesses from the sigma increments.
        for (int k = 0; k < layers; ++k)
            thickness[static_cast<size_t>(k)] =
                (sigmaInterfaces[static_cast<size_t>(k) + 1] -
                 sigmaInterfaces[static_cast<size_t>(k)]) *
                depth;

        if (minLayerThickness > 0.0)
        {
            if (depth < layers * minLayerThickness)
            {
                // Too shallow: collapse to equal layers.
                std::fill(thickness.begin(), thickness.end(), depth / layers);
            }
            else
            {
                // Lift thin layers to the minimum; shrink the surplus of
                // the remaining layers proportionally so the column depth
                // is preserved exactly.
                double deficit = 0.0;
                double surplus = 0.0;
                for (double h : thickness)
                {
                    if (h < minLayerThickness)
                        deficit += minLayerThickness - h;
                    else
                        surplus += h - minLayerThickness;
                }
                if (deficit > 0.0)
                {
                    const double shrink = deficit / surplus;
                    for (double &h : thickness)
                    {
                        if (h < minLayerThickness)
                            h = minLayerThickness;
                        else
                            h -= (h - minLayerThickness) * shrink;
                    }
                }
            }
        }

        // Accumulate interfaces from the surface downward; pin the last
        // interface to the bed to absorb round-off.
        const size_t base = static_cast<size_t>(column) *
                            static_cast<size_t>(layers + 1);
        out.interfaceZ[base] = surface;
        double zLevel        = surface;
        for (int k = 0; k < layers; ++k)
        {
            zLevel -= thickness[static_cast<size_t>(k)];
            out.interfaceZ[base + static_cast<size_t>(k) + 1] = zLevel;
        }
        out.interfaceZ[base + static_cast<size_t>(layers)] = bed;
    }

    message.clear();
    return true;
}

bool SigmaGridGenerator::extrude(const IO::MeshDefinition &horizontal,
                                 const SigmaGrid &sigma, LayeredMesh &out,
                                 std::string &message)
{
    if (!horizontal.validate(message))
        return false;
    if (horizontal.faceCount() == 0)
    {
        message = "horizontal mesh has no faces to extrude";
        return false;
    }
    if (sigma.columnCount != horizontal.nodeCount())
    {
        message = "sigma grid has " + std::to_string(sigma.columnCount) +
                  " columns; horizontal mesh has " +
                  std::to_string(horizontal.nodeCount()) + " nodes";
        return false;
    }

    const int64_t nodes      = horizontal.nodeCount();
    const int     layers     = sigma.layerCount;
    const int     interfaces = layers + 1;

    out.layerCount    = layers;
    out.nodesPerLayer = nodes;
    out.nodeX.resize(static_cast<size_t>(interfaces) *
                     static_cast<size_t>(nodes));
    out.nodeY.resize(out.nodeX.size());
    out.nodeZ.resize(out.nodeX.size());

    for (int k = 0; k < interfaces; ++k)
    {
        const size_t base =
            static_cast<size_t>(k) * static_cast<size_t>(nodes);
        for (int64_t n = 0; n < nodes; ++n)
        {
            out.nodeX[base + static_cast<size_t>(n)] =
                horizontal.nodeX[static_cast<size_t>(n)];
            out.nodeY[base + static_cast<size_t>(n)] =
                horizontal.nodeY[static_cast<size_t>(n)];
            out.nodeZ[base + static_cast<size_t>(n)] = sigma.z(n, k);
        }
    }

    // One prism/hexahedron per (layer, horizontal face): top face nodes at
    // interface k, then the same nodes at interface k + 1.
    const int64_t faces = horizontal.faceCount();
    out.cellNodeOffsets.clear();
    out.cellNodes.clear();
    out.cellNodeOffsets.reserve(
        static_cast<size_t>(layers) * static_cast<size_t>(faces) + 1);
    out.cellNodeOffsets.push_back(0);

    for (int k = 0; k < layers; ++k)
    {
        const int64_t topBase    = static_cast<int64_t>(k) * nodes;
        const int64_t bottomBase = static_cast<int64_t>(k + 1) * nodes;
        for (int64_t f = 0; f < faces; ++f)
        {
            const int64_t begin =
                horizontal.faceNodeOffsets[static_cast<size_t>(f)];
            const int64_t end =
                horizontal.faceNodeOffsets[static_cast<size_t>(f) + 1];
            for (int64_t i = begin; i < end; ++i)
                out.cellNodes.push_back(
                    topBase + horizontal.faceNodes[static_cast<size_t>(i)]);
            for (int64_t i = begin; i < end; ++i)
                out.cellNodes.push_back(
                    bottomBase +
                    horizontal.faceNodes[static_cast<size_t>(i)]);
            out.cellNodeOffsets.push_back(
                static_cast<int64_t>(out.cellNodes.size()));
        }
    }

    message.clear();
    return true;
}

} // namespace HydroCouple::SDK::Tools
